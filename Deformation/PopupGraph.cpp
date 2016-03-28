#include "PopupGraph.h"
#include "PopupUtils.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cmath>
#include <iostream>

using namespace std;
using namespace cv;

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
    return make_pair(min_x, max_x);
  }

  std::map<int, std::map<int, std::set<int> > > PopupGraph::getPatchNeighborFoldLines() const
  {
    std::map<int, std::map<int, std::set<int> > > patch_neighbor_fold_lines;
    
    for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
      int left_original_patch_index = fold_line_it->original_patch_pair.first;   
      int right_original_patch_index = fold_line_it->original_patch_pair.second;
      patch_neighbor_fold_lines[left_original_patch_index][right_original_patch_index].insert(fold_line_it - fold_lines_.begin());
      patch_neighbor_fold_lines[right_original_patch_index][left_original_patch_index].insert(fold_line_it - fold_lines_.begin());
    }
    return patch_neighbor_fold_lines;
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
	      if (delta_x == 0 || patch_index_mask_[(y + delta_y) * IMAGE_WIDTH_ + (x + delta_x)] == -1)
		continue;
	      if (x + delta_x < 0 || x + delta_x >= IMAGE_WIDTH_ || y + delta_y < 0 || y + delta_y >= IMAGE_HEIGHT_)
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
	    double score = 1.0 * (left_window_it->second * right_window_it->second) / pow((FOLD_LINE_WINDOW_HEIGHT_ / 2 * 2 + 1) * (FOLD_LINE_WINDOW_WIDTH_ / 2), 2);
	    
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
	  score_fold_line_index_pairs.push_back(make_pair(fold_lines_[position_it->second].score, position_it->second));
	  position_mask[position_it->first] = true;
	}
	sort(score_fold_line_index_pairs.begin(), score_fold_line_index_pairs.end());
	reverse(score_fold_line_index_pairs.begin(), score_fold_line_index_pairs.end());
	for (vector<pair<double, int> >::const_iterator score_it = score_fold_line_index_pairs.begin(); score_it != score_fold_line_index_pairs.end(); score_it++) {
	  FoldLine fold_line = fold_lines_[score_it->second];
	  int position = fold_line.desirable_position;
          if (position_mask[position] == false)
	    continue;
	  
	  position_mask[position] = false;
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
      vector<bool> fold_line_validity_mask(fold_lines_.size(), true);
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
        vector<bool> position_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, false);
        for (vector<int>::const_iterator position_it = fold_line_it->positions.begin(); position_it != fold_line_it->positions.end(); position_it++)
          position_mask[*position_it] = true;

	vector<bool> same_side_position_mask(IMAGE_WIDTH_ * IMAGE_HEIGHT_, false);
	position_mask[fold_line_it->desirable_position] = false;
	//goes up to find positions on the same side
	{
	  vector<int> border_positions;
	  border_positions.push_back(fold_line_it->desirable_position);
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
          border_positions.push_back(fold_line_it->desirable_position);
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
	  if (same_side_position_mask[other_fold_line_it->desirable_position] == false)
	    continue;
	  if (other_fold_line_it->original_patch_pair.first != fold_line_it->original_patch_pair.second || other_fold_line_it->original_patch_pair.second != fold_line_it->original_patch_pair.first)
	    continue;
	  if (other_fold_line_it->score > fold_line_it->score)
	    continue;
	  fold_line_validity_mask[other_fold_line_it - fold_lines_.begin()] = false;
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
        if (fold_line_it->original_patch_pair.second == ORIGINAL_BACKGROUND_PATCH_INDEX_ && fold_line_it->desirable_position % IMAGE_WIDTH_ < MIDDLE_FOLD_LINE_X_)
          fold_line_validity_mask[fold_line_it - fold_lines_.begin()] = false;
	if (fold_line_it->original_patch_pair.first == ORIGINAL_BACKGROUND_PATCH_INDEX_ && fold_line_it->desirable_position % IMAGE_WIDTH_ > MIDDLE_FOLD_LINE_X_)
	  fold_line_validity_mask[fold_line_it - fold_lines_.begin()] = false;
      }
      vector<FoldLine> new_fold_lines;
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
        if (fold_line_validity_mask[fold_line_it - fold_lines_.begin()])
          new_fold_lines.push_back(*fold_line_it);
      fold_lines_ = new_fold_lines;
    }
    
    NUM_ORIGINAL_FOLD_LINES_ = fold_lines_.size();
    

    line_segment_fold_line_indices_.assign(NUM_LINE_SEGMENTS_, -1);
    //mark line segment using corresponding original fold line indices
    {
      for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++) {
        int left_original_patch_index = fold_line_it->original_patch_pair.first; 
        int right_original_patch_index = fold_line_it->original_patch_pair.second;

        int fold_line_x = fold_line_it->desirable_position % IMAGE_WIDTH_;
        int intersection_y = fold_line_it->desirable_position / IMAGE_WIDTH_;
        set<int> left_line_segment_indices;
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
              left_line_segment_indices.insert(line_segments.begin(), line_segments.end());
              break;
            }
          }
        }
        set<int> right_line_segment_indices;
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
              right_line_segment_indices.insert(line_segments.begin(), line_segments.end());
              break;
            }
          }
        }
        if (left_line_segment_indices.size() == 0 || right_line_segment_indices.size() == 0)
          continue;

        if (left_original_patch_index != ORIGINAL_BACKGROUND_PATCH_INDEX_)
          for (set<int>::const_iterator line_segment_it = left_line_segment_indices.begin(); line_segment_it != left_line_segment_indices.end(); line_segment_it++)
            line_segment_fold_line_indices_[*line_segment_it] = fold_line_it - fold_lines_.begin();
        
        if (right_original_patch_index != ORIGINAL_BACKGROUND_PATCH_INDEX_)
          for (set<int>::const_iterator line_segment_it = right_line_segment_indices.begin(); line_segment_it != right_line_segment_indices.end(); line_segment_it++)
            line_segment_fold_line_indices_[*line_segment_it] = fold_line_it - fold_lines_.begin();
      }
    }
    
    // for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++)
    //   cout << line_segment_fold_line_indices_[line_segment_index] << endl;
    // cout << "number of fold lines:" << fold_lines_.size() << endl;
    
    // for (vector<FoldLine>::const_iterator fold_line_it = fold_lines_.begin(); fold_line_it != fold_lines_.end(); fold_line_it++)
    //   cout << fold_line_it - fold_lines_.begin() << '\t' << fold_line_it->original_patch_pair.first << '\t' << fold_line_it->original_patch_pair.second << '\t' << fold_line_it->score << '\t' << fold_line_it->desirable_position % IMAGE_WIDTH_ << endl;
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
	if (fold_line_index != -1) {
	  original_patch_fold_line_indices[original_patch_index].insert(fold_line_index);
          line_segment_left_fold_lines[line_segment_index] = line_segment_right_fold_lines[line_segment_index] = pow(2, original_patch_fold_line_indices[original_patch_index].size());
	}
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
          int pixel_1 = 580 * IMAGE_WIDTH_ + 116;
          int pixel_2 = 580 * IMAGE_WIDTH_ + 115;
          //imwrite("Test/patch_index_mask_image.png", drawIndexMaskImage(patch_index_mask_, IMAGE_WIDTH_, IMAGE_HEIGHT_));
          Mat test_image = Mat::zeros(IMAGE_HEIGHT_, IMAGE_WIDTH_, CV_8UC3);
          //Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
          map<int, Vec3b> color_table;
          map<pair<int, int>, vector<int> > index_pixels;
          for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
            int index = line_segment_left_fold_lines[pixel_line_segment_indices_[pixel]];
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

      map<long, int> new_patch_index_map;
      int valid_patch_index = NUM_ORIGINAL_FOLD_LINES_ + 1;
      int invalid_patch_index = -1;
      MIDDLE_FOLD_LINE_INDEX_ = NUM_ORIGINAL_FOLD_LINES_;
      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++) {
        if (line_segment_fold_line_indices_[line_segment_index] != -1)        
          continue;
        //if (line_segment_left_fold_lines[line_segment_index] != 0 || line_segment_right_fold_lines[line_segment_index] != 0)
        //cout << line_segment_original_patch_indices[line_segment_index] << '\t' << line_segment_left_fold_lines[line_segment_index] << '\t' << line_segment_right_fold_lines[line_segment_index] << endl;
    
        long index = static_cast<long>(line_segment_original_patch_indices[line_segment_index]) * pow(max_index, 2) + line_segment_left_fold_lines[line_segment_index] * max_index + line_segment_right_fold_lines[line_segment_index];

        if (line_segment_original_patch_indices[line_segment_index] == ORIGINAL_BACKGROUND_PATCH_INDEX_)
          continue;
	
        if (line_segment_left_fold_lines[line_segment_index] == 0 || line_segment_right_fold_lines[line_segment_index] == 0)
          index = -index - 1;
        if (new_patch_index_map.count(index) == 0) {
          if (index >= 0)
            new_patch_index_map[index] = valid_patch_index++;
          else
            new_patch_index_map[index] = invalid_patch_index--;
        }
      }

      cout << valid_patch_index << endl;
      //exit(1);
      for (int line_segment_index = 0; line_segment_index < NUM_LINE_SEGMENTS_; line_segment_index++) {
        if (line_segment_fold_line_indices_[line_segment_index] != -1)
          continue;
    
        long index = static_cast<long>(line_segment_original_patch_indices[line_segment_index]) * pow(max_index, 2) + line_segment_left_fold_lines[line_segment_index] * max_index + line_segment_right_fold_lines[line_segment_index];
    
        if (line_segment_original_patch_indices[line_segment_index] == ORIGINAL_BACKGROUND_PATCH_INDEX_) {
          line_segment_fold_line_indices_[line_segment_index] = MIDDLE_FOLD_LINE_INDEX_;
          continue;
        }
    
        if (line_segment_left_fold_lines[line_segment_index] == 0 || line_segment_right_fold_lines[line_segment_index] == 0)
          index = -index - 1;
	
	line_segment_fold_line_indices_[line_segment_index] = new_patch_index_map[index];
      }

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
      if (fold_line_index == -1)
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
      int pixel = fold_lines_[fold_line_it->first].desirable_position;
      putText(original_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH_, pixel /IMAGE_WIDTH_), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(0, 0, 255));
    }

    return original_graph_image;
    //imwrite("Test/original_graph_image.bmp", original_graph_image);
  }

  //draw popup graph with all patches and fold lines
  Mat PopupGraph::drawPopupGraph()  
  {
    Mat new_graph_image = Mat::zeros(IMAGE_HEIGHT_, IMAGE_WIDTH_, CV_8UC3);
    //Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    map<int, Vec3b> fold_line_color_table;
    map<int, vector<int> > fold_line_pixels;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      int fold_line_index = line_segment_fold_line_indices_[pixel_line_segment_indices_[pixel]];
      if (fold_line_color_table.count(fold_line_index) == 0) {
	if (fold_line_index < 0) {
	  int gray_value = rand() % 256;
	  fold_line_color_table[fold_line_index] = Vec3b(gray_value, gray_value, gray_value);
	} else if (fold_line_index < NUM_ORIGINAL_FOLD_LINES_)
	  fold_line_color_table[fold_line_index] = Vec3b(0, 0, 255);
	else
	  fold_line_color_table[fold_line_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
      }
      new_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH_, pixel % IMAGE_WIDTH_) = fold_line_color_table[fold_line_index];
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

  //find neighboring fold line pairs
  void PopupGraph::findFoldLinePairs()
  {
    map<int, set<int> > fold_line_left_neighbors;
    map<int, set<int> > fold_line_right_neighbors;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++) {
      if (pixel % IMAGE_WIDTH_ == IMAGE_WIDTH_ - 1)
        continue;
      int neighbor_pixel = pixel + 1;
      if (patch_index_mask_[pixel] != patch_index_mask_[neighbor_pixel])
        continue;
      int left_line_segment_index = pixel_line_segment_indices_[pixel];
      int right_line_segment_index = pixel_line_segment_indices_[neighbor_pixel];
      int left_fold_line_index = line_segment_fold_line_indices_[left_line_segment_index];
      int right_fold_line_index = line_segment_fold_line_indices_[right_line_segment_index];

      // if (pixel == 338 * IMAGE_WIDTH + 382) {
      //   cout << left_new_patch_index << '\t' << right_fold_line_index << endl;
      //   exit(1);
      // }

      if (left_fold_line_index >= 0 && right_fold_line_index >= 0 && left_fold_line_index != right_fold_line_index) {
        fold_line_right_neighbors[left_fold_line_index].insert(right_fold_line_index);
        fold_line_left_neighbors[right_fold_line_index].insert(left_fold_line_index);
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

    sort(fold_line_pairs_.begin(), fold_line_pairs_.end());
    fold_line_pairs_.erase(unique(fold_line_pairs_.begin(), fold_line_pairs_.end()), fold_line_pairs_.end());
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
	//cout << left_fold_line_index << '\t' << right_fold_line_index << endl;
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

  void PopupGraph::checkFoldLinePairs()
  {
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_.begin(); fold_line_pair_it != fold_line_pairs_.end(); fold_line_pair_it++) {
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      cout << "fold line pairs: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
    }
  }

  void PopupGraph::checkFoldLinePaths()
  {
    for (int fold_line_index = 0; fold_line_index < getNumFoldLines(); fold_line_index++) {
      cout << "fold line index: " << fold_line_index << endl;
      cout << fold_line_left_paths_.count(fold_line_index) << '\t' << fold_line_right_paths_.count(fold_line_index) << endl;
      for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths_[fold_line_index].begin(); fold_line_it != fold_line_left_paths_[fold_line_index].end(); fold_line_it++) {
	cout << "left: " << fold_line_it->first << endl;
	for (vector<int>::const_iterator path_it = fold_line_it->second.begin(); path_it != fold_line_it->second.end(); path_it++)
	  cout << *path_it << '\t';
	cout << endl;
      }
      for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths_[fold_line_index].begin(); fold_line_it != fold_line_right_paths_[fold_line_index].end(); fold_line_it++) {
	cout << "right: " << fold_line_it->first << endl;
	for (vector<int>::const_iterator path_it = fold_line_it->second.begin(); path_it != fold_line_it->second.end(); path_it++)
	  cout << *path_it << '\t';
	cout << endl;
      }
    } 
  }

  PopupGraph::PopupGraph(const vector<int> &patch_index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, const int MIDDLE_FOLD_LINE_X) : IMAGE_WIDTH_(IMAGE_WIDTH), IMAGE_HEIGHT_(IMAGE_HEIGHT), FOLD_LINE_WINDOW_WIDTH_(FOLD_LINE_WINDOW_WIDTH), FOLD_LINE_WINDOW_HEIGHT_(FOLD_LINE_WINDOW_HEIGHT), MIDDLE_FOLD_LINE_X_(MIDDLE_FOLD_LINE_X), patch_index_mask_(patch_index_mask)
  {
    int original_background_patch = -1;
    int index = 0;
    while (original_background_patch == -1) {
      original_background_patch = patch_index_mask[index * IMAGE_WIDTH_ + index];
      index++;
    }
    ORIGINAL_BACKGROUND_PATCH_INDEX_ = original_background_patch;
  
    int num_original_patches = -1;
    for (int pixel = 0; pixel < IMAGE_WIDTH_ * IMAGE_HEIGHT_; pixel++)
      num_original_patches = max(patch_index_mask[pixel] + 1, num_original_patches);
    NUM_ORIGINAL_PATCHES_ = num_original_patches;

    calcLineSegmentInfo();
    findOriginalFoldLines();
    imwrite("Test/original_popup_graph.png", drawOriginalPopupGraph());
    findAllFoldLines();
    imwrite("Test/popup_graph.png", drawPopupGraph());
    findFoldLinePairs();
    checkFoldLinePairs();
    findFoldLinePaths();
    checkFoldLinePaths();
    exit(1);
  }
}
