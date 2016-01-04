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

using namespace std;
using namespace cv;


//This function compute the approximate deformation costs for all cases and return the position of initial fold lines.
//Input: patch_index_mask is a vector with a size same with the number of pixels. patch_index_mask[y * IMAGE_WIDTH + x] is the patch index at pixel (x, y). IMAGE_WIDTH and IMAGE_HEIGHT define the image dimension. FOLD_LINE_WINDOW_WIDTH and FOLD_LINE_WINDOW_HEIGHT are desired fold line region width and fold line length (smaller values will pay penalties).
//Output: patch_pair_fold_line_positions stores fold line positions for every pair of neighboring patches. For two neighboring patches patch_index_1 and patch_index_2, we can retrieve the fold line between them by the union of patch_pair_fold_line_positions[patch_index_1 * num_patches + patch_index_2] and patch_pair_fold_line_positions[patch_index_2 * num_patches + patch_index_1].
//fold_line_x_score_map stores the scores for fold line between each pair of patches at each x position. For two neighboring patches patch_index_1 and patch_index_2, the score for fold line between them at the position x is max(fold_line_x_score_map[patch_index_1 * num_patches + patch_index_2], fold_line_x_score_map[patch_index_2 * num_patches + patch_index_1]). Each score ranges from 0 to 1, so something like (1 - score) could be used as data cost in optimization stage. Note that here patch_index_1 could be equal to patch_index_2, in this case, the scores stored are for internal fold lines. Something like (2 - score) could be used as their cost.
void computeDeformationCostsApproximately(const vector<int> &patch_index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, vector<vector<int> > &patch_pair_fold_line_positions, vector<map<int, double> > &fold_line_x_score_map)
{
  int num_patches = 0;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++)
    num_patches = max(patch_index_mask[pixel] + 1, num_patches);
  
  map<int, int> patch_min_xs;
  map<int, int> patch_max_xs;
  map<int, set<int> > patch_neighbors;
  for (int patch_index = 0; patch_index < num_patches; patch_index++) {
    patch_min_xs[patch_index] = IMAGE_WIDTH;
    patch_max_xs[patch_index] = 0;
  }

  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    int patch_index = patch_index_mask[pixel];
    if (patch_index == -1)
      continue;
    int x = pixel % IMAGE_WIDTH;
    int y = pixel / IMAGE_WIDTH;
    if (x < patch_min_xs[patch_index])
      patch_min_xs[patch_index] = x;
    if (x > patch_max_xs[patch_index])
      patch_max_xs[patch_index] = x;
    
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
      int neighbor_patch_index = patch_index_mask[*neighbor_pixel_it];
      if (neighbor_patch_index != patch_index && neighbor_patch_index != -1)
	patch_neighbors[patch_index].insert(neighbor_patch_index);
    }
  }
  
  // map<int, map<int, map<int, double> > > patch_neighbor_x_cost_map;
  // map<int, map<int, map<int, int> > > patch_neighbor_x_y_map;
  // for (map<int, set<int> >::const_iterator patch_it = patch_neighbors.begin(); patch_it != patch_neighbors.end(); patch_it++) {
  //   for (set<int>::const_iterator neighbor_patch_it = patch_it->second.begin(); neighbor_patch_it != patch_it->second.end(); neighbor_patch_it++) {
  //     int min_x = max(max(patch_min_xs[patch_it->first], patch_min_xs[*neighbor_patch_it]) - MINIMUM_FOLD_LINE_WINDOW_WIDTH / 2, 0);
  //     int max_x = min(min(patch_max_xs[patch_it->first], patch_max_xs[*neighbor_patch_it]) + MINIMUM_FOLD_LINE_WINDOW_WIDTH / 2, IMAGE_WIDTH - 1);
  //     for (int x = min_x; x <= max_x; x++) {
  //       if (patch_neighbor_x_cost_map[patch_index][*neighbor_patch_it].count(x) == 0) {
  //         if (x % 10 != 0)
  //           continue;
  //         Mat deformed_patch_image;
  //         double deformation_cost;
  //         int fold_line_middle_y;
  //         cout << patch_it->first << '\t' << *neighbor_patch_it << '\t' << x << endl;
  //         if (deformPatchesBesideFoldLine(patch_image, patch_it->first, *neighbor_patch_it, x, deformed_patch_image, deformation_cost, fold_line_middle_y, MINIMUM_FOLD_LINE_WINDOW_WIDTH, MINIMUM_FOLD_LINE_WINDOW_HEIGHT) == true) {
  //           patch_neighbor_x_cost_map[patch_index][*neighbor_patch_it][x] = deformation_cost;
  //           patch_neighbor_x_cost_map[*neighbor_patch_it][patch_index][x] = deformation_cost;
  //           patch_neighbor_x_y_map[patch_index][*neighbor_patch_it][x] = fold_line_middle_y;
  //           patch_neighbor_x_y_map[*neighbor_patch_it][patch_index][x] = fold_line_middle_y;
  //           cout << deformation_cost << '\t' << fold_line_middle_y << endl;
  //         }
  //       }
  //     }
  //   }
  // }

  // Mat fold_line_image = image.clone();
  // for (map<int, map<int, map<int, double> > >::const_iterator patch_it = patch_neighbor_x_cost_map.begin(); patch_it != patch_neighbor_x_cost_map.end(); patch_it++) {
  //   for (map<int, map<int, double> >::const_iterator other_patch_it = patch_it->second.begin(); other_patch_it != patch_it->second.end(); other_patch_it++) {
  //     double min_cost = 1000000;
  //     int min_cost_x = -1;
  //     int min_cost_y = -1;
  //     for (map<int, double>::const_iterator x_it = other_patch_it->second.begin(); x_it != other_patch_it->second.end(); x_it++) {
  //       if (x_it->second < min_cost) {
  //         min_cost_x = x_it->first;
  //         min_cost_y = patch_neighbor_x_y_map[patch_it->first][other_patch_it->first][x_it->first];
  //         min_cost = x_it->second;
  //       }
  //     }
  //     line(fold_line_image, Point(min_cost_x, min_cost_y - MINIMUM_FOLD_LINE_WINDOW_HEIGHT / 2), Point(min_cost_x, min_cost_y + MINIMUM_FOLD_LINE_WINDOW_HEIGHT / 2), Scalar(0, 0, 255));
  //   }
  // }
  // imwrite("Test/fold_line_image.bmp", fold_line_image);
  // exit(1);

  vector<map<int, int> > pixel_left_window_num_pixels(IMAGE_WIDTH * IMAGE_HEIGHT);
  vector<map<int, int> > pixel_right_window_num_pixels(IMAGE_WIDTH * IMAGE_HEIGHT);
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    int x = pixel % IMAGE_WIDTH;
    int y = pixel / IMAGE_WIDTH;
    for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT / 2; delta_y++) {
      for (int delta_x = -FOLD_LINE_WINDOW_WIDTH / 2; delta_x <= FOLD_LINE_WINDOW_WIDTH / 2; delta_x++) {
	if (delta_x == 0 || patch_index_mask[(y + delta_y) * IMAGE_WIDTH + (x + delta_x)] == -1)
	  continue;
	if (x + delta_x < 0 || x + delta_x >= IMAGE_WIDTH || y + delta_y < 0 || y + delta_y >= IMAGE_HEIGHT)
	  continue;
	if (delta_x < 0)
	  pixel_left_window_num_pixels[pixel][patch_index_mask[(y + delta_y) * IMAGE_WIDTH + (x + delta_x)]]++;
	else
	  pixel_right_window_num_pixels[pixel][patch_index_mask[(y + delta_y) * IMAGE_WIDTH + (x + delta_x)]]++;
      }
    }
  }

  vector<pair<int, double> > fold_line_score_pairs(num_patches * num_patches, make_pair(-1, 0));
  vector<map<int, vector<double> > > fold_line_x_scores(num_patches * num_patches);
  vector<vector<tuple<int, int, double> > > left_fold_line_tuples(num_patches, vector<tuple<int, int, double> >(IMAGE_WIDTH, make_tuple(-1, -1, 0)));
  vector<vector<tuple<int, int, double> > > right_fold_line_tuples(num_patches, vector<tuple<int, int, double> >(IMAGE_WIDTH, make_tuple(-1, -1, 0)));
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    map<int, int> left_window_num_pixels = pixel_left_window_num_pixels[pixel];
    map<int, int> right_window_num_pixels = pixel_right_window_num_pixels[pixel];
    for (map<int, int>::const_iterator left_window_it = left_window_num_pixels.begin(); left_window_it != left_window_num_pixels.end(); left_window_it++) {
      for (map<int, int>::const_iterator right_window_it = right_window_num_pixels.begin(); right_window_it != right_window_num_pixels.end(); right_window_it++) {
	if (left_window_it->first != right_window_it->first && patch_neighbors[left_window_it->first].count(right_window_it->first) == 0)
	  continue;
	double score = 1.0 * (left_window_it->second * right_window_it->second) / pow((FOLD_LINE_WINDOW_HEIGHT / 2 * 2 + 1) * (FOLD_LINE_WINDOW_WIDTH / 2), 2);
	int patch_pair_index = left_window_it->first * num_patches + right_window_it->first;
	if (score > fold_line_score_pairs[patch_pair_index].second)
	  fold_line_score_pairs[patch_pair_index] = make_pair(pixel, score);
	int x = pixel % IMAGE_WIDTH;
	fold_line_x_scores[patch_pair_index][x].push_back(score);
	if (left_window_it->first != right_window_it->first) {
	  if (score > get<2>(right_fold_line_tuples[left_window_it->first][x]))
	    right_fold_line_tuples[left_window_it->first][x] = make_tuple(right_window_it->first, pixel, score);
	  if (score > get<2>(left_fold_line_tuples[right_window_it->first][x]))
            left_fold_line_tuples[right_window_it->first][x] = make_tuple(left_window_it->first, pixel, score);
	}
      }
    }
  }

  fold_line_x_score_map.assign(num_patches * num_patches, map<int, double>());
  for (int patch_index_1 = 0; patch_index_1 < num_patches; patch_index_1++) {
    for (int patch_index_2 = 0; patch_index_2 < num_patches; patch_index_2++) {
      int patch_pair_index = patch_index_1 * num_patches + patch_index_2;
      for (map<int, vector<double> >::const_iterator x_it = fold_line_x_scores[patch_pair_index].begin(); x_it != fold_line_x_scores[patch_pair_index].end(); x_it++) {
	double max_score = 0;
        for (vector<double>::const_iterator score_it = x_it->second.begin(); score_it != x_it->second.end(); score_it++)
	  if (*score_it > max_score)
	    max_score = *score_it;
	fold_line_x_score_map[patch_pair_index][x_it->first] = max_score;
      }
    }
  }

  patch_pair_fold_line_positions.assign(num_patches * num_patches, vector<int>());
  vector<vector<int> > patch_left_fold_line_xs(num_patches);
  vector<vector<int> > patch_right_fold_line_xs(num_patches);
  for (int patch_index_1 = 0; patch_index_1 < num_patches; patch_index_1++) {
    for (int patch_index_2 = patch_index_1 + 1; patch_index_2 < num_patches; patch_index_2++) {
      int patch_pair_index = fold_line_score_pairs[patch_index_1 * num_patches + patch_index_2].second >= fold_line_score_pairs[patch_index_2 * num_patches + patch_index_1].second ? patch_index_1 * num_patches + patch_index_2 : patch_index_2 * num_patches + patch_index_1;
      int fold_line_position = fold_line_score_pairs[patch_pair_index].first;
      if (fold_line_position == -1)
	continue;
      patch_pair_fold_line_positions[patch_pair_index].push_back(fold_line_position);
      patch_right_fold_line_xs[patch_pair_index / num_patches].push_back(fold_line_position % IMAGE_WIDTH);
      patch_left_fold_line_xs[patch_pair_index % num_patches].push_back(fold_line_position % IMAGE_WIDTH);
    }
  }

  set<int> background_patches;
  background_patches.insert(patch_index_mask[IMAGE_WIDTH + 1]);
  background_patches.insert(patch_index_mask[(IMAGE_HEIGHT - 2) * IMAGE_WIDTH + (IMAGE_WIDTH - 2)]);
  for (int patch_index = 0; patch_index < num_patches; patch_index++) {
    if (background_patches.count(patch_index) > 0)
      continue;
    double middle_x = 0.5 * (patch_min_xs[patch_index] + patch_max_xs[patch_index]);
    bool has_left_fold_line = false;
    bool has_right_fold_line = false;
    for (vector<int>::const_iterator x_it = patch_left_fold_line_xs[patch_index].begin(); x_it != patch_left_fold_line_xs[patch_index].end(); x_it++)
      if (*x_it < middle_x)
	has_left_fold_line = true;
    for (vector<int>::const_iterator x_it = patch_right_fold_line_xs[patch_index].begin(); x_it != patch_right_fold_line_xs[patch_index].end(); x_it++)
      if (*x_it > middle_x)
	has_right_fold_line = true;
    
    if (has_left_fold_line == false) {
      tuple<int, int, double> highest_score_fold_line_tuple(-1, -1, 0);
      for (vector<tuple<int, int, double> >::const_iterator x_it = left_fold_line_tuples[patch_index].begin(); x_it != left_fold_line_tuples[patch_index].end(); x_it++) {
	if (get<0>(*x_it) == -1 || (x_it - left_fold_line_tuples[patch_index].begin()) >= floor(middle_x))
	  continue;
	if (get<2>(*x_it) > get<2>(highest_score_fold_line_tuple))
	  highest_score_fold_line_tuple = *x_it;
      }
      if (get<0>(highest_score_fold_line_tuple) != -1) {
	int neighbor_patch_index = get<0>(highest_score_fold_line_tuple);
	patch_pair_fold_line_positions[neighbor_patch_index * num_patches + patch_index].push_back(get<1>(highest_score_fold_line_tuple));
	patch_left_fold_line_xs[patch_index].push_back(get<1>(highest_score_fold_line_tuple) % IMAGE_WIDTH);;
	patch_right_fold_line_xs[neighbor_patch_index].push_back(get<1>(highest_score_fold_line_tuple) % IMAGE_WIDTH);
      }
    }
    if (has_right_fold_line == false) {
      tuple<int, int, double> highest_score_fold_line_tuple(-1, -1, 0);
      for (vector<tuple<int, int, double> >::const_iterator x_it = right_fold_line_tuples[patch_index].begin(); x_it != right_fold_line_tuples[patch_index].end(); x_it++) {
        if (get<0>(*x_it) == -1 || (x_it - right_fold_line_tuples[patch_index].begin()) <= ceil(middle_x))
          continue;
        if (get<2>(*x_it) > get<2>(highest_score_fold_line_tuple))
          highest_score_fold_line_tuple = *x_it;
      }
      if (get<0>(highest_score_fold_line_tuple) != -1) {
        int neighbor_patch_index = get<0>(highest_score_fold_line_tuple);
        patch_pair_fold_line_positions[patch_index * num_patches + neighbor_patch_index].push_back(get<1>(highest_score_fold_line_tuple));
        patch_right_fold_line_xs[patch_index].push_back(get<1>(highest_score_fold_line_tuple) % IMAGE_WIDTH);
        patch_left_fold_line_xs[neighbor_patch_index].push_back(get<1>(highest_score_fold_line_tuple) % IMAGE_WIDTH);
      }
    }
  }
}

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
    //cout << mrf->totalEnergy() << endl;
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

//This function compute the actual deformation for fold line appearing at position fold_line_x. Note that although it also returns the deformation score, the score is not used because this function is too slow to compute scores for all cases. This function is only used at the very end to deform the image knowing where fold lines should be.
//Input: patch_image is a gray scale image with the gray value of each pixel representing the patch index. patch_index_1 and patch_index_2 are indices for two patch between which the fold line is going to add. fold_line_x is the denoted x position of the fold line. MINIMUM_FOLD_LINE_WINDOW_WIDTH and MINIMUM_FOLD_LINE_WINDOW_HEIGHT are desired fold line region width and fold line length (smaller values will pay penalties).
//Output: optimal_deformed_patch_image is the image after deformation. optimal_deformation_cost is the cost for the deformation. fold_line_middle_y is the middle point of the fold line for visualization purpose.
bool deformPatchesBesideFoldLine(const Mat &patch_image, const int patch_index_1, const int patch_index_2, const int fold_line_x, Mat &optimal_deformed_patch_image, double &optimal_deformation_cost, int &fold_line_middle_y, const int MINIMUM_FOLD_LINE_WINDOW_WIDTH, const int MINIMUM_FOLD_LINE_WINDOW_HEIGHT)
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
	fold_line_middle_y = intersection_y;
      }
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
	fold_line_middle_y = intersection_y;
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
	fold_line_middle_y = *intersection_it;
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
	fold_line_middle_y = *intersection_it;
      }
    }
  }

  return deformation_exists;
}

void grabCut(const Mat &image)
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
}

int main(int argc, char *argv[])
{
  string image_name = "bear_ill";
  Mat image = imread("Test/" + image_name + ".png");
  //  grabCut(image);
  //exit(1);
  const int IMAGE_WIDTH = image.cols;
  const int IMAGE_HEIGHT = image.rows;
  vector<int> patch_index_mask(IMAGE_WIDTH * IMAGE_HEIGHT, -1);
  int patch_index = 0;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    if (patch_index_mask[pixel] != -1)
      continue;
    vector<int> patch_pixels;
    vector<int> border_pixels;
    border_pixels.push_back(pixel);
    patch_pixels.push_back(pixel);
    patch_index_mask[pixel] = patch_index;
    while (border_pixels.size() > 0) {
      vector<int> new_border_pixels;
      for (vector<int>::const_iterator border_pixel_it = border_pixels.begin(); border_pixel_it != border_pixels.end(); border_pixel_it++) {
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

	Vec3b color = image.at<Vec3b>(*border_pixel_it / IMAGE_WIDTH, *border_pixel_it % IMAGE_WIDTH);
	for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
          if (patch_index_mask[*neighbor_pixel_it] != -1)
	    continue;
	  Vec3b neighbor_color = image.at<Vec3b>(*neighbor_pixel_it / IMAGE_WIDTH, *neighbor_pixel_it % IMAGE_WIDTH);
	  double color_diff = 0;
	  for (int c = 0; c < 3; c++)
	    color_diff += pow(neighbor_color[c] - color[c], 2);
	  color_diff = sqrt(color_diff);
	  if (color_diff > 20)
	    continue;
	  patch_pixels.push_back(*neighbor_pixel_it);
	  patch_index_mask[*neighbor_pixel_it] = patch_index;
	  new_border_pixels.push_back(*neighbor_pixel_it);
	}
      }
      border_pixels = new_border_pixels;
    }
    patch_index++;
  }
  
  map<int, int> patch_pixel_counter;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++)
    patch_pixel_counter[patch_index_mask[pixel]]++;
  const int PATCH_NUM_PIXELS_THRESHOLD = 35;
  map<int, int> patch_index_map;
  int new_patch_index = 0;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    if (patch_pixel_counter[patch_index_mask[pixel]] >= PATCH_NUM_PIXELS_THRESHOLD) {
      if (patch_index_map.count(patch_index_mask[pixel]) == 0)
        patch_index_map[patch_index_mask[pixel]] = new_patch_index++;
      continue;
    }
    
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
    random_shuffle(neighbor_pixels.begin(), neighbor_pixels.end());
    for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
      if (patch_pixel_counter[patch_index_mask[*neighbor_pixel_it]] >= PATCH_NUM_PIXELS_THRESHOLD) {
	patch_index_mask[pixel] = patch_index_mask[*neighbor_pixel_it];
	break;
      }
    }
  }
  int num_patches = 0;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    int new_index = patch_index_map[patch_index_mask[pixel]];
    patch_index_mask[pixel] = new_index;
    if (new_index + 1 > num_patches)
      num_patches = new_index + 1;
  }

  //cout << num_patches << endl;
  
  Mat segmented_patch_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
  Mat patch_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC1);
  map<int, int> color_table;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    int patch_index = patch_index_mask[pixel];
    if (color_table.count(patch_index) == 0)
      color_table[patch_index] = rand() % 256;
    segmented_patch_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(color_table[patch_index], color_table[patch_index], color_table[patch_index]);
    patch_image.at<uchar>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = patch_index;
  }
  imwrite("Test/segmented_patch_image.bmp", segmented_patch_image);
  //exit(1);

  bool test = false;
  if (test) {
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
    int fold_line_middle_y;
    if (deformPatchesBesideFoldLine(patch_image, 0, 255, patch_image.cols / 2 + 10, deformed_patch_image, deformation_cost, fold_line_middle_y, 16, 16) == true) {
      imwrite("Test/deformed_patch_image_" + to_string(image_index) + ".bmp", deformed_patch_image);
      cout << deformation_cost << endl;
    } else
      cout << "Cannot deform patch image." << endl;
  }

  ifstream patch_index_mask_in_str("Test/bear_segmentation.txt");
  int width, height;
  patch_index_mask_in_str >> width >> height;
  patch_index_mask.assign(width * height, 0);
  for (int pixel = 0; pixel < width * height; pixel++)
    patch_index_mask_in_str >> patch_index_mask[pixel];
  patch_index_mask_in_str.close();
  
  vector<vector<int> > patch_pair_fold_line_positions;
  vector<map<int, double> > fold_line_x_score_map;
  const int FOLD_LINE_WINDOW_WIDTH = 10;
  const int FOLD_LINE_WINDOW_HEIGHT = 10;
  computeDeformationCostsApproximately(patch_index_mask, image.cols, image.rows, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, patch_pair_fold_line_positions, fold_line_x_score_map);

  Mat fold_line_image = segmented_patch_image.clone();
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

  for (int patch_index_1 = 0; patch_index_1 < num_patches; patch_index_1++) {
    for (int patch_index_2 = 0; patch_index_2 < num_patches; patch_index_2++) {
      int patch_pair_index = patch_index_1 * num_patches + patch_index_2;
      for (vector<int>::const_iterator position_it = patch_pair_fold_line_positions[patch_pair_index].begin(); position_it != patch_pair_fold_line_positions[patch_pair_index].end(); position_it++) {
        int x = *position_it % IMAGE_WIDTH;
        int y = *position_it / IMAGE_WIDTH;
        line(fold_line_image, Point(x, y - FOLD_LINE_WINDOW_HEIGHT / 2), Point(x, y + FOLD_LINE_WINDOW_HEIGHT / 2), Scalar(0, 0, 255));
        putText(fold_line_image, to_string(patch_index_1) + " " + to_string(patch_index_2), Point(x, y), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
      }
    }
  }
  imwrite("Test/" + image_name + "_fold_line.bmp", fold_line_image);
  
  return 0;
}
