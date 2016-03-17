#include <vector>
#include <map>
#include <set>

struct PopupGraph
{
  int num_fold_lines;
  int num_original_patches;
  int num_original_fold_lines;
  std::vector<std::pair<int, int> > fold_line_pairs;
  std::map<int, std::map<int, std::vector<int> > > fold_line_left_paths;
  std::map<int, std::map<int, std::vector<int> > > fold_line_right_paths;
  std::vector<int> line_segment_fold_line_indices;
  std::vector<int> pixel_line_segment_indices;
  std::map<int, std::map<int, std::set<int> > > original_patch_fold_lines;
  std::set<int> background_patches;
  std::map<int, std::map<int, double> > fold_line_x_score_map;
};

bool optimizeFoldLines(const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const PopupGraph &popup_graph);
