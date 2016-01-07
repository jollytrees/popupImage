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

//This function compute the approximate deformation costs for all cases and return the position of initial fold lines.
//Input: patch_index_mask is a vector with a size same with the number of pixels. patch_index_mask[y * IMAGE_WIDTH + x] is the patch index at pixel (x, y). IMAGE_WIDTH and IMAGE_HEIGHT define the image dimension. FOLD_LINE_WINDOW_WIDTH and FOLD_LINE_WINDOW_HEIGHT are desired fold line region width and fold line length (smaller values will pay penalties).
//Output: patch_pair_fold_line_positions stores fold line positions for every pair of neighboring patches. For two neighboring patches patch_index_1 and patch_index_2, we can retrieve the fold line between them by the union of patch_pair_fold_line_positions[patch_index_1 * num_patches + patch_index_2] and patch_pair_fold_line_positions[patch_index_2 * num_patches + patch_index_1].
//fold_line_x_score_map stores the scores for fold line between each pair of patches at each x position. For two neighboring patches patch_index_1 and patch_index_2, the score for fold line between them at the position x is max(fold_line_x_score_map[patch_index_1 * num_patches + patch_index_2], fold_line_x_score_map[patch_index_2 * num_patches + patch_index_1]). Each score ranges from 0 to 1, so something like (1 - score) could be used as data cost in optimization stage. Note that here patch_index_1 could be equal to patch_index_2, in this case, the scores stored are for internal fold lines. Something like (2 - score) could be used as their cost.
void computeDeformationCostsApproximately(const vector<int> &patch_index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, vector<vector<int> > &patch_pair_fold_line_positions, vector<map<int, double> > &fold_line_x_score_map)
{
  int num_patches = 0;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    num_patches = max(patch_index_mask[pixel] + 1, num_patches);
  }
  
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
    
    vector<int> neighbor_pixels = findNeighbors(pixel, IMAGE_WIDTH, IMAGE_HEIGHT);
    
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

  cout << "done" << endl;  
  vector<pair<int, double> > fold_line_score_pairs(num_patches * num_patches, make_pair(-1, 0));
  vector<map<int, vector<double> > > fold_line_x_scores(num_patches * num_patches);
  vector<vector<tuple<int, int, double> > > left_fold_line_tuples(num_patches, vector<tuple<int, int, double> >(IMAGE_WIDTH, make_tuple(-1, -1, 0)));
  vector<vector<tuple<int, int, double> > > right_fold_line_tuples(num_patches, vector<tuple<int, int, double> >(IMAGE_WIDTH, make_tuple(-1, -1, 0)));
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    map<int, int> left_window_num_pixels = pixel_left_window_num_pixels[pixel];
    map<int, int> right_window_num_pixels = pixel_right_window_num_pixels[pixel];
    for (map<int, int>::const_iterator left_window_it = left_window_num_pixels.begin(); left_window_it != left_window_num_pixels.end(); left_window_it++) {
      for (map<int, int>::const_iterator right_window_it = right_window_num_pixels.begin(); right_window_it != right_window_num_pixels.end(); right_window_it++) {
	//cout << left_window_it->first << '\t' << right_window_it->first << endl;
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
  cout << "done" << endl;  

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

void findAllFoldLines(const vector<int> &patch_index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, const vector<vector<int> > &patch_pair_fold_line_positions, const vector<map<int, double> > &fold_line_x_score_map)
{
  cout << "done" << endl;  
  int num_original_patches = 0;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++)
    num_original_patches = max(patch_index_mask[pixel] + 1, num_original_patches);

  vector<int> line_segment_indices(IMAGE_WIDTH * IMAGE_HEIGHT, -1);
  int line_segment_index = -1;
  for (int x = 0; x < IMAGE_WIDTH; x++) {
    int previous_patch_index = -1;
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
      if (patch_index_mask[y * IMAGE_WIDTH + x] != previous_patch_index)
	line_segment_index++;
      line_segment_indices[y * IMAGE_WIDTH + x] = line_segment_index;
      previous_patch_index = patch_index_mask[y * IMAGE_WIDTH + x];
    }
  }
  
  int num_line_segments = line_segment_index;
  vector<int> original_fold_line_patch_pairs;
  int original_fold_line_index = 0;
  vector<vector<int> > patch_fold_line_segment_indices(num_original_patches);
  vector<vector<int> > patch_fold_line_index_map(num_original_patches);
  for (int patch_index_1 = 0; patch_index_1 < num_original_patches; patch_index_1++) {
    for (int patch_index_2 = 0; patch_index_2 < num_original_patches; patch_index_2++) {
      for (vector<int>::const_iterator position_it = patch_pair_fold_line_positions[patch_index_1 * num_original_patches + patch_index_2].begin(); position_it != patch_pair_fold_line_positions[patch_index_1 * num_original_patches + patch_index_2].end(); position_it++) {
	int fold_line_x = *position_it % IMAGE_WIDTH;
	int intersection_y = *position_it / IMAGE_WIDTH;
	int line_segment_index_1 = -1;
	int line_segment_index_2 = -1;
	for (int delta_x = -FOLD_LINE_WINDOW_WIDTH / 2; delta_x <= 0; delta_x++) {
	  set<int> line_segments;
	  for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT / 2; delta_y++) {
	    if (fold_line_x + delta_x >= 0 && fold_line_x + delta_x < IMAGE_WIDTH && intersection_y + delta_y >= 0 && intersection_y + delta_y < IMAGE_HEIGHT) {
	      int pixel = (intersection_y + delta_y) * IMAGE_WIDTH + (fold_line_x + delta_x);
	      if (patch_index_mask[pixel] == patch_index_1)
		line_segments.insert(line_segment_indices[pixel]);
	    }
	  }
	  if (line_segments.size() == 1)
            line_segment_index_1 = *line_segments.begin();
	  else if (line_segments.size() > 1)
	    break;
	}
	for (int delta_x = FOLD_LINE_WINDOW_WIDTH / 2; delta_x >= 0; delta_x--) {
          set<int> line_segments;
          for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT / 2; delta_y++) {
            if (fold_line_x + delta_x >= 0 && fold_line_x + delta_x < IMAGE_WIDTH && intersection_y + delta_y >= 0 && intersection_y + delta_y < IMAGE_HEIGHT) {
              int pixel = (intersection_y + delta_y) * IMAGE_WIDTH + (fold_line_x + delta_x);
              if (patch_index_mask[pixel] == patch_index_2)
                line_segments.insert(line_segment_indices[pixel]);
            }
          }
          if (line_segments.size() == 1)
            line_segment_index_2 = *line_segments.begin();
          else if (line_segments.size() > 1)
            break;
        }
	if (line_segment_index_1 == -1 || line_segment_index_2 == -1)
	  continue;
	
	patch_fold_line_segment_indices[patch_index_1].push_back(line_segment_index_1);
	patch_fold_line_index_map[patch_index_1].push_back(original_fold_line_index);
	//line_segment_fold_line_indices[line_segment_index_1] = patch_fold_line_segment_indices[patch_index_1].size();
        patch_fold_line_segment_indices[patch_index_2].push_back(line_segment_index_2);
	patch_fold_line_index_map[patch_index_2].push_back(original_fold_line_index);
        //line_segment_fold_line_indices[line_segment_index_2] = patch_fold_line_segment_indices[patch_index_2].size();

	original_fold_line_patch_pairs.push_back(patch_index_1 * num_original_patches + patch_index_2);
	original_fold_line_index++;
      }
    }
  }
  int num_original_fold_lines = original_fold_line_index;
  cout << "done" << endl;
  
  vector<int> line_segment_fold_line_indices(num_line_segments, -1);
  vector<int> line_segment_left_fold_lines(num_line_segments, 0);
  vector<int> line_segment_right_fold_lines(num_line_segments, 0);
  vector<int> line_segment_original_fold_lines(num_line_segments, -1);
  vector<int> line_segment_new_fold_lines(num_line_segments, -1);
  int attendant_fold_line_index = num_original_fold_lines;
  map<int, int> attendant_fold_line_index_map;
  for (vector<vector<int> >::const_iterator patch_it = patch_fold_line_segment_indices.begin(); patch_it != patch_fold_line_segment_indices.end(); patch_it++) {
    for (vector<int>::const_iterator line_segment_it = patch_it->begin(); line_segment_it != patch_it->end(); line_segment_it++) {
      line_segment_fold_line_indices[*line_segment_it] = pow(2, line_segment_it - patch_it->begin());
      line_segment_left_fold_lines[*line_segment_it] = pow(2, line_segment_it - patch_it->begin());
      line_segment_right_fold_lines[*line_segment_it] = pow(2, line_segment_it - patch_it->begin());
      int original_fold_line_index = patch_fold_line_index_map[patch_it - patch_fold_line_segment_indices.begin()][line_segment_it - patch_it->begin()];
      line_segment_original_fold_lines[*line_segment_it] = original_fold_line_index;
      line_segment_new_fold_lines[*line_segment_it] = attendant_fold_line_index;
      attendant_fold_line_index_map[attendant_fold_line_index] = original_fold_line_index;
      attendant_fold_line_index++;
    }
  }
  cout << "done" << endl;  
  while (true) {
    bool has_change = false;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (pixel % IMAGE_WIDTH == IMAGE_WIDTH - 1)
	continue;
      int neighbor_pixel = pixel + 1;
      if (patch_index_mask[pixel] != patch_index_mask[neighbor_pixel])
	continue;
      int line_segment_index = line_segment_indices[pixel];
      int neighbor_line_segment_index = line_segment_indices[neighbor_pixel];
      if (line_segment_fold_line_indices[neighbor_line_segment_index] != -1)
	continue;
      int left_fold_lines = (line_segment_left_fold_lines[line_segment_index] | line_segment_left_fold_lines[neighbor_line_segment_index]);
      if (line_segment_left_fold_lines[neighbor_line_segment_index] != left_fold_lines) {
	line_segment_left_fold_lines[neighbor_line_segment_index] = left_fold_lines;
	has_change = true;
      }
    }
    if (has_change == false)
      break;
  }

  while (true) {
    bool has_change = false;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (pixel % IMAGE_WIDTH == 0)
        continue;
      int neighbor_pixel = pixel - 1;
      if (patch_index_mask[pixel] != patch_index_mask[neighbor_pixel])
        continue;
      int line_segment_index = line_segment_indices[pixel];
      int neighbor_line_segment_index = line_segment_indices[neighbor_pixel];
      if (line_segment_fold_line_indices[neighbor_line_segment_index] != -1)
        continue;
      int right_fold_lines = (line_segment_right_fold_lines[line_segment_index] | line_segment_right_fold_lines[neighbor_line_segment_index]);
      if (line_segment_right_fold_lines[neighbor_line_segment_index] != right_fold_lines) {
        line_segment_right_fold_lines[neighbor_line_segment_index] = right_fold_lines;
        has_change = true;
      }
    }
    if (has_change == false)
      break;
  }
  

  vector<int> line_segment_xs(num_line_segments);
  vector<int> line_segment_patch_index_mask(num_line_segments);
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    line_segment_xs[line_segment_indices[pixel]] = pixel % IMAGE_WIDTH;
    line_segment_patch_index_mask[line_segment_indices[pixel]] = patch_index_mask[pixel];
  }
  
  int max_index = 0;
  for (int line_segment_index = 0; line_segment_index < num_line_segments; line_segment_index++)
    max_index = max(max_index, max(line_segment_left_fold_lines[line_segment_index], line_segment_right_fold_lines[line_segment_index]));

  
  vector<map<long, vector<int> > > patch_group_line_segments(num_original_patches);
  for (int line_segment_index = 0; line_segment_index < num_line_segments; line_segment_index++) {
    if (line_segment_left_fold_lines[line_segment_index] == 0 || line_segment_right_fold_lines[line_segment_index] == 0 || line_segment_new_fold_lines[line_segment_index] != -1)
      continue;
    long group_index = static_cast<long>(line_segment_left_fold_lines[line_segment_index]) * (max_index + 1) + line_segment_right_fold_lines[line_segment_index];
    patch_group_line_segments[line_segment_patch_index_mask[line_segment_index]][group_index].push_back(line_segment_index);
  }
  cout << "done" << endl;  

  int num_attendant_fold_lines = attendant_fold_line_index - num_original_fold_lines;
  int new_fold_line_index = num_original_fold_lines + num_attendant_fold_lines;
  map<int, set<int> > fold_line_xs;
  for (vector<map<long, vector<int> > >::const_iterator patch_it = patch_group_line_segments.begin(); patch_it != patch_group_line_segments.end(); patch_it ++) {
    for (map<long, vector<int> >::const_iterator group_it = patch_it->begin(); group_it != patch_it->end(); group_it++) {
      vector<int> line_segments = group_it->second;
      int selected_line_segment = -1;
      set<int> xs;
      for (vector<int>::const_iterator line_segment_it = line_segments.begin(); line_segment_it != line_segments.end(); line_segment_it++) {
	xs.insert(line_segment_xs[*line_segment_it]);
	if (line_segment_it - line_segments.begin() <= (line_segments.size() - 1) / 2)
	  selected_line_segment = *line_segment_it;
      }
      line_segment_new_fold_lines[selected_line_segment] = new_fold_line_index;
      fold_line_xs[new_fold_line_index] = xs;
      new_fold_line_index++;
    }
  }
  
  vector<int> new_patch_index_mask(IMAGE_WIDTH * IMAGE_HEIGHT, -1);
  int new_patch_index = 0;
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    if (new_patch_index_mask[pixel] != -1 || line_segment_new_fold_lines[line_segment_indices[pixel]] != -1)
      continue;
    vector<int> border_pixels;
    border_pixels.push_back(pixel);
    new_patch_index_mask[pixel] = new_patch_index;
    while (border_pixels.size() > 0) {
      vector<int> new_border_pixels;
      for (vector<int>::const_iterator border_pixel_it = border_pixels.begin(); border_pixel_it != border_pixels.end(); border_pixel_it++) {
        int pixel = *border_pixel_it;
        vector<int> neighbor_pixels = findNeighbors(pixel, IMAGE_WIDTH, IMAGE_HEIGHT);
        for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
          if (patch_index_mask[*neighbor_pixel_it] != patch_index_mask[pixel] || new_patch_index_mask[*neighbor_pixel_it] != -1 || line_segment_new_fold_lines[line_segment_indices[*neighbor_pixel_it]] != -1)
            continue;
          new_border_pixels.push_back(*neighbor_pixel_it);
          new_patch_index_mask[*neighbor_pixel_it] = new_patch_index;
        }
      }
      border_pixels = new_border_pixels;
    }
    new_patch_index++;
  }
  
  int num_new_patches = new_patch_index;
  map<int, set<int> > patch_left_fold_lines;
  map<int, set<int> > patch_right_fold_lines;
  map<int, set<int> > fold_line_left_patches;
  map<int, set<int> > fold_line_right_patches;
  
  for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
    if (pixel % IMAGE_WIDTH == IMAGE_WIDTH - 1)
      continue;
    int neighbor_pixel = pixel + 1;
    if (patch_index_mask[pixel] != patch_index_mask[neighbor_pixel])
      continue;
    int left_new_patch_index = new_patch_index_mask[pixel];
    int right_new_patch_index = new_patch_index_mask[neighbor_pixel];
    int left_new_fold_line_index = line_segment_new_fold_lines[line_segment_indices[pixel]];
    int right_new_fold_line_index = line_segment_new_fold_lines[line_segment_indices[neighbor_pixel]];
    int left_original_fold_line_index = line_segment_original_fold_lines[line_segment_indices[pixel]];
    int right_original_fold_line_index = line_segment_original_fold_lines[line_segment_indices[neighbor_pixel]];

    if (left_new_patch_index != -1 && right_new_fold_line_index != -1) {
      patch_right_fold_lines[left_new_patch_index].insert(right_new_fold_line_index);
      fold_line_left_patches[right_new_fold_line_index].insert(left_new_patch_index);
      if (right_original_fold_line_index != -1) {
	patch_right_fold_lines[left_new_patch_index].insert(right_original_fold_line_index);
        fold_line_left_patches[right_original_fold_line_index].insert(left_new_patch_index);
      }
    }
    if (right_new_patch_index != -1 && left_new_fold_line_index != -1) {
      patch_left_fold_lines[right_new_patch_index].insert(left_new_fold_line_index);
      fold_line_right_patches[left_new_fold_line_index].insert(right_new_patch_index);
      if (left_original_fold_line_index != -1) {
        patch_left_fold_lines[right_new_patch_index].insert(left_original_fold_line_index);
        fold_line_right_patches[left_original_fold_line_index].insert(right_new_patch_index);
      }
    }
  }

  set<int> invalid_patches;
  set<int> invalid_fold_lines;
  if (true) {
    for (int new_patch_index = 0; new_patch_index < num_new_patches; new_patch_index++) {
      if (patch_left_fold_lines[new_patch_index].size() > 0 && patch_right_fold_lines[new_patch_index].size() > 0)
	continue;
      invalid_patches.insert(new_patch_index);
      string left_or_right_patch = patch_left_fold_lines[new_patch_index].size() == 0 ? "left" : "right";
      vector<int> attendant_fold_lines;
      attendant_fold_lines.insert(attendant_fold_lines.end(), patch_left_fold_lines[new_patch_index].begin(), patch_left_fold_lines[new_patch_index].end());
      attendant_fold_lines.insert(attendant_fold_lines.end(), patch_right_fold_lines[new_patch_index].begin(), patch_right_fold_lines[new_patch_index].end());
      for (vector<int>::const_iterator attendant_fold_line_it = attendant_fold_lines.begin(); attendant_fold_line_it != attendant_fold_lines.end(); attendant_fold_line_it++) {
	if (*attendant_fold_line_it < num_original_fold_lines || *attendant_fold_line_it >= num_original_fold_lines + num_attendant_fold_lines)
	  continue;
	invalid_fold_lines.insert(*attendant_fold_line_it);
	int original_fold_line = attendant_fold_line_index_map[*attendant_fold_line_it];
	for (map<int, int>::const_iterator another_attendant_fold_line_it = attendant_fold_line_index_map.begin(); another_attendant_fold_line_it != attendant_fold_line_index_map.end(); another_attendant_fold_line_it++) {
	  if (another_attendant_fold_line_it->first == *attendant_fold_line_it || another_attendant_fold_line_it->second != original_fold_line)
	    continue;
	  invalid_fold_lines.insert(another_attendant_fold_line_it->first);
	  if (left_or_right_patch == "left")
	    for (set<int>::const_iterator patch_it = fold_line_right_patches[another_attendant_fold_line_it->first].begin(); patch_it != fold_line_right_patches[another_attendant_fold_line_it->first].end(); patch_it++)
	      invalid_patches.insert(*patch_it);
	  else
	    for (set<int>::const_iterator patch_it = fold_line_left_patches[another_attendant_fold_line_it->first].begin(); patch_it != fold_line_left_patches[another_attendant_fold_line_it->first].end(); patch_it++)
	      invalid_patches.insert(*patch_it);
	}
      }
    }
  
    for (int attendant_fold_line_index = num_original_fold_lines; attendant_fold_line_index < num_original_fold_lines + num_attendant_fold_lines; attendant_fold_line_index++) {
      if (invalid_fold_lines.count(attendant_fold_line_index) > 0)
	continue;
      if (fold_line_left_patches[attendant_fold_line_index].size() > 0 && fold_line_right_patches[attendant_fold_line_index].size() > 0) {
	invalid_fold_lines.insert(attendant_fold_line_index);
	string left_or_right_fold_line = fold_line_left_patches[attendant_fold_line_index].size() == 0 ? "left" : "right";
	int original_fold_line = attendant_fold_line_index_map[attendant_fold_line_index];
	for (map<int, int>::const_iterator another_attendant_fold_line_it = attendant_fold_line_index_map.begin(); another_attendant_fold_line_it != attendant_fold_line_index_map.end(); another_attendant_fold_line_it++) {
	  if (another_attendant_fold_line_it->first == attendant_fold_line_index || another_attendant_fold_line_it->second != original_fold_line)
	    continue;
	  invalid_fold_lines.insert(another_attendant_fold_line_it->first);
	  if (left_or_right_fold_line == "left")
	    for (set<int>::const_iterator patch_it = fold_line_right_patches[another_attendant_fold_line_it->first].begin(); patch_it != fold_line_right_patches[another_attendant_fold_line_it->first].end(); patch_it++)
	      invalid_patches.insert(*patch_it);
	  else
	    for (set<int>::const_iterator patch_it = fold_line_left_patches[another_attendant_fold_line_it->first].begin(); patch_it != fold_line_left_patches[another_attendant_fold_line_it->first].end(); patch_it++)
	      invalid_patches.insert(*patch_it);
	}
      }
    }
      
      
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (invalid_patches.count(new_patch_index_mask[pixel]) > 0)
	new_patch_index_mask[pixel] = -1;
      if (invalid_fold_lines.count(line_segment_new_fold_lines[line_segment_indices[pixel]]) > 0)
	line_segment_new_fold_lines[line_segment_indices[pixel]] = -1;
    }

    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (new_patch_index_mask[pixel] == -1)
	continue;
      new_patch_index = new_patch_index_mask[pixel];
      vector<int> border_pixels;
      border_pixels.push_back(pixel);
      while (border_pixels.size() > 0) {
	vector<int> new_border_pixels;
	for (vector<int>::const_iterator border_pixel_it = border_pixels.begin(); border_pixel_it != border_pixels.end(); border_pixel_it++) {
	  int pixel = *border_pixel_it;
	  vector<int> neighbor_pixels = findNeighbors(pixel, IMAGE_WIDTH, IMAGE_HEIGHT);
	  for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
	    if (patch_index_mask[*neighbor_pixel_it] != patch_index_mask[pixel] || new_patch_index_mask[*neighbor_pixel_it] != -1 || line_segment_new_fold_lines[line_segment_indices[*neighbor_pixel_it]] != -1)
	      continue;
	    new_border_pixels.push_back(*neighbor_pixel_it);
	    new_patch_index_mask[*neighbor_pixel_it] = new_patch_index;
	  }
	}
	border_pixels = new_border_pixels;
      }
    }
    patch_left_fold_lines.clear();
    patch_right_fold_lines.clear();
    fold_line_left_patches.clear();
    fold_line_right_patches.clear();
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (pixel % IMAGE_WIDTH == IMAGE_WIDTH - 1)
	continue;
      int neighbor_pixel = pixel + 1;
      if (patch_index_mask[pixel] != patch_index_mask[neighbor_pixel])
	continue;
      int left_new_patch_index = new_patch_index_mask[pixel];
      int right_new_patch_index = new_patch_index_mask[neighbor_pixel];
      int left_new_fold_line_index = line_segment_new_fold_lines[line_segment_indices[pixel]];
      int right_new_fold_line_index = line_segment_new_fold_lines[line_segment_indices[neighbor_pixel]];
      int left_original_fold_line_index = line_segment_original_fold_lines[line_segment_indices[pixel]];
      int right_original_fold_line_index = line_segment_original_fold_lines[line_segment_indices[neighbor_pixel]];

      if (left_new_patch_index != -1 && right_new_fold_line_index != -1) {
	patch_right_fold_lines[left_new_patch_index].insert(right_new_fold_line_index);
	fold_line_left_patches[right_new_fold_line_index].insert(left_new_patch_index);
	if (right_original_fold_line_index != -1) {
	  patch_right_fold_lines[left_new_patch_index].insert(right_original_fold_line_index);
	  fold_line_left_patches[right_original_fold_line_index].insert(left_new_patch_index);
	}
      }
      if (right_new_patch_index != -1 && left_new_fold_line_index != -1) {
	patch_left_fold_lines[right_new_patch_index].insert(left_new_fold_line_index);
	fold_line_right_patches[left_new_fold_line_index].insert(right_new_patch_index);
	if (left_original_fold_line_index != -1) {
	  patch_left_fold_lines[right_new_patch_index].insert(left_original_fold_line_index);
	  fold_line_right_patches[left_original_fold_line_index].insert(right_new_patch_index);
	}
      }
    }
  }
  
  if (true) {
    Mat original_graph_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    //Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    map<int, Vec3b> patch_color_table;
    map<int, vector<int> > patch_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      //int patch_index = patch_index_mask[pixel] * max_index + line_segment_left_fold_lines[line_segment_indices[pixel]];
      int patch_index = patch_index_mask[pixel];
      if (patch_index == -1) {
	//fold_line_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(255, 0, 0);
	continue;
      }
      if (patch_color_table.count(patch_index) == 0) {
            //color_table[patch_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
	int gray_value = rand() % 256;
	patch_color_table[patch_index] = Vec3b(gray_value, gray_value, gray_value);
	//color_table[patch_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
      }
      original_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = patch_color_table[patch_index];
      patch_pixels[patch_index].push_back(pixel);
    }
    for (map<int, vector<int> >::const_iterator patch_it = patch_pixels.begin(); patch_it != patch_pixels.end(); patch_it++) {
      int pixel = patch_it->second[rand() % patch_it->second.size()];
      putText(original_graph_image, to_string(patch_it->first), Point(pixel % IMAGE_WIDTH, pixel /IMAGE_WIDTH), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
    }

    map<int, vector<int> > fold_line_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (line_segment_original_fold_lines[line_segment_indices[pixel]] == -1) {
        continue;
      }
      int fold_line_index = line_segment_original_fold_lines[line_segment_indices[pixel]];
      if (fold_line_index < num_original_fold_lines) {
        original_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(0, 0, 255);
	fold_line_pixels[fold_line_index].push_back(pixel);
      }
    }
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_pixels.begin(); fold_line_it != fold_line_pixels.end(); fold_line_it++) {
      int pixel = fold_line_it->second[rand() % fold_line_it->second.size()];
      putText(original_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH, pixel /IMAGE_WIDTH), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(255, 0, 0));
    }

    // for (int patch_index_1 = 0; patch_index_1 < num_original_patches; patch_index_1++) {
    //   for (int patch_index_2 = 0; patch_index_2 < num_original_patches; patch_index_2++) {
    // 	int patch_pair_index = patch_index_1 * num_original_patches + patch_index_2;
    // 	for (vector<int>::const_iterator position_it = patch_pair_fold_line_positions[patch_pair_index].begin(); position_it != patch_pair_fold_line_positions[patch_pair_index].end(); position_it++) {
    // 	  int x = *position_it % IMAGE_WIDTH;
    // 	  int y = *position_it / IMAGE_WIDTH;
    // 	  line(original_graph_image, Point(x, y - FOLD_LINE_WINDOW_HEIGHT / 2), Point(x, y + FOLD_LINE_WINDOW_HEIGHT / 2), Scalar(0, 0, 255));
    // 	  putText(original_graph_image, to_string(patch_index_1) + " " + to_string(patch_index_2), Point(x, y), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
    // 	}
    //   }
    // }
    imwrite("Test/original_graph_image.bmp", original_graph_image);
  }

  if (true) {
    Mat new_graph_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    //Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    map<int, Vec3b> patch_color_table;
    map<int, vector<int> > patch_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      int patch_index = new_patch_index_mask[pixel];
      if (patch_index == -1) {
        continue;
      }
      if (patch_color_table.count(patch_index) == 0) {
        //color_table[patch_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
        int gray_value = rand() % 256;
        patch_color_table[patch_index] = Vec3b(gray_value, gray_value, gray_value);
      }
      new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = patch_color_table[patch_index];
      patch_pixels[patch_index].push_back(pixel);
    }
    for (map<int, vector<int> >::const_iterator patch_it = patch_pixels.begin(); patch_it != patch_pixels.end(); patch_it++) {
      int pixel = patch_it->second[rand() % patch_it->second.size()];
      putText(new_graph_image, to_string(patch_it->first), Point(pixel % IMAGE_WIDTH, pixel /IMAGE_WIDTH), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
    }

    map<int, vector<int> > fold_line_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      if (line_segment_new_fold_lines[line_segment_indices[pixel]] == -1) {
	continue;
      }
      int fold_line_index = line_segment_new_fold_lines[line_segment_indices[pixel]];
      if (fold_line_index < num_original_fold_lines)
	new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(0, 0, 255);
      else if (fold_line_index < num_original_fold_lines + num_attendant_fold_lines)
	new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(0, 255, 0);
      else
        new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(255, 0, 0);
      fold_line_pixels[fold_line_index].push_back(pixel);
    }
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_pixels.begin(); fold_line_it != fold_line_pixels.end(); fold_line_it++) {
      int pixel = fold_line_it->second[rand() % fold_line_it->second.size()];
      putText(new_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH, pixel /IMAGE_WIDTH), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(255, 0, 0));
    }

    imwrite("Test/new_graph_image.bmp", new_graph_image);
  }

  if (true) {
    for (map<int, set<int> >::const_iterator patch_it = patch_left_fold_lines.begin(); patch_it != patch_left_fold_lines.end(); patch_it++)
      for (set<int>::const_iterator fold_line_it = patch_it->second.begin(); fold_line_it != patch_it->second.end(); fold_line_it++)
	cout << "patch left fold line: " << patch_it->first << '\t' << *fold_line_it << endl;
    for (map<int, set<int> >::const_iterator patch_it = patch_right_fold_lines.begin(); patch_it != patch_right_fold_lines.end(); patch_it++)
      for (set<int>::const_iterator fold_line_it = patch_it->second.begin(); fold_line_it != patch_it->second.end(); fold_line_it++)
        cout << "patch right fold line: " << patch_it->first << '\t' << *fold_line_it << endl;
    for (map<int, set<int> >::const_iterator fold_line_it = fold_line_left_patches.begin(); fold_line_it != fold_line_left_patches.end(); fold_line_it++)
          for (set<int>::const_iterator patch_it = fold_line_it->second.begin(); patch_it != fold_line_it->second.end(); patch_it++)
            cout << "fold line left patch: " << fold_line_it->first << '\t' << *patch_it << endl;
    for (map<int, set<int> >::const_iterator fold_line_it = fold_line_right_patches.begin(); fold_line_it != fold_line_right_patches.end(); fold_line_it++)
      for (set<int>::const_iterator patch_it = fold_line_it->second.begin(); patch_it != fold_line_it->second.end(); patch_it++)
        cout << "fold line right patch: " << fold_line_it->first << '\t' << *patch_it << endl;
    
    for (map<int, int>::const_iterator attendant_fold_line_it = attendant_fold_line_index_map.begin(); attendant_fold_line_it != attendant_fold_line_index_map.end(); attendant_fold_line_it++)
      cout << "attendant: " << attendant_fold_line_it->first << '\t' << attendant_fold_line_it->second << endl;

    for (set<int>::const_iterator patch_it = invalid_patches.begin(); patch_it != invalid_patches.end(); patch_it++)
      cout << "invalid patch: " << *patch_it << endl;
    
    for (set<int>::const_iterator fold_line_it = invalid_fold_lines.begin(); fold_line_it != invalid_fold_lines.end(); fold_line_it++)
      cout << "invalid fold line: " << *fold_line_it << endl;
    cout << num_original_patches << '\t' << num_original_fold_lines << '\t' << num_original_patches << '\t' << num_attendant_fold_lines << '\t' << new_fold_line_index - num_original_fold_lines - num_attendant_fold_lines << endl;
  }
}

bool deformPatchesAtIntersection(const Mat &patch_image, const int left_patch_index, const int right_patch_index, const int fold_line_x, const int intersection_y, Mat &deformed_patch_image, double &deformation_cost, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, const int MINIMUM_FOLD_LINE_WINDOW_WIDTH, const int MINIMUM_FOLD_LINE_WINDOW_HEIGHT)
{
  const int WINDOW_WIDTH = FOLD_LINE_WINDOW_WIDTH + 2;
  const int WINDOW_HEIGHT = FOLD_LINE_WINDOW_HEIGHT + 2;

  bool left_patch_exists = false;
  bool right_patch_exists = false;
  vector<vector<int> > window_patch_index_mask(WINDOW_WIDTH, vector<int>(WINDOW_HEIGHT, 2));
  for (int delta_y = -WINDOW_HEIGHT / 2; delta_y < WINDOW_HEIGHT / 2; delta_y++) {
    for (int delta_x = -WINDOW_WIDTH / 2; delta_x < WINDOW_WIDTH / 2; delta_x++) {
      if (fold_line_x + delta_x < 0 || fold_line_x + delta_x >= patch_image.cols || intersection_y + delta_y < 0 || intersection_y + delta_y >= patch_image.rows)
        continue;
      int patch_index = patch_image.at<uchar>(intersection_y + delta_y, fold_line_x + delta_x);
      if (patch_index == left_patch_index) {
	window_patch_index_mask[delta_x + WINDOW_WIDTH / 2][delta_y + WINDOW_HEIGHT / 2] = 0;
	if (delta_x < 0)
	  left_patch_exists = true;
      } else if (patch_index == right_patch_index) {
        window_patch_index_mask[delta_x + WINDOW_WIDTH / 2][delta_y + WINDOW_HEIGHT / 2] = 1;
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
	cost[window_patch_index_mask[x][y]] = 0;
	data_cost.insert(data_cost.end(), cost.begin(), cost.end());
	continue;
      }
      vector<int> cost(3, 0);
      if (window_patch_index_mask[x][y] == 2) {
	//           || (window_patch_index_mask[x][y] == 0 && x < FOLD_LINE_REGION_WIDTH_THRESHOLD)
	//	  || (window_patch_index_mask[x][y] == 1 && x > FOLD_LINE_REGION_WIDTH_THRESHOLD + 1)) {
	for (int label = 0; label < 3; label++)
	  if (label != window_patch_index_mask[x][y])
	    cost[label] = FIXED_PIXEL_INCONSISTENCY_COST;
      } else {
	for (int label = 0; label < 3; label++)
          if (label != window_patch_index_mask[x][y])
            cost[label] = REGION_INCONSISTENCY_COST;
      }
      if (x == FOLD_LINE_WINDOW_WIDTH / 2) {
	if (window_patch_index_mask[x][y] == 2) {
	  cost[1] += FOLD_LINE_INCONSISTENCY_COST;
	  cost[2] += FOLD_LINE_INCONSISTENCY_COST;
	} else {
	  cost[1] += FIXED_PIXEL_INCONSISTENCY_COST;
          cost[2] += FIXED_PIXEL_INCONSISTENCY_COST;
	}
      } else if (x == FOLD_LINE_WINDOW_WIDTH / 2 + 1) {
	if (window_patch_index_mask[x][y] == 2) {
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
  //     if (label != window_patch_index_mask[y][x])
  // 	num_changed_pixels++;
  //     if (label == 0 && x <= FOLD_LINE_REGION_WIDTH_THRESHOLD)
  // 	num_left_patch_pixels++;
  //     if (label == 1 && x > FOLD_LINE_REGION_WIDTH_THRESHOLD)
  //       num_right_patch_pixels++;
  //   }
  //   if (window_patch_index_mask[y * WINDOW_WIDTH + FOLD_LINE_REGION_WIDTH_THRESHOLD] == left_patch_index && window_patch_index_mask[y * WINDOW_WIDTH + FOLD_LINE_REGION_WIDTH_THRESHOLD + 1] == right_patch_index)
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
      vector<int> neighbor_pixels = findNeighbors(pixel, IMAGE_WIDTH, IMAGE_HEIGHT);
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
        vector<int> neighbor_pixels = findNeighbors(pixel, IMAGE_WIDTH, IMAGE_HEIGHT);
	
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
    vector<int> neighbor_pixels = findNeighbors(pixel, IMAGE_WIDTH, IMAGE_HEIGHT);
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

  int background_patch = patch_index_mask[width + 1];
  for (int pixel = 0; pixel < width * height; pixel++)
    if (patch_index_mask[pixel] == -1)
      patch_index_mask[pixel] = background_patch;
  
  
  vector<vector<int> > patch_pair_fold_line_positions;
  vector<map<int, double> > fold_line_x_score_map;
  const int FOLD_LINE_WINDOW_WIDTH = 10;
  const int FOLD_LINE_WINDOW_HEIGHT = 10;

  const int NEW_IMAGE_WIDTH = image.cols * 3;
  const int NEW_IMAGE_HEIGHT = image.rows * 3;
  patch_index_mask = zoomMask(patch_index_mask, image.cols, image.rows, NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT);
  computeDeformationCostsApproximately(patch_index_mask, NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, patch_pair_fold_line_positions, fold_line_x_score_map);
  findAllFoldLines(patch_index_mask, NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, patch_pair_fold_line_positions, fold_line_x_score_map);
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
