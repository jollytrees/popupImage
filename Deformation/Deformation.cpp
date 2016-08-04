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
  ifstream patch_index_mask_in_str("Test/bee_segmentation.txt");
  int IMAGE_WIDTH, IMAGE_HEIGHT;
  patch_index_mask_in_str >> IMAGE_WIDTH >> IMAGE_HEIGHT;
  vector<int> patch_index_mask(IMAGE_WIDTH * IMAGE_HEIGHT);
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++)
    patch_index_mask_in_str >> patch_index_mask[pixel];
  patch_index_mask_in_str.close();

  //Mat test_image = Popup::drawIndexMaskImage(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT);
  //imwrite("Test/patch_index_mask_image.png", test_image);
  //exit(1);

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

  set<int> island_patches;
  if (true) {
    //Mat toy_example_image = imread("Results/cow/patch_index_mask_image.png");
    Mat toy_example_image = imread("Results/bear/patch_index_mask_image.png");
    //Mat toy_example_image = imread("Results/fox/patch_index_mask_image.png");
    //Mat toy_example_image = imread("Results/goat/patch_index_mask_image.png");
    //Mat toy_example_image = imread("Results/angrybird/patch_index_mask_image.png");
    //Mat toy_example_image = imread("Results/baby/patch_index_mask_image.png");
    //Mat toy_example_image = imread("Results/bee/patch_index_mask_image.png");
    IMAGE_WIDTH = toy_example_image.cols;
    IMAGE_HEIGHT = toy_example_image.rows;
    patch_index_mask.resize(toy_example_image.cols * toy_example_image.rows);
    map<int, int> color_index_map;
    int index = 0;
    for (int pixel = 0; pixel < toy_example_image.cols * toy_example_image.rows; pixel++) {
      Vec3b color = toy_example_image.at<Vec3b>(pixel / toy_example_image.cols, pixel % toy_example_image.cols);
      int color_index = color[0] * 256 * 256 + color[1] * 256 + color[2];
      if (color_index_map.count(color_index) == 0) {
	if (color[0] == color[1] && color[0] == color[2])
	  island_patches.insert(index);
	color_index_map[color_index] = index++;
      }
      patch_index_mask[pixel] = color_index_map[color_index];	
    }
    
    if (false) {
      const int SCALE = 3;
      patch_index_mask = Popup::zoomMask(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_WIDTH * SCALE, IMAGE_HEIGHT * SCALE);
      IMAGE_WIDTH *= SCALE;
      IMAGE_HEIGHT *= SCALE;
      Mat test_image = Popup::drawIndexMaskImage(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT);
      imwrite("Test/patch_index_mask_image.png", test_image);
      exit(1);
    }
  }

  const bool ENFORCE_SYMMETRY = true;
  const int MIDDLE_FOLD_LINE_X = IMAGE_WIDTH / 2 - 10;
  Popup::PopupGraph popup_graph(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, MIDDLE_FOLD_LINE_X, island_patches, ENFORCE_SYMMETRY, false);
  vector<vector<int> > excluded_fold_line_combinations;
  int num_new_fold_lines_constraint = 0;
  int index = 0;
  bool is_stable = false;
  while (true) {
    if (optimizeFoldLines(popup_graph, excluded_fold_line_combinations, num_new_fold_lines_constraint, 'T') == false) {
      num_new_fold_lines_constraint++;
      excluded_fold_line_combinations.clear();
      continue;
    }
    Mat optimized_popup_graph_image = popup_graph.drawOptimizedPopupGraph();
    imwrite("Test/optimized_popup_graph_" + to_string(index) + ".png", optimized_popup_graph_image);
    if (optimizeFoldLines(popup_graph, excluded_fold_line_combinations, num_new_fold_lines_constraint, 'S')) {
      is_stable = true;
      break;
    }
    vector<int> new_fold_lines = popup_graph.getNewFoldLines();
    num_new_fold_lines_constraint = new_fold_lines.size();
    if (num_new_fold_lines_constraint == 0)
      num_new_fold_lines_constraint++;
    else
      excluded_fold_line_combinations.push_back(new_fold_lines);
    cout << "constraint: " << num_new_fold_lines_constraint << endl;
    for (vector<int>::const_iterator fold_line_it = new_fold_lines.begin(); fold_line_it != new_fold_lines.end(); fold_line_it++)
      cout << *fold_line_it << '\t';
    cout << endl;
    index++;
    if (index == 10)
      break;
  }
  //optimizeFoldLines(popup_graph, excluded_fold_line_combinations, num_new_fold_lines_constraint, true);
  //vector<FoldLine> optimized_fold_lines = popup_graph.getFoldLines();

  if (is_stable == false) {
    cout << "unstable" << endl;
    exit(1);
  }
  Popup::PopupGraph complete_popup_graph(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, MIDDLE_FOLD_LINE_X, set<int>(), ENFORCE_SYMMETRY, true);
  complete_popup_graph.addOptimizedInfo(popup_graph);
  if (optimizeFoldLines(complete_popup_graph, excluded_fold_line_combinations, num_new_fold_lines_constraint, 'C') == false) {
    cout << "Cannot complete graph" << endl;
    exit(1);
  }
  Mat optimized_popup_graph_image = complete_popup_graph.drawOptimizedPopupGraph();  
  imwrite("Test/optimized_popup_graph.png", optimized_popup_graph_image);  

  complete_popup_graph.writeRenderingInfo();
  
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
