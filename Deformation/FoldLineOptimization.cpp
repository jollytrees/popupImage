#include "FoldLineOptimization.h"

#include <stdio.h>
#include <stdlib.h>
#include <gurobi_c++.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

void checkAndSet(vector<vector<vector<bool> > > &mask, const int index_1, const int index_2, const int index_3, const string &message)
{
  if (mask[index_1][index_2][index_3] == true) {
    cout << "duplicate: " << message << '\t' << index_1 << '\t' << index_2 << '\t' << index_3 << endl;
    exit(1);
  }
  //else
  if (index_1 == 32 && index_2 == 0 && index_3 == 3 && false) {
    cout << message << '\t' << index_1 << '\t' << index_2 << '\t' << index_3 << endl;
    exit(1);
  }
  mask[index_1][index_2][index_3] = true;
}

void checkAndReset(vector<vector<vector<bool> > > &mask, const int index_1, const int index_2, const int index_3, const string &message)
{
  if (mask[index_1][index_2][index_3] == false) {
    cout << message << '\t' << index_1 << '\t' << index_2 << '\t' << index_3 << endl;
    exit(1);
  }
  mask[index_1][index_2][index_3] = false;
}

bool optimizeFoldLines(Popup::PopupGraph &popup_graph)
{
  const int IMAGE_WIDTH = popup_graph.getImageWidth();
  const int IMAGE_HEIGHT = popup_graph.getImageHeight();
  
  GRBEnv env = GRBEnv();
  GRBModel model = GRBModel(env);
  
  
  vector<GRBVar> fold_line_activity_indicators(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    fold_line_activity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY);  
  
  vector<GRBVar> fold_line_convexity_indicators(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    fold_line_convexity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
  model.update();
  

  vector<GRBVar> fold_line_positions(popup_graph.getNumFoldLines());

  vector<GRBVar> fold_line_Xs(popup_graph.getNumFoldLines());
  vector<GRBVar> fold_line_Ys(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    fold_line_Xs[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
    fold_line_Ys[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
  }
  model.update();

  GRBQuadExpr obj(0);

  //enforce symmetry
  if (true) {
    set<int> symmetry_fold_lines = popup_graph.getSymmetryFoldLines();
    for (set<int>::const_iterator fold_line_it = symmetry_fold_lines.begin(); fold_line_it != symmetry_fold_lines.end(); fold_line_it++)
    model.addConstr(fold_line_activity_indicators[*fold_line_it] == 1);
    model.addConstr(fold_line_activity_indicators[31] == 1);
    
    if (symmetry_fold_lines.size() > 0)
      model.addConstr(fold_line_Xs[popup_graph.getMiddleFoldLineIndex()] == popup_graph.getMiddleFoldLineX());
  }

  vector<GRBVar> right_copy_convexity_indicators(popup_graph.getNumFoldLines());
  if (false) {
    set<int> active_fold_lines;

    active_fold_lines.insert(popup_graph.getBorderFoldLineIndices().first);
    active_fold_lines.insert(popup_graph.getBorderFoldLineIndices().second);
    active_fold_lines.insert(popup_graph.getMiddleFoldLineIndex());
    active_fold_lines.insert(24);
    active_fold_lines.insert(14);
    active_fold_lines.insert(4);
    active_fold_lines.insert(27);
    active_fold_lines.insert(1);
    active_fold_lines.insert(8);
    active_fold_lines.insert(13);
    active_fold_lines.insert(18);
    active_fold_lines.insert(17);
    active_fold_lines.insert(19);
    active_fold_lines.insert(11);
    active_fold_lines.insert(16);

    set<int> inactive_fold_lines;
    inactive_fold_lines.insert(31);
    inactive_fold_lines.insert(66);
    inactive_fold_lines.insert(34);
    inactive_fold_lines.insert(42);
    inactive_fold_lines.insert(20);
    inactive_fold_lines.insert(51);
    inactive_fold_lines.insert(61);
    inactive_fold_lines.insert(63);
    inactive_fold_lines.insert(36);
    inactive_fold_lines.insert(50);
    inactive_fold_lines.insert(30);
    inactive_fold_lines.insert(53);
    inactive_fold_lines.insert(35);
    inactive_fold_lines.insert(29);
    inactive_fold_lines.insert(33);
    inactive_fold_lines.insert(62);
    //inactive_fold_lines.insert(48);
    //inactive_fold_lines.insert(46);
    // active_fold_lines.insert(8);
    // active_fold_lines.insert(10);
    // active_fold_lines.insert(2);
    // active_fold_lines.insert(9);
    // active_fold_lines.insert(10);
    // active_fold_lines.insert(6);
    // active_fold_lines.insert(7);
    // active_fold_lines.insert(17);
    
    //active_fold_lines.insert(10);
    //active_fold_lines.insert(11);
    
    // active_fold_lines.insert(0);
    // active_fold_lines.insert(1);
    // active_fold_lines.insert(2);
    // active_fold_lines.insert(3);
    // active_fold_lines.insert(6);
    // active_fold_lines.insert(7);
    
    
    // active_fold_lines.insert(0);
    // active_fold_lines.insert(1);
    // active_fold_lines.insert(3);
    // active_fold_lines.insert(11);
    // active_fold_lines.insert(17);
    // active_fold_lines.insert(2);
    // active_fold_lines.insert(9);
    // active_fold_lines.insert(7);
    // active_fold_lines.insert(8);
    
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      if (active_fold_lines.count(fold_line_index) > 0) {
	cout << "active: " << fold_line_index << endl;
	model.addConstr(fold_line_activity_indicators[fold_line_index] == 1);
      }
    // else
    //   model.addConstr(fold_line_activity_indicators[fold_line_index] == 0);

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      if (inactive_fold_lines.count(fold_line_index) > 0) {
        cout << "inactive: " << fold_line_index << endl;
        model.addConstr(fold_line_activity_indicators[fold_line_index] == 0);
      }
  }   
  
  //model.addConstr(fold_line_activity_indicators[1] == fold_line_activity_indicators[1]);
  // model.addConstr(fold_line_activity_indicators[1] <= 1);
  //model.update();

  vector<pair<int, int> > fold_line_pairs = popup_graph.getFoldLinePairs();
  
  //foldability
  if (true) {
    
    model.addConstr(fold_line_activity_indicators[popup_graph.getMiddleFoldLineIndex()] == 1, "middle fold line activity");
    model.addConstr(fold_line_convexity_indicators[popup_graph.getMiddleFoldLineIndex()] == 1, "middle fold line convexity");
    // model.addConstr(fold_line_Xs[popup_graph.num_original_fold_lines] >= IMAGE_WIDTH / 2 - 100);
    // model.addConstr(fold_line_Xs[popup_graph.num_original_fold_lines] <= IMAGE_WIDTH / 2 + 100);

    const pair<int, int> BORDER_FOLD_LINE_INDICES = popup_graph.getBorderFoldLineIndices();
    model.addConstr(fold_line_activity_indicators[BORDER_FOLD_LINE_INDICES.first] == 1, "left border fold line activity");
    model.addConstr(fold_line_convexity_indicators[BORDER_FOLD_LINE_INDICES.first] == 0, "left border fold line convexity");
    model.addConstr(fold_line_Xs[BORDER_FOLD_LINE_INDICES.first] == 0, "left border fold line X");
    model.addConstr(fold_line_Ys[BORDER_FOLD_LINE_INDICES.first] == 0, "left border fold line Y");
    model.addConstr(fold_line_activity_indicators[BORDER_FOLD_LINE_INDICES.second] == 1, "right border fold line activity");
    model.addConstr(fold_line_convexity_indicators[BORDER_FOLD_LINE_INDICES.second] == 0, "right border fold line convexity");
    model.addConstr(fold_line_Xs[BORDER_FOLD_LINE_INDICES.second] + fold_line_Ys[BORDER_FOLD_LINE_INDICES.second] == IMAGE_WIDTH - 1, "right border fold line X");
    
    
    //for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines - 2; fold_line_index++)
    //model.addQConstr(fold_line_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index] <= fold_line_Xs[popup_graph.num_fold_lines - 1] * fold_line_activity_indicators[popup_graph.num_fold_lines - 1]);
    
    vector<GRBVar> right_copy_Xs(popup_graph.getNumFoldLines());
    vector<GRBVar> right_copy_Ys(popup_graph.getNumFoldLines());
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      right_copy_convexity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
      right_copy_Xs[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
      right_copy_Ys[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
    }
    model.update();

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      model.addQConstr(fold_line_convexity_indicators[fold_line_index] * fold_line_activity_indicators[fold_line_index] == (1 - right_copy_convexity_indicators[fold_line_index]) * fold_line_activity_indicators[fold_line_index], "right copy");
      model.addQConstr(fold_line_Xs[fold_line_index] + fold_line_Ys[fold_line_index] == right_copy_Xs[fold_line_index] + right_copy_Ys[fold_line_index], "right copy");
      
      if (fold_line_index < popup_graph.getNumOriginalFoldLines()) {
	model.addQConstr(fold_line_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index], "right copy");
        model.addQConstr(fold_line_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index], "right copy");
      } else {
	model.addQConstr(fold_line_convexity_indicators[fold_line_index] * (1 - fold_line_activity_indicators[fold_line_index]) == right_copy_convexity_indicators[fold_line_index] * (1 - fold_line_activity_indicators[fold_line_index]), "right copy");
	
        model.addQConstr(fold_line_Xs[fold_line_index] == right_copy_Xs[fold_line_index], "right copy");
        model.addQConstr(fold_line_Ys[fold_line_index] == right_copy_Ys[fold_line_index], "right copy");
      }
    }
    
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      model.addQConstr(right_copy_convexity_indicators[left_fold_line_index] == fold_line_convexity_indicators[right_fold_line_index], "convexity");
      // if (left_fold_line_index >= popup_graph.getNumOriginalFoldLines())
      // 	model.addQConstr((1 - fold_line_convexity_indicators[left_fold_line_index]) * fold_line_activity_indicators[right_fold_line_index] + fold_line_convexity_indicators[left_fold_line_index] * (1 - fold_line_activity_indicators[right_fold_line_index]) == fold_line_convexity_indicators[right_fold_line_index], "convexity");
      // else
      // 	model.addQConstr((1 - right_copy_convexity_indicators[left_fold_line_index]) * fold_line_activity_indicators[right_fold_line_index] + right_copy_convexity_indicators[left_fold_line_index] * (1 - fold_line_activity_indicators[right_fold_line_index]) == fold_line_convexity_indicators[right_fold_line_index], "convexity");
    }

    const int MIN_FOLD_LINE_GAP = popup_graph.getMinFoldLineGap();
    //add fold line pair position constraints
    //vector<pair<int, int> > fold_line_pairs_without_passing = popup_graph.getFoldLinePairs();
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
      //break;
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      cout << "fold line neighbors: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
      // if (left_fold_line_index >= popup_graph.getNumOriginalFoldLines()) {
      //   model.addQConstr(fold_line_Xs[left_fold_line_index] * (1 - fold_line_convexity_indicators[left_fold_line_index]) == fold_line_Xs[right_fold_line_index] * (1 - fold_line_convexity_indicators[left_fold_line_index]), "X == X");
      // 	model.addQConstr((fold_line_Ys[left_fold_line_index] + 1) * (1 - fold_line_convexity_indicators[left_fold_line_index]) <= (fold_line_Ys[right_fold_line_index] - MIN_FOLD_LINE_GAP) * (1 - fold_line_convexity_indicators[left_fold_line_index]), "Y <= Y");
      // 	model.addQConstr(fold_line_Ys[left_fold_line_index] * fold_line_convexity_indicators[left_fold_line_index] == fold_line_Ys[right_fold_line_index] * fold_line_convexity_indicators[left_fold_line_index], "Y == Y");
      // 	model.addQConstr((fold_line_Xs[left_fold_line_index] + 1) * fold_line_convexity_indicators[left_fold_line_index] <= (fold_line_Xs[right_fold_line_index] - MIN_FOLD_LINE_GAP) * fold_line_convexity_indicators[left_fold_line_index], "X <= X");
      //}
      
      // if (left_fold_line_index == popup_graph.getMiddleFoldLineIndex()) {
      // 	cout << right_fold_line_index << endl;
      // 	pair<int, int> x_range = popup_graph.getFoldLineXRange(right_fold_line_index);
      //   cout << x_range.first << '\t' << x_range.second << endl;
      // }
      //break;
      model.addQConstr(right_copy_Xs[left_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]) == fold_line_Xs[right_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]), "X == X");
      model.addQConstr(right_copy_Ys[left_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]) <= (fold_line_Ys[right_fold_line_index] - MIN_FOLD_LINE_GAP) * (1 - right_copy_convexity_indicators[left_fold_line_index]), "Y <= Y");
      model.addQConstr(right_copy_Ys[left_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index] == fold_line_Ys[right_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index], "Y == Y");
      model.addQConstr(right_copy_Xs[left_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index] <= (fold_line_Xs[right_fold_line_index] - MIN_FOLD_LINE_GAP) * right_copy_convexity_indicators[left_fold_line_index], "X <= X");
    }
    //exit(1);
    if (true)
    {
      vector<pair<int, int> > fold_line_pairs_passed = popup_graph.getFoldLinePairsPassed();
      for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_passed.begin(); fold_line_pair_it != fold_line_pairs_passed.end(); fold_line_pair_it++) {
        int left_fold_line_index = fold_line_pair_it->first;
        int right_fold_line_index = fold_line_pair_it->second;
        cout << "fold line neighbors passed: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
	// if (fold_line_pair_it - fold_line_pairs_passed.begin() == 1)
	//   break;
        if (right_fold_line_index >= popup_graph.getNumOriginalFoldLines()) {
	  model.addQConstr(fold_line_Xs[left_fold_line_index] * (1 - fold_line_convexity_indicators[right_fold_line_index]) == fold_line_Xs[right_fold_line_index] * (1 - fold_line_convexity_indicators[right_fold_line_index]), "X == X");
	  model.addQConstr(fold_line_Ys[left_fold_line_index] * (1 - fold_line_convexity_indicators[right_fold_line_index]) <= (fold_line_Ys[right_fold_line_index] - MIN_FOLD_LINE_GAP) * (1 - fold_line_convexity_indicators[right_fold_line_index]), "Y <= Y");
	  model.addQConstr(fold_line_Ys[left_fold_line_index] * fold_line_convexity_indicators[right_fold_line_index] == fold_line_Ys[right_fold_line_index] * fold_line_convexity_indicators[right_fold_line_index], "Y == Y");
	  model.addQConstr(fold_line_Xs[left_fold_line_index] * fold_line_convexity_indicators[right_fold_line_index] <= (fold_line_Xs[right_fold_line_index] - MIN_FOLD_LINE_GAP) * fold_line_convexity_indicators[right_fold_line_index], "X <= X");
        } else {
          model.addQConstr(right_copy_Xs[left_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]) == right_copy_Xs[right_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]), "X == X");
          model.addQConstr(right_copy_Ys[left_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]) <= (right_copy_Ys[right_fold_line_index] - MIN_FOLD_LINE_GAP) * (1 - right_copy_convexity_indicators[left_fold_line_index]), "Y <= Y");
          model.addQConstr(right_copy_Ys[left_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index] == right_copy_Ys[right_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index], "Y == Y");
          model.addQConstr(right_copy_Xs[left_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index] <= (right_copy_Xs[right_fold_line_index] - MIN_FOLD_LINE_GAP) * right_copy_convexity_indicators[left_fold_line_index], "X <= X");
        }
      }
    }
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second) {
	fold_line_positions[fold_line_index] = model.addVar(0, IMAGE_WIDTH - 1, 0, GRB_INTEGER);
        continue;
      }
      pair<int, int> x_range = popup_graph.getFoldLineXRange(fold_line_index);
      //cout << x_range.first << '\t' << x_range.second << endl;
      fold_line_positions[fold_line_index] = model.addVar(x_range.first, x_range.second, 0, GRB_CONTINUOUS);
    }
    //    exit(1);
    model.update();

    //model.addConstr(fold_line_activity_indicators[10] == 1);
    // model.addConstr(fold_line_Ys[51] <= fold_line_Ys[0]);
    // model.addConstr(fold_line_positions[51] <= fold_line_positions[0] - 1);
    // model.addConstr(fold_line_positions[51] >= fold_line_positions[11] + 1);
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second)      
        continue;
      model.addConstr(fold_line_Xs[fold_line_index] + fold_line_Ys[fold_line_index] == fold_line_positions[fold_line_index], "X + Y == position");
    }

    //add middle fold line position constraints
    {
      vector<int> background_left_fold_lines = popup_graph.getBackgroundLeftFoldLines();
      for (vector<int>::const_iterator fold_line_it = background_left_fold_lines.begin(); fold_line_it != background_left_fold_lines.end(); fold_line_it++) {
	cout << "left background fold line: " << *fold_line_it << endl;
	model.addQConstr(fold_line_positions[*fold_line_it] <= fold_line_positions[popup_graph.getMiddleFoldLineIndex()] - MIN_FOLD_LINE_GAP, "x < middle x");
      }
      vector<int> background_right_fold_lines = popup_graph.getBackgroundRightFoldLines();
      for (vector<int>::const_iterator fold_line_it = background_right_fold_lines.begin(); fold_line_it != background_right_fold_lines.end(); fold_line_it++) {
	cout << "right background fold line: " << *fold_line_it << endl;
	model.addQConstr(fold_line_positions[*fold_line_it] >= fold_line_positions[popup_graph.getMiddleFoldLineIndex()] + MIN_FOLD_LINE_GAP, "x > middle x");
      }
    }

    GRBQuadExpr position_obj(0);
    vector<int> desirable_fold_line_positions;
    popup_graph.getDesirableFoldLinePositions(desirable_fold_line_positions);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumOriginalFoldLines(); fold_line_index++)
      position_obj += (fold_line_positions[fold_line_index] - desirable_fold_line_positions[fold_line_index]) * (fold_line_positions[fold_line_index] - desirable_fold_line_positions[fold_line_index]);
    //obj += position_obj;
    //model.setObjective(position_obj, 1);

    // GRBQuadExpr position_obj(0);
    // for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines() - 2; fold_line_index++)
    //   position_obj += (fold_line_positions[fold_line_index] - fold_line_min_cost_positions[fold_line_index]) * (fold_line_positions[fold_line_index] - fold_line_min_cost_positions[fold_line_index]);
    // model.setObjective(position_obj, 1);
    //for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines - 2; fold_line_index++)
    //model.setPWLObj(fold_line_positions[fold_line_index], fold_line_positions_costs_pair[fold_line_index].first.size(), &fold_line_positions_costs_pair[fold_line_index].first[0], &fold_line_positions_costs_pair[fold_line_index].second[0]);
  }

  // model.addConstr(fold_line_activity_indicators[8] == 1);
  // model.addConstr(fold_line_activity_indicators[10] == 1);
  // model.addConstr(fold_line_activity_indicators[36] == 1);
  // model.addConstr(fold_line_activity_indicators[48] == 1);
  // model.addConstr(fold_line_activity_indicators[13] == 1);
  // model.addConstr(fold_line_activity_indicators[33] == 1);
  // model.addConstr(fold_line_activity_indicators[35] == 1);
  //model.addConstr(fold_line_Xs[10] <= 100);
  
  //connectivity
  if (true) {
    vector<GRBVar> island_patch_indicators(popup_graph.getNumOriginalPatches());
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++)
      island_patch_indicators[patch_index] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    model.update();

    map<int, set<int> > island_patch_fold_lines_map = popup_graph.getIslandPatchInfo();
    if (true) {
      for (map<int, set<int> >::const_iterator patch_it = island_patch_fold_lines_map.begin(); patch_it != island_patch_fold_lines_map.end(); patch_it++) {
        cout << "island patch: " << patch_it->first << endl;
	for (set<int>::const_iterator fold_line_it = patch_it->second.begin(); fold_line_it != patch_it->second.end(); fold_line_it++)
	  cout << *fold_line_it << '\t';
	cout << endl;
      }
    }

    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      if (island_patch_fold_lines_map.count(patch_index) == 0)
        model.addConstr(island_patch_indicators[patch_index] == 0);
      else {
	GRBLinExpr fold_line_sum(0);
	for (set<int>::const_iterator fold_line_it = island_patch_fold_lines_map[patch_index].begin(); fold_line_it != island_patch_fold_lines_map[patch_index].end(); fold_line_it++)
	  fold_line_sum += fold_line_activity_indicators[*fold_line_it];
	model.addConstr(island_patch_indicators[patch_index] >= 1 - fold_line_sum);
      }
    }
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++)
      obj += island_patch_indicators[patch_index];

    if (true)
      {
        map<int, set<int> > patch_child_patches = popup_graph.getPatchChildPatches();

	std::map<int, std::map<int, std::set<int> > > patch_neighbor_left_fold_lines = popup_graph.getPatchNeighborFoldLines('L', patch_child_patches);
	std::map<int, std::map<int, std::set<int> > > patch_neighbor_right_fold_lines = popup_graph.getPatchNeighborFoldLines('R', patch_child_patches);
        for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
        if (patch_index == popup_graph.getOriginalBackgroundPatchIndex())
          continue;

	if (patch_neighbor_left_fold_lines.count(patch_index) > 0)
        {
	  set<int> left_fold_lines;
          for (map<int, set<int> >::const_iterator neighbor_patch_it = patch_neighbor_left_fold_lines.at(patch_index).begin(); neighbor_patch_it != patch_neighbor_left_fold_lines.at(patch_index).end(); neighbor_patch_it++)
            for (set<int>::const_iterator fold_line_it = neighbor_patch_it->second.begin(); fold_line_it != neighbor_patch_it->second.end(); fold_line_it++)
              left_fold_lines.insert(*fold_line_it);

          if (left_fold_lines.size() == 0 && island_patch_fold_lines_map.count(patch_index) == 0) {
	    cout << "patch " << patch_index << " has no left fold line" << endl;
	    exit(1);
	  }
	  // if (left_fold_lines.size() == 1 && island_patch_fold_lines_map.count(patch_index) == 0) {
	  //   for (set<int>::const_iterator fold_line_it = left_fold_lines.begin(); fold_line_it != left_fold_lines.end(); fold_line_it++)
	  //     model.addConstr(fold_line_activity_indicators[*fold_line_it] == 1);
	  //   continue;
	  // }
	
	  GRBLinExpr left_fold_line_sum(0);
	  for (set<int>::const_iterator fold_line_it = left_fold_lines.begin(); fold_line_it != left_fold_lines.end(); fold_line_it++) {
	    left_fold_line_sum += fold_line_activity_indicators[*fold_line_it];
	    cout << "left: " << patch_index << '\t' << *fold_line_it << endl;
	  }

	  if (island_patch_fold_lines_map.count(patch_index) == 0)
	    model.addConstr(left_fold_line_sum >= 1, "num left fold lines >= 1");
	  else
	    model.addConstr(left_fold_line_sum >= 1 - island_patch_indicators[patch_index], "num left fold lines >= 1");      
	}

	if (patch_neighbor_right_fold_lines.count(patch_index) > 0)
	{
          set<int> right_fold_lines;
          for (map<int, set<int> >::const_iterator neighbor_patch_it = patch_neighbor_right_fold_lines.at(patch_index).begin(); neighbor_patch_it != patch_neighbor_right_fold_lines.at(patch_index).end(); neighbor_patch_it++)
            for (set<int>::const_iterator fold_line_it = neighbor_patch_it->second.begin(); fold_line_it != neighbor_patch_it->second.end(); fold_line_it++)
              right_fold_lines.insert(*fold_line_it);

          if (right_fold_lines.size() == 0 && island_patch_fold_lines_map.count(patch_index) == 0) {
            cout << "patch " << patch_index << " has no right fold line" << endl;
            exit(1);
          }
          // if (right_fold_lines.size() == 1 && island_patch_fold_lines_map.count(patch_index) == 0) {
          //   for (set<int>::const_iterator fold_line_it = right_fold_lines.begin(); fold_line_it != right_fold_lines.end(); fold_line_it++)
          //     model.addConstr(fold_line_activity_indicators[*fold_line_it] == 1);
          //   continue;
          // }
        
          GRBLinExpr right_fold_line_sum(0);
          for (set<int>::const_iterator fold_line_it = right_fold_lines.begin(); fold_line_it != right_fold_lines.end(); fold_line_it++) {
            right_fold_line_sum += fold_line_activity_indicators[*fold_line_it];
            cout << "right: " << patch_index << '\t' << *fold_line_it << endl;
          }

          if (island_patch_fold_lines_map.count(patch_index) == 0)
            model.addConstr(right_fold_line_sum >= 1, "num right fold lines >= 1");
          else
            model.addConstr(right_fold_line_sum >= 1 - island_patch_indicators[patch_index], "num right fold lines >= 1");      
        }
      }
    }
    //exit(1);
    //model.addConstr(fold_line_activity_indicators[2] + fold_line_activity_indicators[3] + fold_line_activity_indicators[1] >= );

    const int MAX_DISTANCE = popup_graph.getNumOriginalPatches() - 2;
    int starting_patch_index = rand() % popup_graph.getNumOriginalPatches();
    while (starting_patch_index == popup_graph.getOriginalBackgroundPatchIndex())
      starting_patch_index = rand() % popup_graph.getNumOriginalPatches();

    vector<vector<GRBVar> > patch_distances(popup_graph.getNumOriginalPatches(), vector<GRBVar>(MAX_DISTANCE + 1));
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      GRBVar *vars_temp = model.addVars(MAX_DISTANCE);
      patch_distances[patch_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_DISTANCE);
      for (int distance = 0; distance <= MAX_DISTANCE; distance++) {
        patch_distances[patch_index][distance] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
      }
    }
    model.update();
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      if (patch_index == starting_patch_index)
	model.addConstr(patch_distances[patch_index][0] == 1, "starting patch index");
      else
	model.addConstr(patch_distances[patch_index][0] == 0, "patch distance 0 == 0");
    }

    
    std::map<int, std::map<int, std::set<int> > > patch_neighbor_fold_lines = popup_graph.getPatchNeighborFoldLines('B');
    for (int distance = 1; distance <= MAX_DISTANCE; distance++) {
      for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
	if (patch_index == starting_patch_index || patch_index == popup_graph.getOriginalBackgroundPatchIndex())
          continue;
	GRBQuadExpr QRhs = GRBQuadExpr(0);
	for (map<int, set<int> >::const_iterator neighbor_patch_it = patch_neighbor_fold_lines.at(patch_index).begin(); neighbor_patch_it != patch_neighbor_fold_lines.at(patch_index).end(); neighbor_patch_it++) {
	  if (neighbor_patch_it->first == popup_graph.getOriginalBackgroundPatchIndex())
	    continue;
	  //cout << patch_index << '\t' << neighbor_patch_it->first << '\t' << endl;
	  for (set<int>::const_iterator fold_line_it = neighbor_patch_it->second.begin(); fold_line_it != neighbor_patch_it->second.end(); fold_line_it++)
	    QRhs += patch_distances[neighbor_patch_it->first][distance - 1] * fold_line_activity_indicators[*fold_line_it];
	}
	model.addQConstr(patch_distances[patch_index][distance] <= QRhs, "patch distance");
      }
      //exit(1);
    }
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      if (patch_index == popup_graph.getOriginalBackgroundPatchIndex())
	continue;
      GRBLinExpr lhs = GRBLinExpr(0);
      for (int distance = 0; distance <= MAX_DISTANCE; distance++)
	lhs += patch_distances[patch_index][distance];
      if (island_patch_fold_lines_map.count(patch_index) == 0)
	model.addConstr(lhs == 1, "patch distance == 1");
      else
	model.addConstr(lhs == 1 - island_patch_indicators[patch_index], "patch distance == 1");
    }
  }

  
  vector<vector<GRBVar> > same_patch_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(popup_graph.getNumFoldLines()));
  for(int fold_line_index_1 = 0; fold_line_index_1 < popup_graph.getNumFoldLines(); fold_line_index_1++) {
    GRBVar *vars_temp = model.addVars(popup_graph.getNumFoldLines());
    same_patch_indicators[fold_line_index_1] = vector<GRBVar>(vars_temp, vars_temp + popup_graph.getNumFoldLines());
    for(int fold_line_index_2 = 0; fold_line_index_2 < popup_graph.getNumFoldLines(); fold_line_index_2++) {
      same_patch_indicators[fold_line_index_1][fold_line_index_2] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
  }
  model.update();

  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    model.addConstr(same_patch_indicators[fold_line_index][fold_line_index] == 1, "same patch");
    
  std::map<int, std::map<int, std::vector<int> > > fold_line_left_paths = popup_graph.getFoldLineLeftPaths();
  std::map<int, std::map<int, std::vector<int> > > fold_line_right_paths = popup_graph.getFoldLineRightPaths();

  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int other_fold_line_index = 0; other_fold_line_index < popup_graph.getNumFoldLines(); other_fold_line_index++) {
      if (other_fold_line_index == fold_line_index)
        continue;
      model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= fold_line_activity_indicators[fold_line_index], "same patch");
      model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= fold_line_activity_indicators[other_fold_line_index], "same patch");
      if ((fold_line_left_paths.count(fold_line_index) == 0 || fold_line_left_paths.at(fold_line_index).count(other_fold_line_index) == 0) && (fold_line_right_paths.count(fold_line_index) == 0 || fold_line_right_paths.at(fold_line_index).count(other_fold_line_index) == 0)) {
        model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] == 0, "same patch");
        continue;
      }
      if ((fold_line_left_paths.count(fold_line_index) > 0 && fold_line_left_paths.at(fold_line_index).count(other_fold_line_index) > 0) && (fold_line_right_paths.count(fold_line_index) > 0 && fold_line_right_paths.at(fold_line_index).count(other_fold_line_index) > 0)) {
        cout << "both path from left and right exist between: " << fold_line_index << '\t' << other_fold_line_index << endl;
        exit(1);
      }
      //cout << fold_line_index << '\t' << other_fold_line_index << endl;
      if (fold_line_left_paths.count(fold_line_index) > 0 && fold_line_left_paths.at(fold_line_index).count(other_fold_line_index) > 0) {
        vector<int> path = fold_line_left_paths.at(fold_line_index).at(other_fold_line_index);
        //cout << "left: " << fold_line_index << '\t' << other_fold_line_index << '\t' << path.size() << endl;
        for (vector<int>::const_iterator path_it = path.begin(); path_it != path.end(); path_it++)
          model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= 1 - fold_line_activity_indicators[*path_it], "same patch");
      }
      if (fold_line_right_paths.count(fold_line_index) > 0 && fold_line_right_paths.at(fold_line_index).count(other_fold_line_index) > 0) {
        vector<int> path = fold_line_right_paths.at(fold_line_index).at(other_fold_line_index);
        //cout << "right: " << fold_line_index << '\t' << other_fold_line_index << '\t' << path.size() << endl;
        for (vector<int>::const_iterator path_it = path.begin(); path_it != path.end(); path_it++)
          model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= 1 - fold_line_activity_indicators[*path_it], "same patch");
      }
    }
  }

  const int MAX_STABILITY_DEPTH = 5;
  const int MAX_CONNECTION_DEPTH = 4;
  vector<vector<vector<GRBVar> > > left_left_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1));
  vector<vector<vector<bool> > > left_left_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      left_left_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
	left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
  }
  vector<vector<vector<GRBVar> > > left_right_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1));
  vector<vector<vector<bool> > > left_right_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      left_right_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
        left_right_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
  }
  vector<vector<vector<GRBVar> > > right_left_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1));
  vector<vector<vector<bool> > > right_left_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      right_left_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
        right_left_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
  }
  vector<vector<vector<GRBVar> > > right_right_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1));
  vector<vector<vector<bool> > > right_right_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      right_right_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
        right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
  }
  model.update();

  //stability
  if (true) {
    set<int> left_background_fold_lines;
    set<int> right_background_fold_lines;
    left_background_fold_lines.insert(popup_graph.getBorderFoldLineIndices().first);
    right_background_fold_lines.insert(popup_graph.getBorderFoldLineIndices().second);
    left_background_fold_lines.insert(popup_graph.getMiddleFoldLineIndex());
    right_background_fold_lines.insert(popup_graph.getMiddleFoldLineIndex());
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(popup_graph.getBorderFoldLineIndices().first).begin(); fold_line_it != fold_line_right_paths.at(popup_graph.getBorderFoldLineIndices().first).end(); fold_line_it++)
      left_background_fold_lines.insert(fold_line_it->first);
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(popup_graph.getBorderFoldLineIndices().second).begin(); fold_line_it != fold_line_left_paths.at(popup_graph.getBorderFoldLineIndices().second).end(); fold_line_it++)
      right_background_fold_lines.insert(fold_line_it->first);

    //add constraints for background fold lines
    for (set<int>::const_iterator fold_line_it = left_background_fold_lines.begin(); fold_line_it != left_background_fold_lines.end(); fold_line_it++) {
      checkAndSet(left_left_stability_mask, *fold_line_it, 0, 0, "background stable");
      model.addConstr(left_left_stability_indicators[*fold_line_it][0][0] == 1, "background fold line is stable");
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++)
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
	  if (stability_depth > 0 || connection_depth > 0) {
	    checkAndSet(left_left_stability_mask, *fold_line_it, stability_depth, connection_depth, "background fold line is stable");
	    model.addConstr(left_left_stability_indicators[*fold_line_it][stability_depth][connection_depth] == 0, "background fold line is stable");
	  }
    }
    for (set<int>::const_iterator fold_line_it = right_background_fold_lines.begin(); fold_line_it != right_background_fold_lines.end(); fold_line_it++) {
      checkAndSet(right_right_stability_mask, *fold_line_it, 0, 0, "background stable");
      model.addConstr(right_right_stability_indicators[*fold_line_it][0][0] == 1, "background fold line is stable");
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++)
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
	  if (stability_depth > 0 || connection_depth > 0) {
	    checkAndSet(right_right_stability_mask, *fold_line_it, stability_depth, connection_depth, "background fold line is stable");
	    model.addConstr(right_right_stability_indicators[*fold_line_it][stability_depth][connection_depth] == 0, "background fold line is stable");
	  }
    }

    //add constraints for non-background fold lines
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
	continue;
      //checkAndSet(left_left_stability_mask, fold_line_index, 0, 0, "non-background fold line is unstable");
      model.addConstr(left_left_stability_indicators[fold_line_index][0][0] == 0, "non-background fold line is unstable");
    
      //checkAndSet(right_right_stability_mask, fold_line_index, 0, 0, "non-background fold line is unstable");
      model.addConstr(right_right_stability_indicators[fold_line_index][0][0] == 0, "non-background fold line is unstable");
    }

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	  if (fold_line_index < popup_graph.getNumOriginalFoldLines()) {
	    model.addQConstr(left_right_stability_indicators[fold_line_index][stability_depth][connection_depth] <= fold_line_activity_indicators[fold_line_index], "stability stops at inactive original fold lines");
	    model.addQConstr(right_left_stability_indicators[fold_line_index][stability_depth][connection_depth] <= fold_line_activity_indicators[fold_line_index], "stability stops at inactive original fold lines");
	  } else {
	    //checkAndSet(left_right_stability_mask, fold_line_index, stability_depth, connection_depth, "stability passes through inactive fold lines");     
	    model.addQConstr(left_right_stability_indicators[fold_line_index][stability_depth][connection_depth] * (1 - fold_line_activity_indicators[fold_line_index]) == left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] * (1 - fold_line_activity_indicators[fold_line_index]), "stability passes through inactive fold lines");

	    //checkAndSet(right_left_stability_mask, fold_line_index, stability_depth, connection_depth, "stability passes through inactive fold lines");
	    model.addQConstr(right_left_stability_indicators[fold_line_index][stability_depth][connection_depth] * (1 - fold_line_activity_indicators[fold_line_index]) == right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] * (1 - fold_line_activity_indicators[fold_line_index]), "stability passes through inactive fold lines");
	  }
	}
      }
    }
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (fold_line_index == popup_graph.getMiddleFoldLineIndex() || fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second)
	continue;
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	for (int connection_depth = 0; connection_depth < MAX_CONNECTION_DEPTH; connection_depth++) {
	  if (connection_depth == 2)
	    continue;

	  checkAndSet(left_right_stability_mask, fold_line_index, stability_depth, connection_depth + 1, "connection depth increases through active fold lines");     
	  model.addQConstr(left_right_stability_indicators[fold_line_index][stability_depth][connection_depth + 1] * fold_line_activity_indicators[fold_line_index] == left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] * fold_line_activity_indicators[fold_line_index], "connection depth increases through active fold lines");
        
	  checkAndSet(right_left_stability_mask, fold_line_index, stability_depth, connection_depth + 1, "connection depth increases through active fold lines");     
	  model.addQConstr(right_left_stability_indicators[fold_line_index][stability_depth][connection_depth + 1] * fold_line_activity_indicators[fold_line_index] == right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] * fold_line_activity_indicators[fold_line_index], "stability passes through active fold lines");
	}
      }
    }

    map<int, set<int> > fold_line_left_neighbors;
    map<int, set<int> > fold_line_right_neighbors;
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      fold_line_right_neighbors[left_fold_line_index].insert(right_fold_line_index);
      fold_line_left_neighbors[right_fold_line_index].insert(left_fold_line_index);
    }
  
    //    for (set<int>::const_iterator neighbor_it = fold_line_left_neighbors[48].begin(); neighbor_it != fold_line_left_neighbors[48].end(); neighbor_it++)
    //cout << *neighbor_it << endl;
    //exit(1);
    //fold line neighbor constraints
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	  if (connection_depth == 0 || connection_depth == 3)
	    continue;
	  if (fold_line_left_neighbors.count(fold_line_index) > 0) {
	    GRBLinExpr left_neighbor_sum(0);
	    for (set<int>::const_iterator neighbor_it = fold_line_left_neighbors[fold_line_index].begin(); neighbor_it != fold_line_left_neighbors[fold_line_index].end(); neighbor_it++)
	      left_neighbor_sum += left_right_stability_indicators[*neighbor_it][stability_depth][connection_depth];
	  
	    checkAndSet(left_left_stability_mask, fold_line_index, stability_depth, connection_depth, "stability properties are the same for fold line pairs");
	    model.addQConstr(left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] <= left_neighbor_sum, "stability properties are the same for fold line pairs");
	  }
	
	  if (fold_line_right_neighbors.count(fold_line_index) > 0) {
	    GRBLinExpr right_neighbor_sum(0);
	    for (set<int>::const_iterator neighbor_it = fold_line_right_neighbors[fold_line_index].begin(); neighbor_it != fold_line_right_neighbors[fold_line_index].end(); neighbor_it++)
	      right_neighbor_sum += right_left_stability_indicators[*neighbor_it][stability_depth][connection_depth];

	    checkAndSet(right_right_stability_mask, fold_line_index, stability_depth, connection_depth, "stability properties are the same for fold line pairs");
	    model.addQConstr(right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] <= right_neighbor_sum, "stability properties are the same for fold line pairs");
	  }
	}
      }
    }

    //double-connected fold line constraints
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	GRBQuadExpr left_c_sum(0);
	GRBQuadExpr right_c_sum(0);
	for (int previous_stability_depth = 0; previous_stability_depth <= stability_depth; previous_stability_depth++) {
	  if (fold_line_left_paths.count(fold_line_index) > 0) {
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths.at(fold_line_index).end(); fold_line_it++) {
	      left_c_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      right_c_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	    }
	  }
	  if (fold_line_right_paths.count(fold_line_index) > 0) {
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++) {
	      left_c_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      right_c_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	    }
	  }
	}
	checkAndSet(left_left_stability_mask, fold_line_index, stability_depth, 3, "double-connected");
	model.addQConstr(2 * left_left_stability_indicators[fold_line_index][stability_depth][3] <= left_c_sum, "double-connected");

	checkAndSet(right_right_stability_mask, fold_line_index, stability_depth, 3, "double-connected");
	model.addQConstr(2 * right_right_stability_indicators[fold_line_index][stability_depth][3] <= right_c_sum, "double-connected");
      }
    }

    //stable fold line constraints
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 1; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	GRBQuadExpr left_s_sum(0);
	GRBQuadExpr right_s_sum(0);
	vector<GRBLinExpr> left_stability_sum(MAX_CONNECTION_DEPTH + 1);
	vector<GRBLinExpr> right_stability_sum(MAX_CONNECTION_DEPTH + 1);
	for (int previous_stability_depth = 0; previous_stability_depth <= stability_depth; previous_stability_depth++) {
	  if (fold_line_left_paths.count(fold_line_index) > 0) {
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths.at(fold_line_index).end(); fold_line_it++) {
	      left_s_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      left_s_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
	    }
	  }
	  if (fold_line_right_paths.count(fold_line_index) > 0) {
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++) {
	      right_s_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      right_s_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
	    }
	  }
	}
	for (int previous_stability_depth = 0; previous_stability_depth < stability_depth; previous_stability_depth++) {
	  for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	    left_stability_sum[connection_depth] += left_left_stability_indicators[fold_line_index][previous_stability_depth][connection_depth];
	    right_stability_sum[connection_depth] += right_right_stability_indicators[fold_line_index][previous_stability_depth][connection_depth];
	  }
	}
	
	// GRBVar left_s = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
	// GRBVar right_s = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
	// model.update();
	// model.addQConstr(2 * left_s <= left_s_sum);      
	// model.addQConstr(2 * right_s <= right_s_sum);
	
        checkAndSet(left_left_stability_mask, fold_line_index, stability_depth, 0, "stable");
        model.addQConstr(2 * left_left_stability_indicators[fold_line_index][stability_depth][0] <= left_s_sum, "stable");
      
        checkAndSet(right_right_stability_mask, fold_line_index, stability_depth, 0, "stable");
        model.addQConstr(2 * right_right_stability_indicators[fold_line_index][stability_depth][0] <= right_s_sum, "stable");

	GRBQuadExpr stability_sum(0);
	//stability_sum += left_s;
	//stability_sum += right_s;
	stability_sum += left_stability_sum[0] * right_stability_sum[0];
	stability_sum += left_stability_sum[0] * right_stability_sum[1];
	stability_sum += left_stability_sum[1] * right_stability_sum[0];
	stability_sum += left_stability_sum[0] * right_stability_sum[2];
	stability_sum += left_stability_sum[2] * right_stability_sum[0];
	stability_sum += left_stability_sum[0] * right_stability_sum[3];
	stability_sum += left_stability_sum[3] * right_stability_sum[0];
	stability_sum += left_stability_sum[0] * right_stability_sum[4];
	stability_sum += left_stability_sum[4] * right_stability_sum[0];
	stability_sum += left_stability_sum[1] * right_stability_sum[1];
	stability_sum += left_stability_sum[3] * right_stability_sum[3];
	stability_sum += left_stability_sum[1] * right_stability_sum[3];
	stability_sum += left_stability_sum[3] * right_stability_sum[1];
	stability_sum += left_stability_sum[3] * right_stability_sum[4];
	stability_sum += left_stability_sum[4] * right_stability_sum[3];
	checkAndSet(left_right_stability_mask, fold_line_index, stability_depth, 0, "stable");
	model.addQConstr(left_right_stability_indicators[fold_line_index][stability_depth][0] <= stability_sum, "stable");
      
	checkAndSet(right_left_stability_mask, fold_line_index, stability_depth, 0, "stable");
	model.addQConstr(right_left_stability_indicators[fold_line_index][stability_depth][0] <= stability_sum, "stable");
      }
    }

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
	continue;
      vector<GRBLinExpr> left_stability_sum(MAX_CONNECTION_DEPTH + 1);
      vector<GRBLinExpr> right_stability_sum(MAX_CONNECTION_DEPTH + 1);

      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	  left_stability_sum[connection_depth] += left_left_stability_indicators[fold_line_index][stability_depth][connection_depth];
	  right_stability_sum[connection_depth] += right_right_stability_indicators[fold_line_index][stability_depth][connection_depth];
	}
      }

      model.addConstr(left_stability_sum[0] + right_stability_sum[0] >= fold_line_activity_indicators[fold_line_index], "stable sum >= 1");
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	model.addConstr(left_stability_sum[connection_depth] <= 1, "property sum <= 1");
	model.addConstr(right_stability_sum[connection_depth] <= 1, "property sum <= 1");
      }
    }

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      //    if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
      //      continue;
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	  if (left_left_stability_mask[fold_line_index][stability_depth][connection_depth] == false)
	    model.addConstr(left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] == 0);
	  if (left_right_stability_mask[fold_line_index][stability_depth][connection_depth] == false)
	    model.addConstr(left_right_stability_indicators[fold_line_index][stability_depth][connection_depth] == 0);
	  if (right_left_stability_mask[fold_line_index][stability_depth][connection_depth] == false)
	    model.addConstr(right_left_stability_indicators[fold_line_index][stability_depth][connection_depth] == 0);
	  if (right_right_stability_mask[fold_line_index][stability_depth][connection_depth] == false)
	    model.addConstr(right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] == 0);

	  // checkAndReset(left_left_stability_mask, fold_line_index, stability_depth, connection_depth, "missing constraint");
	  // checkAndReset(left_right_stability_mask, fold_line_index, stability_depth, connection_depth, "missing constraint");
	  // checkAndReset(right_left_stability_mask, fold_line_index, stability_depth, connection_depth, "missing constraint");
	  // checkAndReset(right_right_stability_mask, fold_line_index, stability_depth, connection_depth, "missing constraint");
	}
      }
    }

    if (false) {
      model.addConstr(left_right_stability_indicators[6][0][2] == 1);
      model.addConstr(left_right_stability_indicators[32][0][2] == 1);
      model.addConstr(left_left_stability_indicators[45][0][3] == 1);
      model.addConstr(right_right_stability_indicators[45][0][3] == 1);
      model.addConstr(right_right_stability_indicators[45][1][0] == 1);
      model.addConstr(left_left_stability_indicators[32][2][0] == 1);
      model.addConstr(right_right_stability_indicators[60][2][0] == 1);
      model.addConstr(left_left_stability_indicators[6][2][0] == 1);
      model.addConstr(right_right_stability_indicators[10][2][0] == 1);
      model.addConstr(left_left_stability_indicators[16][3][0] == 1);
      model.addConstr(right_right_stability_indicators[11][3][0] == 1);
      model.addConstr(right_right_stability_indicators[41][4][0] == 1);
      model.addConstr(right_right_stability_indicators[17][5][0] == 1);
      model.addConstr(right_right_stability_indicators[19][5][0] == 1);
      model.addConstr(right_right_stability_indicators[49][6][0] == 1);
      model.addConstr(right_right_stability_indicators[13][6][0] == 1);
      model.addConstr(right_right_stability_indicators[18][6][0] == 1);
      model.addConstr(left_left_stability_indicators[39][6][0] == 1);
      model.addConstr(right_right_stability_indicators[55][6][0] == 1);
      //model.addConstr(right_right_stability_indicators[47][5][0] == 1);
    }
  }  
  //model.setObjective(obj, 1);
  
  model.update();
  model.set(GRB_StringAttr_ModelName,"fold line assignment");
  model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
  /*model.getEnv().set(GRB_DoubleParam_MIPGap, 0.005);
    model.getEnv().set(GRB_DoubleParam_NodeLimit, 1000)*/;
  model.getEnv().set(GRB_DoubleParam_TimeLimit, 300);

  
  try {
    //model.set("OutputFlag", false);
    model.update();
    model.optimize();
    
    if (model.get(GRB_IntAttr_IsMIP) == 0)
      throw GRBException("Model is not a MIP");
    
  }catch(GRBException e) {
    cout << "Error code = " << e.getErrorCode() << endl;
    cout << e.getMessage() << endl;
  }catch (...) {printf("Exception...\n");exit(1);}

  
  if (true) {
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      cout << fold_line_index << '\t' << fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << '\t' << fold_line_Xs[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_Ys[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X) << endl;

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (static_cast<int>(fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5) == 0)
	cout << "inactive ";
      //continue;
      cout << "fold line index: " << fold_line_index << endl;
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	  if (static_cast<int>(left_left_stability_indicators[fold_line_index][stability_depth][connection_depth].get(GRB_DoubleAttr_X) + 0.5) == 1)              
            cout << "left left: " << stability_depth << '\t' << connection_depth << endl;
	  if (static_cast<int>(left_right_stability_indicators[fold_line_index][stability_depth][connection_depth].get(GRB_DoubleAttr_X) + 0.5) == 1)              
            cout << "left right: " << stability_depth << '\t' << connection_depth << endl;
	  if (static_cast<int>(right_right_stability_indicators[fold_line_index][stability_depth][connection_depth].get(GRB_DoubleAttr_X) + 0.5) == 1)    
            cout << "right right: " << stability_depth << '\t' << connection_depth << endl;
	  if (static_cast<int>(right_left_stability_indicators[fold_line_index][stability_depth][connection_depth].get(GRB_DoubleAttr_X) + 0.5) == 1)    
            cout << "right left: " << stability_depth << '\t' << connection_depth << endl;
	}
      }
    }
      
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      for (int other_fold_line_index = 0; other_fold_line_index < popup_graph.getNumFoldLines(); other_fold_line_index++)
	if (other_fold_line_index != fold_line_index && same_patch_indicators[fold_line_index][other_fold_line_index].get(GRB_DoubleAttr_X) == 1)
	  cout << fold_line_index << '\t' << other_fold_line_index << endl;
  }
  //  popup_graph.checkFoldLineInfo();

  vector<int> optimized_fold_line_positions(popup_graph.getNumFoldLines(), -1);
  vector<bool> optimized_fold_line_convexities(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    if (fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second) {
      optimized_fold_line_positions[fold_line_index] = -1;
      continue;
    }
    if (static_cast<int>(fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5) == 1)
      optimized_fold_line_positions[fold_line_index] = fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X);      
    optimized_fold_line_convexities[fold_line_index] = static_cast<int>(fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5) == 1;
  }
  popup_graph.setOptimizedFoldLineInfo(optimized_fold_line_positions, optimized_fold_line_convexities);

  return true;
}
