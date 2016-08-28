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

#include "PopupOptimization.h"
#include "PopupUtils.h"

#include <gflags/gflags.h>

using namespace std;
using namespace cv;


DEFINE_string(patch_index_mask_image_path, "Examples/bee/patch_index_mask_image.png", "The patch index mask image path. Patch index mask image is a color image with different colors indicating different patches. (A patch with gray color is treated as an island patch (such as eye patches).");
DEFINE_bool(enforce_symmetry, false, "Whether symmetry is enforced or not.");
DEFINE_int32(middle_fold_line_x_offset, 0, "Denote where the fold line should be put (image_width /  2 + offset).");
DEFINE_int32(fold_line_window_width, 10, "The preferable window width for a fold line.");
DEFINE_int32(fold_line_window_height, 10, "The preferable window height for a fold line.");
DEFINE_int32(min_fold_line_gap, 5, "The minimum distance between two neighboring fold lines.");
DEFINE_bool(write_intermediate_results, true, "Whether intermediate results are saved or not.");

int main(int argc, char *argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  
  Mat patch_index_mask_image = imread(FLAGS_patch_index_mask_image_path);
  if (patch_index_mask_image.empty()) {
    cout << "cannot open the patch index mask image" << endl;
    exit(1);
  }
  
  const bool ENFORCE_SYMMETRY = FLAGS_enforce_symmetry;
  
  const int IMAGE_WIDTH = patch_index_mask_image.cols;
  const int IMAGE_HEIGHT = patch_index_mask_image.rows;
  const int MIDDLE_FOLD_LINE_X = IMAGE_WIDTH / 2 + FLAGS_middle_fold_line_x_offset;
  
  const int FOLD_LINE_WINDOW_WIDTH = FLAGS_fold_line_window_width;    
  const int FOLD_LINE_WINDOW_HEIGHT = FLAGS_fold_line_window_height;  
  const int MIN_FOLD_LINE_GAP = FLAGS_min_fold_line_gap;

  vector<int> patch_index_mask(IMAGE_WIDTH * IMAGE_HEIGHT);
  map<int, int> color_index_map;
  set<int> island_patches;
  int island_index = 0;
  for (int pixel = 0; pixel < patch_index_mask_image.cols * patch_index_mask_image.rows; pixel++) {
    Vec3b color = patch_index_mask_image.at<Vec3b>(pixel / patch_index_mask_image.cols, pixel % patch_index_mask_image.cols);
    int color_index = color[0] * 256 * 256 + color[1] * 256 + color[2];
    if (color_index_map.count(color_index) == 0) {
      if (color[0] == color[1] && color[0] == color[2])
	island_patches.insert(island_index);
      color_index_map[color_index] = island_index++;
    }
    patch_index_mask[pixel] = color_index_map[color_index];	
  }
    
  
  Popup::PopupGraph popup_graph(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, MIDDLE_FOLD_LINE_X, island_patches, ENFORCE_SYMMETRY, false, MIN_FOLD_LINE_GAP);
  vector<vector<int> > excluded_fold_line_combinations;
  int num_new_fold_lines_constraint = 0;
  int index = 0;
  bool is_stable = false;
  const int MAX_NUM_ATTEMPTS = 10;
  while (true) {
    //set desired properties here:
    //popup_graph.setOptimizedFoldLineInfo(fold_line_index, optimized_activity, optimized_convexity, optimized_positions);
    //-1 indicates leaving the property unset
    //activity = 0, 1 or -1
    //convexity = 0, 1 or -1 (fold lines pointing inwards has 1 as convexity)
    //position = POSITIVE INTEGER or -1 (theoretically it could be a real number but current implementation only supports integers)
    //you could also pass in three vectors containing all the properties like this:
    //popup_graph.setOptimizedFoldLineInfo(optimized_fold_line_activities, optimized_fold_line_convexities, optimized_fold_line_positions);
    if (optimizeFoldLines(popup_graph, excluded_fold_line_combinations, num_new_fold_lines_constraint, 'T') == false) {
      num_new_fold_lines_constraint++;
      excluded_fold_line_combinations.clear();
      continue;
    }
    if (FLAGS_write_intermediate_results) {
      Mat optimized_popup_graph_image = popup_graph.drawOptimizedPopupGraph();    
      imwrite("Test/optimized_popup_graph_" + to_string(index) + ".png", optimized_popup_graph_image);
    }
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
    if (index == MAX_NUM_ATTEMPTS)
      break;
  }
  
  if (is_stable == false) {
    cout << "cannot find a stable structure" << endl;
    exit(1);
  }
  Popup::PopupGraph complete_popup_graph(patch_index_mask, IMAGE_WIDTH, IMAGE_HEIGHT, FOLD_LINE_WINDOW_WIDTH, FOLD_LINE_WINDOW_HEIGHT, MIDDLE_FOLD_LINE_X, set<int>(), ENFORCE_SYMMETRY, true, MIN_FOLD_LINE_GAP);
  complete_popup_graph.addOptimizedInfo(popup_graph);
  if (optimizeFoldLines(complete_popup_graph, excluded_fold_line_combinations, num_new_fold_lines_constraint, 'C') == false) {
    cout << "Cannot complete graph" << endl;
    exit(1);
  }
  if (FLAGS_write_intermediate_results) {
    Mat optimized_popup_graph_image = complete_popup_graph.drawOptimizedPopupGraph();  
    imwrite("Test/optimized_popup_graph.png", optimized_popup_graph_image);  
  }
  
  complete_popup_graph.writeRenderingInfo();
  
  return 0;
}
