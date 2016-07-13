#include "PopupUtils.h"

#include <cmath>
#include <map>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "MRF2.2/mrf.h"
#include "MRF2.2/GCoptimization.h"

using namespace std;
using namespace cv;

namespace Popup
{
  vector<int> zoomMask(const vector<int> &mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int NEW_IMAGE_WIDTH, const int NEW_IMAGE_HEIGHT)
  {
    vector<int> zoomed_mask(NEW_IMAGE_WIDTH * NEW_IMAGE_HEIGHT);
    for (int zoomed_pixel = 0; zoomed_pixel < NEW_IMAGE_WIDTH * NEW_IMAGE_HEIGHT; zoomed_pixel++) {
      int zoomed_x = zoomed_pixel % NEW_IMAGE_WIDTH;
      int zoomed_y = zoomed_pixel / NEW_IMAGE_WIDTH;
      int x = round(1.0 * zoomed_x / NEW_IMAGE_WIDTH * IMAGE_WIDTH);
      x = min(x, IMAGE_WIDTH - 1);
      int y = round(1.0 * zoomed_y / NEW_IMAGE_HEIGHT * IMAGE_HEIGHT);
      y = min(y, IMAGE_HEIGHT - 1);
      zoomed_mask[zoomed_pixel] = mask[y * IMAGE_WIDTH + x];
    }
    return zoomed_mask;
  }
  
  vector<int> findNeighbors(const int pixel, const int IMAGE_WIDTH, const int IMAGE_HEIGHT)
  {
    int x = pixel % IMAGE_WIDTH;
    int y = pixel / IMAGE_WIDTH;
    vector<int> neighbor_pixels;
    if (x > 0)
      neighbor_pixels.push_back(pixel - 1);
    if (x < IMAGE_WIDTH - 1)
      neighbor_pixels.push_back(pixel + 1);
    if (y > 0)
      neighbor_pixels.push_back(pixel - IMAGE_WIDTH);
    if (y < IMAGE_HEIGHT - 1)
      neighbor_pixels.push_back(pixel + IMAGE_WIDTH);
    if (x > 0 && y > 0)
      neighbor_pixels.push_back(pixel - IMAGE_WIDTH - 1);
    if (x < IMAGE_WIDTH - 1 && y > 0)
      neighbor_pixels.push_back(pixel - IMAGE_WIDTH + 1);
    if (x > 0 && y < IMAGE_HEIGHT - 1)
      neighbor_pixels.push_back(pixel + IMAGE_WIDTH - 1);
    if (x < IMAGE_WIDTH - 1 && y < IMAGE_HEIGHT - 1)
      neighbor_pixels.push_back(pixel + IMAGE_WIDTH + 1);
    return neighbor_pixels;
  }

  Mat drawIndexMaskImage(const vector<int> &index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT)
  {
    Mat index_mask_image = Mat::zeros(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    map<int, Vec3b> color_table;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      int index = index_mask[pixel];
      if (index < 0)
	continue;
      if (color_table.count(index) == 0)
	color_table[index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
      index_mask_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = color_table[index];
    }
    return index_mask_image;
  }

  void grabCut(const Mat &image, const string &result_filename)
  {
    int border = 5;
    int border2 = border + border;
    cv::Rect rectangle(border,border,image.cols-border2,image.rows-border2);
 
    cv::Mat result; // segmentation result (4 possible values)
    cv::Mat bgModel,fgModel; // the models (internally used)
 
    // GrabCut segmentation
    cv::grabCut(image,    // input image
                result,   // segmentation result
                rectangle,// rectangle containing foreground 
                bgModel,fgModel, // models
                1,        // number of iterations
                cv::GC_INIT_WITH_RECT); // use rectangle
    // Get the pixels marked as likely foreground
    cv::compare(result,cv::GC_PR_FGD,result,cv::CMP_EQ);
    // Generate output image
    cv::Mat foreground(image.size(),CV_8UC3,cv::Scalar(255,255,255));
    image.copyTo(foreground,result); // bg pixels not copied
 
    // draw rectangle on original image
    //cv::rect(image, rectangle, cv::Scalar(255,255,255),1);
    cv::namedWindow("Image");
    cv::imshow("Image",image);
 
    // display result
    cv::namedWindow("Segmented Image");
    cv::imshow("Segmented Image",foreground);
 
    waitKey();
    imwrite(result_filename, result);
  }


  void deformPatches(const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int NUM_PATCHES, const vector<int> &patch_index_mask, const vector<int> &enforced_patch_index_mask, vector<int> &deformed_patch_index_mask)
  {
    const int ENFORCED_DATA_WEIGHT = 1000;
    const int ENCOURAGED_DATA_WEIGHT = 1;
    const int SMOOTHNESS_WEIGHT = 5;
    const int DATA_WEIGHT = 1000;
    const int WINDOW_WIDTH = 20;
    const int WINDOW_HEIGHT = 20;
    vector<int> data_cost;
    vector<bool> fold_line_window_pixel_mask(IMAGE_WIDTH * IMAGE_HEIGHT, false);

    // vector<int> test = patch_index_mask;
    // for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    //   int enforced_patch_index = enforced_patch_index_mask[pixel];
    //   if (enforced_patch_index >= 0)
    // 	test[pixel] = enforced_patch_index % 10000;
    // }
    // Mat patch_index_image = Popup::drawIndexMaskImage(test, IMAGE_WIDTH, IMAGE_HEIGHT);
    // imwrite("Test/test.png", patch_index_image);
    
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      int enforced_patch_index = enforced_patch_index_mask[pixel];
      int patch_index = patch_index_mask[pixel];
      if (enforced_patch_index >= 0 && enforced_patch_index < 10000) {
        int x = pixel % IMAGE_WIDTH;
	int y = pixel / IMAGE_WIDTH;
	for (int delta_x = -(WINDOW_WIDTH - 1) / 2; delta_x <= (WINDOW_WIDTH - 1) / 2; delta_x++)
	  for (int delta_y = -(WINDOW_HEIGHT - 1) / 2; delta_y <= (WINDOW_HEIGHT - 1) / 2; delta_y++)
	    if (x + delta_x >= 0 && x + delta_x < IMAGE_WIDTH && y + delta_y >= 0 && y + delta_y < IMAGE_HEIGHT)
	      fold_line_window_pixel_mask[(y + delta_y) * IMAGE_WIDTH + (x + delta_x)] = true;
      }
      
      // if (enforced_patch_index >= NUM_PATCHES) {
      // 	cout << "enforced: " << enforced_patch_index << '\t' << NUM_PATCHES << endl;
      // 	exit(1);
      // }
      // if (patch_index >= NUM_PATCHES) {
      //   cout << "original: " << patch_index << '\t' << NUM_PATCHES << endl;
      //   exit(1);
      // }
      
      // if (enforced_patch_index >= 10000) {
      //        vector<int> cost(NUM_PATCHES, ENCOURAGED_DATA_WEIGHT);      
      //        cost[enforced_patch_index - 10000] = 0;      
      //        data_cost.insert(data_cost.end(), cost.begin(), cost.end());
      // } else
      if (enforced_patch_index != -1 && enforced_patch_index < 10000) {
        vector<int> cost(NUM_PATCHES, ENFORCED_DATA_WEIGHT);
        cost[enforced_patch_index] = 0;
        data_cost.insert(data_cost.end(), cost.begin(), cost.end());
      } else if (enforced_patch_index >= 10000) {
	vector<int> cost(NUM_PATCHES, ENCOURAGED_DATA_WEIGHT);      
	cost[enforced_patch_index - 10000] = 0;      
	data_cost.insert(data_cost.end(), cost.begin(), cost.end());
      } else if (patch_index != -1) {
	vector<int> cost(NUM_PATCHES, DATA_WEIGHT);
        cost[patch_index] = 0;
	data_cost.insert(data_cost.end(), cost.begin(), cost.end());
      } else {
	vector<int> cost(NUM_PATCHES, 0);
	data_cost.insert(data_cost.end(), cost.begin(), cost.end());
      }
    }
    vector<int> smoothness_cost(NUM_PATCHES * NUM_PATCHES, SMOOTHNESS_WEIGHT);
    for (int label = 0; label < NUM_PATCHES; label++)
      smoothness_cost[label * NUM_PATCHES + label] = 0;

    unique_ptr<DataCost> data(new DataCost(&data_cost[0]));
    unique_ptr<SmoothnessCost> smoothness(new SmoothnessCost(&smoothness_cost[0]));
    unique_ptr<EnergyFunction> energy(new EnergyFunction(data.get(), smoothness.get()));
    
    unique_ptr<Expansion> mrf(new Expansion(IMAGE_WIDTH * IMAGE_HEIGHT, NUM_PATCHES, energy.get()));
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
      for (int x = 0; x < IMAGE_WIDTH; x++) {
        int pixel = y * IMAGE_WIDTH + x;
	if (fold_line_window_pixel_mask[pixel] == false)
	  continue;
        if (x > 0 && true)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel - 1])
            mrf->setNeighbors(pixel, pixel - 1, 1);
        if (x < IMAGE_WIDTH - 1)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel + 1])
	    mrf->setNeighbors(pixel, pixel + 1, 1);
        if (y > 0 && true)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel - IMAGE_WIDTH])
	    mrf->setNeighbors(pixel, pixel - IMAGE_WIDTH, 1);
        if (y < IMAGE_HEIGHT - 1)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel + IMAGE_WIDTH])
	    mrf->setNeighbors(pixel, pixel + IMAGE_WIDTH, 1);
        if (x > 0 && y > 0 && true)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel - IMAGE_WIDTH - 1])
            mrf->setNeighbors(pixel, pixel - IMAGE_WIDTH - 1, 1);
        if (x < IMAGE_WIDTH - 1 && y > 0 && true)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel - IMAGE_WIDTH + 1])
	    mrf->setNeighbors(pixel, pixel - IMAGE_WIDTH + 1, 1);
        if (x > 0 && y < IMAGE_HEIGHT - 1)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel + IMAGE_WIDTH - 1])
	    mrf->setNeighbors(pixel, pixel + IMAGE_WIDTH - 1, 1);
        if (x < IMAGE_WIDTH - 1 && y < IMAGE_HEIGHT - 1)
	  //if (patch_index_mask[pixel] == patch_index_mask[pixel + IMAGE_WIDTH + 1])
	    mrf->setNeighbors(pixel, pixel + IMAGE_WIDTH + 1, 1);
      }
    }

    mrf->initialize();
    mrf->clearAnswer();
            
    int initial_energy = mrf->totalEnergy();
    for (int iter = 0; iter < min(NUM_PATCHES, 1); iter++) {
      float running_time;
      mrf->optimize(1, running_time);
    }
    deformed_patch_index_mask = vector<int>(mrf->getAnswerPtr(), mrf->getAnswerPtr() + IMAGE_WIDTH * IMAGE_HEIGHT);
    //for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++)
    //cout << deformed_patch_index_mask[pixel] << endl;
  }
}
