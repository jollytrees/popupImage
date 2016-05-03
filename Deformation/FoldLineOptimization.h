#include "PopupGraph.h"

bool optimizeFoldLines(Popup::PopupGraph &popup_graph, const std::vector<std::vector<int> > &excluded_fold_line_combinations, const int num_new_fold_lines_constraint, const bool optimize_position = false, const bool check_stability = false);
bool optimizeFoldLinePositions(Popup::PopupGraph &popup_graph);
