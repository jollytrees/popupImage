#include "FoldLineOptimization.h"

#include <stdio.h>
#include <stdlib.h>
#include <gurobi_c++.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


bool optimizeFoldLines(const Popup::PopupGraph &popup_graph)
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

  //model.addConstr(fold_line_activity_indicators[1] == fold_line_activity_indicators[1]);
  // model.addConstr(fold_line_activity_indicators[1] <= 1);
  //model.update();
  
  //foldability
  if (true) {
    vector<pair<int, int> > fold_line_pairs = popup_graph.getFoldLinePairs();
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      model.addQConstr((1 - fold_line_convexity_indicators[left_fold_line_index]) * fold_line_activity_indicators[right_fold_line_index] + fold_line_convexity_indicators[left_fold_line_index] * (1 - fold_line_activity_indicators[right_fold_line_index]) == fold_line_convexity_indicators[right_fold_line_index]);
    }
    
    model.addConstr(fold_line_activity_indicators[popup_graph.getMiddleFoldLineIndex()] == 1);
    model.addConstr(fold_line_convexity_indicators[popup_graph.getMiddleFoldLineIndex()] == 0);
    // model.addConstr(fold_line_Xs[popup_graph.num_original_fold_lines] >= IMAGE_WIDTH / 2 - 100);
    // model.addConstr(fold_line_Xs[popup_graph.num_original_fold_lines] <= IMAGE_WIDTH / 2 + 100);

    const pair<int, int> BORDER_FOLD_LINE_INDICES = popup_graph.getBorderFoldLineIndices();
    model.addConstr(fold_line_activity_indicators[BORDER_FOLD_LINE_INDICES.first] == 1);
    model.addConstr(fold_line_convexity_indicators[BORDER_FOLD_LINE_INDICES.first] == 1);
    model.addConstr(fold_line_Xs[BORDER_FOLD_LINE_INDICES.first] == 0);
    model.addConstr(fold_line_Ys[BORDER_FOLD_LINE_INDICES.first] == 0);
    model.addConstr(fold_line_activity_indicators[BORDER_FOLD_LINE_INDICES.second] == 1);
    model.addConstr(fold_line_convexity_indicators[BORDER_FOLD_LINE_INDICES.second] == 1);

    //for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines - 2; fold_line_index++)
    //model.addQConstr(fold_line_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index] <= fold_line_Xs[popup_graph.num_fold_lines - 1] * fold_line_activity_indicators[popup_graph.num_fold_lines - 1]);


    vector<GRBVar> right_copy_convexity_indicators(popup_graph.getNumOriginalFoldLines());
    vector<GRBVar> right_copy_Xs(popup_graph.getNumOriginalFoldLines());
    vector<GRBVar> right_copy_Ys(popup_graph.getNumOriginalFoldLines());
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumOriginalFoldLines(); fold_line_index++) {
      right_copy_convexity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY);
      right_copy_Xs[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
      right_copy_Ys[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER);
    }
    model.update();
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumOriginalFoldLines(); fold_line_index++) {
      model.addQConstr(fold_line_convexity_indicators[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_convexity_indicators[fold_line_index] * fold_line_activity_indicators[fold_line_index]);
      model.addQConstr(fold_line_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index]);
      model.addQConstr(fold_line_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index]);
    }
    
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
      //break;
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      //cout << left_fold_line_index << '\t' << right_fold_line_index << endl;
      if (left_fold_line_index >= popup_graph.getNumOriginalFoldLines()) {
	model.addQConstr(fold_line_Xs[left_fold_line_index] * (1 - fold_line_convexity_indicators[left_fold_line_index]) == fold_line_Xs[right_fold_line_index] * (1 - fold_line_convexity_indicators[left_fold_line_index]));
	model.addQConstr((fold_line_Ys[left_fold_line_index] + 1) * (1 - fold_line_convexity_indicators[left_fold_line_index]) <= fold_line_Ys[right_fold_line_index] * (1 - fold_line_convexity_indicators[left_fold_line_index]));
	model.addQConstr(fold_line_Ys[left_fold_line_index] * fold_line_convexity_indicators[left_fold_line_index] == fold_line_Ys[right_fold_line_index] * fold_line_convexity_indicators[left_fold_line_index]);
	model.addQConstr((fold_line_Xs[left_fold_line_index] + 1) * fold_line_convexity_indicators[left_fold_line_index] <= fold_line_Xs[right_fold_line_index] * fold_line_convexity_indicators[left_fold_line_index]);
      } else {
	model.addQConstr(right_copy_Xs[left_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]) == fold_line_Xs[right_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]));
        model.addQConstr((right_copy_Ys[left_fold_line_index] + 1) * (1 - right_copy_convexity_indicators[left_fold_line_index]) <= fold_line_Ys[right_fold_line_index] * (1 - right_copy_convexity_indicators[left_fold_line_index]));
        model.addQConstr(right_copy_Ys[left_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index] == fold_line_Ys[right_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index]);
        model.addQConstr((right_copy_Xs[left_fold_line_index] + 1) * right_copy_convexity_indicators[left_fold_line_index] <= fold_line_Xs[right_fold_line_index] * right_copy_convexity_indicators[left_fold_line_index]);
      }
    }
    //exit(1);

    // map<int, pair<int, int> > fold_line_min_max_positions;
    // map<int, pair<vector<double>, vector<double> > > fold_line_positions_costs_pair;
    // map<int, int> fold_line_min_cost_positions;
    // for (map<int, map<int, double> >::const_iterator fold_line_it = popup_graph.fold_line_x_score_map.begin(); fold_line_it != popup_graph.fold_line_x_score_map.end(); fold_line_it++) {
    //   int min_position = IMAGE_WIDTH;
    //   int max_position = 0;
    //   vector<double> positions;
    //   vector<double> costs;
    //   int min_cost_position = -1;
    //   int min_cost = 1000000;
    //   for (map<int, double>::const_iterator position_it = fold_line_it->second.begin(); position_it != fold_line_it->second.end(); position_it++) {
    // 	min_position = min(position_it->first, min_position);
    // 	max_position = max(position_it->first, max_position);
    // 	positions.push_back(position_it->first);
    // 	costs.push_back(position_it->second);
    // 	if (position_it->second < min_cost) {
    // 	  min_cost_position = position_it->first;
    // 	  min_cost = position_it->second;
    // 	}
    // 	//cout << position_it->second << endl;
    //   }

    //   //      min_position = 0;
    //   //      max_position = IMAGE_WIDTH;
    //   fold_line_min_max_positions[fold_line_it->first] = make_pair(min_position, max_position);
    //   //fold_line_min_max_positions[fold_line_it->first] = make_pair(max(min_position, min_cost_position - 5), min(max_position, min_cost_position + 5));
    //   fold_line_positions_costs_pair[fold_line_it->first] = make_pair(positions, costs);

    //   fold_line_min_cost_positions[fold_line_it->first] = min_cost_position;
    //   //cout << fold_line_it->first << '\t' << min_position << '\t' << max_position << endl;
    // }
    //exit(1);
    //cout << popup_graph.num_fold_lines << endl;
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      pair<int, int> x_range = popup_graph.getFoldLineXRange(fold_line_index);
      fold_line_positions[fold_line_index] = model.addVar(x_range.first, x_range.second, 0, GRB_INTEGER);
    }
    model.update();

    //model.addConstr(fold_line_activity_indicators[10] == 1);
    // model.addConstr(fold_line_Ys[51] <= fold_line_Ys[0]);
    // model.addConstr(fold_line_positions[51] <= fold_line_positions[0] - 1);
    // model.addConstr(fold_line_positions[51] >= fold_line_positions[11] + 1);
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      model.addConstr(fold_line_Xs[fold_line_index] + fold_line_Ys[fold_line_index] == fold_line_positions[fold_line_index]);

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
  if (false)
  {
    const int MAX_DISTANCE = popup_graph.getNumOriginalPatches() - 2;
    int starting_patch_index = rand() % popup_graph.getNumOriginalPatches();
    while (starting_patch_index == popup_graph.getOriginalBackgroundPatchIndex())
      starting_patch_index = rand() % popup_graph.getNumOriginalPatches();
    
    vector<vector<GRBVar> > patch_distances(popup_graph.getNumOriginalPatches(), vector<GRBVar>(MAX_DISTANCE));
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      GRBVar *vars_temp = model.addVars(MAX_DISTANCE);
      patch_distances[patch_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_DISTANCE);
      for (int distance = 0; distance < MAX_DISTANCE; distance++) {
        patch_distances[patch_index][distance] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
      }
    }
    model.update();
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      if (patch_index == starting_patch_index)
	model.addQConstr(patch_distances[patch_index][0] == 1);
      else
	model.addQConstr(patch_distances[patch_index][0] == 0);
    }

    std::map<int, std::map<int, std::set<int> > > patch_neighbor_fold_lines = popup_graph.getPatchNeighborFoldLines();
    for (int distance = 1; distance < MAX_DISTANCE; distance++) {
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
	model.addQConstr(patch_distances[patch_index][distance] <= QRhs);
      }
      //exit(1);
    }
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      if (patch_index == popup_graph.getOriginalBackgroundPatchIndex())
	continue;
      GRBLinExpr lhs = GRBLinExpr(0);
      for (int distance = 0; distance < MAX_DISTANCE; distance++)
	lhs += patch_distances[patch_index][distance];
      model.addConstr(lhs == 1);
    }
  }

  if (true)
  {
    map<int, vector<int> > patch_left_fold_lines;
    map<int, vector<int> > patch_right_fold_lines;

    std::map<int, std::map<int, std::set<int> > > patch_neighbor_fold_lines = popup_graph.getPatchNeighborFoldLines();
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      for (map<int, set<int> >::const_iterator neighbor_patch_it = patch_neighbor_fold_lines.at(patch_index).begin(); neighbor_patch_it != patch_neighbor_fold_lines.at(patch_index).end(); neighbor_patch_it++) {
	for (set<int>::const_iterator fold_line_it = neighbor_patch_it->second.begin(); fold_line_it != neighbor_patch_it->second.end(); fold_line_it++) {
	  patch_right_fold_lines[patch_index].push_back(*fold_line_it);
	  patch_left_fold_lines[neighbor_patch_it->first].push_back(*fold_line_it);
	}
      }
    }
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
      GRBLinExpr left_fold_line_sum(0);
      for (vector<int>::const_iterator fold_line_it = patch_left_fold_lines[patch_index].begin(); fold_line_it != patch_left_fold_lines[patch_index].end(); fold_line_it++) {
	left_fold_line_sum += fold_line_activity_indicators[*fold_line_it];
	//	cout << "left: " << patch_index << '\t' << *fold_line_it << endl;
      }
      model.addConstr(left_fold_line_sum >= 2);
      
      // GRBLinExpr right_fold_line_sum(0);
      // for (vector<int>::const_iterator fold_line_it = patch_right_fold_lines[patch_index].begin(); fold_line_it != patch_right_fold_lines[patch_index].end(); fold_line_it++) {
      //   right_fold_line_sum += fold_line_activity_indicators[*fold_line_it];
      // 	cout << "right: " << patch_index << '\t' << *fold_line_it << endl;
      // }
      // model.addConstr(right_fold_line_sum >= 1);
    }
  }

  //stability
  const int MAX_STABILITY_DEPTH = 5;
  // if (true)
  // {
  vector<vector<GRBVar> > same_patch_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(popup_graph.getNumFoldLines()));
  for(int fold_line_index_1 = 0; fold_line_index_1 < popup_graph.getNumFoldLines(); fold_line_index_1++) {
    GRBVar *vars_temp = model.addVars(popup_graph.getNumFoldLines());
    same_patch_indicators[fold_line_index_1] = vector<GRBVar>(vars_temp, vars_temp + popup_graph.getNumFoldLines());
    for(int fold_line_index_2 = 0; fold_line_index_2 < popup_graph.getNumFoldLines(); fold_line_index_2++) {
        same_patch_indicators[fold_line_index_1][fold_line_index_2] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
      }
    }
    model.update();

    // GRBLinExpr same_patch_obj(0);
    // for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines; fold_line_index++)
    //   for (int other_fold_line_index = 0; other_fold_line_index < popup_graph.num_fold_lines; other_fold_line_index++)
    // 	if (other_fold_line_index != fold_line_index)
    // 	  same_patch_obj += 1 - same_patch_indicators[fold_line_index][other_fold_line_index];
    // model.setObjective(same_patch_obj, 1);


    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      model.addConstr(same_patch_indicators[fold_line_index][fold_line_index] == 1, "same_patch_indicators" + to_string(fold_line_index) + to_string(fold_line_index));
    
    std::map<int, std::map<int, std::vector<int> > > fold_line_left_paths = popup_graph.getFoldLineLeftPaths();
    std::map<int, std::map<int, std::vector<int> > > fold_line_right_paths = popup_graph.getFoldLineRightPaths();

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      for (int other_fold_line_index = 0; other_fold_line_index < popup_graph.getNumOriginalPatches(); other_fold_line_index++) {
	if (other_fold_line_index == fold_line_index)
          continue;
	model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= fold_line_activity_indicators[fold_line_index]);
	model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= fold_line_activity_indicators[other_fold_line_index]);
	if ((fold_line_left_paths.count(fold_line_index) == 0 || fold_line_left_paths.at(fold_line_index).count(other_fold_line_index) == 0) && (fold_line_right_paths.count(fold_line_index) == 0 || fold_line_right_paths.at(fold_line_index).count(other_fold_line_index) == 0)) {
	  model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] == 0);
	  continue;
	}
	if ((fold_line_left_paths.count(fold_line_index) > 0 && fold_line_left_paths.at(fold_line_index).count(other_fold_line_index) > 0) && (fold_line_right_paths.count(fold_line_index) > 0 && fold_line_right_paths.at(fold_line_index).count(other_fold_line_index) > 0)) {
	  cout << fold_line_index << '\t' << other_fold_line_index << endl;
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
	      model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= 1 - fold_line_activity_indicators[*path_it]);
        }
        if (fold_line_right_paths.count(fold_line_index) > 0 && fold_line_right_paths.at(fold_line_index).count(other_fold_line_index) > 0) {
	  vector<int> path = fold_line_right_paths.at(fold_line_index).at(other_fold_line_index);
	  //cout << "right: " << fold_line_index << '\t' << other_fold_line_index << '\t' << path.size() << endl;
          // if (path.size() == 0)
          //   model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] == 1);
          // else
	    for (vector<int>::const_iterator path_it = path.begin(); path_it != path.end(); path_it++)
	      model.addConstr(same_patch_indicators[fold_line_index][other_fold_line_index] <= 1 - fold_line_activity_indicators[*path_it]);
	}
      }
    }
    //exit(1);
    //model.addConstr(same_patch_indicators[50][85] == 1);
    //model.addConstr(same_patch_indicators[85][10] == 1);
    
    //if (true) {
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
    

    set<int> background_fold_lines;
    background_fold_lines.insert(popup_graph.getBorderFoldLineIndices().first);
    background_fold_lines.insert(popup_graph.getBorderFoldLineIndices().second);
    background_fold_lines.insert(popup_graph.getMiddleFoldLineIndex());
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(popup_graph.getNumFoldLines() - 2).begin(); fold_line_it != fold_line_right_paths.at(popup_graph.getNumFoldLines() - 2).end(); fold_line_it++)
      background_fold_lines.insert(fold_line_it->first);
    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(popup_graph.getNumFoldLines() - 1).begin(); fold_line_it != fold_line_left_paths.at(popup_graph.getNumFoldLines() - 1).end(); fold_line_it++)
      background_fold_lines.insert(fold_line_it->first);
    
    for (set<int>::const_iterator fold_line_it = background_fold_lines.begin(); fold_line_it != background_fold_lines.end(); fold_line_it++)
      model.addConstr(s_indicators[*fold_line_it][0] == 1);
    
    // model.addConstr(s_indicators[popup_graph.getNumFoldLines() - 2][0] == 1);
    // model.addConstr(s_indicators[popup_graph.getNumFoldLines() - 1][0] == 1);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines() - 2; fold_line_index++)
      if (background_fold_lines.count(fold_line_index) == 0)
        model.addConstr(s_indicators[fold_line_index][0] == 1 - fold_line_activity_indicators[fold_line_index]);

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

	if (fold_line_index >= popup_graph.getNumOriginalFoldLines() || fold_line_index % 2 == 0) {
	  model.addQConstr(left_c_indicators[fold_line_index][stability_depth] <= left_s_sum);
	  model.addQConstr(left_cc_indicators[fold_line_index][stability_depth] <= left_c_sum);
          model.addQConstr(2 * left_d_indicators[fold_line_index][stability_depth] <= left_c_sum);
          model.addQConstr(left_e_indicators[fold_line_index][stability_depth] <= left_d_sum);
	}
	if (fold_line_index >= popup_graph.getNumOriginalFoldLines() || fold_line_index % 2 == 1) {
	  model.addQConstr(right_c_indicators[fold_line_index][stability_depth] <= right_s_sum);
	  model.addQConstr(right_cc_indicators[fold_line_index][stability_depth] <= right_c_sum);
          model.addQConstr(2 * right_d_indicators[fold_line_index][stability_depth] <= right_c_sum);
          model.addQConstr(right_e_indicators[fold_line_index][stability_depth] <= right_d_sum);
        }
      }
    }

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
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines() - 2; fold_line_index++) {
      if (background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 1; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
	GRBLinExpr left_c(0);
	GRBLinExpr right_c(0);
	GRBLinExpr left_cc(0);
        GRBLinExpr right_cc(0);
	GRBLinExpr left_d(0);
        GRBLinExpr right_d(0);
        GRBLinExpr left_e(0);
        GRBLinExpr right_e(0);
	for (int previous_stability_depth = 0; previous_stability_depth < stability_depth; previous_stability_depth++) {
	  left_c += left_c_indicators[fold_line_index][previous_stability_depth];
	  right_c += right_c_indicators[fold_line_index][previous_stability_depth];
          left_d += left_d_indicators[fold_line_index][previous_stability_depth];
          right_d += right_d_indicators[fold_line_index][previous_stability_depth];
	  left_e += left_e_indicators[fold_line_index][previous_stability_depth];
          right_e += right_e_indicators[fold_line_index][previous_stability_depth];
	}
	GRBQuadExpr stability_sum(0);
	stability_sum += left_c * right_c;
        stability_sum += left_d * right_d;
	stability_sum += left_c * right_d;
	stability_sum += left_d * right_c;
	stability_sum += left_d * right_e;
	stability_sum += left_e * right_d;
 	model.addQConstr(s_indicators[fold_line_index][stability_depth] <= stability_sum);
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
	model.addConstr(left_c_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index]);
	model.addConstr(right_c_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index]);
	model.addConstr(left_d_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index]);
	model.addConstr(right_d_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index]);
	model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index]);
	model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= fold_line_activity_indicators[fold_line_index]);
	
	
	model.addConstr(left_c_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth]);
	model.addConstr(right_c_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth]);
	model.addConstr(left_d_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth]);
        model.addConstr(right_d_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth]);
	model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth]);
        model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= 1 - s_indicators[fold_line_index][stability_depth]);

	model.addConstr(left_d_indicators[fold_line_index][stability_depth] <= 1 - left_c_indicators[fold_line_index][stability_depth]);
        model.addConstr(right_d_indicators[fold_line_index][stability_depth] <= 1 - right_c_indicators[fold_line_index][stability_depth]);
	model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= 1 - left_c_indicators[fold_line_index][stability_depth]);
        model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= 1 - right_c_indicators[fold_line_index][stability_depth]);

        model.addConstr(left_e_indicators[fold_line_index][stability_depth] <= 1 - left_d_indicators[fold_line_index][stability_depth]);
        model.addConstr(right_e_indicators[fold_line_index][stability_depth] <= 1 - right_d_indicators[fold_line_index][stability_depth]);
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
      for (int stability_depth = 0; stability_depth < MAX_STABILITY_DEPTH; stability_depth++) {
    	lhs += s_indicators[fold_line_index][stability_depth];
      }
      //lhs += 1 - fold_line_activity_indicators[fold_line_index];
      model.addQConstr(lhs <= 1);
      // if (fold_line_index == 50 && false)
      // 	model.addConstr(lhs == 1);
      // else
      // 	model.addConstr(lhs <= 1);
    }
    //}
  
  
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
  
  return true;
}
