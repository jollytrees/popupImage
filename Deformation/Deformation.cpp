#include <vector>
#include <iostream>
#include <sstream>

#include "MRF2.2/mrf.h"
#include "MRF2.2/GCoptimization.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;


bool deformPatchesAtIntersection(const Mat &patch_image, const int left_patch_index, const int right_patch_index, const int fold_line_x, const int intersection_y, Mat &deformed_patch_image, double &deformation_cost, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, const int MINIMUM_FOLD_LINE_WINDOW_WIDTH, const int MINIMUM_FOLD_LINE_WINDOW_HEIGHT)
{
  const int WINDOW_WIDTH = FOLD_LINE_WINDOW_WIDTH + 2;
  const int WINDOW_HEIGHT = FOLD_LINE_WINDOW_HEIGHT + 2;

  bool left_patch_exists = false;
  bool right_patch_exists = false;
  vector<vector<int> > window_patch_indices(WINDOW_WIDTH, vector<int>(WINDOW_HEIGHT, 2));
  for (int delta_y = -WINDOW_HEIGHT / 2; delta_y < WINDOW_HEIGHT / 2; delta_y++) {
    for (int delta_x = -WINDOW_WIDTH / 2; delta_x < WINDOW_WIDTH / 2; delta_x++) {
      if (fold_line_x + delta_x < 0 || fold_line_x + delta_x >= patch_image.cols || intersection_y + delta_y < 0 || intersection_y + delta_y >= patch_image.rows)
	continue;
      int patch_index = patch_image.at<uchar>(intersection_y + delta_y, fold_line_x + delta_x);
      if (patch_index == left_patch_index) {
	window_patch_indices[delta_x + WINDOW_WIDTH / 2][delta_y + WINDOW_HEIGHT / 2] = 0;
	if (delta_x < 0)
	  left_patch_exists = true;
      } else if (patch_index == right_patch_index) {
        window_patch_indices[delta_x + WINDOW_WIDTH / 2][delta_y + WINDOW_HEIGHT / 2] = 1;
	if (delta_x >= 0)
	  right_patch_exists = true;
      }
    }
  }
  if (left_patch_exists == false || right_patch_exists == false)
    return false;

  
  const int FIXED_PIXEL_INCONSISTENCY_COST = 1000000;
  const int SMOOTHNESS_WEIGHT = 10000;
  const int DATA_WEIGHT = 1000;
  //We may want to change the following values to better compute scores.
  const int FOLD_LINE_INCONSISTENCY_COST = DATA_WEIGHT * (1.0 / FOLD_LINE_WINDOW_HEIGHT);
  const int REGION_INCONSISTENCY_COST = DATA_WEIGHT * (1.0 / (MINIMUM_FOLD_LINE_WINDOW_WIDTH * MINIMUM_FOLD_LINE_WINDOW_HEIGHT));
  const int REGION_COVERAGE_COST = DATA_WEIGHT * (1.0 / (FOLD_LINE_WINDOW_WIDTH * FOLD_LINE_WINDOW_HEIGHT));
  
  vector<int> data_cost;
  for (int y = 0; y < WINDOW_HEIGHT; y++) {
    for (int x = 0; x < WINDOW_WIDTH; x++) {
      if (x == 0 || x == WINDOW_WIDTH - 1 || y == 0 || y == WINDOW_HEIGHT - 1) {
	vector<int> cost(3, FIXED_PIXEL_INCONSISTENCY_COST);
	cost[window_patch_indices[x][y]] = 0;
	data_cost.insert(data_cost.end(), cost.begin(), cost.end());
	continue;
      }
      vector<int> cost(3, 0);
      if (window_patch_indices[x][y] == 2) {
	//           || (window_patch_indices[x][y] == 0 && x < FOLD_LINE_REGION_WIDTH_THRESHOLD)
	//	  || (window_patch_indices[x][y] == 1 && x > FOLD_LINE_REGION_WIDTH_THRESHOLD + 1)) {
	for (int label = 0; label < 3; label++)
	  if (label != window_patch_indices[x][y])
	    cost[label] = FIXED_PIXEL_INCONSISTENCY_COST;
      } else {
	for (int label = 0; label < 3; label++)
          if (label != window_patch_indices[x][y])
            cost[label] = REGION_INCONSISTENCY_COST;
      }
      if (x == FOLD_LINE_WINDOW_WIDTH / 2) {
	if (window_patch_indices[x][y] == 2) {
	  cost[1] += FOLD_LINE_INCONSISTENCY_COST;
	  cost[2] += FOLD_LINE_INCONSISTENCY_COST;
	} else {
	  cost[1] += FIXED_PIXEL_INCONSISTENCY_COST;
          cost[2] += FIXED_PIXEL_INCONSISTENCY_COST;
	}
      } else if (x == FOLD_LINE_WINDOW_WIDTH / 2 + 1) {
	if (window_patch_indices[x][y] == 2) {
	  cost[0] += FOLD_LINE_INCONSISTENCY_COST;
	  cost[2] += FOLD_LINE_INCONSISTENCY_COST;
	} else {
	  cost[0] += FIXED_PIXEL_INCONSISTENCY_COST;
          cost[2] += FIXED_PIXEL_INCONSISTENCY_COST;
	}
      }
      if (x <= FOLD_LINE_WINDOW_WIDTH / 2) {
	cost[1] += REGION_COVERAGE_COST;
        cost[2] += REGION_COVERAGE_COST;
      } else {
	cost[0] += REGION_COVERAGE_COST;
	cost[2] += REGION_COVERAGE_COST;
      }
      data_cost.insert(data_cost.end(), cost.begin(), cost.end());
    }
  }

  if (false) {
    cout << intersection_y << endl;
    for (int y = 0; y < WINDOW_HEIGHT; y++) {
      for (int x = 0; x < WINDOW_WIDTH; x++) {
	cout << x << '\t' << y << endl;
	for (int label = 0; label < 3; label++)
	  cout << data_cost[(y * WINDOW_WIDTH + x) * 3 + label] << '\t';
	cout << endl;
      }
    }
    exit(1);
  }
  
  vector<int> smoothness_cost(9, SMOOTHNESS_WEIGHT);
  for (int label = 0; label < 3; label++)
    smoothness_cost[label * 3 + label] = 0;

  unique_ptr<DataCost> data(new DataCost(&data_cost[0]));
  unique_ptr<SmoothnessCost> smoothness(new SmoothnessCost(&smoothness_cost[0]));
  unique_ptr<EnergyFunction> energy(new EnergyFunction(data.get(), smoothness.get()));

  unique_ptr<Expansion> mrf(new Expansion(WINDOW_WIDTH * WINDOW_HEIGHT, 3, energy.get()));
  for (int y = 0; y < WINDOW_HEIGHT; y++) {
    for (int x = 0; x < WINDOW_WIDTH; x++) {
      int pixel = y * WINDOW_WIDTH + x;
      if (x > 0)
        mrf->setNeighbors(pixel, pixel - 1, 1);
      if (x < WINDOW_WIDTH - 1)
	mrf->setNeighbors(pixel, pixel + 1, 1);
      if (y > 0)
	mrf->setNeighbors(pixel, pixel - WINDOW_WIDTH, 1);
      if (y < WINDOW_HEIGHT - 1)
	mrf->setNeighbors(pixel, pixel + WINDOW_WIDTH, 1);
      if (x > 0 && y > 0)
	mrf->setNeighbors(pixel, pixel - WINDOW_WIDTH - 1, 1);
      if (x < WINDOW_WIDTH - 1 && y > 0)
        mrf->setNeighbors(pixel, pixel - WINDOW_WIDTH + 1, 1);
      if (x > 0 && y < WINDOW_HEIGHT - 1)
        mrf->setNeighbors(pixel, pixel + WINDOW_WIDTH - 1, 1);
      if (x < WINDOW_WIDTH - 1 && y < WINDOW_HEIGHT - 1)
        mrf->setNeighbors(pixel, pixel + WINDOW_WIDTH + 1, 1);
    }
  }

  mrf->initialize();
  mrf->clearAnswer();
            
  int initial_energy = mrf->totalEnergy();
  for (int iter = 0; iter < 2; iter++) {
    float running_time;
    mrf->optimize(1, running_time);
    //    cout << mrf->totalEnergy() << endl;
  }
  vector<int> solution_labels(mrf->getAnswerPtr(), mrf->getAnswerPtr() + WINDOW_WIDTH * WINDOW_HEIGHT);
  
  deformed_patch_image = patch_image.clone();
  for (int delta_y = -WINDOW_HEIGHT / 2; delta_y < WINDOW_HEIGHT / 2; delta_y++) {
    for (int delta_x = -WINDOW_WIDTH / 2; delta_x < WINDOW_WIDTH / 2; delta_x++) {
      if (fold_line_x + delta_x < 0 || fold_line_x + delta_x >= patch_image.cols || intersection_y + delta_y < 0 || intersection_y + delta_y >= patch_image.rows)
        continue;
      int label = solution_labels[(delta_y + WINDOW_HEIGHT / 2) * WINDOW_WIDTH + (delta_x + WINDOW_WIDTH / 2)];
      if (label == 0)
	deformed_patch_image.at<uchar>(intersection_y + delta_y, fold_line_x + delta_x) = left_patch_index;
      else if (label == 1)
        deformed_patch_image.at<uchar>(intersection_y + delta_y, fold_line_x + delta_x) = right_patch_index;
    }
  }
  //double max_data_cost = FOLD_LINE_HEIGHT_THRESHOLD * 2 * 2 * FOLD_LINE_INCONSISTENCY_COST + (WINDOW_WIDTH - 2) * (WINDOW_HEIGHT - 2) * REGION_COVERAGE_COST + (WINDOW_WIDTH - 2) * (WINDOW_HEIGHT - 2) * REGION_INCONSISTENCY_COST;
  double max_data_cost = DATA_WEIGHT * 3;
  deformation_cost = min(mrf->dataEnergy() / max_data_cost, 1.0);
  cout << "cost: " << deformation_cost << endl;
  return true;

  // int num_changed_pixels = 0;
  // int num_left_patch_pixels = 0;
  // int num_right_patch_pixels = 0;
  // int fold_line_length = 0;
  // for (int y = 1; y < WINDOW_HEIGHT - 1; delta_y++) {
  //   for (int x = 1; x < WINDOW_WIDTH - 1; delta_x++) {
  //     int label = solution_labels[y * WINDOW_WIDTH + x];
  //     if (label != window_patch_indices[y][x])
  // 	num_changed_pixels++;
  //     if (label == 0 && x <= FOLD_LINE_REGION_WIDTH_THRESHOLD)
  // 	num_left_patch_pixels++;
  //     if (label == 1 && x > FOLD_LINE_REGION_WIDTH_THRESHOLD)
  //       num_right_patch_pixels++;
  //   }
  //   if (window_patch_indices[y * WINDOW_WIDTH + FOLD_LINE_REGION_WIDTH_THRESHOLD] == left_patch_index && window_patch_indices[y * WINDOW_WIDTH + FOLD_LINE_REGION_WIDTH_THRESHOLD + 1] == right_patch_index)
  //     fold_line_length++;
  // }

  // const double SHAPE_CHANGE_WEIGHT = 2;
  // const double REGION_COVERAGE_WEIGHT = 1;
  // const double LENGTH_COVERAGE_WEIGHT = 2;
  // double shape_change_cost = 1.0 * num_change_pixels / ((WINDOW_WIDTH - 2) * (WINDOW_HEIGHT - 2));
  // double region_coverage_cost = 1.0 * ((WINDOW_WIDTH - 2) * (WINDOW_HEIGHT - 2) - num_left_patch_pixels - num_right_patch_pixels) / ((WINDOW_WIDTH - 2) * (WINDOW_HEIGHT - 2));
  // double length_coverage_cost = 1.0 * (FOLD_LINE_HEIGHT_THRESHOLD * 2 - fold_line_length) / (FOLD_LINE_HEIGHT_THRESHOLD * 2);
  // double cost = (shape_change_cost * SHAPE_CHANGE_WEIGHT + region_coverage_cost * REGION_COVERAGE_WEIGHT + length_coverage_cost * LENGTH_COVERAGE_WEIGHT) / (SHAPE_CHANGE_WEIGHT + REGION_COVERAGE_WEIGHT + LENGTH_COVERAGE_WEIGHT);
  // return cost;
}

vector<int> findFoldLineWindowSize(const Mat &patch_image, const int left_patch_index, const int right_patch_index, const int fold_line_x, const vector<int> &region)
{
  assert(region.size() > 0);
  int patch_index = patch_image.at<uchar>(*region.begin(), fold_line_x);
  assert(patch_index == left_patch_index || patch_index == right_patch_index);

  const int IMAGE_WIDTH = patch_image.cols;
  const int IMAGE_HEIGHT = patch_image.rows;
  
  vector<bool> visited_pixel_mask(IMAGE_WIDTH * IMAGE_HEIGHT, false);
  vector<int> window_pixels;
  vector<int> border_pixels;
  for (vector<int>::const_iterator y_it = region.begin(); y_it != region.end(); y_it++) {
    int pixel = *y_it * IMAGE_WIDTH + fold_line_x;
    visited_pixel_mask[pixel] = true;
    window_pixels.push_back(pixel);
    if (patch_index == left_patch_index && fold_line_x < IMAGE_WIDTH - 1 && patch_image.at<uchar>(*y_it, fold_line_x + 1) == patch_index)
	border_pixels.push_back(pixel + 1);
    if (patch_index == right_patch_index && fold_line_x > 0 && patch_image.at<uchar>(*y_it, fold_line_x - 1) == patch_index)
      border_pixels.push_back(pixel - 1);
  }
  while (border_pixels.size() > 0) {
    vector<int> new_border_pixels;
    for (vector<int>::const_iterator border_pixel_it = border_pixels.begin(); border_pixel_it != border_pixels.end(); border_pixel_it++) {
      //      cout << *border_pixel_it << endl;
      visited_pixel_mask[*border_pixel_it] = true;
      window_pixels.push_back(*border_pixel_it);

      int pixel = *border_pixel_it;
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
      for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
	if (visited_pixel_mask[*neighbor_pixel_it] || patch_image.at<uchar>(*neighbor_pixel_it / IMAGE_WIDTH, *neighbor_pixel_it % IMAGE_WIDTH) != patch_index)
	  continue;
	new_border_pixels.push_back(*neighbor_pixel_it);
	visited_pixel_mask[*neighbor_pixel_it] = true;
      }
    }
    border_pixels = new_border_pixels;
  }

  int window_width = 0;
  int window_height = 0;
  double sum_y = 0;
  for (vector<int>::const_iterator y_it = region.begin(); y_it != region.end(); y_it++)
    sum_y += *y_it;
  int fold_line_y = ceil(sum_y / region.size());
  for (vector<int>::const_iterator window_pixel_it = window_pixels.begin(); window_pixel_it != window_pixels.end(); window_pixel_it++) {
    if (abs(*window_pixel_it % IMAGE_WIDTH - fold_line_x) + 1 > window_width)
      window_width = abs(*window_pixel_it % IMAGE_WIDTH - fold_line_x) + 1;
    if (abs(*window_pixel_it / IMAGE_WIDTH - fold_line_y) + 1 > window_height)
      window_height = abs(*window_pixel_it / IMAGE_WIDTH - fold_line_y) + 1;
  }
  vector<int> window_size;
  window_size.push_back(window_width * 2);
  window_size.push_back(window_height * 2);
  //cout << window_width << '\t' << window_height << endl;
  //exit(1);
  return window_size;
}

//Input: patch_image is a gray scale image with the gray value of each pixel representing the patch index. patch_index_1 and patch_index_2 are indices for two patch between which the fold line is going to add. fold_line_x is the denoted x position of the fold line. MINIMUM_FOLD_LINE_WINDOW_WIDTH and MINIMUM_FOLD_LINE_WINDOW_HEIGHT are desired fold line region width and fold line length (smaller values will pay penalties).
//Output: optimal_deformed_patch_image is the image after deformation. optimal_deformation_cost is the cost for the deformation.
bool deformPatchesBesideFoldLine(const Mat &patch_image, const int patch_index_1, const int patch_index_2, const int fold_line_x, Mat &optimal_deformed_patch_image, double &optimal_deformation_cost, const int MINIMUM_FOLD_LINE_WINDOW_WIDTH, const int MINIMUM_FOLD_LINE_WINDOW_HEIGHT)
{
  const int IMAGE_WIDTH = patch_image.cols;
  const int IMAGE_HEIGHT = patch_image.rows;

  int patch_min_x_1 = IMAGE_WIDTH;
  int patch_max_x_1 = 0;
  int patch_min_x_2 = IMAGE_WIDTH;
  int patch_max_x_2 = 0;
  for (int y = 0; y < IMAGE_HEIGHT; y++) {
    for (int x = 0; x < IMAGE_WIDTH; x++) {
      int patch_index = patch_image.at<uchar>(y, x);
      if (patch_index == patch_index_1) {
	if (x < patch_min_x_1)
	  patch_min_x_1 = x;
	if (x > patch_max_x_1)
	  patch_max_x_1 = x;
      }
      if (patch_index == patch_index_2) {
        if (x < patch_min_x_2)
          patch_min_x_2 = x;
        if (x > patch_max_x_2)
          patch_max_x_2 = x;
      }
    }
  }

  vector<vector<int> > patch_regions_1;
  vector<vector<int> > patch_regions_2;
  vector<double> intersections;

  bool previous_intersection_exists = false;
  int previous_patch_index = -1;
  int previous_patch_y = -1;
  vector<int> region;
  for (int y = 0; y < IMAGE_HEIGHT; y++) {
    int patch_index = patch_image.at<uchar>(y, fold_line_x);
    if (patch_index == previous_patch_index) {
      region.push_back(y);
      previous_patch_y = y;
      continue;
    }
    if (previous_patch_index == patch_index_1)
      patch_regions_1.push_back(region);
    if (previous_patch_index == patch_index_2)
      patch_regions_2.push_back(region);
    
    if (previous_patch_index == patch_index_1 && patch_index == patch_index_2) {
      intersections.push_back(round((previous_patch_y + y) * 0.5));
      //previous_intersection_exists = true;
    } else if (previous_patch_index == patch_index_2 && patch_index == patch_index_1) {
      intersections.push_back(round((previous_patch_y + y) * 0.5));
      //previous_intersection_exists = true;
    }
    region.clear();
    region.push_back(y);
    previous_patch_index = patch_index;
    previous_patch_y = y;
  }
  if (previous_patch_index == patch_index_1)
    patch_regions_1.push_back(region);
  if (previous_patch_index == patch_index_2)
    patch_regions_2.push_back(region);
  

  optimal_deformation_cost = 1000000;
  bool deformation_exists = false;

  vector<vector<int> > patch_regions;
  patch_regions.insert(patch_regions.end(), patch_regions_1.begin(), patch_regions_1.end());
  patch_regions.insert(patch_regions.end(), patch_regions_2.begin(), patch_regions_2.end());
  //cout << intersections.size() << '\t' << patch_regions.size() << endl;
  optimal_deformation_cost = 1000000;
  for (vector<vector<int> >::const_iterator region_it = patch_regions.begin(); region_it != patch_regions.end(); region_it++) {
    double sum_y = 0;
    for (vector<int>::const_iterator y_it = region_it->begin(); y_it != region_it->end(); y_it++)
      sum_y += *y_it;
    int intersection_y = ceil(sum_y / region_it->size());
    {
      double deformation_cost;
      Mat deformed_patch_image;
      vector<int> window_size = findFoldLineWindowSize(patch_image, patch_index_1, patch_index_2, fold_line_x, *region_it);
      int fold_line_window_width = max(MINIMUM_FOLD_LINE_WINDOW_WIDTH, window_size[0]);
      int fold_line_window_height = max(MINIMUM_FOLD_LINE_WINDOW_HEIGHT, window_size[1]);
      //cout << fold_line_window_width << '\t' << fold_line_window_height << endl;
      bool success = deformPatchesAtIntersection(patch_image, patch_index_1, patch_index_2, fold_line_x, intersection_y, deformed_patch_image, deformation_cost, fold_line_window_width, fold_line_window_height, MINIMUM_FOLD_LINE_WINDOW_WIDTH, MINIMUM_FOLD_LINE_WINDOW_HEIGHT);
      if (success && deformation_cost < optimal_deformation_cost) {
        optimal_deformed_patch_image = deformed_patch_image.clone();
        optimal_deformation_cost = deformation_cost;
        deformation_exists = true;
      }
      break;
    }
    {
      double deformation_cost;
      Mat deformed_patch_image;
      vector<int> window_size = findFoldLineWindowSize(patch_image, patch_index_2, patch_index_1, fold_line_x, *region_it);
      int fold_line_window_width = max(MINIMUM_FOLD_LINE_WINDOW_WIDTH, window_size[0]);
      int fold_line_window_height = max(MINIMUM_FOLD_LINE_WINDOW_HEIGHT, window_size[1]);
      //cout << fold_line_window_width << '\t' << fold_line_window_height << endl;
      bool success = deformPatchesAtIntersection(patch_image, patch_index_2, patch_index_1, fold_line_x, intersection_y, deformed_patch_image, deformation_cost, fold_line_window_width, fold_line_window_height, MINIMUM_FOLD_LINE_WINDOW_WIDTH, MINIMUM_FOLD_LINE_WINDOW_HEIGHT);
      if (success && deformation_cost < optimal_deformation_cost) {
        optimal_deformed_patch_image = deformed_patch_image.clone();
        optimal_deformation_cost = deformation_cost;
        deformation_exists = true;
      }
    }
  }
  // if (deformation_exists) {
  //   //refineDeformedImage(deformed_image, fold_line_x);
  //   return true;
  // }

  for (vector<double>::const_iterator intersection_it = intersections.begin(); intersection_it != intersections.end(); intersection_it++) {
    {
      double deformation_cost;
      Mat deformed_patch_image;
      bool success = deformPatchesAtIntersection(patch_image, patch_index_1, patch_index_2, fold_line_x, ceil(*intersection_it), deformed_patch_image, deformation_cost, MINIMUM_FOLD_LINE_WINDOW_WIDTH, MINIMUM_FOLD_LINE_WINDOW_HEIGHT, MINIMUM_FOLD_LINE_WINDOW_WIDTH, MINIMUM_FOLD_LINE_WINDOW_HEIGHT);
      if (success && deformation_cost < optimal_deformation_cost) {
	optimal_deformed_patch_image = deformed_patch_image.clone();
	optimal_deformation_cost = deformation_cost;
	deformation_exists = true;
      }
    }
    {
      double deformation_cost;
      Mat deformed_patch_image;
      bool success = deformPatchesAtIntersection(patch_image, patch_index_2, patch_index_1, fold_line_x, ceil(*intersection_it), deformed_patch_image, deformation_cost, MINIMUM_FOLD_LINE_WINDOW_WIDTH, MINIMUM_FOLD_LINE_WINDOW_HEIGHT, MINIMUM_FOLD_LINE_WINDOW_WIDTH, MINIMUM_FOLD_LINE_WINDOW_HEIGHT);
      if (success && deformation_cost < optimal_deformation_cost) {
        optimal_deformed_patch_image = deformed_patch_image.clone();
        optimal_deformation_cost = deformation_cost;
        deformation_exists = true;
      }
    }
  }

  return deformation_exists;
}

int main(int argc, char *argv[])
{
  int image_index;
  stringstream image_index_ss(argv[1]);
  image_index_ss >> image_index;
  Mat patch_image = imread("Test/patch_image_" + to_string(image_index) + ".bmp", 0);
  for (int y = 0; y < patch_image.rows; y++)
    for (int x = 0; x < patch_image.rows; x++)
      if (patch_image.at<uchar>(y, x) < 100)
	patch_image.at<uchar>(y, x) = 0;
      else if (patch_image.at<uchar>(y, x) > 200)
	patch_image.at<uchar>(y, x) = 255;

  Mat deformed_patch_image;
  double deformation_cost;
  if (deformPatchesBesideFoldLine(patch_image, 0, 255, patch_image.cols / 2 + 10, deformed_patch_image, deformation_cost, 16, 16) == true) {
    imwrite("Test/deformed_patch_image_" + to_string(image_index) + ".bmp", deformed_patch_image);
    cout << deformation_cost << endl;
  } else
    cout << "Cannot deform patch image." << endl;
  
  return 0;
}
