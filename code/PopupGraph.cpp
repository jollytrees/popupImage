#include "PopupGraph.h"
#include "PopupUtils.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cmath>
#include <iostream>
#include <fstream>

#include <gflags/gflags.h>

using namespace std;
using namespace cv;

DEFINE_string(output_folder, "Examples", "The output folder.");
DEFINE_string(output_prefix, "", "The output prefix (to distinguish different results.");

namespace Popup
{
  std::pair<int, int> PopupGraph::getFoldLineXRange(const int fold_line_index) const
  {
    int min_x = IMAGE_WIDTH_;
    int max_x = 0;
    vector<int> positions = fold_lines_[fold_line_index].positions;
    for (vector<int>::const_iterator position_it = positions.begin(); position_it != positions.end(); position_it++) {
      int x = *position_it % IMAGE_WIDTH_;
      if (x < min_x)
	min_x = x;
      if (x > max_x)
        max_x = x;
    }
    //min_x -= 1;
    //max_x += 1;
    //min_x -= FOLD_LINE_WINDOW_WIDTH_ / 2;
    //max_x += FOLD_LINE_WINDOW_WIDTH_ / 2;
    //cout << "min max x: " << min_x << '\t' << max_x << endl;
    return make_pair(min_x, max_x);
  }

  std::map<int, std::map<int, std::set<int> > > PopupGraph::getPatchNeighborFoldLines(const char left_or_right, const std::map<int, std::set<int> > &patch_child_patches) const
  {
    std::map<int, std::map<int, std::set<int> > > patch_neighbor_fold_lines;
    
    for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
      if (fold_line_it - fold_lines_.begin() >= getNumOriginalFoldLines())
	continue;
      int left_original_patch_index = fold_line_it->original_patch_pair.first;   
      int right_original_patch_index = fold_line_it->original_patch_pair.second;
      if (left_or_right == 'L' || left_or_right == 'B')
	if (patch_child_patches.count(right_original_patch_index) == 0 || patch_child_patches.at(right_original_patch_index).count(left_original_patch_index) == 0)
	  patch_neighbor_fold_lines[right_original_patch_index][left_original_patch_index].insert(fold_line_it - fold_lines_.begin());
      
      if (left_or_right == 'R' || left_or_right == 'B')
	if (patch_child_patches.count(left_original_patch_index) == 0 || patch_child_patches.at(left_original_patch_index).count(right_original_patch_index) == 0)
	  patch_neighbor_fold_lines[left_original_patch_index][right_original_patch_index].insert(fold_line_it - fold_lines_.begin());
    }
    return patch_neighbor_fold_lines;
  }

  std::map<int, std::set<int> > PopupGraph::getPatchFoldLines() const
  {
    map<int, set<int> > patch_fold_lines;
    for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
      patch_fold_lines[fold_line_it->original_patch_pair.first].insert(fold_line_it - fold_lines_.begin());
      patch_fold_lines[fold_line_it->original_patch_pair.second].insert(fold_line_it - fold_lines_.begin());
    }
    return patch_fold_lines;
  }

  std::vector<int> PopupGraph::getBackgroundLeftFoldLines() const
  {
    vector<int> background_left_fold_lines;
    for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
      if (fold_line_it->original_patch_pair.first == ORIGINAL_BACKGROUND_PATCH_INDEX_ && fold_line_it - fold_lines_.begin() != MIDDLE_FOLD_LINE_INDEX_ && fold_line_it - fold_lines_.begin() != RIGHT_BORDER_FOLD_LINE_INDEX_ && fold_line_it - fold_lines_.begin() != LEFT_BORDER_FOLD_LINE_INDEX_)
	background_left_fold_lines.push_back(fold_line_it - fold_lines_.begin());
    return background_left_fold_lines;
  }

  std::vector<int> PopupGraph::getBackgroundRightFoldLines() const
  {
    vector<int> background_right_fold_lines;
    for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
      if (fold_line_it->original_patch_pair.second == ORIGINAL_BACKGROUND_PATCH_INDEX_ && fold_line_it - fold_lines_.begin() != MIDDLE_FOLD_LINE_INDEX_ && fold_line_it - fold_lines_.begin() != LEFT_BORDER_FOLD_LINE_INDEX_ && fold_line_it - fold_lines_.begin() != RIGHT_BORDER_FOLD_LINE_INDEX_)
        background_right_fold_lines.push_back(fold_line_it - fold_lines_.begin());
    return background_right_fold_lines;
  }
  
  void PopupGraph::findOriginalFoldLines()
  {
    //compute the x range of each patch and find the neighbor relation among patches
    map<int, pair<int, int> > patch_x_range_map;
    map<int, set<int> > patch_neighbors;
    {
      for (int patch_index = 0; patch_index < NUM_ORIGINAL_PATCHES_; patch_index++)
	patch_x_range_map[patch_index] = make_pair(IMAGE_WIDTH_, 0);
      
      for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
	int patch_index = patch_index_mask_[pixel];
	if (patch_index == -1)
	  continue;
	int x = pixel % IMAGE_WIDTH_;
	int y = pixel / IMAGE_WIDTH_;
	if (x < patch_x_range_map[patch_index].first)
	  patch_x_range_map[patch_index].first = x;
	if (x > patch_x_range_map[patch_index].second)
          patch_x_range_map[patch_index].second = x;
        
    
	vector<int> neighbor_pixels = Popup::findNeighbors(pixel, IMAGE_WIDTH_, IMAGE_HEIGHT_);
    
	for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
	  int neighbor_patch_index = patch_index_mask_[*neighbor_pixel_it];
	  if (neighbor_patch_index != patch_index && neighbor_patch_index != -1)
	    patch_neighbors[patch_index].insert(neighbor_patch_index);
	}
      }
    }
    

    //find all possible fold lines at all positions
    fold_lines_.clear();
    map<int, map<int, map<int, int> > > patch_position_fold_line_indices;
    {
      for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
	int x = pixel % IMAGE_WIDTH_;
	int y = pixel / IMAGE_WIDTH_;

	//Count how many pixels of each patch in left half and right half of a local window. A good fold line is likely to have different patches on different sides.
	map<int, int> left_window_num_pixels;
	map<int, int> right_window_num_pixels;
	{
	  for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y++) {
	    for (int delta_x = -FOLD_LINE_WINDOW_WIDTH_ / 2; delta_x <= FOLD_LINE_WINDOW_WIDTH_ / 2; delta_x++) {
              if (x + delta_x < 0 || x + delta_x >= IMAGE_WIDTH_ || y + delta_y < 0 || y + delta_y >= IMAGE_HEIGHT_)
		continue;
	      if (delta_x == 0 || patch_index_mask_[(y + delta_y) * IMAGE_WIDTH_ + (x + delta_x)] == -1)
                continue;
              if (delta_x < 0)
		left_window_num_pixels[patch_index_mask_[(y + delta_y) * IMAGE_WIDTH_ + (x + delta_x)]]++;
	      else
		right_window_num_pixels[patch_index_mask_[(y + delta_y) * IMAGE_WIDTH_ + (x + delta_x)]]++;
	    }
	  }
	}
      	
	for (map<int, int>::const_iterator left_window_it = left_window_num_pixels.begin(); left_window_it != left_window_num_pixels.end(); left_window_it++) {
	  for (map<int, int>::const_iterator right_window_it = right_window_num_pixels.begin(); right_window_it != right_window_num_pixels.end(); right_window_it++) {
	    if (left_window_it->first == right_window_it->first || patch_neighbors[left_window_it->first].count(right_window_it->first) == 0)
	      continue;
	    double score = (1.0 * left_window_it->second * right_window_it->second) / pow((FOLD_LINE_WINDOW_HEIGHT_ / 2 * 2 + 1) * (FOLD_LINE_WINDOW_WIDTH_ / 2), 2);
	    
            fold_lines_.push_back(FoldLine(make_pair(left_window_it->first, right_window_it->first), pixel, score));
	    patch_position_fold_line_indices[left_window_it->first][right_window_it->first][pixel] = fold_lines_.size() - 1;
	  }
	}
      }
    }

    //We want to keep only the fold lines which has the maximum score among fold lines in the same group. Here a "group" can be defined as fold lines between same pair of patches. But to be more robust, fold lines belonging to the same "group" are fold lines 1) between same pair of patches, and 2) belong to the same valley.
    vector<FoldLine> selected_fold_lines;
    for (map<int, map<int, map<int, int> > >::const_iterator left_patch_it = patch_position_fold_line_indices.begin(); left_patch_it != patch_position_fold_line_indices.end(); left_patch_it++) {
      for (map<int, map<int, int> >::const_iterator right_patch_it = left_patch_it->second.begin(); right_patch_it != left_patch_it->second.end(); right_patch_it++) {
        vector<bool> position_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, false);
        vector<pair<double, int> > score_fold_line_index_pairs;
	for (map<int, int>::const_iterator position_it = right_patch_it->second.begin(); position_it != right_patch_it->second.end(); position_it++) {
	  score_fold_line_index_pairs.push_back(make_pair(fold_lines_[position_it->second].score + 0.001 * abs(position_it->first % IMAGE_WIDTH_ - IMAGE_WIDTH_ / 2) / IMAGE_WIDTH_, position_it->second));
	  position_mask[position_it->first] = true;
	}
	sort(score_fold_line_index_pairs.begin(), score_fold_line_index_pairs.end());
	reverse(score_fold_line_index_pairs.begin(), score_fold_line_index_pairs.end());
	for (vector<pair<double, int> >::const_iterator score_it = score_fold_line_index_pairs.begin(); score_it != score_fold_line_index_pairs.end(); score_it++) {
	  FoldLine fold_line = fold_lines_[score_it->second];
	  int position = fold_line.desirable_center;
          if (position_mask[position] == false)
	    continue;
	  
	  position_mask[position] = false;

	  if (ENFORCE_SYMMETRY_ && abs(position % IMAGE_WIDTH_ - MIDDLE_FOLD_LINE_X_) <= FOLD_LINE_WINDOW_WIDTH_ / 2 + 1)       
            continue;     

	  vector<int> suppressed_positions;
	  //suppress other candidates in the same valley on the left side
          {
            vector<int> border_positions;
            border_positions.push_back(position);
            while (border_positions.size() > 0) {
              vector<int> new_border_positions;
              for (vector<int>::const_iterator position_it = border_positions.begin(); position_it != border_positions.end(); position_it++) {
                vector<int> neighbor_pixels = Popup::findNeighbors(*position_it, IMAGE_WIDTH_, IMAGE_HEIGHT_);
                for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
                  if (*neighbor_pixel_it % IMAGE_WIDTH_ > *position_it % IMAGE_WIDTH_)
                    continue;
                  if (position_mask[*neighbor_pixel_it] == false)
                    continue;
                  position_mask[*neighbor_pixel_it] = false;
                  new_border_positions.push_back(*neighbor_pixel_it);
		  suppressed_positions.push_back(*neighbor_pixel_it);
                }
	      }
	      border_positions = new_border_positions;
	    }
	  }
	  //suppress other candidates in the same valley on the right side
          {
            vector<int> border_positions;
            border_positions.push_back(position);
            while (border_positions.size() > 0) {
              vector<int> new_border_positions;
              for (vector<int>::const_iterator position_it = border_positions.begin(); position_it != border_positions.end(); position_it++) {
                vector<int> neighbor_pixels = Popup::findNeighbors(*position_it, IMAGE_WIDTH_, IMAGE_HEIGHT_);
                for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
                  if (*neighbor_pixel_it % IMAGE_WIDTH_ < *position_it % IMAGE_WIDTH_)
                    continue;
                  if (position_mask[*neighbor_pixel_it] == false)
                    continue;
                  position_mask[*neighbor_pixel_it] = false;
                  new_border_positions.push_back(*neighbor_pixel_it);
		  suppressed_positions.push_back(*neighbor_pixel_it);
                }
              }
              border_positions = new_border_positions;
            }
          }

          //suppress other candidates in the local window
	  {
	    int x = position % IMAGE_WIDTH_;
	    int y = position / IMAGE_WIDTH_;
            for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y++) {
	      for (int delta_x = -FOLD_LINE_WINDOW_WIDTH_ / 2; delta_x <= FOLD_LINE_WINDOW_WIDTH_ / 2; delta_x++) {
		if (x + delta_x < 0 || x + delta_x >= IMAGE_WIDTH_ || y + delta_y < 0 || y + delta_y >= IMAGE_HEIGHT_)
		  continue;
		position_mask[(y + delta_y) * IMAGE_WIDTH_ + (x + delta_x)] = false;
		suppressed_positions.push_back((y + delta_y) * IMAGE_WIDTH_ + (x + delta_x));
              }
	    }
	  }

	  fold_line.positions = suppressed_positions;
	  selected_fold_lines.push_back(fold_line);
        }
      }
    }
    
    fold_lines_ = selected_fold_lines;

    //keep only one fold line if two fold lines between same pair of patches with opposite direction (i.e. f1 is between (p, q) and f2 is between (q, p)) are on the same side
    {
      map<int, int> left_patch_fold_line_counter;
      map<int, int> right_patch_fold_line_counter;
      for (vector<FoldLine>::iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
	left_patch_fold_line_counter[fold_line_it->original_patch_pair.first]++;        
        right_patch_fold_line_counter[fold_line_it->original_patch_pair.second]++;
      }
      
      vector<bool> fold_line_validity_mask(fold_lines_.size(), true);
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
        vector<bool> position_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, false);
        for (vector<int>::const_iterator position_it = fold_line_it->positions.begin(); position_it != fold_line_it->positions.end(); position_it++)
          position_mask[*position_it] = true;

	vector<bool> same_side_position_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, false);
	position_mask[fold_line_it->desirable_center] = false;
	//goes up to find positions on the same side
	{
	  vector<int> border_positions;
	  border_positions.push_back(fold_line_it->desirable_center);
	  while (border_positions.size() > 0) {
	    vector<int> new_border_positions;
	    for (vector<int>::const_iterator position_it = border_positions.begin(); position_it != border_positions.end(); position_it++) {
              same_side_position_mask[*position_it] = true;
              vector<int> neighbor_pixels = Popup::findNeighbors(*position_it, IMAGE_WIDTH_, IMAGE_HEIGHT_);
	      for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
		if (*neighbor_pixel_it / IMAGE_WIDTH_ > *position_it / IMAGE_WIDTH_)
		  continue;
		if (position_mask[*neighbor_pixel_it] == false)
		  continue;
		position_mask[*neighbor_pixel_it] = false;
		new_border_positions.push_back(*neighbor_pixel_it);
              }
            }
	    border_positions = new_border_positions;
          }
        }
	//goes down to find positions on the same side
	{
          vector<int> border_positions;
          border_positions.push_back(fold_line_it->desirable_center);
          while (border_positions.size() > 0) {
            vector<int> new_border_positions;
            for (vector<int>::const_iterator position_it = border_positions.begin(); position_it != border_positions.end(); position_it++) {
              same_side_position_mask[*position_it] = true;
              vector<int> neighbor_pixels = Popup::findNeighbors(*position_it, IMAGE_WIDTH_, IMAGE_HEIGHT_);
              for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
                if (*neighbor_pixel_it / IMAGE_WIDTH_ < *position_it / IMAGE_WIDTH_)
                  continue;
                if (position_mask[*neighbor_pixel_it] == false)
                  continue;
                position_mask[*neighbor_pixel_it] = false;
                new_border_positions.push_back(*neighbor_pixel_it);
              }
            }
	    border_positions = new_border_positions;
          }
        }
	
	//suppress other fold lines 1) between same pair of patches, 2) has opposite direction, 3) in the same valley, and 4) has smaller score
        for (vector<FoldLine>::const_iterator other_fold_line_it = fold_lines_.begin(); other_fold_line_it != fold_lines_.end(); other_fold_line_it++) {
	  if (other_fold_line_it == fold_line_it)
	    continue;
	  if (same_side_position_mask[other_fold_line_it->desirable_center] == false)
	    continue;
	  if (other_fold_line_it->original_patch_pair.first != fold_line_it->original_patch_pair.second || other_fold_line_it->original_patch_pair.second != fold_line_it->original_patch_pair.first)
	    continue;
	  if (other_fold_line_it->score > fold_line_it->score)
	    continue;
	  if (left_patch_fold_line_counter[other_fold_line_it->original_patch_pair.first] <= 1 || right_patch_fold_line_counter[other_fold_line_it->original_patch_pair.second] <= 1)
	    continue;
	  fold_line_validity_mask[other_fold_line_it - fold_lines_.begin()] = false;
	  left_patch_fold_line_counter[fold_line_it->original_patch_pair.first]--;
          right_patch_fold_line_counter[fold_line_it->original_patch_pair.second]--;
	}
      }

      vector<FoldLine> new_fold_lines;
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
	if (fold_line_validity_mask[fold_line_it - fold_lines_.begin()])
	  new_fold_lines.push_back(*fold_line_it);
      fold_lines_ = new_fold_lines;
    }

    //remove fold lines on the original background patch if they are reverse
    if (MIDDLE_FOLD_LINE_X_ > 0) {
      vector<bool> fold_line_validity_mask(fold_lines_.size(), true);
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
        if (fold_line_it->original_patch_pair.second == ORIGINAL_BACKGROUND_PATCH_INDEX_ && fold_line_it->desirable_center % IMAGE_WIDTH_ < MIDDLE_FOLD_LINE_X_)
          fold_line_validity_mask[fold_line_it - fold_lines_.begin()] = false;
	if (fold_line_it->original_patch_pair.first == ORIGINAL_BACKGROUND_PATCH_INDEX_ && fold_line_it->desirable_center % IMAGE_WIDTH_ > MIDDLE_FOLD_LINE_X_)
	  fold_line_validity_mask[fold_line_it - fold_lines_.begin()] = false;
      }
      vector<FoldLine> new_fold_lines;
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
        if (fold_line_validity_mask[fold_line_it - fold_lines_.begin()])
          new_fold_lines.push_back(*fold_line_it);
      fold_lines_ = new_fold_lines;
    }

    //keep only one fold line if two fold lines between same pair of patches are too close (touching each other)
    {
      vector<bool> fold_line_validity_mask(fold_lines_.size(), true);
      vector<int> line_segment_fold_line_indices(NUM_LINE_SEGMENTS_, -1);
      vector<pair<int, int> > conflicted_fold_line_pairs;
      //find fold line pairs with overlap
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
	int fold_line_index = fold_line_it - fold_lines_.begin();
	
        int fold_line_x = fold_line_it->desirable_center % IMAGE_WIDTH_;
        int intersection_y = fold_line_it->desirable_center / IMAGE_WIDTH_;
        set<int> line_segments;
        {
          for (int delta_x = -FOLD_LINE_WINDOW_WIDTH_ / 2 - 1; delta_x <= FOLD_LINE_WINDOW_WIDTH_ / 2 + 1; delta_x++)
            for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y++)
              if (fold_line_x + delta_x >= 0 && fold_line_x + delta_x < IMAGE_WIDTH_ && intersection_y + delta_y >= 0 && intersection_y + delta_y < IMAGE_HEIGHT_)
		line_segments.insert(pixel_line_segment_indices_[(intersection_y + delta_y) * IMAGE_WIDTH_ + (fold_line_x + delta_x)]);
	}
	
        for (set<int>::const_iterator line_segment_index_it = line_segments.begin(); line_segment_index_it != line_segments.end(); line_segment_index_it++) {
    	  if (line_segment_fold_line_indices[*line_segment_index_it] != -1 && line_segment_fold_line_indices[*line_segment_index_it] != fold_line_index)
    	    conflicted_fold_line_pairs.push_back(make_pair(line_segment_fold_line_indices[*line_segment_index_it], fold_line_index));
          line_segment_fold_line_indices[*line_segment_index_it] = fold_line_index;
    	}
      }
      
      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = conflicted_fold_line_pairs.begin(); fold_line_pair_it != conflicted_fold_line_pairs.end(); fold_line_pair_it++) {
        if (fold_lines_[fold_line_pair_it->first].original_patch_pair.first == fold_lines_[fold_line_pair_it->second].original_patch_pair.first && fold_lines_[fold_line_pair_it->first].original_patch_pair.second == fold_lines_[fold_line_pair_it->second].original_patch_pair.second) {
	  //cout << fold_line_pair_it->first << '\t' << fold_line_pair_it->second << endl;
    	  if (fold_lines_[fold_line_pair_it->first].score < fold_lines_[fold_line_pair_it->second].score)
    	    fold_line_validity_mask[fold_line_pair_it->first] = false;
          else
    	    fold_line_validity_mask[fold_line_pair_it->second] = false;
	}
      }
      
      vector<FoldLine> new_fold_lines;
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
        if (fold_line_validity_mask[fold_line_it - fold_lines_.begin()])
          new_fold_lines.push_back(*fold_line_it);
      fold_lines_ = new_fold_lines;
    }

    //remove asymmetric fold lines if symmetry is enforce
    if (ENFORCE_SYMMETRY_) {
      vector<bool> fold_line_validity_mask(fold_lines_.size(), true);

      map<int, int> bilateral_symmetric_patch_map = symmetric_patch_map_;
      for (map<int, int>::const_iterator patch_it = symmetric_patch_map_.begin(); patch_it != symmetric_patch_map_.end(); patch_it++)
        bilateral_symmetric_patch_map[patch_it->second] = patch_it->first;      

      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
        int left_original_patch_index = fold_line_it->original_patch_pair.first;
        int right_original_patch_index = fold_line_it->original_patch_pair.second;
	if (bilateral_symmetric_patch_map.count(left_original_patch_index) == 0 || bilateral_symmetric_patch_map.count(right_original_patch_index) == 0)
	  continue;

	pair<int, int> symmetric_fold_line_patch_pair(bilateral_symmetric_patch_map[right_original_patch_index], bilateral_symmetric_patch_map[left_original_patch_index]);
	bool symmetric_fold_line_exists = false;
	for (vector<FoldLine>::const_iterator other_fold_line_it = fold_lines_.begin(); other_fold_line_it != fold_lines_.end(); other_fold_line_it++) {
	  if (other_fold_line_it->original_patch_pair == symmetric_fold_line_patch_pair) {
	    //cout << fold_line_it - fold_lines_.begin() << '\t' << other_fold_line_it - fold_lines_.begin() << endl;
	    if (abs(other_fold_line_it->desirable_center % IMAGE_WIDTH_ + fold_line_it->desirable_center % IMAGE_WIDTH_ - 2 * MIDDLE_FOLD_LINE_X_) < IMAGE_WIDTH_ / 10 && abs(other_fold_line_it->desirable_center / IMAGE_WIDTH_ - fold_line_it->desirable_center / IMAGE_WIDTH_) < IMAGE_HEIGHT_ / 10) {
	      symmetric_fold_line_exists = true;
	      break;
	    }
	  }
	}
	if (symmetric_fold_line_exists == false) {
	  //cout << "asymmetric fold lines: " << fold_line_it - fold_lines_.begin() << endl;
          fold_line_validity_mask[fold_line_it - fold_lines_.begin()] = false;
	}
      }
      
      vector<FoldLine> new_fold_lines;
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
        if (fold_line_validity_mask[fold_line_it - fold_lines_.begin()])
          new_fold_lines.push_back(*fold_line_it);
      fold_lines_ = new_fold_lines;

      //exit(1);
    }


    //find fold lines between symmetric patches
    if (ENFORCE_SYMMETRY_) {
      for (map<int, int>::const_iterator patch_it = symmetric_patch_map_.begin(); patch_it != symmetric_patch_map_.end(); patch_it++) {
        int left_patch_index = patch_it->first;
        int right_patch_index = patch_it->second;
	if (left_patch_index == ORIGINAL_BACKGROUND_PATCH_INDEX_)
	  continue;
        vector<int> positions;
        for (int y = 0; y < IMAGE_HEIGHT_; y++) {
          if (patch_index_mask_[y * IMAGE_WIDTH_ + MIDDLE_FOLD_LINE_X_ - 1] == left_patch_index && patch_index_mask_[y * IMAGE_WIDTH_ + MIDDLE_FOLD_LINE_X_ + 1] == right_patch_index) {
            positions.push_back(y * IMAGE_WIDTH_ + MIDDLE_FOLD_LINE_X_ - 1);
            positions.push_back(y * IMAGE_WIDTH_ + MIDDLE_FOLD_LINE_X_);
            positions.push_back(y * IMAGE_WIDTH_ + MIDDLE_FOLD_LINE_X_ + 1);
          }
        }
	if (positions.size() > 0) {
	  symmetry_fold_lines_.insert(fold_lines_.size());
	  
	  FoldLine fold_line(make_pair(left_patch_index, right_patch_index), positions);
	  fold_line.desirable_center = positions[rand() % positions.size()] / IMAGE_WIDTH_ * IMAGE_WIDTH_ + MIDDLE_FOLD_LINE_X_;
          fold_lines_.push_back(fold_line);
	}
      }
    }

    NUM_ORIGINAL_FOLD_LINES_ = fold_lines_.size();

    
    //mark line segment using corresponding original fold line indices
    line_segment_fold_line_indices_.assign(NUM_LINE_SEGMENTS_, -1);

    vector<set<int> > line_segment_left_neighbors(NUM_LINE_SEGMENTS_);
    vector<set<int> > line_segment_right_neighbors(NUM_LINE_SEGMENTS_);
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      if (pixel % IMAGE_WIDTH_ == IMAGE_WIDTH_ - 1)
	continue;
      int neighbor_pixel = pixel + 1;
      if (patch_index_mask_[pixel] != patch_index_mask_[neighbor_pixel])
	continue;
      line_segment_right_neighbors[pixel_line_segment_indices_[pixel]].insert(pixel_line_segment_indices_[neighbor_pixel]);
      line_segment_left_neighbors[pixel_line_segment_indices_[neighbor_pixel]].insert(pixel_line_segment_indices_[pixel]);
    }
    
    {
      for (vector<FoldLine>::iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
        int left_original_patch_index = fold_line_it->original_patch_pair.first; 
        int right_original_patch_index = fold_line_it->original_patch_pair.second;
	vector<set<int> > left_line_segment_indices;    
        vector<set<int> > right_line_segment_indices;
        if (symmetric_patch_map_.count(left_original_patch_index) > 0 && symmetric_patch_map_[left_original_patch_index] == right_original_patch_index) {
	  set<int> line_segment_indices;
	  for (vector<int>::const_iterator position_it = fold_line_it->positions.begin(); position_it != fold_line_it->positions.end(); position_it++)
	    line_segment_indices.insert(pixel_line_segment_indices_[*position_it]);
	  left_line_segment_indices.push_back(line_segment_indices);
	  right_line_segment_indices.push_back(line_segment_indices);
	} else {
	  int fold_line_x = fold_line_it->desirable_center % IMAGE_WIDTH_;
	  int intersection_y = fold_line_it->desirable_center / IMAGE_WIDTH_;
	  {
	    for (int delta_x = 0; delta_x >= -FOLD_LINE_WINDOW_WIDTH_ / 2; delta_x--) {
	      set<int> line_segments;
	      for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y++) {
		if (fold_line_x + delta_x >= 0 && fold_line_x + delta_x < IMAGE_WIDTH_ && intersection_y + delta_y >= 0 && intersection_y + delta_y < IMAGE_HEIGHT_) {
		  int pixel = (intersection_y + delta_y) * IMAGE_WIDTH_ + (fold_line_x + delta_x);
		  if (patch_index_mask_[pixel] == left_original_patch_index)
		    line_segments.insert(pixel_line_segment_indices_[pixel]);
		}
	      }
	      if (line_segments.size() >= 1) {
		//left_line_segment_indices.insert(line_segments.begin(), line_segments.end());
		//break;
		left_line_segment_indices.push_back(line_segments);
	      }
	    }

	    {
	      int max_count_index = 0;
	      int max_count = 0;
	      for (vector<set<int> >::const_iterator line_segments_it = left_line_segment_indices.begin(); line_segments_it != left_line_segment_indices.end(); line_segments_it++) {
		int num_neighbor_line_segments = 0;
		for (set<int>::const_iterator line_segment_index_it = line_segments_it->begin(); line_segment_index_it != line_segments_it->end(); line_segment_index_it++)
		  num_neighbor_line_segments += line_segment_right_neighbors[*line_segment_index_it].size();
	    
		if (num_neighbor_line_segments > max_count) {
		  max_count_index = line_segments_it - left_line_segment_indices.begin();
		  max_count = line_segments_it->size();
		}
	      }
	      left_line_segment_indices.erase(left_line_segment_indices.begin() + max_count_index + 1, left_line_segment_indices.end());
	    }
	  }
	  {
	    for (int delta_x = 0; delta_x <= FOLD_LINE_WINDOW_WIDTH_ / 2; delta_x++) {
	      set<int> line_segments;
	      for (int delta_y = -FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT_ / 2; delta_y++) {
		if (fold_line_x + delta_x >= 0 && fold_line_x + delta_x < IMAGE_WIDTH_ && intersection_y + delta_y >= 0 && intersection_y + delta_y < IMAGE_HEIGHT_) {
		  int pixel = (intersection_y + delta_y) * IMAGE_WIDTH_ + (fold_line_x + delta_x);
		  if (patch_index_mask_[pixel] == right_original_patch_index)
		    line_segments.insert(pixel_line_segment_indices_[pixel]);
		}
	      }
	      if (line_segments.size() >= 1) {
		//right_line_segment_indices.insert(line_segments.begin(), line_segments.end());
		//break;
		right_line_segment_indices.push_back(line_segments);
	      }
	    }

	    {
	      int max_count_index = 0;
	      int max_count = 0;
	      for (vector<set<int> >::const_iterator line_segments_it = right_line_segment_indices.begin(); line_segments_it != right_line_segment_indices.end(); line_segments_it++) {
		int num_neighbor_line_segments = 0;
		for (set<int>::const_iterator line_segment_index_it = line_segments_it->begin(); line_segment_index_it != line_segments_it->end(); line_segment_index_it++)
		  num_neighbor_line_segments += line_segment_left_neighbors[*line_segment_index_it].size();

		if (num_neighbor_line_segments > max_count) {
		  max_count_index = line_segments_it - right_line_segment_indices.begin();
		  max_count = line_segments_it->size();
		}
	      }
	      right_line_segment_indices.erase(right_line_segment_indices.begin() + max_count_index + 1, right_line_segment_indices.end());
	    }
	  }
	}
	
	if (left_line_segment_indices.size() == 0 || right_line_segment_indices.size() == 0) {
	  cout << "no line segments found beside fold line: " << fold_line_it - fold_lines_.begin() << endl;
	  exit(1);
	}

	// if (fold_line_it - fold_lines_.begin() == 0) {
	//   for (set<int>::const_iterator line_segment_index_it = left_line_segment_indices.begin(); line_segment_index_it != left_line_segment_indices.end(); line_segment_index_it++) {
	//     cout << *line_segment_index_it << endl;
	//   }
	// }
	// exit(1);
	
        if (left_original_patch_index != ORIGINAL_BACKGROUND_PATCH_INDEX_) {
	  for (vector<set<int> >::const_iterator line_segments_it = left_line_segment_indices.begin(); line_segments_it != left_line_segment_indices.end(); line_segments_it++) {
	    for (set<int>::const_iterator line_segment_index_it = line_segments_it->begin(); line_segment_index_it != line_segments_it->end(); line_segment_index_it++)
              line_segment_fold_line_indices_[*line_segment_index_it] = fold_line_it - fold_lines_.begin();
	    fold_line_it->line_segment_indices.insert(line_segments_it->begin(), line_segments_it->end());
	  }
        }
        
        if (right_original_patch_index != ORIGINAL_BACKGROUND_PATCH_INDEX_) {
	  for (vector<set<int> >::const_iterator line_segments_it = right_line_segment_indices.begin(); line_segments_it != right_line_segment_indices.end(); line_segments_it++) {
	    for (set<int>::const_iterator line_segment_index_it = line_segments_it->begin(); line_segment_index_it != line_segments_it->end(); line_segment_index_it++)
	      line_segment_fold_line_indices_[*line_segment_index_it] = fold_line_it - fold_lines_.begin();
	    fold_line_it->line_segment_indices.insert(line_segments_it->begin(), line_segments_it->end());
	  }
	}
      }
    }
  }

  void PopupGraph::findAllFoldLines()
  {
    //find new fold lines based on topology
    {
      vector<int> line_segment_left_fold_lines(NUM_LINE_SEGMENTS_, 0);
      vector<int> line_segment_right_fold_lines(NUM_LINE_SEGMENTS_, 0);
      //These two vectors store the bitwise summation of fold line indices on the left side and right side of same original patch. Since the summation only matters for each patch independently, a fold line is encoded as pow(2, the order it appears on this patch).

      //original patch index for each line segment
      vector<int> line_segment_original_patch_indices(NUM_LINE_SEGMENTS_, 0);
      for (int x = 0; x < IMAGE_WIDTH_; x++)
        for (int y = 0; y < IMAGE_HEIGHT_; y++)
          if (patch_index_mask_[y * IMAGE_WIDTH_ + x] != -1) {
	    //if (pixel_line_segment_indices_[y * IMAGE_WIDTH_ + x] == 483) {
	    //cout << patch_index_mask_[y * IMAGE_WIDTH_ + x] << endl;
	    //if (patch_index_mask_[y * IMAGE_WIDTH_ + x] == 0)
	    //cout << x << '\t' << y << endl;
	    //}
	    line_segment_original_patch_indices[pixel_line_segment_indices_[y * IMAGE_WIDTH_ + x]] = patch_index_mask_[y * IMAGE_WIDTH_ + x];
	  }
      
      //encode original fold lines as explained above
      map<int, set<int> > original_patch_fold_line_indices;
      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++) {
        int original_patch_index = line_segment_original_patch_indices[line_segment_index];
        int fold_line_index = line_segment_fold_line_indices_[line_segment_index];
	if (fold_line_index >= 0)
          original_patch_fold_line_indices[original_patch_index].insert(fold_line_index);
      }

      // for (set<int>::const_iterator fold_line_it = original_patch_fold_line_indices[0].begin(); fold_line_it != original_patch_fold_line_indices[0].end(); fold_line_it++)
      // 	cout << *fold_line_it << endl;	
      // exit(1);
      
      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++) {
	int original_patch_index = line_segment_original_patch_indices[line_segment_index];
	int fold_line_index = line_segment_fold_line_indices_[line_segment_index];
	if (fold_line_index >= 0)
	  line_segment_left_fold_lines[line_segment_index] = line_segment_right_fold_lines[line_segment_index] = pow(2, distance(original_patch_fold_line_indices[original_patch_index].begin(), original_patch_fold_line_indices[original_patch_index].find(fold_line_index)) + 1);
      }

      while (true) {
        bool has_change = false;
	for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
          if (pixel % IMAGE_WIDTH_ == IMAGE_WIDTH_ - 1)
            continue;
          int neighbor_pixel = pixel + 1;
          if (patch_index_mask_[pixel] != patch_index_mask_[neighbor_pixel])
            continue;
          int line_segment_index = pixel_line_segment_indices_[pixel];
          int neighbor_line_segment_index = pixel_line_segment_indices_[neighbor_pixel];
          int left_fold_lines = (line_segment_left_fold_lines[line_segment_index] & (~line_segment_right_fold_lines[neighbor_line_segment_index])) | line_segment_left_fold_lines[neighbor_line_segment_index];      
      
          if (line_segment_left_fold_lines[neighbor_line_segment_index] != left_fold_lines) {
	    // if (neighbor_line_segment_index == pixel_line_segment_indices_[354 * IMAGE_WIDTH_ + 119])
	    //   cout << left_fold_lines << '\t' << line_segment_index << endl;
            line_segment_left_fold_lines[neighbor_line_segment_index] = left_fold_lines;
            has_change = true;
          }
        }
        for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
          if (pixel % IMAGE_WIDTH_ == 0)
            continue;
          int neighbor_pixel = pixel - 1;
          if (patch_index_mask_[pixel] != patch_index_mask_[neighbor_pixel])
            continue;
          int line_segment_index = pixel_line_segment_indices_[pixel];
          int neighbor_line_segment_index = pixel_line_segment_indices_[neighbor_pixel];
          int right_fold_lines = (line_segment_right_fold_lines[line_segment_index] & (~line_segment_left_fold_lines[neighbor_line_segment_index])) | line_segment_right_fold_lines[neighbor_line_segment_index];
      
          if (line_segment_right_fold_lines[neighbor_line_segment_index] != right_fold_lines) {
            line_segment_right_fold_lines[neighbor_line_segment_index] = right_fold_lines;
            has_change = true;
          }
        }
        if (has_change == false)
          break;
      }

      if (false) {
	int pixel_1 = 650 * IMAGE_WIDTH_ + 142;
	int pixel_2 = 650 * IMAGE_WIDTH_ + 280;
	//int pixel_2 = 450 * IMAGE_WIDTH_ + 80;
	cout << line_segment_original_patch_indices[pixel_line_segment_indices_[pixel_1]] << '\t' << line_segment_original_patch_indices[pixel_line_segment_indices_[pixel_2]] << '\t' << line_segment_left_fold_lines[pixel_line_segment_indices_[pixel_1]] << '\t' << line_segment_left_fold_lines[pixel_line_segment_indices_[pixel_2]] << '\t' << line_segment_right_fold_lines[pixel_line_segment_indices_[pixel_1]] << '\t' << line_segment_right_fold_lines[pixel_line_segment_indices_[pixel_2]] << endl;
	//exit(1);
	//imwrite("Test/patch_index_mask_image.png", drawIndexMaskImage(patch_index_mask_, IMAGE_WIDTH_, IMAGE_HEIGHT_));
	Mat test_image = Mat::zeros(IMAGE_HEIGHT_, IMAGE_WIDTH_, CV_8UC3);
	//Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
	map<int, Vec3b> color_table;
	map<pair<int, int>, vector<int> > index_pixels;
	for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
	  int index = line_segment_right_fold_lines[pixel_line_segment_indices_[pixel]];
	  if (color_table.count(index) == 0) {
	    if (index < 0) {
	      int gray_value = rand() % 256;
	      color_table[index] = Vec3b(gray_value, gray_value, gray_value);
	    } else
	      color_table[index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
	  }
	  test_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = color_table[index];
	  int path_index = line_segment_left_fold_lines[pixel_line_segment_indices_[pixel]];
	  // if (path_index > 0) {
	  //   //if (pixel == 65 * IMAGE_WIDTH_ + 107)
	  //   //cout << pixel_line_segment_indices_[pixel] << '\t' << path_index << endl;     
	  //   test_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = Vec3b(0, 0, 255);
	  //   index_pixels[make_pair(index, path_index)].push_back(pixel);
	  // }
	}
	for (map<pair<int, int>, vector<int> >::const_iterator index_it = index_pixels.begin(); index_it != index_pixels.end(); index_it++) {
	  int pixel = index_it->second[rand() % index_it->second.size()];
	  putText(test_image, to_string(index_it->first.second), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(255, 0, 0));
	}

	imwrite("Test/test_image.png", test_image);
	exit(1);
      }
      
      int max_index = 0;
      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++)
        max_index = max(max_index, max(line_segment_left_fold_lines[line_segment_index], line_segment_right_fold_lines[line_segment_index]));

      map<tuple<int, int, int>, int> new_patch_index_map;
      int valid_patch_index = NUM_ORIGINAL_FOLD_LINES_ + 1;
      int invalid_patch_index = -1;
      MIDDLE_FOLD_LINE_INDEX_ = NUM_ORIGINAL_FOLD_LINES_;
      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++) {
        if (line_segment_fold_line_indices_[line_segment_index] >= 0)
          continue;
        //if (line_segment_left_fold_lines[line_segment_index] != 0 || line_segment_right_fold_lines[line_segment_index] != 0)
        //cout << line_segment_original_patch_indices[line_segment_index] << '\t' << line_segment_left_fold_lines[line_segment_index] << '\t' << line_segment_right_fold_lines[line_segment_index] << endl;
    
        //long index = static_cast<long>(line_segment_original_patch_indices[line_segment_index]) * pow(max_index, 2) + line_segment_left_fold_lines[line_segment_index] * static_cast<long>(max_index) + line_segment_right_fold_lines[line_segment_index];
	tuple<int, int, int> index(line_segment_original_patch_indices[line_segment_index], line_segment_left_fold_lines[line_segment_index], line_segment_right_fold_lines[line_segment_index]);

	// if (line_segment_index == pixel_line_segment_indices_[650 * IMAGE_WIDTH_ + 280]) {
	//   cout << line_segment_index << '\t' << index << endl;
	//   cout << line_segment_original_patch_indices[line_segment_index] << '\t' << max_index << '\t' << line_segment_left_fold_lines[line_segment_index] << '\t' << line_segment_right_fold_lines[line_segment_index] << endl;
	//   cout << pow(max_index, 2) << '\t' << line_segment_left_fold_lines[line_segment_index] * max_index + line_segment_right_fold_lines[line_segment_index] << endl;
	//   exit(1);
	// }
        if (line_segment_original_patch_indices[line_segment_index] == ORIGINAL_BACKGROUND_PATCH_INDEX_)
          continue;
	
	if (new_patch_index_map.count(index) == 0) {
	  if (line_segment_left_fold_lines[line_segment_index] > 0 && line_segment_right_fold_lines[line_segment_index] > 0)
            new_patch_index_map[index] = valid_patch_index++;
          else
            new_patch_index_map[index] = invalid_patch_index--;
        }
      }

      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++) {
        if (line_segment_fold_line_indices_[line_segment_index] >= 0)
          continue;
    
        //long index = static_cast<long>(line_segment_original_patch_indices[line_segment_index]) * pow(max_index, 2) + line_segment_left_fold_lines[line_segment_index] * max_index + line_segment_right_fold_lines[line_segment_index];
	tuple<int, int, int> index(line_segment_original_patch_indices[line_segment_index], line_segment_left_fold_lines[line_segment_index], line_segment_right_fold_lines[line_segment_index]);
    
        if (line_segment_original_patch_indices[line_segment_index] == ORIGINAL_BACKGROUND_PATCH_INDEX_) {
          line_segment_fold_line_indices_[line_segment_index] = MIDDLE_FOLD_LINE_INDEX_;
          continue;
        }
    
        // if (line_segment_left_fold_lines[line_segment_index] == 0 || line_segment_right_fold_lines[line_segment_index] == 0)
        //   index = -index - 1;
	
	line_segment_fold_line_indices_[line_segment_index] = new_patch_index_map[index];
      }

      // if (BUILD_COMPLETE_POPUP_GRAPH_) {
      // 	int y = 300;
      // 	for (int x = 290; x <= 315; x++)
      // 	  cout << x << '\t' << pixel_line_segment_indices_[y * IMAGE_WIDTH_ + x] << '\t' << line_segment_left_fold_lines[pixel_line_segment_indices_[y * IMAGE_WIDTH_ + x]] << '\t' << line_segment_right_fold_lines[pixel_line_segment_indices_[y * IMAGE_WIDTH_ + x]] << endl;
      // 	for (vector<int>::const_iterator position_it = fold_lines_[43].positions.begin(); position_it != fold_lines_[43].positions.end(); position_it++)
      // 	  cout << *position_it / IMAGE_WIDTH_ << endl;
      // 	exit(1);
      // }
      
      
      //write new fold lines
      {
	map<int, vector<int> > fold_line_positions;
	map<int, int> fold_line_original_patch_indices;
	for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
	  int line_segment_index = pixel_line_segment_indices_[pixel];
	  int fold_line_index = line_segment_fold_line_indices_[line_segment_index];
          if (fold_line_index >= NUM_ORIGINAL_FOLD_LINES_) {
	    fold_line_positions[fold_line_index].push_back(pixel);
	    fold_line_original_patch_indices[fold_line_index] = line_segment_original_patch_indices[line_segment_index];
	  }
	}
	for (map<int, int>::const_iterator fold_line_it = fold_line_original_patch_indices.begin(); fold_line_it != fold_line_original_patch_indices.end(); fold_line_it++)
	  fold_lines_.push_back(FoldLine(make_pair(fold_line_it->second, fold_line_it->second), fold_line_positions[fold_line_it->first]));
      }
    }

    //add left and right image borders as special fold lines
    {
      LEFT_BORDER_FOLD_LINE_INDEX_ = fold_lines_.size();
      fold_lines_.push_back(FoldLine(make_pair(ORIGINAL_BACKGROUND_PATCH_INDEX_, ORIGINAL_BACKGROUND_PATCH_INDEX_), vector<int>(1, 0)));
      
      RIGHT_BORDER_FOLD_LINE_INDEX_ = fold_lines_.size();
      fold_lines_.push_back(FoldLine(make_pair(ORIGINAL_BACKGROUND_PATCH_INDEX_, ORIGINAL_BACKGROUND_PATCH_INDEX_), vector<int>(1, IMAGE_WIDTH_ - 1)));
    }
  }

  //draw popup graph with original patches and fold lines
  Mat PopupGraph::drawOriginalPopupGraph()  
  {
    Mat original_graph_image = Mat::zeros(IMAGE_HEIGHT_, IMAGE_WIDTH_, CV_8UC3);
    //Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    map<int, Vec3b> patch_color_table;
    map<int, vector<int> > patch_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      int patch_index = patch_index_mask_[pixel];
      if (patch_color_table.count(patch_index) == 0) {
	if (patch_index < 0) {
	  int gray_value = rand() % 256;
	  patch_color_table[patch_index] = Vec3b(gray_value, gray_value, gray_value);
	} else
	  patch_color_table[patch_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
	//int gray_value = rand() % 256;
	//patch_color_table[patch_index] = Vec3b(gray_value, gray_value, gray_value);
      }
      original_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = patch_color_table[patch_index];
      patch_pixels[patch_index].push_back(pixel);
    }
    for (map<int, vector<int> >::const_iterator patch_it = patch_pixels.begin(); patch_it != patch_pixels.end(); patch_it++) {
      int pixel = patch_it->second[rand() % patch_it->second.size()];
      putText(original_graph_image, to_string(patch_it->first), Point(pixel % IMAGE_WIDTH_, pixel / IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(255, 0, 0));
    }

    map<int, vector<int> > fold_line_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      int fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[pixel]];
      if (fold_line_index < 0)
	continue;
      if (fold_line_index < NUM_ORIGINAL_FOLD_LINES_) {
	if (rand() % 5 == 0) {
	  if (patch_index_mask_[pixel] == fold_lines_[fold_line_index].original_patch_pair.first)
            original_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = Vec3b(255, 0, 0);
	  else
	    original_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = Vec3b(0, 255, 0);
	}
        fold_line_pixels[fold_line_index].push_back(pixel);
      }
    }
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_pixels.begin(); fold_line_it != fold_line_pixels.end(); fold_line_it++) {
      //int pixel = fold_line_it->second[rand() % fold_line_it->second.size()];
      int pixel = fold_lines_[fold_line_it->first].desirable_center;
      putText(original_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
    }

    return original_graph_image;
    //imwrite("Test/original_graph_image.bmp", original_graph_image);
  }

  //draw popup graph with all patches and fold lines
  Mat PopupGraph::drawPopupGraph()  
  {
    Mat new_graph_image = Mat::zeros(IMAGE_HEIGHT_, IMAGE_WIDTH_, CV_8UC3);
    new_graph_image.setTo(Vec3b(255, 255, 255));
    //Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    map<int, Vec3b> fold_line_color_table;
    map<int, vector<int> > fold_line_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      int fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[pixel]];
      if (fold_line_color_table.count(fold_line_index) == 0) {
	if (fold_line_index < 0) {
	  int gray_value = rand() % 256;
	  fold_line_color_table[fold_line_index] = Vec3b(gray_value, gray_value, gray_value);
	} else if (fold_line_index < NUM_ORIGINAL_FOLD_LINES_) {
	  //if (patch_index_mask_[pixel] == fold_lines_[fold_line_index].original_patch_pair.first)
	  //fold_line_color_table[fold_line_index] = Vec3b(0, 0, 0);
          //else
	  fold_line_color_table[fold_line_index] = Vec3b(0, 255, 0);
	} else
	  fold_line_color_table[fold_line_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
      }
      if (fold_line_index < 0 || fold_line_index >= NUM_ORIGINAL_FOLD_LINES_)
	new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = fold_line_color_table[fold_line_index];
      else {
	if (patch_index_mask_[pixel] == fold_lines_[fold_line_index].original_patch_pair.first && rand() % 5 == 0)
	  new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = Vec3b(0, 255, 0);
        //else
	//new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = Vec3b(0, 0, 0);
      }
      fold_line_pixels[fold_line_index].push_back(pixel);
    }

    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_pixels.begin(); fold_line_it != fold_line_pixels.end(); fold_line_it++) {
      int pixel = fold_line_it->second[rand() % fold_line_it->second.size()];
      if (fold_line_it->first < 0)
	continue;
      else if (fold_line_it->first < NUM_ORIGINAL_FOLD_LINES_)
	putText(new_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
      else
        putText(new_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(255, 0, 0));
    }

    return new_graph_image;
    //imwrite("Test/new_graph_image.bmp", new_graph_image);
  }

  //draw optimized popup graph
  Mat PopupGraph::drawOptimizedPopupGraph()  
  {
    cout << "draw optimized popup graph" << endl;
    Mat optimized_graph_image = Mat::zeros(IMAGE_HEIGHT_, IMAGE_WIDTH_, CV_8UC3);
    optimized_graph_image.setTo(Vec3b(255, 255, 255));
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      vector<int> neighbor_pixels = Popup::findNeighbors(pixel, IMAGE_WIDTH_, IMAGE_HEIGHT_);
      for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++)
	if (patch_index_mask_[*neighbor_pixel_it] != patch_index_mask_[pixel])
	  optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = Vec3b(0, 0, 0);
    }

    for (int fold_line_index = 0; fold_line_index < getNumOriginalFoldLines(); fold_line_index++) {
      if (fold_lines_[fold_line_index].optimized_activity != 1)      
        continue;

      int fold_line_min_y = IMAGE_HEIGHT_ - 1;
      int fold_line_max_y = 0;
      {
	vector<int> fold_line_optimized_positions;
	for (vector<int>::const_iterator position_it = fold_lines_[fold_line_index].positions.begin(); position_it != fold_lines_[fold_line_index].positions.end(); position_it++)
	  if (*position_it % IMAGE_WIDTH_ == fold_lines_[fold_line_index].optimized_position)
	    fold_line_optimized_positions.push_back(*position_it);
	if (fold_line_optimized_positions.size() == 0) {
	  fold_line_min_y = max(fold_lines_[fold_line_index].desirable_center / IMAGE_WIDTH_ - FOLD_LINE_WINDOW_HEIGHT_ / 2, 0);
	  fold_line_max_y = max(fold_lines_[fold_line_index].desirable_center / IMAGE_WIDTH_ + FOLD_LINE_WINDOW_HEIGHT_ / 2, 0);
	  fold_lines_[fold_line_index].optimized_pixels.push_back(fold_lines_[fold_line_index].desirable_center);
	  if (false) {
	    cout << fold_line_index << endl;
	    cout << fold_lines_[fold_line_index].optimized_position << endl;
	    for (vector<int>::const_iterator position_it = fold_lines_[fold_line_index].positions.begin(); position_it != fold_lines_[fold_line_index].positions.end(); position_it++)
	      cout << *position_it % IMAGE_WIDTH_ << endl;
	    cout << "optimized fold line position not available" << endl;
	    exit(1);
	  }
	} else if (fold_line_optimized_positions.size() == 1) {
	  fold_line_min_y = max(fold_line_optimized_positions[0] / IMAGE_WIDTH_ - FOLD_LINE_WINDOW_HEIGHT_ / 2, 0);
          fold_line_max_y = max(fold_line_optimized_positions[0] / IMAGE_WIDTH_ + FOLD_LINE_WINDOW_HEIGHT_ / 2, IMAGE_HEIGHT_ - 1);
	  fold_lines_[fold_line_index].optimized_pixels = fold_line_optimized_positions;
        } else {
	  for (vector<int>::const_iterator position_it = fold_line_optimized_positions.begin(); position_it != fold_line_optimized_positions.end(); position_it++) {
	    fold_line_min_y = min(fold_line_min_y, *position_it / IMAGE_WIDTH_);
	    fold_line_max_y = max(fold_line_max_y, *position_it / IMAGE_WIDTH_);
	  }
	  fold_lines_[fold_line_index].optimized_pixels = fold_line_optimized_positions;
        }
      }
      
      Vec3b fold_line_color = fold_lines_[fold_line_index].optimized_convexity ? Vec3b(255, 0, 0) : Vec3b(0, 255, 0);
      for (int y = fold_line_min_y; y <= fold_line_max_y; y++)
	optimized_graph_image.at<Vec3b>(y, fold_lines_[fold_line_index].optimized_position) = fold_line_color;
      putText(optimized_graph_image, to_string(fold_line_index), Point(fold_lines_[fold_line_index].optimized_position, (fold_line_min_y + fold_line_max_y) / 2), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
      //      if (fold_line_index == 27) {
      //cout << fold_line_index << '\t' << fold_lines_[fold_line_index].optimized_pixels.size() << '\t' << fold_lines_[fold_line_index].optimized_position << endl;
    }
    
    map<int, vector<int> > fold_line_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      int fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[pixel]];
      if (fold_line_index < getNumOriginalFoldLines())
        continue;
      if (fold_lines_[fold_line_index].optimized_activity != 1)
        continue;
      if (fold_lines_[fold_line_index].optimized_position != pixel % IMAGE_WIDTH_)
        continue;
      Vec3b fold_line_color = fold_lines_[fold_line_index].optimized_convexity ? Vec3b(255, 0, 0) : Vec3b(0, 255, 0);
      optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = fold_line_color;
      fold_line_pixels[fold_line_index].push_back(pixel);

      fold_lines_[fold_line_index].optimized_pixels.push_back(pixel);
    }

    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_pixels.begin(); fold_line_it != fold_line_pixels.end(); fold_line_it++) {
      int pixel = fold_line_it->second[rand() % fold_line_it->second.size()];
      putText(optimized_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
    }

    return optimized_graph_image;
    //imwrite("Test/new_graph_image.bmp", new_graph_image);
  }

  //find neighboring fold line pairs
  void PopupGraph::findFoldLinePairs()
  {
    map<int, set<int> > fold_line_left_neighbors;
    map<int, set<int> > fold_line_right_neighbors;
    {
      vector<set<int> > line_segment_fold_line_indices(IMAGE_WIDTH_ * IMAGE_HEIGHT_);
      for (int fold_line_index = 0; fold_line_index < getNumOriginalFoldLines(); fold_line_index++)
        for (set<int>::const_iterator line_segment_index_it = fold_lines_[fold_line_index].line_segment_indices.begin(); line_segment_index_it != fold_lines_[fold_line_index].line_segment_indices.end(); line_segment_index_it++)
	  line_segment_fold_line_indices[*line_segment_index_it].insert(fold_line_index);


      bool fix_close_fold_line_issue = true;
      while (fix_close_fold_line_issue) {
	bool has_change = false;
	for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {        
	  if (pixel % IMAGE_WIDTH_ == IMAGE_WIDTH_ - 1)     
	    continue;       
	  int neighbor_pixel = pixel + 1;   
	  if (patch_index_mask_[pixel] != patch_index_mask_[neighbor_pixel])        
	    continue;       
	  int left_line_segment_index = pixel_line_segment_indices_[pixel];
	  int right_line_segment_index = pixel_line_segment_indices_[neighbor_pixel];
	  set<int> left_fold_line_indices = line_segment_fold_line_indices[left_line_segment_index];
	  set<int> right_fold_line_indices = line_segment_fold_line_indices[right_line_segment_index];
	  if (right_fold_line_indices.size() > 0) {
	    for (set<int>::const_iterator left_fold_line_it = left_fold_line_indices.begin(); left_fold_line_it != left_fold_line_indices.end(); left_fold_line_it++) {
	      if (right_fold_line_indices.count(*left_fold_line_it) == 0) {
		line_segment_fold_line_indices[right_line_segment_index].insert((*left_fold_line_it));
		// cout << "left" << endl;
        	// cout << pixel % IMAGE_WIDTH_ << '\t' << pixel / IMAGE_WIDTH_ << endl;
                // cout << *left_fold_line_it << endl;
		// exit(1);
        	has_change = true;
	      }
	    }
	  }
          if (left_fold_line_indices.size() > 0) {
            for (set<int>::const_iterator right_fold_line_it = right_fold_line_indices.begin(); right_fold_line_it != right_fold_line_indices.end(); right_fold_line_it++) {
              if (left_fold_line_indices.count(*right_fold_line_it) == 0) {
		// cout << "right" << endl;
        	// cout << pixel % IMAGE_WIDTH_ + 1 << '\t' << pixel / IMAGE_WIDTH_ << endl;
                // cout << *right_fold_line_it << endl;
		// exit(1);
                line_segment_fold_line_indices[left_line_segment_index].insert((*right_fold_line_it));
                has_change = true;
              }
            }
          }
	}
	if (has_change == false)          
          break;          
      }  

      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++)
        if (line_segment_fold_line_indices_[line_segment_index] >= 0)
          line_segment_fold_line_indices[line_segment_index].insert(line_segment_fold_line_indices_[line_segment_index]);

      // if (BUILD_COMPLETE_POPUP_GRAPH_) {
      // 	cout << 22141 << '\t' << patch_index_mask_[22141] << endl;
      // 	for (set<int>::const_iterator fold_line_it = line_segment_fold_line_indices[pixel_line_segment_indices_[22141]].begin(); fold_line_it != line_segment_fold_line_indices[pixel_line_segment_indices_[22141]].end(); fold_line_it++)
      // 	  cout << *fold_line_it << endl;
      // 	cout << 22142 << '\t' << patch_index_mask_[22142] << endl;
      // 	for (set<int>::const_iterator fold_line_it = line_segment_fold_line_indices[pixel_line_segment_indices_[22142]].begin(); fold_line_it != line_segment_fold_line_indices[pixel_line_segment_indices_[22142]].end(); fold_line_it++)
      // 	  cout << *fold_line_it << endl;
      // 	exit(1);
      // }
      for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {    
	if (pixel % IMAGE_WIDTH_ == IMAGE_WIDTH_ - 1)         
	  continue;   
	int neighbor_pixel = pixel + 1;       
	if (patch_index_mask_[pixel] != patch_index_mask_[neighbor_pixel])    
	  continue;   
	int left_line_segment_index = pixel_line_segment_indices_[pixel];     
	int right_line_segment_index = pixel_line_segment_indices_[neighbor_pixel];   
	set<int> left_fold_line_indices = line_segment_fold_line_indices[left_line_segment_index];    
	set<int> right_fold_line_indices = line_segment_fold_line_indices[right_line_segment_index];
	// if (BUILD_COMPLETE_POPUP_GRAPH_) {
	//   if (left_fold_line_indices.count(27) && right_fold_line_indices.count(55))
        //     cout << pixel << '\t' << neighbor_pixel << endl;

	//   if (left_fold_line_indices.count(55))
	//     cout << pixel << endl;
        // }
	// if (pixel == 338 * IMAGE_WIDTH + 382) {
	//   cout << left_new_patch_index << '\t' << right_fold_line_index << endl;
	//   exit(1);
	// }

	for (set<int>::const_iterator left_fold_line_index_it = left_fold_line_indices.begin(); left_fold_line_index_it != left_fold_line_indices.end(); left_fold_line_index_it++) {
	  for (set<int>::const_iterator right_fold_line_index_it = right_fold_line_indices.begin(); right_fold_line_index_it != right_fold_line_indices.end(); right_fold_line_index_it++) {
	    if (*left_fold_line_index_it != *right_fold_line_index_it && (*left_fold_line_index_it >= NUM_ORIGINAL_FOLD_LINES_ || *right_fold_line_index_it >= NUM_ORIGINAL_FOLD_LINES_)) {
	      fold_line_right_neighbors[*left_fold_line_index_it].insert(*right_fold_line_index_it);
	      fold_line_left_neighbors[*right_fold_line_index_it].insert(*left_fold_line_index_it);
	    }
	  }
	}
      }
    }
    
    fold_line_pairs_.clear();
    {
      for (map<int, set<int> >::const_iterator fold_line_it = fold_line_left_neighbors.begin(); fold_line_it != fold_line_left_neighbors.end(); fold_line_it++)
	for (set<int>::const_iterator neighbor_it = fold_line_it->second.begin(); neighbor_it != fold_line_it->second.end(); neighbor_it++)
	  fold_line_pairs_.push_back(make_pair(*neighbor_it, fold_line_it->first));
      for (map<int, set<int> >::const_iterator fold_line_it = fold_line_right_neighbors.begin(); fold_line_it != fold_line_right_neighbors.end(); fold_line_it++)
	for (set<int>::const_iterator neighbor_it = fold_line_it->second.begin(); neighbor_it != fold_line_it->second.end(); neighbor_it++)
	  fold_line_pairs_.push_back(make_pair(fold_line_it->first, *neighbor_it));

    
      {
	for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
	  if (fold_line_it->is_original_fold_line == true && fold_line_it->original_patch_pair.first == ORIGINAL_BACKGROUND_PATCH_INDEX_)
            fold_line_pairs_.push_back(make_pair(LEFT_BORDER_FOLD_LINE_INDEX_, fold_line_it - fold_lines_.begin()));
	
	fold_line_pairs_.push_back(make_pair(LEFT_BORDER_FOLD_LINE_INDEX_, MIDDLE_FOLD_LINE_INDEX_));
      }
      {
	for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
	  if (fold_line_it->is_original_fold_line == true && fold_line_it->original_patch_pair.second == ORIGINAL_BACKGROUND_PATCH_INDEX_)
	    fold_line_pairs_.push_back(make_pair(fold_line_it - fold_lines_.begin(), RIGHT_BORDER_FOLD_LINE_INDEX_));
	
        fold_line_pairs_.push_back(make_pair(MIDDLE_FOLD_LINE_INDEX_, RIGHT_BORDER_FOLD_LINE_INDEX_));
      }
    }

    //remove fold line pairs if both fold lines are original fold lines
    {
      vector<pair<int, int> > new_fold_line_pairs;
      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_.begin(); fold_line_pair_it != fold_line_pairs_.end(); fold_line_pair_it++)
        if (fold_line_pair_it->first >= getNumOriginalFoldLines() || fold_line_pair_it->second >= getNumOriginalFoldLines())
	  new_fold_line_pairs.push_back(*fold_line_pair_it);
      fold_line_pairs_ = new_fold_line_pairs;
    }

    //add missing fold lines if a fold line does not have a left neighbor or right neighbor
    {
      map<int, set<int> > fold_line_left_neighbors;
      map<int, set<int> > fold_line_right_neighbors;
      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_.begin(); fold_line_pair_it != fold_line_pairs_.end(); fold_line_pair_it++) {
        int left_fold_line_index = fold_line_pair_it->first;
        int right_fold_line_index = fold_line_pair_it->second;
        fold_line_right_neighbors[left_fold_line_index].insert(right_fold_line_index);
        fold_line_left_neighbors[right_fold_line_index].insert(left_fold_line_index);
      }

      map<int, vector<int> > fold_line_positions;
      for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
	int fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[pixel]];
	if (fold_line_index >= 0 && fold_line_index < getNumOriginalFoldLines())
	  fold_line_positions[fold_line_index].push_back(pixel);
      }
      
      for (int fold_line_index = 0; fold_line_index < getNumOriginalFoldLines(); fold_line_index++) {
	if (fold_line_left_neighbors.count(fold_line_index) == 0) {
	  //for (vector<int>::const_iterator position_it = fold_lines_[fold_line_index].positions.begin(); position_it != fold_lines_[fold_line_index].positions.end(); position_it++) {
	  for (vector<int>::const_iterator position_it = fold_line_positions[fold_line_index].begin(); position_it != fold_line_positions[fold_line_index].end(); position_it++) {
	    bool neighbor_fold_line_found = false;
	    int neighbor_position = *position_it - 1;
	    int patch_index = patch_index_mask_[*position_it];
            while ((neighbor_position + 1) % IMAGE_WIDTH_ > 0) {
	      if (patch_index_mask_[neighbor_position] != fold_lines_[fold_line_index].original_patch_pair.first || patch_index_mask_[neighbor_position] != patch_index)
		break;
	      int neighbor_fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[neighbor_position]];
	      if (neighbor_fold_line_index > getNumOriginalFoldLines()) {
		fold_line_pairs_.push_back(make_pair(neighbor_fold_line_index, fold_line_index));
		neighbor_fold_line_found = true;
		break;
	      }
	      neighbor_position--;
	    }
	    if (neighbor_fold_line_found)
	      break;
	  }
	}
	if (fold_line_right_neighbors.count(fold_line_index) == 0) {
	  for (vector<int>::const_iterator position_it = fold_line_positions[fold_line_index].begin(); position_it != fold_line_positions[fold_line_index].end(); position_it++) {
	    if (fold_line_index == 27)
	      cout << *position_it % IMAGE_WIDTH_ << '\t' << *position_it << endl;
            bool neighbor_fold_line_found = false;
            int neighbor_position = *position_it + 1;
	    int patch_index = patch_index_mask_[*position_it];
            while ((neighbor_position - 1) % IMAGE_WIDTH_ < IMAGE_WIDTH_ - 1) {
              if (patch_index_mask_[neighbor_position] != fold_lines_[fold_line_index].original_patch_pair.second || patch_index_mask_[neighbor_position] != patch_index)
                break;
              int neighbor_fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[neighbor_position]];
              if (neighbor_fold_line_index > getNumOriginalFoldLines()) {
                fold_line_pairs_.push_back(make_pair(fold_line_index, neighbor_fold_line_index));
                neighbor_fold_line_found = true;
                break;
              }
              neighbor_position++;
            }
            if (neighbor_fold_line_found)
              break;
          }
	}
      }
    }

    // if (BUILD_COMPLETE_POPUP_GRAPH_) {
    //   cout << "fold line pairs: " << endl;
    //   checkFoldLinePairs();
    //   exit(1);
    // }

    //pass neighbor relation across original fold lines
    {
      fold_line_pairs_without_passing_ = fold_line_pairs_;
      
      set<pair<int, int> > invalid_fold_line_pairs;
      vector<pair<int, int> > new_fold_line_pairs;
      map<int, set<int> > fold_line_left_neighbors;
      map<int, set<int> > fold_line_right_neighbors;
      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_.begin(); fold_line_pair_it != fold_line_pairs_.end(); fold_line_pair_it++) {
        int left_fold_line_index = fold_line_pair_it->first;
        int right_fold_line_index = fold_line_pair_it->second;
	fold_line_right_neighbors[left_fold_line_index].insert(right_fold_line_index);
	fold_line_left_neighbors[right_fold_line_index].insert(left_fold_line_index);
      }

      for (int fold_line_index = 0; fold_line_index < getNumOriginalFoldLines(); fold_line_index++) {
        {
          int left_original_patch_index = fold_lines_[fold_line_index].original_patch_pair.first;
	  
	  set<int> same_patch_left_neighbors;
	  for (set<int>::const_iterator neighbor_it = fold_line_left_neighbors[fold_line_index].begin(); neighbor_it != fold_line_left_neighbors[fold_line_index].end(); neighbor_it++)
	    if (*neighbor_it >= getNumOriginalFoldLines() && fold_lines_[*neighbor_it].original_patch_pair.first == left_original_patch_index)
	      same_patch_left_neighbors.insert(*neighbor_it);
	  
	  set<int> same_patch_right_neighbors;
	  for (set<int>::const_iterator neighbor_it = fold_line_right_neighbors[fold_line_index].begin(); neighbor_it != fold_line_right_neighbors[fold_line_index].end(); neighbor_it++)
	    if (*neighbor_it >= getNumOriginalFoldLines() && fold_lines_[*neighbor_it].original_patch_pair.first == left_original_patch_index)
	      same_patch_right_neighbors.insert(*neighbor_it);
	  
	  for (set<int>::const_iterator right_neighbor_it = same_patch_right_neighbors.begin(); right_neighbor_it != same_patch_right_neighbors.end(); right_neighbor_it++) {
            invalid_fold_line_pairs.insert(make_pair(fold_line_index, *right_neighbor_it));
	    for (set<int>::const_iterator left_neighbor_it = same_patch_left_neighbors.begin(); left_neighbor_it != same_patch_left_neighbors.end(); left_neighbor_it++)
	      new_fold_line_pairs.push_back(make_pair(*left_neighbor_it, *right_neighbor_it));
	  }
        }
        {
	  int right_original_patch_index = fold_lines_[fold_line_index].original_patch_pair.second;
	  
	  set<int> same_patch_left_neighbors;
	  for (set<int>::const_iterator neighbor_it = fold_line_left_neighbors[fold_line_index].begin(); neighbor_it != fold_line_left_neighbors[fold_line_index].end(); neighbor_it++)
	    if (*neighbor_it >= getNumOriginalFoldLines() && fold_lines_[*neighbor_it].original_patch_pair.first == right_original_patch_index)
	      same_patch_left_neighbors.insert(*neighbor_it);
	    
	  set<int> same_patch_right_neighbors;
	  for (set<int>::const_iterator neighbor_it = fold_line_right_neighbors[fold_line_index].begin(); neighbor_it != fold_line_right_neighbors[fold_line_index].end(); neighbor_it++)
	    if (*neighbor_it >= getNumOriginalFoldLines() && fold_lines_[*neighbor_it].original_patch_pair.first == right_original_patch_index)
	      same_patch_right_neighbors.insert(*neighbor_it);
	  
	  for (set<int>::const_iterator left_neighbor_it = same_patch_left_neighbors.begin(); left_neighbor_it != same_patch_left_neighbors.end(); left_neighbor_it++) {
	    invalid_fold_line_pairs.insert(make_pair(*left_neighbor_it, fold_line_index));
	    for (set<int>::const_iterator right_neighbor_it = same_patch_right_neighbors.begin(); right_neighbor_it != same_patch_right_neighbors.end(); right_neighbor_it++)
              new_fold_line_pairs.push_back(make_pair(*left_neighbor_it, *right_neighbor_it));
          }
	}
      }

      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_.begin(); fold_line_pair_it != fold_line_pairs_.end(); fold_line_pair_it++)
	if (invalid_fold_line_pairs.count(*fold_line_pair_it) == 0)
	  new_fold_line_pairs.push_back(*fold_line_pair_it);
      fold_line_pairs_ = new_fold_line_pairs;
    }

    sort(fold_line_pairs_.begin(), fold_line_pairs_.end());
    fold_line_pairs_.erase(unique(fold_line_pairs_.begin(), fold_line_pairs_.end()), fold_line_pairs_.end());

    
    sort(fold_line_pairs_without_passing_.begin(), fold_line_pairs_without_passing_.end());
    fold_line_pairs_without_passing_.erase(unique(fold_line_pairs_without_passing_.begin(), fold_line_pairs_without_passing_.end()), fold_line_pairs_without_passing_.end());
  }
  
  void PopupGraph::findFoldLinePaths()
  {
    //group fold line pairs based on their original patch index
    vector<vector<pair<int, int> > > original_patch_fold_line_pairs(NUM_ORIGINAL_PATCHES_);
    {
      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_.begin(); fold_line_pair_it != fold_line_pairs_.end(); fold_line_pair_it++) {
	int left_fold_line_index = fold_line_pair_it->first;
	int right_fold_line_index = fold_line_pair_it->second;
	//int left_original_patch_index = fold_lines_[left_fold_line_index].is_original_fold_line ? fold_lines_[left_fold_line_index].original_patch_pair.first : fold_lines_[left_fold_line_index].original_patch_index;
	//int right_original_patch_index = fold_lines_[right_fold_line_index].is_original_fold_line ? fold_lines_[right_fold_line_index].original_patch_pair.first : fold_lines_[right_fold_line_index].original_patch_index;
	// if (left_fold_line_index == 26 && right_fold_line_index == 91) {
	//   cout << left_original_patch_index << '\t' << right_original_patch_index << endl;
	//   exit(1);
	// }
	pair<int, int> left_original_patch_pair = fold_lines_[left_fold_line_index].original_patch_pair;
	pair<int, int> right_original_patch_pair = fold_lines_[right_fold_line_index].original_patch_pair;
	if (left_original_patch_pair.first == right_original_patch_pair.first)
	  original_patch_fold_line_pairs[left_original_patch_pair.first].push_back(*fold_line_pair_it);
        if (left_original_patch_pair.first == right_original_patch_pair.second)
	  original_patch_fold_line_pairs[left_original_patch_pair.first].push_back(*fold_line_pair_it);
        if (left_original_patch_pair.second == right_original_patch_pair.first)
	  original_patch_fold_line_pairs[left_original_patch_pair.second].push_back(*fold_line_pair_it);
	if (left_original_patch_pair.second == right_original_patch_pair.second)
	  original_patch_fold_line_pairs[left_original_patch_pair.second].push_back(*fold_line_pair_it);
      }
    }
    
    for (vector<vector<pair<int, int> > >::const_iterator patch_it = original_patch_fold_line_pairs.begin(); patch_it != original_patch_fold_line_pairs.end(); patch_it++) {
      map<int, map<int, vector<int> > > patch_fold_line_left_paths;
      map<int, map<int, vector<int> > > patch_fold_line_right_paths;
      vector<pair<int, int> > same_patch_fold_line_pairs = *patch_it;
      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = same_patch_fold_line_pairs.begin(); fold_line_pair_it != same_patch_fold_line_pairs.end(); fold_line_pair_it++) {
	int left_fold_line_index = fold_line_pair_it->first;
	int right_fold_line_index = fold_line_pair_it->second;
	patch_fold_line_left_paths[right_fold_line_index][left_fold_line_index] = vector<int>();
	patch_fold_line_right_paths[left_fold_line_index][right_fold_line_index] = vector<int>();
      }
      
      if (patch_it - original_patch_fold_line_pairs.begin() != ORIGINAL_BACKGROUND_PATCH_INDEX_) {
	while (true) {
	  bool has_change = false;
	  for (vector<pair<int, int> >::const_iterator fold_line_pair_it = same_patch_fold_line_pairs.begin(); fold_line_pair_it != same_patch_fold_line_pairs.end(); fold_line_pair_it++) {
	    int left_fold_line_index = fold_line_pair_it->first;
	    int right_fold_line_index = fold_line_pair_it->second;

	    for (map<int, vector<int> >::const_iterator fold_line_it = patch_fold_line_left_paths[left_fold_line_index].begin(); fold_line_it != patch_fold_line_left_paths[left_fold_line_index].end(); fold_line_it++) {
	      if (patch_fold_line_left_paths[right_fold_line_index].count(fold_line_it->first) == 0 && fold_line_it->first != right_fold_line_index) {
		vector<int> path = fold_line_it->second;
		if (find(path.begin(), path.end(), right_fold_line_index) < path.end())
		  continue;
		path.push_back(left_fold_line_index);
		patch_fold_line_left_paths[right_fold_line_index][fold_line_it->first] = path;
		has_change = true;
	      }
	    }
	    for (map<int, vector<int> >::const_iterator fold_line_it = patch_fold_line_right_paths[left_fold_line_index].begin(); fold_line_it != patch_fold_line_right_paths[left_fold_line_index].end(); fold_line_it++) {
	      if (patch_fold_line_left_paths[right_fold_line_index].count(fold_line_it->first) == 0 && fold_line_it->first != right_fold_line_index) {
		vector<int> path = fold_line_it->second;
		if (find(path.begin(), path.end(), right_fold_line_index) < path.end())
		  continue;
		patch_fold_line_left_paths[right_fold_line_index][fold_line_it->first] = path;
		has_change = true;
	      }
	    }

	    for (map<int, vector<int> >::const_iterator fold_line_it = patch_fold_line_right_paths[right_fold_line_index].begin(); fold_line_it != patch_fold_line_right_paths[right_fold_line_index].end(); fold_line_it++) {
	      if (patch_fold_line_right_paths[left_fold_line_index].count(fold_line_it->first) == 0 && fold_line_it->first != left_fold_line_index) {
		vector<int> path = fold_line_it->second;
		if (find(path.begin(), path.end(), left_fold_line_index) < path.end())
		  continue;
		path.push_back(right_fold_line_index);
		patch_fold_line_right_paths[left_fold_line_index][fold_line_it->first] = path;
		has_change = true;
	      }
	    }
	    for (map<int, vector<int> >::const_iterator fold_line_it = patch_fold_line_left_paths[right_fold_line_index].begin(); fold_line_it != patch_fold_line_left_paths[right_fold_line_index].end(); fold_line_it++) {
	      if (patch_fold_line_right_paths[left_fold_line_index].count(fold_line_it->first) == 0 && fold_line_it->first != left_fold_line_index) {
		vector<int> path = fold_line_it->second;
		if (find(path.begin(), path.end(), left_fold_line_index) < path.end())
		  continue;
		patch_fold_line_right_paths[left_fold_line_index][fold_line_it->first] = path;
		has_change = true;
	      }
	    }
	  }
	  if (has_change == false)
	    break;
	}
      }

      for (map<int, map<int, vector<int> > >::const_iterator fold_line_it_1 = patch_fold_line_left_paths.begin(); fold_line_it_1 != patch_fold_line_left_paths.end(); fold_line_it_1++)
	for (map<int, vector<int> >::const_iterator fold_line_it_2 = fold_line_it_1->second.begin(); fold_line_it_2 != fold_line_it_1->second.end(); fold_line_it_2++)
	  fold_line_left_paths_[fold_line_it_1->first][fold_line_it_2->first] = fold_line_it_2->second;
      for (map<int, map<int, vector<int> > >::const_iterator fold_line_it_1 = patch_fold_line_right_paths.begin(); fold_line_it_1 != patch_fold_line_right_paths.end(); fold_line_it_1++)
	for (map<int, vector<int> >::const_iterator fold_line_it_2 = fold_line_it_1->second.begin(); fold_line_it_2 != fold_line_it_1->second.end(); fold_line_it_2++)
	  fold_line_right_paths_[fold_line_it_1->first][fold_line_it_2->first] = fold_line_it_2->second;
    }

    //remove fold line paths if there are both a left path and a right path from one patch to another
    {
      map<int, set<int> > invalid_fold_line_pairs;
      for (int fold_line_index = 0; fold_line_index < getNumFoldLines(); fold_line_index++)
        for (int other_fold_line_index = 0; other_fold_line_index < getNumFoldLines(); other_fold_line_index++)
	  if ((fold_line_left_paths_.count(fold_line_index) > 0 && fold_line_left_paths_.at(fold_line_index).count(other_fold_line_index) > 0) && (fold_line_right_paths_.count(fold_line_index) > 0 && fold_line_right_paths_.at(fold_line_index).count(other_fold_line_index) > 0))
	    invalid_fold_line_pairs[fold_line_index].insert(other_fold_line_index);
	    
      map<int, map<int, vector<int> > > new_fold_line_left_paths;
      for (int fold_line_index = 0; fold_line_index < getNumFoldLines(); fold_line_index++)
	for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths_[fold_line_index].begin(); fold_line_it != fold_line_left_paths_[fold_line_index].end(); fold_line_it++)
	  if (invalid_fold_line_pairs[fold_line_index].count(fold_line_it->first) == 0)
	    new_fold_line_left_paths[fold_line_index][fold_line_it->first] = fold_line_it->second;
      fold_line_left_paths_ = new_fold_line_left_paths;
  
      map<int, map<int, vector<int> > > new_fold_line_right_paths;
      for (int fold_line_index = 0; fold_line_index < getNumFoldLines(); fold_line_index++)
	for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths_[fold_line_index].begin(); fold_line_it != fold_line_right_paths_[fold_line_index].end(); fold_line_it++)
	  if (invalid_fold_line_pairs[fold_line_index].count(fold_line_it->first) == 0)
	    new_fold_line_right_paths[fold_line_index][fold_line_it->first] = fold_line_it->second;
      fold_line_right_paths_ = new_fold_line_right_paths;
    }
  }  

  //compute line segments
  void PopupGraph::calcLineSegmentInfo()
  {
    pixel_line_segment_indices_.assign(IMAGE_WIDTH_ * IMAGE_HEIGHT_, -1);
    int line_segment_index = -1;
    for (int x = 0; x < IMAGE_WIDTH_; x++) {
      int previous_patch_index = -1;
      for (int y = 0; y < IMAGE_HEIGHT_; y++) {
	if (patch_index_mask_[y * IMAGE_WIDTH_ + x] == -1)
	  continue;
	if (patch_index_mask_[y * IMAGE_WIDTH_ + x] != previous_patch_index)
          line_segment_index++;
	pixel_line_segment_indices_[y * IMAGE_WIDTH_ + x] = line_segment_index;
        previous_patch_index = patch_index_mask_[y * IMAGE_WIDTH_ + x];
      }
    }
  
    NUM_LINE_SEGMENTS_ = line_segment_index + 1;
  }

  void PopupGraph::checkFoldLinePairs() const
  {
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_.begin(); fold_line_pair_it != fold_line_pairs_.end(); fold_line_pair_it++) {
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      cout << "fold line pairs: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
    }
  }

  void PopupGraph::checkFoldLinePaths() const
  {
    for (int fold_line_index = 0; fold_line_index < getNumFoldLines(); fold_line_index++) {
      cout << "fold line index: " << fold_line_index << endl;
      //cout << fold_line_left_paths_.count(fold_line_index) << '\t' << fold_line_right_paths_.count(fold_line_index) << endl;
      if (fold_line_left_paths_.count(fold_line_index) > 0) {
	for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths_.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths_.at(fold_line_index).end(); fold_line_it++) {
	  cout << "left: " << fold_line_it->first << endl;
	  for (vector<int>::const_iterator path_it = fold_line_it->second.begin(); path_it != fold_line_it->second.end(); path_it++)
	    cout << *path_it << '\t';
	  cout << endl;
	}
      }
      if (fold_line_right_paths_.count(fold_line_index) > 0) {
	for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths_.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths_.at(fold_line_index).end(); fold_line_it++) {
	  cout << "right: " << fold_line_it->first << endl;
	  for (vector<int>::const_iterator path_it = fold_line_it->second.begin(); path_it != fold_line_it->second.end(); path_it++)
	    cout << *path_it << '\t';
	  cout << endl;
	}
      }
    } 
  }

  void PopupGraph::checkFoldLineInfo() const
  {
    for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
      cout << "index: " << fold_line_it - fold_lines_.begin() << "\tleft: " << fold_line_it->original_patch_pair.first << "\tright: " << fold_line_it->original_patch_pair.second << "\tscore: " << fold_line_it->score << "\tx: " << fold_line_it->desirable_center % IMAGE_WIDTH_ << endl;
  }

  map<int, set<int> >  PopupGraph::getIslandPatchInfo() const
  {
    map<int, set<int> > island_patch_fold_lines_map;
    {
      map<int, set<int> > patch_child_patches = getPatchChildPatches();
      
      std::map<int, std::map<int, std::set<int> > > patch_neighbor_fold_lines = getPatchNeighborFoldLines('B', patch_child_patches);
      for(int patch_index = 0; patch_index < getNumOriginalPatches(); patch_index++) {
	if (patch_index == getOriginalBackgroundPatchIndex())
          continue;
        if (patch_neighbor_fold_lines[patch_index].size() > 1)
	  continue;

	set<int> patch_fold_lines;
        for (map<int, set<int> >::const_iterator neighbor_patch_it = patch_neighbor_fold_lines.at(patch_index).begin(); neighbor_patch_it != patch_neighbor_fold_lines.at(patch_index).end(); neighbor_patch_it++)
	  for (set<int>::const_iterator fold_line_it = neighbor_patch_it->second.begin(); fold_line_it != neighbor_patch_it->second.end(); fold_line_it++)
            patch_fold_lines.insert(*fold_line_it);
    
	vector<bool> island_patch_fold_line_mask(getNumFoldLines(), true);
	for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
	  if (fold_line_it - fold_lines_.begin() < getNumOriginalFoldLines() || fold_line_it->original_patch_pair.first == patch_index)
	    island_patch_fold_line_mask[fold_line_it - fold_lines_.begin()] = false;

	for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_without_passing_.begin(); fold_line_pair_it != fold_line_pairs_without_passing_.end(); fold_line_pair_it++) {
	  if (patch_fold_lines.count(fold_line_pair_it->first) == 0)
	    island_patch_fold_line_mask[fold_line_pair_it->second] = false;
          if (patch_fold_lines.count(fold_line_pair_it->second) == 0)
            island_patch_fold_line_mask[fold_line_pair_it->first] = false;
	}

	set<int> island_patch_fold_lines;
	for (int fold_line_index = 0; fold_line_index < getNumFoldLines(); fold_line_index++)
	  if (island_patch_fold_line_mask[fold_line_index])
	    island_patch_fold_lines.insert(fold_line_index);
	
	island_patch_fold_lines_map[patch_index] = island_patch_fold_lines;
      }
    }
    return island_patch_fold_lines_map;
  }
  
  void PopupGraph::setOptimizedFoldLineInfo(const vector<int> &optimized_fold_line_activities, const vector<int> &optimized_fold_line_convexities, const vector<int> &optimized_fold_line_positions)
  {
    for (vector<FoldLine>::iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
      fold_line_it->optimized_activity = optimized_fold_line_activities[fold_line_it - fold_lines_.begin()];
      fold_line_it->optimized_convexity = optimized_fold_line_convexities[fold_line_it - fold_lines_.begin()];
      fold_line_it->optimized_position = optimized_fold_line_positions[fold_line_it - fold_lines_.begin()];
    }
  }

  void PopupGraph::setOptimizedFoldLineInfo(const int fold_line_index, const int optimized_activity, const int optimized_convexity, const int optimized_position)
  {
    fold_lines_[fold_line_index].optimized_activity = optimized_activity;
    fold_lines_[fold_line_index].optimized_convexity = optimized_convexity;
    fold_lines_[fold_line_index].optimized_position = optimized_position;
  }

  void PopupGraph::getOptimizedFoldLineInfo(std::vector<int> &optimized_fold_line_activities, std::vector<int> &optimized_fold_line_convexities, std::vector<int> &optimized_fold_line_positions)
  {
    optimized_fold_line_activities.assign(fold_lines_.size(), -1);
    optimized_fold_line_convexities.assign(fold_lines_.size(), -1);
    optimized_fold_line_positions.assign(fold_lines_.size(), -1);
    
    for (vector<FoldLine>::iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
      optimized_fold_line_activities[fold_line_it - fold_lines_.begin()] = fold_line_it->optimized_activity;
      optimized_fold_line_convexities[fold_line_it - fold_lines_.begin()] = fold_line_it->optimized_convexity;
      optimized_fold_line_positions[fold_line_it - fold_lines_.begin()] = fold_line_it->optimized_position;
    }
  }

  void PopupGraph::getDesirableFoldLinePositions(vector<int> &desirable_fold_line_positions) const
  {
    desirable_fold_line_positions.assign(fold_lines_.size(), -1);
    for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
      desirable_fold_line_positions[fold_line_it - fold_lines_.begin()] = fold_line_it->desirable_center % IMAGE_WIDTH_;
    }
  }

  void PopupGraph::findPatchChildPatches(const set<int> &island_patches)
  {
    map<int, set<int> > patch_child_patches;
    map<int, set<int> > patch_neighbor_patches;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      int patch_index = patch_index_mask_[pixel];
      if (patch_index == -1)
        continue;
      vector<int> neighbor_pixels = Popup::findNeighbors(pixel, IMAGE_WIDTH_, IMAGE_HEIGHT_);
    
      for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
        int neighbor_patch_index = patch_index_mask_[*neighbor_pixel_it];
        if (neighbor_patch_index != patch_index && neighbor_patch_index != -1)
	  patch_neighbor_patches[patch_index].insert(neighbor_patch_index);
      }
    }

    for(int patch_index = 0; patch_index < getNumOriginalPatches(); patch_index++) {
      if (patch_index == getOriginalBackgroundPatchIndex())
        continue;
      vector<int> neighbor_patches;
      for (set<int>::const_iterator neighbor_patch_it = patch_neighbor_patches.at(patch_index).begin(); neighbor_patch_it != patch_neighbor_patches.at(patch_index).end(); neighbor_patch_it++)
        neighbor_patches.push_back(*neighbor_patch_it);
      
      vector<bool> visited_patch_mask(getNumOriginalPatches(), false);
      visited_patch_mask[patch_index] = true;
      for (vector<int>::const_iterator neighbor_patch_it = neighbor_patches.begin(); neighbor_patch_it != neighbor_patches.end(); neighbor_patch_it++) {
        if (visited_patch_mask[*neighbor_patch_it])
          continue;
        visited_patch_mask[*neighbor_patch_it] = true;
        //cout << patch_index << '\t' << *neighbor_patch_it << endl;
        set<int> group_patches;
        group_patches.insert(*neighbor_patch_it);
        while (true) {
          bool has_change = false;
          for (set<int>::const_iterator group_patch_it = group_patches.begin(); group_patch_it != group_patches.end(); group_patch_it++) {
            for (set<int>::const_iterator group_patch_neighbor_patch_it = patch_neighbor_patches.at(*group_patch_it).begin(); group_patch_neighbor_patch_it != patch_neighbor_patches.at(*group_patch_it).end(); group_patch_neighbor_patch_it++) {
              if (visited_patch_mask[*group_patch_neighbor_patch_it] == false) {
                group_patches.insert(*group_patch_neighbor_patch_it);
                visited_patch_mask[*group_patch_neighbor_patch_it] = true;
                has_change = true;
              }
            }
          }
          if (has_change == false)
            break;
        }
        if (group_patches.count(getOriginalBackgroundPatchIndex()) == 0) {
          patch_child_patches_[patch_index].insert(group_patches.begin(), group_patches.end());
	  //for (set<int>::const_iterator group_patch_it = group_patches.begin(); group_patch_it != group_patches.end(); group_patch_it++)       
	  //cout << "patch group: " << patch_index << '\t' << *group_patch_it << endl;
        }
      }
    }

    //Add denoted island patches
    {
      map<int, map<int, int> > patch_neighbor_counter;
      for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
	int patch_index = patch_index_mask_[pixel];
	vector<int> neighbor_pixels = Popup::findNeighbors(pixel, IMAGE_WIDTH_, IMAGE_HEIGHT_);
	for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
	  int neighbor_patch_index = patch_index_mask_[*neighbor_pixel_it];
	  if (neighbor_patch_index != patch_index)
	    patch_neighbor_counter[patch_index][neighbor_patch_index]++;
	}
      }

      for (set<int>::const_iterator island_patch_it = island_patches.begin(); island_patch_it != island_patches.end(); island_patch_it++) {
	int max_count = 0;
	int max_count_neighbor_patch = -1;
	for (map<int, int>::const_iterator neighbor_patch_it = patch_neighbor_counter[*island_patch_it].begin(); neighbor_patch_it != patch_neighbor_counter[*island_patch_it].end(); neighbor_patch_it++) {
	  if (neighbor_patch_it->second > max_count) {
	    max_count_neighbor_patch = neighbor_patch_it->first;
	    max_count = neighbor_patch_it->second;
	  }
	}
	patch_child_patches_[max_count_neighbor_patch].insert(*island_patch_it );
      }
    }
  }

  void PopupGraph::buildSubGraphes()
  {
    map<int, int> patch_parent_counter;
    for (map<int, set<int> >::const_iterator patch_it = patch_child_patches_.begin(); patch_it != patch_child_patches_.end(); patch_it++)
      for (set<int>::const_iterator child_patch_it = patch_it->second.begin(); child_patch_it != patch_it->second.end(); child_patch_it++)
	patch_parent_counter[*child_patch_it]++;
    map<int, int> patch_parent_map;
    int child_depth = 1;
    while (patch_parent_map.size() < patch_parent_counter.size()) {
      for (map<int, set<int> >::const_iterator patch_it = patch_child_patches_.begin(); patch_it != patch_child_patches_.end(); patch_it++)
        for (set<int>::const_iterator child_patch_it = patch_it->second.begin(); child_patch_it != patch_it->second.end(); child_patch_it++)
	  if (patch_parent_counter[*child_patch_it] == child_depth) {
	    if (patch_parent_map.count(patch_it->first) > 0)
	      patch_parent_map[*child_patch_it] = patch_parent_map[patch_it->first];
            else
              patch_parent_map[*child_patch_it] = patch_it->first;
	  }
      child_depth++;
    }

    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
      if (patch_parent_map.count(patch_index_mask_[pixel]) > 0)
	patch_index_mask_[pixel] = patch_parent_map[patch_index_mask_[pixel]];

    map<int, int> patch_index_map;
    int new_patch_index = 0;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
      if (patch_index_map.count(patch_index_mask_[pixel]) == 0)
	patch_index_map[patch_index_mask_[pixel]] = new_patch_index++;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
      patch_index_mask_[pixel] = patch_index_map[patch_index_mask_[pixel]];

    NUM_ORIGINAL_PATCHES_ = patch_index_map.size();

    for (map<int, int>::iterator patch_it = patch_parent_map.begin(); patch_it != patch_parent_map.end(); patch_it++)
      patch_it->second = patch_index_map[patch_it->second];

    patch_child_patches_.clear();
    for (map<int, int>::const_iterator patch_it = patch_parent_map.begin(); patch_it != patch_parent_map.end(); patch_it++)
      patch_child_patches_[patch_it->second].insert(patch_it->first);
  }
	

  // map<int, set<int> > PopupGraph::getPatchChildPatches() const
  // {
  //   map<int, set<int> > patch_child_patches;
  //   std::map<int, std::map<int, std::set<int> > > patch_neighbor_fold_lines = getPatchNeighborFoldLines('B');
  //   for(int patch_index = 0; patch_index < getNumOriginalPatches(); patch_index++) {
  //     if (patch_index == getOriginalBackgroundPatchIndex())
  //       continue;
  //     vector<int> neighbor_patches;
  //     for (map<int, set<int> >::const_iterator neighbor_patch_it = patch_neighbor_fold_lines.at(patch_index).begin(); neighbor_patch_it != patch_neighbor_fold_lines.at(patch_index).end(); neighbor_patch_it++)
  //       neighbor_patches.push_back(neighbor_patch_it->first);
      
  //     vector<bool> visited_patch_mask(getNumOriginalPatches(), false);
  //     visited_patch_mask[patch_index] = true;
  //     for (vector<int>::const_iterator neighbor_patch_it = neighbor_patches.begin(); neighbor_patch_it != neighbor_patches.end(); neighbor_patch_it++) {
  //       if (visited_patch_mask[*neighbor_patch_it])
  // 	  continue;
  // 	visited_patch_mask[*neighbor_patch_it] = true;
  // 	//cout << patch_index << '\t' << *neighbor_patch_it << endl;
  // 	set<int> group_patches;
  // 	group_patches.insert(*neighbor_patch_it);
  // 	while (true) {
  // 	  bool has_change = false;
  // 	  for (set<int>::const_iterator group_patch_it = group_patches.begin(); group_patch_it != group_patches.end(); group_patch_it++) {
  //           for (map<int, set<int> >::const_iterator group_patch_neighbor_patch_it = patch_neighbor_fold_lines.at(*group_patch_it).begin(); group_patch_neighbor_patch_it != patch_neighbor_fold_lines.at(*group_patch_it).end(); group_patch_neighbor_patch_it++) {
  // 	      if (visited_patch_mask[group_patch_neighbor_patch_it->first] == false) {
  // 		group_patches.insert(group_patch_neighbor_patch_it->first);
  //               visited_patch_mask[group_patch_neighbor_patch_it->first] = true;
  // 		has_change = true;
  // 	      }
  // 	    }
  // 	  }
  // 	  if (has_change == false)
  // 	    break;
  //       }
  // 	if (group_patches.count(getOriginalBackgroundPatchIndex()) == 0) {
  // 	  patch_child_patches[patch_index].insert(group_patches.begin(), group_patches.end());
  // 	  // for (set<int>::const_iterator group_patch_it = group_patches.begin(); group_patch_it != group_patches.end(); group_patch_it++)
  // 	  //   cout << "patch group: " << patch_index << '\t' << *group_patch_it << endl;
  // 	}
  //     }
  //   }
  //   return patch_child_patches;
  // }

  void PopupGraph::enforceSymmetry()
  {
    map<int, map<int, int> > left_patch_mirror_patch_counter;
    map<int, map<int, int> > right_patch_mirror_patch_counter;
    for (int y = 0; y < IMAGE_HEIGHT_; y++) {
      for (int x = 0; x < MIDDLE_FOLD_LINE_X_; x++) {
	if (MIDDLE_FOLD_LINE_X_ * 2 - x < 0 || MIDDLE_FOLD_LINE_X_ * 2 - x >= IMAGE_WIDTH_)     
          continue;
	int patch_index = patch_index_mask_[y * IMAGE_WIDTH_ + x];
	int mirror_patch_index = patch_index_mask_[y * IMAGE_WIDTH_ + (MIDDLE_FOLD_LINE_X_ * 2 - x)];
	left_patch_mirror_patch_counter[patch_index][mirror_patch_index]++;
      }
      for (int x = MIDDLE_FOLD_LINE_X_ + 1; x < IMAGE_WIDTH_; x++) {
	if (MIDDLE_FOLD_LINE_X_ * 2 - x < 0 || MIDDLE_FOLD_LINE_X_ * 2 - x >= IMAGE_WIDTH_)     
          continue;
        int patch_index = patch_index_mask_[y * IMAGE_WIDTH_ + x];
        int mirror_patch_index = patch_index_mask_[y * IMAGE_WIDTH_ + (MIDDLE_FOLD_LINE_X_ * 2 - x)];
        right_patch_mirror_patch_counter[patch_index][mirror_patch_index]++;
      }
    }
    map<int, int> left_patch_mirror_patches;
    for (map<int, map<int, int> >::const_iterator patch_it = left_patch_mirror_patch_counter.begin(); patch_it != left_patch_mirror_patch_counter.end(); patch_it++) {
      int max_count = 0;
      int mirror_patch = -1;
      for (map<int, int>::const_iterator mirror_patch_it = patch_it->second.begin(); mirror_patch_it != patch_it->second.end(); mirror_patch_it++) {
	if (mirror_patch_it->second > max_count) {
	  mirror_patch = mirror_patch_it->first;
	  max_count = mirror_patch_it->second;
	}
      }
      left_patch_mirror_patches[patch_it->first] = mirror_patch;
    }
    map<int, int> right_patch_mirror_patches;
    for (map<int, map<int, int> >::const_iterator patch_it = right_patch_mirror_patch_counter.begin(); patch_it != right_patch_mirror_patch_counter.end(); patch_it++) {
      int max_count = 0;
      int mirror_patch = -1;
      for (map<int, int>::const_iterator mirror_patch_it = patch_it->second.begin(); mirror_patch_it != patch_it->second.end(); mirror_patch_it++) {
        if (mirror_patch_it->second > max_count) {
	  mirror_patch = mirror_patch_it->first;
	  max_count = mirror_patch_it->second;
	}
      }
      right_patch_mirror_patches[patch_it->first] = mirror_patch;
    }

    int new_patch_index = NUM_ORIGINAL_PATCHES_;
    for (map<int, int>::const_iterator patch_it = left_patch_mirror_patches.begin(); patch_it != left_patch_mirror_patches.end(); patch_it++) {
      if (right_patch_mirror_patches[patch_it->second] != patch_it->first)
	continue;
      if (patch_it->first == patch_it->second && patch_it->first != getOriginalBackgroundPatchIndex())
	symmetric_patch_map_[patch_it->first] = new_patch_index++;
      else
	symmetric_patch_map_[patch_it->first] = patch_it->second;
    }
    NUM_ORIGINAL_PATCHES_ = new_patch_index;
    
    // for (int y = 0; y < IMAGE_HEIGHT_; y++) {
    //   int patch_index = patch_index_mask_[y * IMAGE_WIDTH_ + MIDDLE_FOLD_LINE_X_];
    //   if (patch_index == getOriginalBackgroundPatchIndex())
    // 	continue;
    //   if (symmetric_patch_map_.count(patch_index) == 0)
    // 	symmetric_patch_map_[patch_index] = new_patch_index++;
    // }
    for (int y = 0; y < IMAGE_HEIGHT_; y++) {
      for (int x = MIDDLE_FOLD_LINE_X_ + 1; x < IMAGE_WIDTH_; x++) {
        int patch_index = patch_index_mask_[y * IMAGE_WIDTH_ + x];
	if (symmetric_patch_map_.count(patch_index) > 0)
	  patch_index_mask_[y * IMAGE_WIDTH_ + x] = symmetric_patch_map_[patch_index];
      }
    }
  }

  std::set<int> PopupGraph::getSymmetryFoldLines()
  {
    return symmetry_fold_lines_;
    // if (ENFORCE_SYMMETRY_ == false)
    //   return set<int>();
    // set<int> symmetry_fold_lines;
    // for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
    //   if (fold_line_it - fold_lines_.begin() >= getNumOriginalFoldLines())
    //     continue;
    //   int left_original_patch_index = fold_line_it->original_patch_pair.first;   
    //   int right_original_patch_index = fold_line_it->original_patch_pair.second;
    //   if (symmetric_patch_map_.count(left_original_patch_index) > 0 && symmetric_patch_map_[left_original_patch_index] == right_original_patch_index)
    // 	symmetry_fold_lines.insert(fold_line_it - fold_lines_.begin());
    // }
    // return symmetry_fold_lines;
  }

  vector<pair<int, int> > PopupGraph::getFoldLinePairsPassed() const
  {
    vector<pair<int, int> > fold_line_pairs_passed;
    set_difference(fold_line_pairs_without_passing_.begin(), fold_line_pairs_without_passing_.end(), fold_line_pairs_.begin(), fold_line_pairs_.end(), inserter(fold_line_pairs_passed, fold_line_pairs_passed.begin()));
    return fold_line_pairs_passed;
  }

  void PopupGraph::countNumOriginalPatches()
  {
    int num_original_patches = -1;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
      num_original_patches = max(patch_index_mask_[pixel] + 1, num_original_patches);
    NUM_ORIGINAL_PATCHES_ = num_original_patches;
  }

  void PopupGraph::findOriginalBackgroundPatchIndex()
  {
    int original_background_patch_index = -1;
    int index = 0;
    while (original_background_patch_index == -1) {
      original_background_patch_index = patch_index_mask_[index * IMAGE_WIDTH_ + index];
      index++;
    }
    ORIGINAL_BACKGROUND_PATCH_INDEX_ = original_background_patch_index;  
  }

  vector<pair<int, int> > PopupGraph::getSymmetricFoldLinePairs() const
  {
    vector<pair<int, int> > symmetric_fold_line_pairs;
    if (ENFORCE_SYMMETRY_) {
      map<int, int> bilateral_symmetric_patch_map = symmetric_patch_map_;
      for (map<int, int>::const_iterator patch_it = symmetric_patch_map_.begin(); patch_it != symmetric_patch_map_.end(); patch_it++)
        bilateral_symmetric_patch_map[patch_it->second] = patch_it->first;      

      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
	if (fold_line_it - fold_lines_.begin() >= NUM_ORIGINAL_FOLD_LINES_)
	  continue;
	if (symmetry_fold_lines_.count(fold_line_it - fold_lines_.begin()) > 0)
	  continue;
	if (fold_line_it->desirable_center % IMAGE_WIDTH_ >= MIDDLE_FOLD_LINE_X_)
	  continue;
	
        int left_original_patch_index = fold_line_it->original_patch_pair.first;
        int right_original_patch_index = fold_line_it->original_patch_pair.second;
        if (bilateral_symmetric_patch_map.count(left_original_patch_index) == 0 || bilateral_symmetric_patch_map.count(right_original_patch_index) == 0)
          continue;

        pair<int, int> symmetric_fold_line_patch_pair(bilateral_symmetric_patch_map[right_original_patch_index], bilateral_symmetric_patch_map[left_original_patch_index]);
	double min_distance = 1000000;
        int symmetric_fold_line = -1;
        for (vector<FoldLine>::const_iterator other_fold_line_it = fold_lines_.begin(); other_fold_line_it != fold_lines_.end(); other_fold_line_it++) {
	  if (other_fold_line_it - fold_lines_.begin() >= NUM_ORIGINAL_FOLD_LINES_)
            continue;
          if (symmetry_fold_lines_.count(other_fold_line_it - fold_lines_.begin()) > 0)
            continue;
          if (other_fold_line_it->original_patch_pair == symmetric_fold_line_patch_pair) {
            //cout << fold_line_it - fold_lines_.begin() << '\t' << other_fold_line_it - fold_lines_.begin() << endl;
	    double distance = sqrt(pow(other_fold_line_it->desirable_center % IMAGE_WIDTH_ + fold_line_it->desirable_center % IMAGE_WIDTH_ - 2 * MIDDLE_FOLD_LINE_X_, 2) + pow(other_fold_line_it->desirable_center / IMAGE_WIDTH_ - fold_line_it->desirable_center / IMAGE_WIDTH_, 2));
            if (distance < min_distance) {
              symmetric_fold_line = other_fold_line_it - fold_lines_.begin();
	      min_distance = distance;
            }
          }
        }
	symmetric_fold_line_pairs.push_back(make_pair(fold_line_it - fold_lines_.begin(), symmetric_fold_line));
      }

      map<int, map<int, int> > fold_line_mirror_counter;
      for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
        int patch_index = patch_index_mask_[pixel];
        if (patch_index == -1)
          continue;
        int x = pixel % IMAGE_WIDTH_;
        int y = pixel / IMAGE_WIDTH_;
	if (x >= MIDDLE_FOLD_LINE_X_)
	  continue;
	int mirror_pixel = y * IMAGE_WIDTH_ + 2 * MIDDLE_FOLD_LINE_X_ - x;
	int fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[pixel]];
	int mirror_fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[mirror_pixel]];
	if (fold_line_index < NUM_ORIGINAL_FOLD_LINES_ || mirror_fold_line_index < NUM_ORIGINAL_FOLD_LINES_)
	  continue;
	if (fold_line_index == MIDDLE_FOLD_LINE_INDEX_ || mirror_fold_line_index == MIDDLE_FOLD_LINE_INDEX_)
	  continue;
	fold_line_mirror_counter[fold_line_index][mirror_fold_line_index]++;
	fold_line_mirror_counter[mirror_fold_line_index][fold_line_index]++;
      }

      map<int, int> fold_line_mirrors;
      for (map<int, map<int, int> >::const_iterator fold_line_it = fold_line_mirror_counter.begin(); fold_line_it != fold_line_mirror_counter.end(); fold_line_it++) {
        int max_count = 0;
        int mirror = -1;
        for (map<int, int>::const_iterator mirror_it = fold_line_it->second.begin(); mirror_it != fold_line_it->second.end(); mirror_it++) {
          if (mirror_it->second > max_count) {
            mirror = mirror_it->first;
            max_count = mirror_it->second;
          }
        }
        fold_line_mirrors[fold_line_it->first] = mirror;
      }

      for (map<int, int>::const_iterator fold_line_it = fold_line_mirrors.begin(); fold_line_it != fold_line_mirrors.end(); fold_line_it++) {
	//cout << fold_line_it->first << '\t' << fold_line_it->second << endl;
	if (fold_line_it->second <= fold_line_it->first)
	  continue;
	if (fold_line_mirrors[fold_line_it->second] != fold_line_it->first)
          continue;
        symmetric_fold_line_pairs.push_back(make_pair(fold_line_it->first, fold_line_it->second));
      }
    }
    return symmetric_fold_line_pairs;
  }

  vector<int> PopupGraph::getNewFoldLines() const
  {
    vector<int> new_fold_lines;
    for (int fold_line_index = getNumOriginalFoldLines(); fold_line_index < getNumFoldLines(); fold_line_index++) {
      if (fold_line_index == MIDDLE_FOLD_LINE_INDEX_ || fold_line_index == LEFT_BORDER_FOLD_LINE_INDEX_ || fold_line_index == RIGHT_BORDER_FOLD_LINE_INDEX_)
	continue;
      if (fold_lines_[fold_line_index].optimized_activity == 1)
	new_fold_lines.push_back(fold_line_index);
    }
    return new_fold_lines;
  }

  void PopupGraph::addOptimizedInfo(const PopupGraph &optimized_popup_graph)
  {
    vector<int> optimized_patch_index_mask = optimized_popup_graph.getPatchIndexMask();
    map<int, int> patch_index_map;
    map<int, map<int, int> > patch_index_map_counter;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      int patch_index = patch_index_mask_[pixel];
      if (patch_index == -1)      
        continue;      
      int optimized_patch_index = optimized_patch_index_mask[pixel];
      if (optimized_patch_index == -1)      
        continue;
      //cout << "count " << optimized_patch_index << '\t' << patch_index << endl;
      patch_index_map_counter[optimized_patch_index][patch_index]++;
    }
    for (map<int, map<int, int> >::const_iterator optimized_patch_it = patch_index_map_counter.begin(); optimized_patch_it != patch_index_map_counter.end(); optimized_patch_it++) {
      int max_count_patch_index = -1;
      int max_count = 0;
      for (map<int, int>::const_iterator patch_it = optimized_patch_it->second.begin(); patch_it != optimized_patch_it->second.end(); patch_it++) {
        if (patch_it->second > max_count) {
	  max_count_patch_index = patch_it->first;
	  max_count = patch_it->second;
	  //cout << optimized_patch_it->first << '\t' << patch_it->second << '\t' << patch_it->first << endl;
        }
      }
      patch_index_map[optimized_patch_it->first] = max_count_patch_index;
    }

    vector<int> island_patch_mask(NUM_ORIGINAL_PATCHES_, true);
    for (map<int, int>::const_iterator patch_it = patch_index_map.begin(); patch_it != patch_index_map.end(); patch_it++) {
      //cout << "patch index map: " << patch_it->first << '\t' << patch_it->second << endl;
      island_patch_mask[patch_it->second] = false;
    }

    vector<FoldLine> optimized_fold_lines = optimized_popup_graph.getFoldLines();
    for (vector<FoldLine>::const_iterator optimized_fold_line_it = optimized_fold_lines.begin(); optimized_fold_line_it != optimized_fold_lines.end(); optimized_fold_line_it++) {
      if (optimized_fold_line_it - optimized_fold_lines.begin() == optimized_popup_graph.getBorderFoldLineIndices().first || optimized_fold_line_it - optimized_fold_lines.begin() == optimized_popup_graph.getBorderFoldLineIndices().second)
        continue;
      if (optimized_fold_line_it->optimized_activity != 1)
	continue;
      int left_patch_index = patch_index_map[optimized_fold_line_it->original_patch_pair.first];
      int right_patch_index = patch_index_map[optimized_fold_line_it->original_patch_pair.second];
      //cout << optimized_fold_line_it->optimized_pixels.size() << '\t' << fold_lines_[44].positions.size() << endl;
      pair<int, int> original_patch_pair(left_patch_index, right_patch_index);
      vector<bool> position_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, false);
      for (vector<int>::const_iterator position_it = optimized_fold_line_it->optimized_pixels.begin(); position_it != optimized_fold_line_it->optimized_pixels.end(); position_it++)
        position_mask[*position_it] = true;

      int max_num_matched_pixels = 0;
      int matched_fold_line_index = -1;
      map<int, int> fold_line_matched_pixel_counter;
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
        if (fold_line_it->original_patch_pair != original_patch_pair)
	  continue;

	int num_matched_pixels = 0;
        for (vector<int>::const_iterator position_it = fold_line_it->positions.begin(); position_it != fold_line_it->positions.end(); position_it++) {
	  if (position_mask[*position_it]) {
	    num_matched_pixels++;
	    fold_line_matched_pixel_counter[fold_line_it - fold_lines_.begin()]++;
	  }
	}
        if (num_matched_pixels > max_num_matched_pixels) {
	  matched_fold_line_index = fold_line_it - fold_lines_.begin();
	  max_num_matched_pixels = num_matched_pixels;
	}
      }
      if (matched_fold_line_index == -1) {
	cout << "there is no matched fold line for " << optimized_fold_line_it - optimized_fold_lines.begin() << endl;
	exit(1);
      }

      // if (island_patch_mask[fold_lines_[matched_fold_line_index].original_patch_pair.first] == true || island_patch_mask[fold_lines_[matched_fold_line_index].original_patch_pair.second] == true) {
      //   cout << "mismatched fold line: " << matched_fold_line_index << '\t' << optimized_fold_line_it - optimized_fold_lines.begin() << endl;
      //   exit(1);
      // }
      // // if (optimized_fold_line_it - optimized_fold_lines.begin() >= getMiddleFoldLineIndex())
      // //        continue;
      // cout << "fold line match map: " << matched_fold_line_index << '\t' << optimized_fold_line_it - optimized_fold_lines.begin() << endl;
      // fold_lines_[matched_fold_line_index].optimized_flag = true;      
      // fold_lines_[matched_fold_line_index].optimized_position = optimized_fold_line_it->optimized_position;      
      // fold_lines_[matched_fold_line_index].optimized_convexity = optimized_fold_line_it->optimized_convexity;

      for (map<int, int>::const_iterator fold_line_it = fold_line_matched_pixel_counter.begin(); fold_line_it != fold_line_matched_pixel_counter.end(); fold_line_it++) {
        if (island_patch_mask[fold_lines_[fold_line_it->first].original_patch_pair.first] == true || island_patch_mask[fold_lines_[fold_line_it->first].original_patch_pair.second] == true) {
          cout << "mismatched fold line: " << fold_line_it->first << '\t' << optimized_fold_line_it - optimized_fold_lines.begin() << endl;
          exit(1);
        }
	// if (optimized_fold_line_it - optimized_fold_lines.begin() >= getMiddleFoldLineIndex())
	//        continue;
	cout << "fold line match map: " << fold_line_it->first << '\t' << optimized_fold_line_it - optimized_fold_lines.begin() << endl;
	fold_lines_[fold_line_it->first].optimized_activity = optimized_fold_line_it->optimized_activity;
	fold_lines_[fold_line_it->first].optimized_convexity = optimized_fold_line_it->optimized_convexity;
	fold_lines_[fold_line_it->first].optimized_position = optimized_fold_line_it->optimized_position;
      }
    }
    
    for (vector<FoldLine>::iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
      if (fold_line_it - fold_lines_.begin() == getBorderFoldLineIndices().first || fold_line_it - fold_lines_.begin() == getBorderFoldLineIndices().second) 
        continue;
      if (fold_line_it->optimized_activity == 1)
    	continue;
      if (island_patch_mask[fold_line_it->original_patch_pair.first] == false && island_patch_mask[fold_line_it->original_patch_pair.second] == false) {
        fold_line_it->optimized_activity = 0;
	fold_line_it->optimized_convexity = -1;
        fold_line_it->optimized_position = -1;
      }
    }
    

    fold_lines_[getBorderFoldLineIndices().first].optimized_activity = 1;
    fold_lines_[getBorderFoldLineIndices().first].optimized_convexity = 0;
    fold_lines_[getBorderFoldLineIndices().first].optimized_position = 0;
    
    fold_lines_[getBorderFoldLineIndices().second].optimized_activity = 1;
    fold_lines_[getBorderFoldLineIndices().second].optimized_convexity = 0;
    fold_lines_[getBorderFoldLineIndices().second].optimized_position = IMAGE_WIDTH_ - 1;


    // for (int patch_index = 0; patch_index < getNumOriginalPatches(); patch_index++)
    //   cout << patch_index << '\t' << island_patch_mask[patch_index] << endl;
    // for (vector<FoldLine>::iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
    //   //if (fold_line_it->optimized_flag == false) {    
    //   cout << "fold line: " << fold_line_it - fold_lines_.begin() << '\t' << fold_line_it->original_patch_pair.first << '\t' << fold_line_it->original_patch_pair.second << '\t' << fold_line_it->optimized_activity << endl;
    // 	//cout << getFoldLineXRange(fold_line_it - fold_lines_.begin()).first << '\t' << getFoldLineXRange(fold_line_it - fold_lines_.begin()).second << endl;
    // } 
    // 	//cout << fold_line_it - fold_lines_.begin() << '\t' << fold_line_it->optimized_position << '\t' << fold_line_it->optimized_convexity << endl;
    
    // exit(1);
  }

  void PopupGraph::writeRenderingInfo()
  {
    cout << "write rendering info" << endl;

    vector<int> enforced_patch_index_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, -1);
    vector<vector<int> > fold_line_ys(getNumFoldLines());
    for (int fold_line_index = 0; fold_line_index < getNumOriginalFoldLines(); fold_line_index++) {
      if (fold_lines_[fold_line_index].optimized_activity != 1)
        continue;

      if (fold_lines_[fold_line_index].optimized_position == IMAGE_WIDTH_ - 1) {
	cout << "fold line is at the right border" << endl;
	exit(1);
      }
      
      int fold_line_min_y = IMAGE_HEIGHT_ - 1;
      int fold_line_max_y = 0;
      {
        vector<int> fold_line_optimized_positions;
        for (vector<int>::const_iterator position_it = fold_lines_[fold_line_index].positions.begin(); position_it != fold_lines_[fold_line_index].positions.end(); position_it++)
          if (*position_it % IMAGE_WIDTH_ == fold_lines_[fold_line_index].optimized_position)
            fold_line_optimized_positions.push_back(*position_it);
        if (fold_line_optimized_positions.size() == 0) {
          fold_line_min_y = max(fold_lines_[fold_line_index].desirable_center / IMAGE_WIDTH_ - FOLD_LINE_WINDOW_HEIGHT_ / 2, 0);
          fold_line_max_y = min(fold_lines_[fold_line_index].desirable_center / IMAGE_WIDTH_ + FOLD_LINE_WINDOW_HEIGHT_ / 2, IMAGE_WIDTH_ -1);
          fold_lines_[fold_line_index].optimized_pixels.push_back(fold_lines_[fold_line_index].desirable_center);
          if (true) {
            cout << fold_line_index << endl;
            cout << fold_lines_[fold_line_index].optimized_position << endl;
            //for (vector<int>::const_iterator position_it = fold_lines_[fold_line_index].positions.begin(); position_it != fold_lines_[fold_line_index].positions.end(); position_it++)
	    //cout << *position_it % IMAGE_WIDTH_ << endl;
            cout << "optimized fold line position not available" << endl;
            exit(1);
          }
        } else if (fold_line_optimized_positions.size() == 1) {
          fold_line_min_y = max(fold_line_optimized_positions[0] / IMAGE_WIDTH_ - FOLD_LINE_WINDOW_HEIGHT_ / 2, 0);
          fold_line_max_y = min(fold_line_optimized_positions[0] / IMAGE_WIDTH_ + FOLD_LINE_WINDOW_HEIGHT_ / 2, IMAGE_HEIGHT_ - 1);
        } else {
          for (vector<int>::const_iterator position_it = fold_line_optimized_positions.begin(); position_it != fold_line_optimized_positions.end(); position_it++) {
            fold_line_min_y = min(fold_line_min_y, *position_it / IMAGE_WIDTH_);
            fold_line_max_y = max(fold_line_max_y, *position_it / IMAGE_WIDTH_);
          }
        }
      }

      if (fold_line_max_y - fold_line_min_y < FOLD_LINE_WINDOW_HEIGHT_) {
	int fold_line_middle_y = (fold_line_max_y + fold_line_min_y) / 2;
	fold_line_min_y = fold_line_middle_y - FOLD_LINE_WINDOW_HEIGHT_ / 2;
	fold_line_max_y = fold_line_middle_y + FOLD_LINE_WINDOW_HEIGHT_ / 2;
      }


      int left_patch_max_count = 0;
      int right_patch_max_count = 0;
      int left_major_patch_index = -1;
      int right_major_patch_index = -1;
      int left_previous_y = fold_line_min_y;
      int right_previous_y = fold_line_min_y;
      int left_previous_patch_index = -1;
      int right_previous_patch_index = -1;
      for (int y = fold_line_min_y; y <= fold_line_max_y; y++) {
	int pixel = y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
	{
	  int patch_index = patch_index_mask_[pixel];
	  if (patch_index != left_previous_patch_index) {
	    if (left_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.first || left_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.second) {
              int count = y - left_previous_y;
	      if (count > left_patch_max_count) {
		left_major_patch_index = left_previous_patch_index;
		left_patch_max_count = count;
	      }
	    }
	    left_previous_y = y;
	    left_previous_patch_index = patch_index;
	  }
	}
	{
          int patch_index = patch_index_mask_[pixel + 1];
          if (patch_index != right_previous_patch_index) {
            if (right_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.first || right_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.second) {
              int count = y - right_previous_y;
              if (count > right_patch_max_count) {
                right_major_patch_index = right_previous_patch_index;
                right_patch_max_count = count;
              }
            }
            right_previous_y = y;
            right_previous_patch_index = patch_index;
          }
        }
      }
      if (left_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.first || left_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.second) {
	int count = fold_line_max_y - left_previous_y;
	if (count > left_patch_max_count) {
	  left_major_patch_index = left_previous_patch_index;
	  left_patch_max_count = count;
	}
      }
      if (right_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.first || right_previous_patch_index == fold_lines_[fold_line_index].original_patch_pair.second) {
        int count = fold_line_max_y - right_previous_y;
        if (count > right_patch_max_count) {
          right_major_patch_index = right_previous_patch_index;
          right_patch_max_count = count;
        }
      }
      
      for (int y = fold_line_min_y; y <= fold_line_max_y; y++) {
	int pixel = y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
	if (patch_index_mask_[pixel] == left_major_patch_index) 
          enforced_patch_index_mask[pixel] = fold_lines_[fold_line_index].original_patch_pair.first;    
        if (patch_index_mask_[pixel + 1] == right_major_patch_index)
	  enforced_patch_index_mask[pixel + 1] = fold_lines_[fold_line_index].original_patch_pair.second;

	if (false) {
	  for (int delta_y = -FOLD_LINE_WINDOW_WIDTH_ * 2 / 2; delta_y <= FOLD_LINE_WINDOW_HEIGHT_ * 2 / 2; delta_y++) {
	    for (int delta_x = -FOLD_LINE_WINDOW_WIDTH_ * 2 / 2; delta_x <= FOLD_LINE_WINDOW_WIDTH_ * 2 / 2; delta_x++) {
	      if (delta_x == 0 || delta_x == 1)
		continue;
	      int pixel = (y + delta_y) * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position + delta_x;
              if (delta_x < 0) {
		if (patch_index_mask_[pixel] == left_major_patch_index)
		  enforced_patch_index_mask[pixel] = 10000 + fold_lines_[fold_line_index].original_patch_pair.first;
	      } else {
		if (patch_index_mask_[pixel] == right_major_patch_index)
		  enforced_patch_index_mask[pixel] = 10000 + fold_lines_[fold_line_index].original_patch_pair.second;
	      }
	    }
	  }
	}
      }

      if (ENFORCE_SYMMETRY_ && symmetry_fold_lines_.count(fold_line_index) > 0) {
	for (int y = fold_line_min_y; y <= fold_line_max_y; y++) {
	  int pixel = y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
	  if (patch_index_mask_[pixel] == left_major_patch_index && patch_index_mask_[pixel + 1] == right_major_patch_index)
	    fold_line_ys[fold_line_index].push_back(y);
	}
	continue;
      }

      int min_y = fold_line_min_y;
      int max_y = fold_line_max_y;
      while (true) {
	bool has_change = false;
	if (max_y - min_y <= FOLD_LINE_WINDOW_HEIGHT_)
	  break;
	{
	  int pixel = min_y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
	  if (patch_index_mask_[pixel] != left_major_patch_index && patch_index_mask_[pixel + 1] != right_major_patch_index) {
	    min_y++;
	    has_change = true;
	  }
	}
	{
	  int pixel = max_y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
          if (patch_index_mask_[pixel] != left_major_patch_index && patch_index_mask_[pixel + 1] != right_major_patch_index) {
            max_y--;
            has_change = true;
          }
	}
	if (has_change == false)
	  break;
      }
      
      if (min_y == fold_line_min_y && max_y == fold_line_max_y) {
	if (max_y - min_y < FOLD_LINE_WINDOW_HEIGHT_ * 2) {
	  int fold_line_middle_y = (fold_line_max_y + fold_line_min_y) / 2;
          min_y = fold_line_middle_y - FOLD_LINE_WINDOW_HEIGHT_ / 2;
          max_y = fold_line_middle_y + FOLD_LINE_WINDOW_HEIGHT_ / 2;
	} else {
          min_y += FOLD_LINE_WINDOW_HEIGHT_ / 2;
	  max_y -= FOLD_LINE_WINDOW_HEIGHT_ / 2;
        }
      }

      for (int y = min_y; y <= max_y; y++)
	fold_line_ys[fold_line_index].push_back(y);
    }
    
    for (int fold_line_index = getNumOriginalFoldLines(); fold_line_index < getNumFoldLines(); fold_line_index++) {
      if (fold_line_index == getBorderFoldLineIndices().first || fold_line_index == getBorderFoldLineIndices().second)            
        continue;      
      if (fold_lines_[fold_line_index].optimized_activity != 1)
        continue;
      int min_y = IMAGE_HEIGHT_;
      int max_y = 0;
      for (int y = 0; y < IMAGE_HEIGHT_; y++) {
	int pixel = y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
	int line_segment_index = pixel_line_segment_indices_[pixel];
	if (line_segment_index == -1)
	  continue;
	if (line_segment_fold_line_indices_[line_segment_index] == fold_line_index)
          fold_line_ys[fold_line_index].push_back(y);
      }
      
    }

    //vector<int> deformed_patch_index_mask;
    //Popup::deformPatches(IMAGE_WIDTH_, IMAGE_HEIGHT_, getNumOriginalPatches(), patch_index_mask_, enforced_patch_index_mask, deformed_patch_index_mask);
    
    // //patch_index_mask_ = deformed_patch_index_mask;

    // map<int, int> fold_line_index_map;
    // map<int, int> reverse_fold_line_index_map;
    // int new_fold_line_index = 0;
    // for (int fold_line_index = 0; fold_line_index < getNumFoldLines(); fold_line_index++) {
    //   if (fold_lines_[fold_line_index].optimized_position == -1)      
    //     continue;
    //   fold_line_index_map[fold_line_index] = new_fold_line_index;
    //   reverse_fold_line_index_map[new_fold_line_index] = fold_line_index;
    //   new_fold_line_index++;
    // }
    
    // for (int fold_line_index = 0; fold_line_index < getNumOriginalFoldLines(); fold_line_index++) {
    //   if (fold_lines_[fold_line_index].optimized_position == -1)      
    //     continue;
    //   for (int y = 0; y < IMAGE_HEIGHT_; y++) {
    // 	int pixel = y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
    // 	if (deformed_patch_index_mask[pixel] == fold_lines_[fold_line_index].original_patch_pair.first && deformed_patch_index_mask[pixel + 1] == fold_lines_[fold_line_index].original_patch_pair.second)
    // 	  deformed_patch_index_mask[pixel] = 10000 + fold_line_index_map[fold_line_index];
    //   }
    // }

    // for (int fold_line_index = getNumOriginalFoldLines(); fold_line_index < getNumFoldLines(); fold_line_index++) {
    //   if (fold_line_index == getBorderFoldLineIndices().first || fold_line_index == getBorderFoldLineIndices().second)      
    //     continue;
    //   if (fold_lines_[fold_line_index].optimized_position == -1)      
    //     continue;
    //   //      cout << "x: " << fold_lines_[fold_line_index].optimized_position << endl;
    //   for (int y = 0; y < IMAGE_HEIGHT_; y++) {
    //     int pixel = y * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
    // 	int line_segment_index = pixel_line_segment_indices_[pixel];
    //     if (line_segment_index == -1)
    //       continue;
    // 	//cout << pixel_line_segment_indices_[pixel] << '\t' << line_segment_fold_line_indices_[pixel_line_segment_indices_[pixel]] << endl;
    // 	if (line_segment_fold_line_indices_[line_segment_index] == fold_line_index)
    // 	  deformed_patch_index_mask[pixel] = 10000 + fold_line_index_map[fold_line_index];
    //   }
    // }

    vector<int> new_patch_index_mask = vector<int>(IMAGE_WIDTH_ * IMAGE_HEIGHT_, -1);
    for (int fold_line_index = getNumOriginalFoldLines(); fold_line_index < getNumFoldLines(); fold_line_index++) {
      if (fold_line_index == getBorderFoldLineIndices().first || fold_line_index == getBorderFoldLineIndices().second)            
        continue;      
      if (fold_lines_[fold_line_index].optimized_activity != 1)            
        continue;
      for (vector<int>::const_iterator y_it = fold_line_ys[fold_line_index].begin(); y_it != fold_line_ys[fold_line_index].end(); y_it++) {
        int pixel = *y_it * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
	if (fold_line_index == MIDDLE_FOLD_LINE_INDEX_ && patch_index_mask_[pixel] != ORIGINAL_BACKGROUND_PATCH_INDEX_)
	  continue;
        new_patch_index_mask[pixel] = fold_line_index + 10000;
      }
    }
    
    int new_patch_index = 0;
    vector<bool> visited_pixel_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, false);
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
      if (new_patch_index_mask[pixel] >= 10000 + getNumOriginalFoldLines())
	visited_pixel_mask[pixel] = true;
    
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {    
      if (visited_pixel_mask[pixel])
        continue;
      int patch_index = patch_index_mask_[pixel];
      if (patch_index == -1)
        continue;
      vector<int> border_pixels;
      border_pixels.push_back(pixel);
      visited_pixel_mask[pixel] = true;
      new_patch_index_mask[pixel] = new_patch_index;
      while (border_pixels.size() > 0) {
	vector<int> new_border_pixels;
	for (vector<int>::const_iterator pixel_it = border_pixels.begin(); pixel_it != border_pixels.end(); pixel_it++) {
	  vector<int> neighbor_pixels = Popup::findNeighbors(*pixel_it, IMAGE_WIDTH_, IMAGE_HEIGHT_);
          for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++) {
	    if (visited_pixel_mask[*neighbor_pixel_it] || patch_index_mask_[*neighbor_pixel_it] != patch_index)
              continue;
	    new_border_pixels.push_back(*neighbor_pixel_it);
	    visited_pixel_mask[*neighbor_pixel_it] = true;
	    new_patch_index_mask[*neighbor_pixel_it] = new_patch_index;
          }
	}
	border_pixels = new_border_pixels;
      }
      new_patch_index++;
    }

    for (int fold_line_index = 0; fold_line_index < getNumOriginalFoldLines(); fold_line_index++) {
      if (fold_lines_[fold_line_index].optimized_activity != 1) 
        continue;
      for (vector<int>::const_iterator y_it = fold_line_ys[fold_line_index].begin(); y_it != fold_line_ys[fold_line_index].end(); y_it++) {
        int pixel = *y_it * IMAGE_WIDTH_ + fold_lines_[fold_line_index].optimized_position;
        new_patch_index_mask[pixel] = fold_line_index + 10000;
      }
    }
    
    map<int, map<pair<int, int>, int> > fold_line_patch_pair_counter;
    map<int, map<int, int> > fold_line_left_patch_counter;
    map<int, map<int, int> > fold_line_right_patch_counter;
    set<int> active_fold_lines;
    
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      if (pixel % IMAGE_WIDTH_ == 0 || pixel % IMAGE_WIDTH_ == IMAGE_WIDTH_ - 1)
    	continue;
      if (new_patch_index_mask[pixel] < 10000)
    	continue;
      
      int fold_line_index = new_patch_index_mask[pixel] - 10000;
      active_fold_lines.insert(fold_line_index);
      
      int left_patch_index = -1;
      for (int delta_x = -1; delta_x >= -FOLD_LINE_WINDOW_WIDTH_; delta_x--) {
	if (pixel % IMAGE_WIDTH_ + delta_x < 0)
	  break;
	if (patch_index_mask_[pixel + delta_x] == fold_lines_[fold_line_index].original_patch_pair.first) {
	  left_patch_index = new_patch_index_mask[pixel + delta_x];
	  break;
	}
      }
      int right_patch_index = -1;
      for (int delta_x = 1; delta_x <= FOLD_LINE_WINDOW_WIDTH_; delta_x++) {
        if (pixel % IMAGE_WIDTH_ + delta_x == IMAGE_WIDTH_)
          break;
        if (patch_index_mask_[pixel + delta_x] == fold_lines_[fold_line_index].original_patch_pair.second) {
          right_patch_index = new_patch_index_mask[pixel + delta_x];
          break;
        }
      }

      //if (fold_line_index == 6) {
      //cout << pixel % IMAGE_WIDTH_ << '\t' << pixel / IMAGE_WIDTH_ << endl;
      //cout << left_patch_index << '\t' << right_patch_index << endl;
      //}
      
      if (left_patch_index != -1 && right_patch_index != -1)      
        fold_line_patch_pair_counter[fold_line_index][make_pair(left_patch_index, right_patch_index)]++;
      
      if (left_patch_index != -1)      
        fold_line_left_patch_counter[fold_line_index][left_patch_index]++;
      if (right_patch_index != -1)      
        fold_line_right_patch_counter[fold_line_index][right_patch_index]++;
    }
    
    map<int, pair<int, int> > fold_line_patch_pair_map;
    for (map<int, map<pair<int, int>, int> >::const_iterator fold_line_it = fold_line_patch_pair_counter.begin(); fold_line_it != fold_line_patch_pair_counter.end(); fold_line_it++) {
      pair<int, int> patch_pair;
      int max_count = 0;      
      for (map<pair<int, int>, int>::const_iterator patch_pair_it = fold_line_it->second.begin(); patch_pair_it != fold_line_it->second.end(); patch_pair_it++) {      
        if (patch_pair_it->second > max_count) {      
          patch_pair = patch_pair_it->first;      
          max_count = patch_pair_it->second;      
        }      
      }
      fold_line_patch_pair_map[fold_line_it->first] = patch_pair;
    }
    for (set<int>::const_iterator fold_line_it = active_fold_lines.begin(); fold_line_it != active_fold_lines.end(); fold_line_it++) {
      if (fold_line_patch_pair_map.count(*fold_line_it) > 0)
	continue;
      if (fold_line_left_patch_counter.count(*fold_line_it) == 0) {       
	cout << "no left patch" << endl;      
	exit(1);
      }
      if (fold_line_right_patch_counter.count(*fold_line_it) == 0) {       
	cout << "no right patch" << endl;      
	exit(1);
      }
      int left_patch_index = -1;
      {
	int max_count = 0;      
	for (map<int, int>::const_iterator left_patch_it = fold_line_left_patch_counter[*fold_line_it].begin(); left_patch_it != fold_line_left_patch_counter[*fold_line_it].end(); left_patch_it++) {
	  if (left_patch_it->second > max_count) {      
	    left_patch_index = left_patch_it->first;      
	    max_count = left_patch_it->second;      
	  }    
	}
      }
      int right_patch_index = -1;
      {
	int max_count = 0;      
	for (map<int, int>::const_iterator right_patch_it = fold_line_right_patch_counter[*fold_line_it].begin(); right_patch_it != fold_line_right_patch_counter[*fold_line_it].end(); right_patch_it++) {
	  if (right_patch_it->second > max_count) {      
	    right_patch_index = right_patch_it->first;      
	    max_count = right_patch_it->second;      
	  }      
	}
      }
      fold_line_patch_pair_map[*fold_line_it] = make_pair(left_patch_index, right_patch_index);
  }
  
  Mat optimized_graph_image = Mat::zeros(IMAGE_HEIGHT_, IMAGE_WIDTH_, CV_8UC3);
  optimized_graph_image.setTo(Vec3b(255, 255, 255));
  map<int, vector<int> > region_pixels;
  map<int, Vec3b> color_map;
  for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      
    if (new_patch_index_mask[pixel] >= 10000) {
      Vec3b color = fold_lines_[new_patch_index_mask[pixel] - 10000].optimized_convexity ? Vec3b(255, 0, 0) : Vec3b(0, 255, 0);
      optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = color;
      region_pixels[new_patch_index_mask[pixel]].push_back(pixel);
      continue;
    }
      
    vector<int> neighbor_pixels = Popup::findNeighbors(pixel, IMAGE_WIDTH_, IMAGE_HEIGHT_);
    for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++)
      if (new_patch_index_mask[*neighbor_pixel_it] != new_patch_index_mask[pixel] && new_patch_index_mask[*neighbor_pixel_it] < 10000)
	optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = Vec3b(0, 0, 0);
    region_pixels[new_patch_index_mask[pixel]].push_back(pixel);
      
    if (color_map.count(new_patch_index_mask[pixel]) == 0)
      color_map[new_patch_index_mask[pixel]] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
    optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = color_map[new_patch_index_mask[pixel]];
  }

  for (map<int, vector<int> >::const_iterator region_it = region_pixels.begin(); region_it != region_pixels.end(); region_it++) {
    int pixel = region_it->second[rand() % region_it->second.size()];
    if (region_it->first < 10000) {
      Vec3b color = Vec3b(255, 255, 0);
      putText(optimized_graph_image, to_string(region_it->first % 10000), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, color);
    } else {
      Vec3b color = Vec3b(0, 0, 255);
      int fold_line_index = region_it->first - 10000;
      //to_string(fold_line_patch_pair_map[fold_line_index].first) + " " + to_string(fold_line_patch_pair_map[fold_line_index].second)
      putText(optimized_graph_image, to_string(fold_line_index), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, color);
    }
  }

  imwrite(FLAGS_output_folder + "/" + FLAGS_output_prefix + "output_graph_image.png", optimized_graph_image);

  {
    ofstream patch_index_mask_out_str(FLAGS_output_folder + "/" + FLAGS_output_prefix + "output_patch_index_mask.txt");
    patch_index_mask_out_str << IMAGE_WIDTH_ << '\t' << IMAGE_HEIGHT_ << endl;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
      patch_index_mask_out_str << patch_index_mask_[pixel] << endl;
    patch_index_mask_out_str.close();
  }
    {
      ofstream patch_index_mask_out_str(FLAGS_output_folder + "/" + FLAGS_output_prefix + "output_new_patch_index_mask.txt");
      patch_index_mask_out_str << IMAGE_WIDTH_ << '\t' << IMAGE_HEIGHT_ << endl;
      for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
	patch_index_mask_out_str << new_patch_index_mask[pixel] << endl;
      patch_index_mask_out_str.close();
    }
    {
      ofstream fold_line_out_str(FLAGS_output_folder + "/" + FLAGS_output_prefix + "output_fold_line.txt");
      for (map<int, pair<int, int> >::const_iterator fold_line_it = fold_line_patch_pair_map.begin(); fold_line_it != fold_line_patch_pair_map.end(); fold_line_it++) {
	fold_line_out_str << fold_line_it->first << '\t' << fold_line_it->second.first << '\t' << fold_line_it->second.second << endl;
      }
      fold_line_out_str.close();
    }
}
  
  
PopupGraph::PopupGraph(const vector<int> &patch_index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, const int MIDDLE_FOLD_LINE_X, const set<int> &island_patches, const bool ENFORCE_SYMMETRY, const bool BUILD_COMPLETE_POPUP_GRAPH, const int MIN_FOLD_LINE_GAP) : IMAGE_WIDTH_(IMAGE_WIDTH), IMAGE_HEIGHT_(IMAGE_HEIGHT), FOLD_LINE_WINDOW_WIDTH_(FOLD_LINE_WINDOW_WIDTH), FOLD_LINE_WINDOW_HEIGHT_(FOLD_LINE_WINDOW_HEIGHT), MIDDLE_FOLD_LINE_X_(MIDDLE_FOLD_LINE_X), ENFORCE_SYMMETRY_(ENFORCE_SYMMETRY), patch_index_mask_(patch_index_mask), MIN_FOLD_LINE_GAP_(MIN_FOLD_LINE_GAP), BUILD_COMPLETE_POPUP_GRAPH_(BUILD_COMPLETE_POPUP_GRAPH)
{
  findOriginalBackgroundPatchIndex();
  countNumOriginalPatches();

  if (BUILD_COMPLETE_POPUP_GRAPH == false) {
    findPatchChildPatches(island_patches);
    buildSubGraphes();
    findOriginalBackgroundPatchIndex();
  }
    
  if (ENFORCE_SYMMETRY_)
    enforceSymmetry();

  calcLineSegmentInfo();
  findOriginalFoldLines();    
  imwrite("Test/original_popup_graph.png", drawOriginalPopupGraph());
  findAllFoldLines();
  imwrite("Test/popup_graph.png", drawPopupGraph());
  //checkFoldLineInfo();
  findFoldLinePairs();
  //checkFoldLinePairs();
  //exit(1);
  findFoldLinePaths();
  //checkFoldLinePaths();
}
}
