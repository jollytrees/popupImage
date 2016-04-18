#include "FoldLineOptimization.h"

#include <stdio.h>
#include <stdlib.h>
#include <gurobi_c++.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


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

  GRBLinExpr obj(0);

  //enforce symmetry
  if (true) {
    vector<int> symmetry_fold_lines = popup_graph.getSymmetryFoldLines();
    for (vector<int>::const_iterator fold_line_it = symmetry_fold_lines.begin(); fold_line_it != symmetry_fold_lines.end(); fold_line_it++)
      model.addConstr(fold_line_activity_indicators[*fold_line_it] == 1);
    if (symmetry_fold_lines.size() > 0)
      model.addConstr(fold_line_Xs[popup_graph.getMiddleFoldLineIndex()] == popup_graph.getMiddleFoldLineX());
  }
   
  if (false) {
    set<int> active_fold_lines;

    active_fold_lines.insert(popup_graph.getBorderFoldLineIndices().first);
    active_fold_lines.insert(popup_graph.getBorderFoldLineIndices().second);
    active_fold_lines.insert(popup_graph.getMiddleFoldLineIndex());
    active_fold_lines.insert(0);
    active_fold_lines.insert(1);
    active_fold_lines.insert(8);
    active_fold_lines.insert(10);
    active_fold_lines.insert(2);
    active_fold_lines.insert(9);
    active_fold_lines.insert(10);
    active_fold_lines.insert(6);
    active_fold_lines.insert(7);
    active_fold_lines.insert(17);
    
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
    
    
    // active_fold_lines.insert(37);
    // active_fold_lines.insert(30);
    // active_fold_lines.insert(31);
    // active_fold_lines.insert(26);
    // active_fold_lines.insert(23);
    // active_fold_lines.insert(16);
    // active_fold_lines.insert(14);
    // active_fold_lines.insert(13);
    // active_fold_lines.insert(66);
    // active_fold_lines.insert(61);
    // active_fold_lines.insert(65);
    // active_fold_lines.insert(55);
    // active_fold_lines.insert(42);
    // active_fold_lines.insert(85);
    // active_fold_lines.insert(34);
    // active_fold_lines.insert(28);
    // active_fold_lines.insert(21);
    // active_fold_lines.insert(12);
    // active_fold_lines.insert(11);
    // active_fold_lines.insert(8);
    // active_fold_lines.insert(68);
    // active_fold_lines.insert(67);
    // active_fold_lines.insert(63);
    // active_fold_lines.insert(20);
    // active_fold_lines.insert(10);
    // active_fold_lines.insert(48);
    // active_fold_lines.insert(77);
    // active_fold_lines.insert(3);
    // active_fold_lines.insert(18);
    // active_fold_lines.insert(33);
    // active_fold_lines.insert(6);
    // active_fold_lines.insert(5);
    // active_fold_lines.insert(0);
    // active_fold_lines.insert(63);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      if (active_fold_lines.count(fold_line_index) > 0) {
	cout << fold_line_index << endl;
	model.addConstr(fold_line_activity_indicators[fold_line_index] == 1);
      }
    else
      model.addConstr(fold_line_activity_indicators[fold_line_index] == 0);
  }   
  
  //model.addConstr(fold_line_activity_indicators[1] == fold_line_activity_indicators[1]);
  // model.addConstr(fold_line_activity_indicators[1] <= 1);
  //model.update();
  
  //foldability
  if (true) {
    vector<pair<int, int> > fold_line_pairs = popup_graph.getFoldLinePairs();
    
    model.addConstr(fold_line_activity_indicators[popup_graph.getMiddleFoldLineIndex()] == 1, "middle fold line activity");
    model.addConstr(fold_line_convexity_indicators[popup_graph.getMiddleFoldLineIndex()] == 0, "middle fold line convexity");
    // model.addConstr(fold_line_Xs[popup_graph.num_original_fold_lines] >= IMAGE_WIDTH / 2 - 100);
    // model.addConstr(fold_line_Xs[popup_graph.num_original_fold_lines] <= IMAGE_WIDTH / 2 + 100);

    const pair<int, int> BORDER_FOLD_LINE_INDICES = popup_graph.getBorderFoldLineIndices();
    model.addConstr(fold_line_activity_indicators[BORDER_FOLD_LINE_INDICES.first] == 1, "left border fold line activity");
    model.addConstr(fold_line_convexity_indicators[BORDER_FOLD_LINE_INDICES.first] == 1, "left border fold line convexity");
    model.addConstr(fold_line_Xs[BORDER_FOLD_LINE_INDICES.first] == 0, "left border fold line X");
    model.addConstr(fold_line_Ys[BORDER_FOLD_LINE_INDICES.first] == 0, "left border fold line Y");
    model.addConstr(fold_line_activity_indicators[BORDER_FOLD_LINE_INDICES.second] == 1, "right border fold line activity");
    model.addConstr(fold_line_convexity_indicators[BORDER_FOLD_LINE_INDICES.second] == 1, "right border fold line convexity");
    model.addConstr(fold_line_Xs[BORDER_FOLD_LINE_INDICES.second] + fold_line_Ys[BORDER_FOLD_LINE_INDICES.second] == IMAGE_WIDTH - 1, "right border fold line X");
    
    
    //for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines - 2; fold_line_index++)
    //model.addQConstr(fold_line_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index] <= fold_line_Xs[popup_graph.num_fold_lines - 1] * fold_line_activity_indicators[popup_graph.num_fold_lines - 1]);


    vector<GRBVar> right_copy_convexity_indicators(popup_graph.getNumFoldLines());
    vector<GRBVar> right_copy_Xs(popup_graph.getNumFoldLines());
    vector<GRBVar> right_copy_Ys(popup_graph.getNumFoldLines());
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      right_copy_convexity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
      right_copy_Xs[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
      right_copy_Ys[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
    }
    model.update();

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (fold_line_index < popup_graph.getNumOriginalFoldLines()) {
	model.addQConstr(fold_line_convexity_indicators[fold_line_index] * fold_line_activity_indicators[fold_line_index] == (1 - right_copy_convexity_indicators[fold_line_index]) * fold_line_activity_indicators[fold_line_index], "right copy");
        model.addQConstr(fold_line_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index], "right copy");
	model.addQConstr(fold_line_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index], "right copy");
      } else {
	model.addQConstr(fold_line_convexity_indicators[fold_line_index] * fold_line_activity_indicators[fold_line_index] == (1 - right_copy_convexity_indicators[fold_line_index]) * fold_line_activity_indicators[fold_line_index], "right copy");
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
      // } else {
      model.addQConstr(right_copy_Xs[left_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]) == fold_line_Xs[right_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]), "X == X");
      model.addQConstr(right_copy_Ys[left_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]) <= (fold_line_Ys[right_fold_line_index] - MIN_FOLD_LINE_GAP) * (1 - right_copy_convexity_indicators[left_fold_line_index]), "Y <= Y");
      model.addQConstr(right_copy_Ys[left_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index] == fold_line_Ys[right_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index], "Y == Y");
      model.addQConstr(right_copy_Xs[left_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index] <= (fold_line_Xs[right_fold_line_index] - MIN_FOLD_LINE_GAP) * right_copy_convexity_indicators[left_fold_line_index], "X <= X");
    }
    
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
      fold_line_positions[fold_line_index] = model.addVar(x_range.first, x_range.second, 0, GRB_INTEGER);
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
      for (vector<int>::const_iterator fold_line_it = background_left_fold_lines.begin(); fold_line_it != background_left_fold_lines.end(); fold_line_it++)    
	model.addQConstr(fold_line_positions[*fold_line_it] * (1 - fold_line_activity_indicators[*fold_line_it]) <= (fold_line_positions[popup_graph.getMiddleFoldLineIndex()] - 1) * (1 - fold_line_activity_indicators[*fold_line_it]), "x < middle x");
      vector<int> background_right_fold_lines = popup_graph.getBackgroundRightFoldLines();
      for (vector<int>::const_iterator fold_line_it = background_right_fold_lines.begin(); fold_line_it != background_right_fold_lines.end(); fold_line_it++)    
	model.addQConstr(fold_line_positions[*fold_line_it] * (1 - fold_line_activity_indicators[*fold_line_it]) >= (fold_line_positions[popup_graph.getMiddleFoldLineIndex()] + 1) * (1 - fold_line_activity_indicators[*fold_line_it]), "x > middle x");
    }
    
    // GRBQuadExpr position_obj(0);
    // for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines() - 2; fold_line_index++)
    //   position_obj += (fold_line_positions[fold_line_index] - fold_line_min_cost_positions[fold_line_index]) * (fold_line_positions[fold_line_index] - fold_line_min_cost_positions[fold_line_index]);
    //    model.setObjective(position_obj, 1);
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

  
  const int MAX_STABILITY_DEPTH = 4;
  
  vector<vector<GRBVar> > same_patch_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(popup_graph.getNumFoldLines()));
  for(int fold_line_index_1 = 0; fold_line_index_1 < popup_graph.getNumFoldLines(); fold_line_index_1++) {
    GRBVar *vars_temp = model.addVars(popup_graph.getNumFoldLines());
    same_patch_indicators[fold_line_index_1] = vector<GRBVar>(vars_temp, vars_temp + popup_graph.getNumFoldLines());
    for(int fold_line_index_2 = 0; fold_line_index_2 < popup_graph.getNumFoldLines(); fold_line_index_2++) {
      same_patch_indicators[fold_line_index_1][fold_line_index_2] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
  }
  model.update();

  vector<vector<GRBVar> > s_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > left_c_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > right_c_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > left_cc_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > right_cc_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > left_d_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > right_d_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > left_e_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
  vector<vector<GRBVar> > right_e_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));

  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    s_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      s_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    left_c_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      left_c_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    right_c_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      right_c_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    left_cc_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      left_cc_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    right_cc_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      right_cc_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    left_d_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      left_d_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    right_d_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      right_d_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    left_e_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      left_e_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
    right_e_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
      right_e_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  }
  model.update();
  
  //stability
  if (false) {
    // GRBLinExpr same_patch_obj(0);
    // for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines; fold_line_index++)
    //   for (int other_fold_line_index = 0; other_fold_line_index < popup_graph.num_fold_lines; other_fold_line_index++)
    // 	if (other_fold_line_index != fold_line_index)
    // 	  same_patch_obj += 1 - same_patch_indicators[fold_line_index][other_fold_line_index];
    // model.setObjective(same_patch_obj, 1);

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
	//break;
	//int connected_fold_line_index = fold_line_index < popup_graph.num_original_fold_lines ? (fold_line_index % 2 == 0 ? fold_lindex_index + 1 : fold_line_index - 1) : -1;
	//cout << fold_line_index << '\t' << other_fold_line_index << endl;
	if (fold_line_left_paths.count(fold_line_index) > 0 && fold_line_left_paths.at(fold_line_index).count(other_fold_line_index) > 0) {
	  vector<int> path = fold_line_left_paths.at(fold_line_index).at(other_fold_line_index);
	  //cout << "left: " << fold_line_index << '\t' << other_fold_line_index << '\t' << path.size() << endl;
	  // if (path.size() == 0)
	  //   model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] == 1);
	  // else
	  for (vector<int>::const_iterator path_it = path.begin(); path_it != path.end(); path_it++)
	    model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= 1 - fold_line_activity_indicators[*path_it], "same patch");
	}
	if (fold_line_right_paths.count(fold_line_index) > 0 && fold_line_right_paths.at(fold_line_index).count(other_fold_line_index) > 0) {
	  vector<int> path = fold_line_right_paths.at(fold_line_index).at(other_fold_line_index);
	  //cout << "right: " << fold_line_index << '\t' << other_fold_line_index << '\t' << path.size() << endl;
	  // if (path.size() == 0)
	  //   model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] == 1);
	  // else
	  for (vector<int>::const_iterator path_it = path.begin(); path_it != path.end(); path_it++)
	    model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= 1 - fold_line_activity_indicators[*path_it], "same patch");
	}
      }
    }
    
    //model.addConstr(same_patch_indicators[50][85] == 1);
    //model.addConstr(same_patch_indicators[85][10] == 1);
    
    //if (true) {    

    set<int> background_fold_lines;
    background_fold_lines.insert(popup_graph.getBorderFoldLineIndices().first);
    background_fold_lines.insert(popup_graph.getBorderFoldLineIndices().second);
    background_fold_lines.insert(popup_graph.getMiddleFoldLineIndex());
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(popup_graph.getBorderFoldLineIndices().first).begin(); fold_line_it != fold_line_right_paths.at(popup_graph.getBorderFoldLineIndices().first).end(); fold_line_it++)
      background_fold_lines.insert(fold_line_it->first);
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(popup_graph.getBorderFoldLineIndices().second).begin(); fold_line_it != fold_line_left_paths.at(popup_graph.getBorderFoldLineIndices().second).end(); fold_line_it++)
      background_fold_lines.insert(fold_line_it->first);

    //add constraints for background fold lines
    for (set<int>::const_iterator fold_line_it = background_fold_lines.begin(); fold_line_it != background_fold_lines.end(); fold_line_it++) {
      cout << "background fold line: " << *fold_line_it << endl;
      model.addConstr(s_indicators[*fold_line_it][0] <= fold_line_activity_indicators[*fold_line_it], "background fold line s0 <= activity");
      for (int stability_depth = 1; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
	model.addConstr(s_indicators[*fold_line_it][stability_depth] == 0, "background fold line sk == 0 (k > 0)");
      for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
	model.addConstr(left_c_indicators[*fold_line_it][stability_depth] == 0, "background fold line c == 0");
	model.addConstr(right_c_indicators[*fold_line_it][stability_depth] == 0, "background fold line c == 0");
	model.addConstr(left_cc_indicators[*fold_line_it][stability_depth] == 0, "background fold line cc == 0");
	model.addConstr(right_cc_indicators[*fold_line_it][stability_depth] == 0, "background fold line cc == 0");
	model.addConstr(left_d_indicators[*fold_line_it][stability_depth] == 0, "background fold line d == 0");
	model.addConstr(right_d_indicators[*fold_line_it][stability_depth] == 0, "background fold line d == 0");
	model.addConstr(left_e_indicators[*fold_line_it][stability_depth] == 0, "background fold line e == 0");
	model.addConstr(right_e_indicators[*fold_line_it][stability_depth] == 0, "background fold line e == 0");
      }
    }
    
    //add constraints for s0 and inactive fold lines
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (background_fold_lines.count(fold_line_index) == 0) {
	model.addConstr(s_indicators[fold_line_index][0] == 0, "s0 == 0");
	for (int stability_depth = 1; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
	  model.addConstr(s_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index], "s <= activity");
      }
    }
    
    // for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines() - 2; fold_line_index++)
    //   if (background_fold_lines.count(fold_line_index) == 0)
    //     model.addConstr(s_indicators[fold_line_index][0] == 1 - fold_line_activity_indicators[fold_line_index], "s <= 1 - activity");
    

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
	GRBQuadExpr left_s_sum = GRBQuadExpr(0);
	GRBQuadExpr right_s_sum = GRBQuadExpr(0);
	GRBQuadExpr left_c_sum = GRBQuadExpr(0);
	GRBQuadExpr right_c_sum = GRBQuadExpr(0);
	GRBQuadExpr left_d_sum = GRBQuadExpr(0);
	GRBQuadExpr right_d_sum = GRBQuadExpr(0);
	// left_s_sum += s_indicators[fold_line_index][stability_depth];
	// right_s_sum += s_indicators[fold_line_index][stability_depth];
	// left_c_sum += left_c_indicators[fold_line_index][stability_depth];
	// right_c_sum += right_c_indicators[fold_line_index][stability_depth];
	// left_d_sum += left_d_indicators[fold_line_index][stability_depth];
	// right_d_sum += right_d_indicators[fold_line_index][stability_depth];

	if (fold_line_left_paths.count(fold_line_index) > 0) {
	  for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths.at(fold_line_index).end(); fold_line_it++) {
	    // if (fold_line_index == 10 && fold_line_it->first == 85) {
	    //   cout << stability_depth << endl;;
	    //   exit(1);
	    // }
	    left_s_sum += s_indicators[fold_line_it->first][stability_depth] * same_patch_indicators[fold_line_index][fold_line_it->first];
	    left_c_sum += left_c_indicators[fold_line_it->first][stability_depth] * same_patch_indicators[fold_line_index][fold_line_it->first];
	    left_d_sum += left_d_indicators[fold_line_it->first][stability_depth] * same_patch_indicators[fold_line_index][fold_line_it->first];
	  }
	}
	if (fold_line_right_paths.count(fold_line_index) > 0) {
	  for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++) {
	    right_s_sum += s_indicators[fold_line_it->first][stability_depth] * same_patch_indicators[fold_line_index][fold_line_it->first];
	    right_c_sum += right_c_indicators[fold_line_it->first][stability_depth] * same_patch_indicators[fold_line_index][fold_line_it->first];
	    right_d_sum += right_d_indicators[fold_line_it->first][stability_depth] * same_patch_indicators[fold_line_index][fold_line_it->first];
	  }
	}


	model.addQConstr(left_c_indicators[fold_line_index][stability_depth] <= left_s_sum, "c <= s sum");
	model.addQConstr(left_cc_indicators[fold_line_index][stability_depth] <= left_c_sum, "cc <= c sum");
	model.addQConstr(2 * left_d_indicators[fold_line_index][stability_depth] <= left_c_sum, "2d <= c sum");
	model.addQConstr(left_e_indicators[fold_line_index][stability_depth] <= left_d_sum, "e <= d sum");
	model.addQConstr(right_c_indicators[fold_line_index][stability_depth] <= right_s_sum, "c <= s sum");
	model.addQConstr(right_cc_indicators[fold_line_index][stability_depth] <= right_c_sum, "cc <= c sum");
	model.addQConstr(2 * right_d_indicators[fold_line_index][stability_depth] <= right_c_sum, "2d <= c sum");
	model.addQConstr(right_e_indicators[fold_line_index][stability_depth] <= right_d_sum, "e <= d sum");	  
      }
    }
      
    //model.addConstr(same_patch_indicators[3][0] == 1);
    //model.addConstr(left_c_indicators[3][0] == 1);
    // model.addConstr(s_indicators[3][2] == 1);
    // model.addConstr(s_indicators[11][2] == 1);
    // model.addConstr(s_indicators[7][2] == 1);
    // model.addConstr(s_indicators[8][2] == 1);
    // model.addConstr(s_indicators[17][1] == 1);
    // model.addConstr(left_c_indicators[6][0] == 1);
    // model.addConstr(right_c_indicators[6][0] == 1);
    // model.addConstr(s_indicators[6][1] == 1);
    // model.addConstr(s_indicators[2][2] == 1);
    // model.addConstr(s_indicators[3][2] == 1);
    // model.addConstr(s_indicators[7][3] == 1);

    //model.addConstr(left_s_sum_indicators[10][0] == 1);
    
    //for (vector<pair<int, int> >::const_iterator fold_line_pair_it = popup_graph.fold_line_pairs.begin(); fold_line_pair_it != popup_graph.fold_line_pairs.end(); fold_line_pair_it++) {
    //int left_fold_line_index = fold_line_pair_it->first;
    //int right_fold_line_index = fold_line_pair_it->second;
    // for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    //   for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
    // 	model.addQConstr(left_c_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index] * left_s_sum_indicators[fold_line_index][stability_depth]);
    //     model.addQConstr(right_c_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index] * right_s_sum_indicators[fold_line_index][stability_depth]);
    // 	model.addQConstr(left_cc_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index] * left_c_sum_indicators[fold_line_index][stability_depth]);
    //     model.addQConstr(right_cc_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index] * right_c_sum_indicators[fold_line_index][stability_depth]);
    //     model.addQConstr(left_d_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index] * left_dcc_sum_indicators[fold_line_index][stability_depth]);
    //     model.addQConstr(right_d_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index] * right_dcc_sum_indicators[fold_line_index][stability_depth]);
    //   }
    // }


    vector<vector<GRBVar> > left_s_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
    vector<vector<GRBVar> > right_s_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(MAX_STABILITY_DEPTH));
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
      left_s_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
      for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
        left_s_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      GRBVar *vars_temp = model.addVars(MAX_STABILITY_DEPTH);
      right_s_indicators[fold_line_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_STABILITY_DEPTH);
      for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++)
        right_s_indicators[fold_line_index][stability_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
    }
    model.update();

    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines() - 2; fold_line_index++) {
      if (background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 1; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
	GRBQuadExpr left_s_sum(0);
	GRBQuadExpr right_s_sum(0);
	GRBLinExpr left_c(0);
	GRBLinExpr right_c(0);
	GRBLinExpr left_cc(0);
	GRBLinExpr right_cc(0);
	GRBLinExpr left_d(0);
	GRBLinExpr right_d(0);
	GRBLinExpr left_e(0);
	GRBLinExpr right_e(0);
	for (int previous_stability_depth = 0; previous_stability_depth < stability_depth; previous_stability_depth++) {
	  if (fold_line_left_paths.count(fold_line_index) > 0)
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths.at(fold_line_index).end(); fold_line_it++)
	      left_s_sum += s_indicators[fold_line_it->first][previous_stability_depth] * same_patch_indicators[fold_line_it->first][fold_line_index];
	  if (fold_line_right_paths.count(fold_line_index) > 0)
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++)
	      right_s_sum += s_indicators[fold_line_it->first][previous_stability_depth] * same_patch_indicators[fold_line_it->first][fold_line_index];

	  left_c += left_c_indicators[fold_line_index][previous_stability_depth];
	  right_c += right_c_indicators[fold_line_index][previous_stability_depth];
	  left_d += left_d_indicators[fold_line_index][previous_stability_depth];
	  right_d += right_d_indicators[fold_line_index][previous_stability_depth];
	  left_e += left_e_indicators[fold_line_index][previous_stability_depth];
	  right_e += right_e_indicators[fold_line_index][previous_stability_depth];
	}

	
	//model.addQConstr(2 * left_s_indicators[fold_line_index][stability_depth] <= left_s_sum);      
        //model.addQConstr(2 * right_s_indicators[fold_line_index][stability_depth] <= right_s_sum);
	//      stability_sum += left_s_indicators[fold_line_index][stability_depth];   
        //stability_sum += right_s_indicators[fold_line_index][stability_depth];        

	GRBQuadExpr stability_sum(0);
	GRBVar left_s = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
	GRBVar right_s = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
	model.update();
	model.addQConstr(2 * left_s <= left_s_sum);      
        model.addQConstr(2 * right_s <= right_s_sum);
	
	stability_sum += left_s;
        stability_sum += right_s;
	stability_sum += left_c * right_c;
	stability_sum += left_d * right_d;
	stability_sum += left_c * right_d;
	stability_sum += left_d * right_c;
	stability_sum += left_d * right_e;
	stability_sum += left_e * right_d;
	model.addQConstr(s_indicators[fold_line_index][stability_depth] <= stability_sum, "s");
      }
    }

    // for (int fold_line_index = 0; fold_line_index < popup_graph.num_original_fold_lines / 2; fold_line_index++) {
    //   for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
    // 	//    	model.addQConstr(s_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == s_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    // 	model.addQConstr(left_c_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == left_c_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    // 	model.addQConstr(right_c_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == right_c_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    // 	model.addQConstr(left_cc_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == left_cc_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    // 	model.addQConstr(right_cc_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == right_cc_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    // 	model.addQConstr(left_d_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == left_d_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    // 	model.addQConstr(right_d_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == right_d_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    // 	model.addQConstr(left_e_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == left_e_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    //     model.addQConstr(right_e_indicators[fold_line_index * 2][stability_depth] * fold_line_activity_indicators[fold_line_index * 2] == right_e_indicators[fold_line_index * 2 + 1][stability_depth] * fold_line_activity_indicators[fold_line_index * 2 + 1]);
    //   }
    // }

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
	model.addConstr(left_c_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index], "c <= activity");
	model.addConstr(right_c_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index], "c <= activity");
	model.addConstr(left_d_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index], "d <= activity");
	model.addConstr(right_d_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index], "d <= activity");
	model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index], "e <= activity");
	model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index], "e <= activity");
	

	model.addConstr(left_c_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth], "c <= 1 - s");
	model.addConstr(right_c_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth], "c <= 1 - s");
	model.addConstr(left_d_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth], "d <= 1 - s");
	model.addConstr(right_d_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth], "d <= 1 - s");
	model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth], "e <= 1 - s");
	model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth], "e <= 1 - s");

	model.addConstr(left_d_indicators[fold_line_index][stability_depth] <= 1 - left_c_indicators[fold_line_index][stability_depth], "d <= 1 - c");
	model.addConstr(right_d_indicators[fold_line_index][stability_depth] <= 1 - right_c_indicators[fold_line_index][stability_depth], "d <= 1 - c");
	model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= 1 - left_c_indicators[fold_line_index][stability_depth], "e <= 1 - c");
	model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= 1 - right_c_indicators[fold_line_index][stability_depth], "e <= 1 - c");

	model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= 1 - left_d_indicators[fold_line_index][stability_depth], "e <= 1 - d");
	model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= 1 - right_d_indicators[fold_line_index][stability_depth], "e <= 1 - d");
      }
    }

    GRBLinExpr stability_obj(0);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
	stability_obj += 1 - s_indicators[fold_line_index][stability_depth];
	break;
	stability_obj += 1 - left_c_indicators[fold_line_index][stability_depth];
	stability_obj += 1 - right_c_indicators[fold_line_index][stability_depth];
	stability_obj += 1 - left_cc_indicators[fold_line_index][stability_depth];
	stability_obj += 1 - right_cc_indicators[fold_line_index][stability_depth];
	stability_obj += 1 - left_d_indicators[fold_line_index][stability_depth];
	stability_obj += 1 - right_d_indicators[fold_line_index][stability_depth];
	stability_obj += 1 - left_e_indicators[fold_line_index][stability_depth];
	stability_obj += 1 - right_e_indicators[fold_line_index][stability_depth];	
      }
    }
    //model.setObjective(stability_obj, 1);
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (background_fold_lines.count(fold_line_index) > 0)
	continue;
      GRBLinExpr lhs(0);
      for (int stability_depth = 1; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
	lhs += s_indicators[fold_line_index][stability_depth];
      }
      //lhs += 1 - fold_line_activity_indicators[fold_line_index];
      model.addConstr(lhs == fold_line_activity_indicators[fold_line_index], "s == 1");
      
      // if (fold_line_index == 50 && false)
      // 	model.addConstr(lhs == 1);
      // else
      // 	model.addConstr(lhs <= 1);
    }
  }
  
  //model.setObjective(obj, 1);
  
  model.update();
  model.set(GRB_StringAttr_ModelName,"fold line assignment");
  model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
  /*model.getEnv().set(GRB_DoubleParam_MIPGap, 0.005);
    model.getEnv().set(GRB_DoubleParam_NodeLimit, 1000)*/;
  model.getEnv().set(GRB_DoubleParam_TimeLimit, 300);

  // //set objective function
  // GRBQuadExpr GRBobj = 0;
  // for (size_t j = 0; j < foldLineSize; j++){
  //   //length cost
  //   if(!obj->foldLine[j]->isOriginalType){
  //     double dist = sqrt(pow((double)obj->foldLine[j]->line.first.y-obj->foldLine[j]->line.second.y,2.0));
  //     GRBobj += (obj->initMatSize.height - dist)/(obj->initMatSize.height)*f[j];
  //   }
  // }

  // //position cost
  // for(size_t i=0; i< obj->positionLineIdxOfPatch.size(); i++){
  //   for(size_t j=0; j< obj->positionLineIdxOfPatch[i].size(); j++){
  //     int fIdx = obj->positionLineToFoldLine[obj->positionLineIdxOfPatch[i][j].first];
  //     int oriX = obj->foldLine[fIdx]->line.first.x;
  //     int pIdx = obj->positionLineIdxOfPatch[i][j].first;
  //     GRBobj += (X[pIdx]+Y[pIdx]-oriX)*(X[pIdx]+Y[pIdx]-oriX);
  //   }
  // }
  // model.setObjective(GRBobj);

  
  try {
    //model.set("OutputFlag", false);
    model.update();
    model.optimize();

    // //print & set result
    // for (size_t i=0;i<foldLineSize;i++) {
    //   cout << " f[" << i << "] = " << (double)f[i].get(GRB_DoubleAttr_X) <<endl;
    //   obj->activeFoldLine.push_back((double)f[i].get(GRB_DoubleAttr_X));
    // }
    // for (size_t i=0;i<patchSize;i++) {
    //   cout << " o[" << i << "] = " << (double) o[i].get(GRB_DoubleAttr_X) <<endl;
    //   obj->orientation.push_back((double)o[i].get(GRB_DoubleAttr_X));
    // }
    // /*for (size_t i=0;i<foldLineSize;i++)
    //   cout << " c[" << i << "] = " << c[i].get(GRB_DoubleAttr_X) <<endl;*/
        
    // obj->X.resize(positionLineSize);
    // obj->Y.resize(positionLineSize);
    // for(size_t i=0; i< obj->positionLineIdxOfPatch.size(); i++){
    //   for(size_t j=0; j< obj->positionLineIdxOfPatch[i].size(); j++){
    // 	int fIdx = obj->positionLineToFoldLine[obj->positionLineIdxOfPatch[i][j].first];
    // 	int oriX = obj->foldLine[fIdx]->line.first.x;
    // 	int pIdx = obj->positionLineIdxOfPatch[i][j].first;
    // 	cout << " X[" << pIdx << "] = " << (double)X[pIdx].get(GRB_DoubleAttr_X)<<",";
    // 	cout << " Y[" << pIdx << "] = " << (double)Y[pIdx].get(GRB_DoubleAttr_X)<<", ";
    // 	cout << "X+Y = "<<(double)X[pIdx].get(GRB_DoubleAttr_X)+ (double)Y[pIdx].get(GRB_DoubleAttr_X)<<", Original X = "<<oriX << endl;
    // 	obj->X[pIdx] = (double)X[pIdx].get(GRB_DoubleAttr_X);
    // 	obj->Y[pIdx] = (double)Y[pIdx].get(GRB_DoubleAttr_X);
    // 	obj->oriX.push_back((double)obj->foldLine[fIdx]->line.first.x);
    //   }
    // }
 
    if (model.get(GRB_IntAttr_IsMIP) == 0)
      throw GRBException("Model is not a MIP");
    
  }catch(GRBException e) {
    cout << "Error code = " << e.getErrorCode() << endl;
    cout << e.getMessage() << endl;
  }catch (...) {printf("Exception...\n");exit(1);}

  // for(int patch_index = 0; patch_index < NUM_ORIGINAL_PATCHES; patch_index++) {
  //   //if (patch_index == starting_patch_index || background_patches.count(patch_index) > 0)
  //   //continue;
  //   for (int distance = 1; distance < MAX_DISTANCE; distance++)
  //     if (patch_distances[patch_index][distance].get(GRB_DoubleAttr_X) == 1)
  // 	cout << patch_index << '\t' << distance << endl;
  // }

  //vector<int> fold_line_positions(popup_graph.getNumFoldLines());
  
  // if (true) {
  //   Mat optimized_graph_image = Mat::zeros(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
  //   //Mat original_fold_line_image(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
  //   map<int, Vec3b> color_table;
  //   map<int, vector<int> > fold_line_pixels;
  //   for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
  //     int fold_line_index = popup_graph.line_segment_fold_line_indices[popup_graph.pixel_line_segment_indices[pixel]];
  //     if (fold_line_index < 0) {
  //     }
  //     else if (fold_line_index < popup_graph.num_original_fold_lines) {
  // 	if (fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) == 0)
  // 	  fold_line_pixels[fold_line_index].push_back(pixel);
  // 	else if (fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) == 0)
  // 	  optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(255, 0, 0);
  //       else
  // 	  optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(0, 255, 0);
  // 	fold_line_pixels[fold_line_index].push_back(pixel);
  //     } else {
  // 	if (color_table.count(fold_line_index) == 0) {
  //         //int gray_value = rand() % 256;
  //         //color_table[fold_line_index] = Vec3b(gray_value, gray_value, gray_value);
  // 	  color_table[fold_line_index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
  // 	}
  // 	optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = color_table[fold_line_index];
	
  //       if (fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X) == pixel % IMAGE_WIDTH && fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) == 1) {
  // 	  if (fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) == 0)
  // 	    optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(255, 0, 0);
  // 	  else
  //           optimized_graph_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = Vec3b(0, 255, 0);
  // 	}

  // 	fold_line_pixels[fold_line_index].push_back(pixel);       

  //         // if (fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) == 0)
  //         //   color_table[fold_line_index] = Vec3b(255, 255, 255);
  //         // else
  //         //   color_table[fold_line_index] = Vec3b(0, 0, 0);
  //     }
  //   }
  //   for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_pixels.begin(); fold_line_it != fold_line_pixels.end(); fold_line_it++) {
  //     if (fold_line_activity_indicators[fold_line_it->first].get(GRB_DoubleAttr_X) == 0)
  // 	continue;
  //     int pixel = fold_line_it->second[rand() % fold_line_it->second.size()];
  //     putText(optimized_graph_image, to_string(fold_line_it->first), Point(pixel % IMAGE_WIDTH, pixel /IMAGE_WIDTH), FONT_HERSHEY_SIMPLEX, 0.3, Scalar(255, 0, 0));
  //   }

  //   imwrite("Test/optimized_graph_image.bmp", optimized_graph_image);
  // }
  
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    cout << fold_line_index << '\t' << fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << '\t' << fold_line_Xs[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_Ys[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X) << endl;

  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    if (static_cast<int>(fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5) == 0)
      continue;
    cout << fold_line_index << endl;
    for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
      if (s_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
  	cout << "s: " << stability_depth << endl;
      if (left_c_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "left c: " << stability_depth << endl;
      if (right_c_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "right c: " << stability_depth << endl;
      if (left_cc_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "left cc: " << stability_depth << endl;
      if (right_cc_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "right cc: " << stability_depth << endl;
      if (left_d_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "left d: " << stability_depth << endl;
      if (right_d_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "right d: " << stability_depth << endl;
      if (left_e_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "left e: " << stability_depth << endl;
      if (right_e_indicators[fold_line_index][stability_depth].get(GRB_DoubleAttr_X))
        cout << "right e: " << stability_depth << endl;
    }
  }
      
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    for (int other_fold_line_index = 0; other_fold_line_index < popup_graph.getNumFoldLines(); other_fold_line_index++)
      if (other_fold_line_index != fold_line_index && same_patch_indicators[fold_line_index][other_fold_line_index].get(GRB_DoubleAttr_X) == 1)
   	cout << fold_line_index << '\t' << other_fold_line_index << endl;

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
