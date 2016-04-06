#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <tuple>

#include "MRF2.2/mrf.h"
#include "MRF2.2/GCoptimization.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "FoldLineOptimization.h"
#include "PopupUtils.h"

using namespace std;
using namespace cv;




int main(int argc, char *argv[])
{
  //read patch index mask
  ifstream patch_index_mask_in_str("Test/bear_segmentation.txt");
  int IMAGE_WIDTH, IMAGE_HEIGHT;
  patch_index_mask_in_str >> IMAGE_WIDTH >> IMAGE_HEIGHT;
  vector<int> patch_index_mask(IMAGE_WIDTH * IMAGE_HEIGHT);
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++)
    patch_index_mask_in_str >> patch_index_mask[pixel];
  patch_index_mask_in_str.close();

  //remove boundary pixels
  while (true) {
    bool has_boundary_pixels = false;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (patch_index_mask[pixel] != -1)
        continue;
      has_boundary_pixels = true;
      vector<int> neighbor_pixels;
      if (pixel % IMAGE_WIDTH < IMAGE_WIDTH / 2) {
	if (pixel % IMAGE_WIDTH > 0)
	  neighbor_pixels.push_back(pixel - 1);
	else
	  neighbor_pixels.push_back(pixel + 1);
      } else {
	if (pixel % IMAGE_WIDTH < IMAGE_WIDTH - 1)
	  neighbor_pixels.push_back(pixel + 1);
	else
	  neighbor_pixels.push_back(pixel - 1);
      }
      if (pixel / IMAGE_WIDTH == 0)
	neighbor_pixels.push_back(pixel + IMAGE_WIDTH);
      else if (pixel / IMAGE_WIDTH == IMAGE_HEIGHT - 1)
	neighbor_pixels.push_back(pixel - IMAGE_WIDTH);
      for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++)
        if (patch_index_mask[*neighbor_pixel_it] != -1)
          patch_index_mask[pixel] = patch_index_mask[*neighbor_pixel_it];
    }
    if (has_boundary_pixels == false)
      break;
  }
  
  //make index start from 0
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++)
    patch_index_mask[pixel]--;
  
  
  const int FOLD_LINE_WINDOW_WIDTH = 10;
  const int FOLD_LINE_WINDOW_HEIGHT = 10;

  //make patch index mask larger
  const int SCALE = 3;
  patch_index_mask = Popup::zoomMask(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_WIDTH * SCALE, IMAGE_HEIGHT * SCALE);
  IMAGE_WIDTH *= SCALE;
  IMAGE_HEIGHT *= SCALE;
  //cout << IMAGE_WIDTH << '\t' << IMAGE_HEIGHT << '\t' << patch_index_mask.size() << endl;
  //exit(1);
  
  if (false) {
    Mat toy_example_image = imread("Test/toy_example_2.png");
    IMAGE_WIDTH = toy_example_image.cols;
    IMAGE_HEIGHT = toy_example_image.rows;
    patch_index_mask.resize(toy_example_image.cols * toy_example_image.rows);
    map<int, int> color_index_map;
    int index = 0;
    for (int pixel = 0; pixel < toy_example_image.cols * toy_example_image.rows; pixel++) {
      Vec3b color = toy_example_image.at<Vec3b>(pixel / toy_example_image.cols, pixel % toy_example_image.cols);
      int intensity = (color[0] + color[1] + color[2]) / 3;
      if (color_index_map.count(intensity) == 0)
        color_index_map[intensity] = index++;
      patch_index_mask[pixel] = color_index_map[intensity];
    }
  }
  
  Popup::PopupGraph popup_graph(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, IMAGE_WIDTH / 2);
  optimizeFoldLines(popup_graph);
  
  //cout << *popup_graph.background_patches.begin() << endl;
  //exit(1);
  
  // if (false) {
  //   for (map<int, map<int, set<int> > >::const_iterator patch_it_1 = popup_graph.original_patch_fold_lines.begin(); patch_it_1 != popup_graph.original_patch_fold_lines.end(); patch_it_1++) {
  //     for (map<int, set<int> >::const_iterator patch_it_2 = patch_it_1->second.begin(); patch_it_2 != patch_it_1->second.end(); patch_it_2++) {
  // 	for (set<int>::const_iterator fold_line_it = patch_it_2->second.begin(); fold_line_it != patch_it_2->second.end(); fold_line_it++)
  //         cout << patch_it_1->first << '\t' << patch_it_2->first << '\t' << *fold_line_it << endl;
  //     }
  //   }
  //   exit(1);
  // }
  

  // for (map<int, map<int, set<int> > >::const_iterator patch_it_1 = popup_graph.original_patch_fold_lines.begin(); patch_it_1 != popup_graph.original_patch_fold_lines.end(); patch_it_1++)
  //   for (map<int, set<int> >::const_iterator patch_it_2 = patch_it_1->second.begin(); patch_it_2 != patch_it_1->second.end(); patch_it_2++)
  //     for (set<int>::const_iterator fold_line_it = patch_it_2->second.begin(); fold_line_it != patch_it_2->second.end(); fold_line_it++)
  // 	popup_graph.original_patch_fold_lines[patch_it_2->first][patch_it_1->first].insert(*fold_line_it);
  
  
  //Mat fold_line_image = segmented_patch_image.clone();
  // map<int, Vec3b> color_table;
  // for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
  //   int patch_index = patch_index_mask[pixel];
  //   if (color_table.count(patch_index) == 0) {
  //     //color_table[patch_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
  //     int gray_value = rand() % 256;
  //     color_table[patch_index] = Vec3b(gray_value, gray_value, gray_value);
  //   }
  //   fold_line_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = color_table[patch_index];
  // }

  // for (int patch_index_1 = 0; patch_index_1 < num_patches; patch_index_1++) {
  //   for (int patch_index_2 = 0; patch_index_2 < num_patches; patch_index_2++) {
  //     int patch_pair_index = patch_index_1 * num_patches + patch_index_2;
  //     for (vector<int>::const_iterator position_it = patch_pair_fold_line_positions[patch_pair_index].begin(); position_it != patch_pair_fold_line_positions[patch_pair_index].end(); position_it++) {
  //       int x = *position_it % IMAGE_WIDTH;
  //       int y = *position_it / IMAGE_WIDTH;
  //       line(fold_line_image, Point(x, y - FOLD_LINE_WINDOW_HEIGHT / 2), Point(x, y + FOLD_LINE_WINDOW_HEIGHT / 2), Scalar(0, 0, 255));
  //       putText(fold_line_image, to_string(patch_index_1) + " " + to_string(patch_index_2), Point(x, y), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
  //     }
  //   }
  // }
  // imwrite("Test/" + image_name + "_fold_line.bmp", fold_line_image);
  
  return 0;
}
