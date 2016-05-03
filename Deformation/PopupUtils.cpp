#include "PopupUtils.h"

#include <cmath>
#include <map>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

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
}
