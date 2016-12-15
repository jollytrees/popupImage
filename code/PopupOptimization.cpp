#include "PopupOptimization.h"

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

bool optimizeFoldLines(Popup::PopupGraph &popup_graph, const vector<vector<int> > &excluded_fold_line_combinations, const int num_new_fold_lines_constraint, const char optimization_type)
{
  if (optimization_type == 'C')
    cout << "Optimize complete graph" << endl;
  else if (optimization_type == 'S')
    cout << "Check stability" << endl;
  else
    cout << "Optimize topology" << endl;
  
  const int IMAGE_WIDTH = popup_graph.getImageWidth();
  const int IMAGE_HEIGHT = popup_graph.getImageHeight();
  
  GRBEnv env = GRBEnv();
  GRBModel model = GRBModel(env);

  vector<GRBVar> fold_line_activity_indicators(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    fold_line_activity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY, "activity " + to_string(fold_line_index));
  
  vector<GRBVar> fold_line_convexity_indicators(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    fold_line_convexity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY, "convexity " + to_string(fold_line_index));
  model.update();
  

  vector<GRBVar> fold_line_positions(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    if (fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second) {
      fold_line_positions[fold_line_index] = model.addVar(0, IMAGE_WIDTH - 1, 0, GRB_INTEGER, "position " + to_string(fold_line_index));
      continue;
    }
    pair<int, int> x_range = popup_graph.getFoldLineXRange(fold_line_index);
    //cout << x_range.first << '\t' << x_range.second << endl;
    fold_line_positions[fold_line_index] = model.addVar(x_range.first, x_range.second, 0, GRB_INTEGER, "position " + to_string(fold_line_index));
  }
  //    exit(1);
  model.update();

  vector<GRBVar> fold_line_Xs(popup_graph.getNumFoldLines());
  vector<GRBVar> fold_line_Ys(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    fold_line_Xs[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER, "X " + to_string(fold_line_index));
    fold_line_Ys[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER, "Y " + to_string(fold_line_index));
  }
  model.update();

  vector<GRBVar> right_copy_convexity_indicators(popup_graph.getNumFoldLines());
  vector<GRBVar> right_copy_Xs(popup_graph.getNumFoldLines());
  vector<GRBVar> right_copy_Ys(popup_graph.getNumFoldLines());
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    right_copy_convexity_indicators[fold_line_index] = model.addVar(0.0, 1.0, 0, GRB_BINARY, "right copy convexity " + to_string(fold_line_index));
    right_copy_Xs[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER, "right copy X " + to_string(fold_line_index));
    right_copy_Ys[fold_line_index] = model.addVar(0.0, IMAGE_WIDTH, 0, GRB_INTEGER, "right copy Y " + to_string(fold_line_index));
  }
  model.update();

  //GRBQuadExpr obj(0);

  vector<int> denoted_fold_line_activities;
  vector<int> denoted_fold_line_convexities;
  vector<int> denoted_fold_line_positions;
  if (true) {
    popup_graph.getOptimizedFoldLineInfo(denoted_fold_line_activities, denoted_fold_line_convexities, denoted_fold_line_positions);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (denoted_fold_line_activities[fold_line_index] == 0)
	model.addConstr(fold_line_activity_indicators[fold_line_index] == 0);
      else if (denoted_fold_line_activities[fold_line_index] == 1)
	model.addConstr(fold_line_activity_indicators[fold_line_index] == 1);
      
      if (denoted_fold_line_convexities[fold_line_index] == 0)
        model.addConstr(fold_line_convexity_indicators[fold_line_index] == 0);
      else if (denoted_fold_line_convexities[fold_line_index] == 1)
        model.addConstr(fold_line_convexity_indicators[fold_line_index] == 1);
      
      if (denoted_fold_line_positions[fold_line_index] >= 0) {
	model.addConstr(fold_line_positions[fold_line_index] <= denoted_fold_line_positions[fold_line_index] + 1);	
        model.addConstr(fold_line_positions[fold_line_index] >= denoted_fold_line_positions[fold_line_index] - 1);
      }      
      // if (optimization_type == 'T') {
      // 	cout << "there should not be an optimized fold line " << fold_line_index << endl;
      // 	exit(1);
      // }
    }
  }
  
  if (optimization_type == 'C' && false) {
    set<int> active_fold_lines;
    //active_fold_lines.insert(42);
    set<int> inactive_fold_lines;
    inactive_fold_lines.insert(15);
    inactive_fold_lines.insert(29);
    
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
    //model.addConstr(fold_line_positions[35] <= 320);
    //model.addConstr(fold_line_positions[13] <= fold_line_positions[69]);
    //model.addConstr(fold_line_positions[24] <= fold_line_positions[58]);
    //model.addConstr(fold_line_positions[51] <= fold_line_positions[14]);

    //model.addConstr(fold_line_Ys[69] == fold_line_Ys[72]);
    //model.addConstr(fold_line_Xs[69] <= fold_line_Xs[72] - 5);
  }   
  
  //model.addConstr(fold_line_activity_indicators[1] == fold_line_activity_indicators[1]);
  // model.addConstr(fold_line_activity_indicators[1] <= 1);
  //model.update();

  vector<pair<int, int> > fold_line_pairs = popup_graph.getFoldLinePairs();
  GRBQuadExpr position_obj(0);    
  
  //foldability
  if (optimization_type == 'T' || optimization_type == 'C') {
    
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

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      model.addQConstr(fold_line_convexity_indicators[fold_line_index] * fold_line_activity_indicators[fold_line_index] == (1 - right_copy_convexity_indicators[fold_line_index]) * fold_line_activity_indicators[fold_line_index], "right copy");      
      model.addConstr(fold_line_Xs[fold_line_index] + fold_line_Ys[fold_line_index] == right_copy_Xs[fold_line_index] + right_copy_Ys[fold_line_index], "right copy");

      if (fold_line_index < popup_graph.getNumOriginalFoldLines()) {
	model.addQConstr(fold_line_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Xs[fold_line_index] * fold_line_activity_indicators[fold_line_index], "right copy");
        model.addQConstr(fold_line_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index] == right_copy_Ys[fold_line_index] * fold_line_activity_indicators[fold_line_index], "right copy");
      } else {
	model.addQConstr(fold_line_convexity_indicators[fold_line_index] * (1 - fold_line_activity_indicators[fold_line_index]) == right_copy_convexity_indicators[fold_line_index] * (1 - fold_line_activity_indicators[fold_line_index]), "right copy");
	
        model.addConstr(fold_line_Xs[fold_line_index] == right_copy_Xs[fold_line_index], "right copy");
        model.addConstr(fold_line_Ys[fold_line_index] == right_copy_Ys[fold_line_index], "right copy");
      }
    }
    
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      if (denoted_fold_line_convexities[left_fold_line_index] != -1 && denoted_fold_line_convexities[right_fold_line_index] != -1)      
        continue;
      model.addConstr(right_copy_convexity_indicators[left_fold_line_index] == fold_line_convexity_indicators[right_fold_line_index], "convexity");
      // if (left_fold_line_index >= popup_graph.getNumOriginalFoldLines())
      // 	model.addQConstr((1 - fold_line_convexity_indicators[left_fold_line_index]) * fold_line_activity_indicators[right_fold_line_index] + fold_line_convexity_indicators[left_fold_line_index] * (1 - fold_line_activity_indicators[right_fold_line_index]) == fold_line_convexity_indicators[right_fold_line_index], "convexity");
      // else
      // 	model.addQConstr((1 - right_copy_convexity_indicators[left_fold_line_index]) * fold_line_activity_indicators[right_fold_line_index] + right_copy_convexity_indicators[left_fold_line_index] * (1 - fold_line_activity_indicators[right_fold_line_index]) == fold_line_convexity_indicators[right_fold_line_index], "convexity");
    }

    //const int MIN_FOLD_LINE_GAP = optimization_type == 'C' ? 1 : popup_graph.getMinFoldLineGap();
    const int MIN_FOLD_LINE_GAP = popup_graph.getMinFoldLineGap();
    
    for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
      //break;
      int left_fold_line_index = fold_line_pair_it->first;
      int right_fold_line_index = fold_line_pair_it->second;
      if (denoted_fold_line_positions[left_fold_line_index] != -1 && denoted_fold_line_positions[right_fold_line_index] != -1)
        continue;
      //cout << "fold line neighbors: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
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
      
      if (optimization_type == 'C' && false) {
	set<int> active_fold_lines;
	
          // island fold line: 16
	  //   island fold line: 27
	  
          // island fold line: 30
          // island fold line: 57
          // island fold line: 59
          // island fold line: 65
	  // island fold line: 70
	//active_fold_lines.insert(16);
	active_fold_lines.insert(27);
	active_fold_lines.insert(57);
	active_fold_lines.insert(55);
	// active_fold_lines.insert(11);
	// active_fold_lines.insert(15);
	
	// active_fold_lines.insert(29);
	// active_fold_lines.insert(20);
	// active_fold_lines.insert(14);
	// active_fold_lines.insert(26);
	// active_fold_lines.insert(25);
	// active_fold_lines.insert(17);
	// active_fold_lines.insert(4);
	// active_fold_lines.insert(13);
	// active_fold_lines.insert(24);
	// active_fold_lines.insert(22);
	// active_fold_lines.insert(23);
	//for (int i = 0; i < popup_graph.getNumFoldLines(); i++) 
	//if (denoted_fold_line_activities[i] == 0)
	//active_fold_lines.insert(i);
        if (active_fold_lines.count(left_fold_line_index) > 0 || active_fold_lines.count(right_fold_line_index) > 0) {
	  cout << "fold line neighbors: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
	  continue;
	}
      }

      
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
	if (denoted_fold_line_positions[left_fold_line_index] != -1 && denoted_fold_line_positions[right_fold_line_index] != -1)
          continue;
        //cout << "fold line neighbors passed: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
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

    //model.addConstr(fold_line_activity_indicators[10] == 1);
    // model.addConstr(fold_line_Ys[51] <= fold_line_Ys[0]);
    // model.addConstr(fold_line_positions[51] <= fold_line_positions[0] - 1);
    // model.addConstr(fold_line_positions[51] >= fold_line_positions[11] + 1);
    
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second)      
        continue;
      model.addConstr(fold_line_Xs[fold_line_index] + fold_line_Ys[fold_line_index] == fold_line_positions[fold_line_index], "X + Y == position");
    }

    vector<int> background_left_fold_lines = popup_graph.getBackgroundLeftFoldLines();      
    vector<int> background_right_fold_lines = popup_graph.getBackgroundRightFoldLines();
    //add middle fold line position constraints
    {
      for (vector<int>::const_iterator fold_line_it = background_left_fold_lines.begin(); fold_line_it != background_left_fold_lines.end(); fold_line_it++) {
	//cout << "left background fold line: " << *fold_line_it << endl;
	model.addConstr(fold_line_positions[*fold_line_it] <= fold_line_positions[popup_graph.getMiddleFoldLineIndex()] - MIN_FOLD_LINE_GAP, "x < middle x");
      }
      for (vector<int>::const_iterator fold_line_it = background_right_fold_lines.begin(); fold_line_it != background_right_fold_lines.end(); fold_line_it++) {
	//cout << "right background fold line: " << *fold_line_it << endl;
	model.addConstr(fold_line_positions[*fold_line_it] >= fold_line_positions[popup_graph.getMiddleFoldLineIndex()] + MIN_FOLD_LINE_GAP, "x > middle x");
      }
    }

    const int BACKGROUND_FOLD_LINE_POSITION_WEIGHT = 100;
    vector<int> desirable_fold_line_positions;
    popup_graph.getDesirableFoldLinePositions(desirable_fold_line_positions);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumOriginalFoldLines(); fold_line_index++) {
      int position_weight = 1;
      if (find(background_left_fold_lines.begin(), background_left_fold_lines.end(), fold_line_index) < background_left_fold_lines.end() || find(background_right_fold_lines.begin(), background_right_fold_lines.end(), fold_line_index) < background_right_fold_lines.end())
	position_weight = 100;
      
      position_obj += (fold_line_positions[fold_line_index] - desirable_fold_line_positions[fold_line_index]) * (fold_line_positions[fold_line_index] - desirable_fold_line_positions[fold_line_index]) * position_weight;
    }
    //obj += position_obj;
    //model.setObjective(position_obj, 1);

    // GRBQuadExpr position_obj(0);
    // for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines() - 2; fold_line_index++)
    //   position_obj += (fold_line_positions[fold_line_index] - fold_line_min_cost_positions[fold_line_index]) * (fold_line_positions[fold_line_index] - fold_line_min_cost_positions[fold_line_index]);
    // model.setObjective(position_obj, 1);
    //for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines - 2; fold_line_index++)
    //model.setPWLObj(fold_line_positions[fold_line_index], fold_line_positions_costs_pair[fold_line_index].first.size(), &fold_line_positions_costs_pair[fold_line_index].first[0], &fold_line_positions_costs_pair[fold_line_index].second[0]);


    //enforce symmetry
    if (popup_graph.getEnforceSymmetryFlag()) {
      set<int> symmetry_fold_lines = popup_graph.getSymmetryFoldLines();
      for (set<int>::const_iterator fold_line_it = symmetry_fold_lines.begin(); fold_line_it != symmetry_fold_lines.end(); fold_line_it++)
        model.addConstr(fold_line_activity_indicators[*fold_line_it] == 1, "symmetry");
      //model.addConstr(fold_line_activity_indicators[31] == 1, "symmetry");

      vector<pair<int, int> > symmetric_fold_line_pairs = popup_graph.getSymmetricFoldLinePairs();
      if (true) {
        for (vector<pair<int, int> >::const_iterator symmetric_fold_line_pair_it = symmetric_fold_line_pairs.begin(); symmetric_fold_line_pair_it != symmetric_fold_line_pairs.end(); symmetric_fold_line_pair_it++) {
          //cout << "symmetric fold line pair: " << symmetric_fold_line_pair_it->first << ' ' << symmetric_fold_line_pair_it->second << endl;
          model.addConstr(fold_line_activity_indicators[symmetric_fold_line_pair_it->first] == fold_line_activity_indicators[symmetric_fold_line_pair_it->second]);
	  model.addConstr(fold_line_convexity_indicators[symmetric_fold_line_pair_it->first] == 1 - right_copy_convexity_indicators[symmetric_fold_line_pair_it->second]);
	  model.addConstr(right_copy_convexity_indicators[symmetric_fold_line_pair_it->first] == 1 - fold_line_convexity_indicators[symmetric_fold_line_pair_it->second]);
        }
      }
    
      if (true) {
        const int SYMMETRY_POSITION_TOLERANCE = optimization_type == 'C' ? 2 : 2;
        for (set<int>::const_iterator fold_line_it = symmetry_fold_lines.begin(); fold_line_it != symmetry_fold_lines.end(); fold_line_it++) {
	  cout << "symmetry fold line: " << *fold_line_it << endl;
          model.addConstr(fold_line_positions[*fold_line_it] == popup_graph.getMiddleFoldLineX(), "symmetry");
        }
        model.addConstr(fold_line_positions[popup_graph.getMiddleFoldLineIndex()] == popup_graph.getMiddleFoldLineX(), "symmetry");
      
        for (vector<pair<int, int> >::const_iterator symmetric_fold_line_pair_it = symmetric_fold_line_pairs.begin(); symmetric_fold_line_pair_it != symmetric_fold_line_pairs.end(); symmetric_fold_line_pair_it++) {
          //cout << "symmetric fold line pair: " << symmetric_fold_line_pair_it->first << ' ' << symmetric_fold_line_pair_it->second << endl;
          //model.addConstr(fold_line_activity_indicators[symmetric_fold_line_pair_it->first] == fold_line_activity_indicators[symmetric_fold_line_pair_it->second]);
          //model.addConstr(fold_line_convexity_indicators[symmetric_fold_line_pair_it->first] == fold_line_convexity_indicators[symmetric_fold_line_pair_it->second]);
          model.addConstr(fold_line_positions[symmetric_fold_line_pair_it->first] + fold_line_positions[symmetric_fold_line_pair_it->second] >= popup_graph.getMiddleFoldLineX() * 2 - SYMMETRY_POSITION_TOLERANCE);
          model.addConstr(fold_line_positions[symmetric_fold_line_pair_it->first] + fold_line_positions[symmetric_fold_line_pair_it->second] <= popup_graph.getMiddleFoldLineX() * 2 + SYMMETRY_POSITION_TOLERANCE);
        }
      }
    }
  }

  // model.addConstr(fold_line_activity_indicators[8] == 1);
  // model.addConstr(fold_line_activity_indicators[10] == 1);
  // model.addConstr(fold_line_activity_indicators[36] == 1);
  // model.addConstr(fold_line_activity_indicators[48] == 1);
  // model.addConstr(fold_line_activity_indicators[13] == 1);
  // model.addConstr(fold_line_activity_indicators[33] == 1);
  // model.addConstr(fold_line_activity_indicators[35] == 1);
  //model.addConstr(fold_line_Xs[10] <= 100);

  //cout << popup_graph.getNumOriginalPatches() << endl;
  //exit(1);
  
  GRBLinExpr island_obj(0);
  //connectivity
  if (true && optimization_type == 'T' && popup_graph.getNumOriginalPatches() > 2) {
    vector<GRBVar> island_patch_indicators(popup_graph.getNumOriginalPatches());
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++)
      island_patch_indicators[patch_index] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "island patch " + to_string(patch_index));
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
      if (island_patch_fold_lines_map.count(patch_index) > 0) {
	GRBLinExpr fold_line_sum(0);
	for (set<int>::const_iterator fold_line_it = island_patch_fold_lines_map[patch_index].begin(); fold_line_it != island_patch_fold_lines_map[patch_index].end(); fold_line_it++)
	  fold_line_sum += fold_line_activity_indicators[*fold_line_it];
	model.addConstr(island_patch_indicators[patch_index] >= 1 - fold_line_sum);
      }// else
      //model.addConstr(island_patch_indicators[patch_index] == 0);
    }
    for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++)
      island_obj += island_patch_indicators[patch_index];

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
	    //cout << "left: " << patch_index << '\t' << *fold_line_it << endl;
	  }

	  // if (island_patch_fold_lines_map.count(patch_index) == 0)
	  //   model.addConstr(left_fold_line_sum >= 1, "num left fold lines >= 1");
	  // else
	  //model.addConstr(left_fold_line_sum >= 1 - island_patch_indicators[patch_index], "num left fold lines >= 1");
	  
	  model.addConstr(left_fold_line_sum >= 1 - island_patch_indicators[patch_index], "num left fold lines >= 1");
          for (set<int>::const_iterator fold_line_it = left_fold_lines.begin(); fold_line_it != left_fold_lines.end(); fold_line_it++)
	    model.addConstr(fold_line_activity_indicators[*fold_line_it] <= 1 - island_patch_indicators[patch_index], "island patch has no fold line");
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
            //cout << "right: " << patch_index << '\t' << *fold_line_it << endl;
          }

          // if (island_patch_fold_lines_map.count(patch_index) == 0)
          //   model.addConstr(right_fold_line_sum >= 1, "num right fold lines >= 1");
          // else
	  //model.addConstr(right_fold_line_sum >= 1 - island_patch_indicators[patch_index], "num right fold lines >= 1");

	  model.addConstr(right_fold_line_sum >= 1 - island_patch_indicators[patch_index], "num right fold lines >= 1");
	  for (set<int>::const_iterator fold_line_it = right_fold_lines.begin(); fold_line_it != right_fold_lines.end(); fold_line_it++)
            model.addConstr(fold_line_activity_indicators[*fold_line_it] <= 1 - island_patch_indicators[patch_index], "island patch has no fold line");
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
      //GRBVar *vars_temp = model.addVars(MAX_DISTANCE);
      //patch_distances[patch_index] = vector<GRBVar>(vars_temp, vars_temp + MAX_DISTANCE);
      for (int distance = 0; distance <= MAX_DISTANCE; distance++) {
        patch_distances[patch_index][distance] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "patch distance " + to_string(patch_index) + to_string(distance));
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
      // if (island_patch_fold_lines_map.count(patch_index) == 0)
      // 	model.addConstr(lhs == 1, "patch distance == 1");
      // else
      // 	model.addConstr(lhs == 1 - island_patch_indicators[patch_index], "patch distance == 1");

      model.addConstr(lhs == 1 - island_patch_indicators[patch_index], "patch distance == 1");
    }
  }

  
  vector<vector<GRBVar> > same_patch_indicators(popup_graph.getNumFoldLines(), vector<GRBVar>(popup_graph.getNumFoldLines()));
  for(int fold_line_index_1 = 0; fold_line_index_1 < popup_graph.getNumFoldLines(); fold_line_index_1++) {
    //GRBVar *vars_temp = model.addVars(popup_graph.getNumFoldLines());
    //same_patch_indicators[fold_line_index_1] = vector<GRBVar>(vars_temp, vars_temp + popup_graph.getNumFoldLines());
    for(int fold_line_index_2 = 0; fold_line_index_2 < popup_graph.getNumFoldLines(); fold_line_index_2++) {
      same_patch_indicators[fold_line_index_1][fold_line_index_2] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "same patch " + to_string(fold_line_index_1) + to_string(fold_line_index_2));
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

  const int MAX_STABILITY_DEPTH = 7;
  const int MAX_CONNECTION_DEPTH = 4;
  vector<vector<vector<GRBVar> > > left_left_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1, vector<GRBVar>(MAX_CONNECTION_DEPTH + 1)));
  vector<vector<vector<bool> > > left_left_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      //GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      //left_left_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
	left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "left left " + to_string(fold_line_index) + to_string(stability_depth) + to_string(connection_depth));
    }
  }
  vector<vector<vector<GRBVar> > > left_right_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1, vector<GRBVar>(MAX_CONNECTION_DEPTH + 1)));
  vector<vector<vector<bool> > > left_right_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      //GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      //left_right_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
        left_right_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "left right " + to_string(fold_line_index) + to_string(stability_depth) + to_string(connection_depth));
    }
  }
  vector<vector<vector<GRBVar> > > right_left_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1, vector<GRBVar>(MAX_CONNECTION_DEPTH + 1)));
  vector<vector<vector<bool> > > right_left_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      //GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      //right_left_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
        right_left_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "right left " + to_string(fold_line_index) + to_string(stability_depth) + to_string(connection_depth));
    }
  }
  vector<vector<vector<GRBVar> > > right_right_stability_indicators(popup_graph.getNumFoldLines(), vector<vector<GRBVar> >(MAX_STABILITY_DEPTH + 1, vector<GRBVar>(MAX_CONNECTION_DEPTH + 1)));
  vector<vector<vector<bool> > > right_right_stability_mask(popup_graph.getNumFoldLines(), vector<vector<bool> >(MAX_STABILITY_DEPTH + 1, vector<bool>(MAX_CONNECTION_DEPTH + 1, false)));
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
      //GRBVar *vars_temp = model.addVars(MAX_CONNECTION_DEPTH + 1);
      //right_right_stability_indicators[fold_line_index][stability_depth] = vector<GRBVar>(vars_temp, vars_temp + (MAX_CONNECTION_DEPTH + 1));
      for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++)
        right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "right right " + to_string(fold_line_index) + to_string(stability_depth) + to_string(connection_depth));
    }
  }
  model.update();

  //stability
  if (optimization_type == 'S') {
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
      // if (optimization_type == 'C' && optimized_flags[fold_line_index]) {
      // 	model.addConstr(left_left_stability_indicators[fold_line_index][0][0] == 1, "optimized fold line is stable");
      // 	model.addConstr(right_right_stability_indicators[fold_line_index][0][0] == 1, "optimized fold line is stable");
      // 	continue;
      // }
      
      model.addConstr(left_left_stability_indicators[fold_line_index][0][0] == 0, "non-background fold line is unstable");
    
      //checkAndSet(right_right_stability_mask, fold_line_index, 0, 0, "non-background fold line is unstable");
      model.addConstr(right_right_stability_indicators[fold_line_index][0][0] == 0, "non-background fold line is unstable");
    }

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (denoted_fold_line_activities[fold_line_index] != -1)      
        continue;

      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	  if (fold_line_index < popup_graph.getNumOriginalFoldLines()) {
	    model.addConstr(left_right_stability_indicators[fold_line_index][stability_depth][connection_depth] <= fold_line_activity_indicators[fold_line_index], "stability stops at inactive original fold lines");
	    model.addConstr(right_left_stability_indicators[fold_line_index][stability_depth][connection_depth] <= fold_line_activity_indicators[fold_line_index], "stability stops at inactive original fold lines");
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
      //if (optimized_flags[fold_line_index])      
      //continue;

      if (fold_line_index == popup_graph.getMiddleFoldLineIndex() || fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second)
        continue;
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	for (int connection_depth = 0; connection_depth < MAX_CONNECTION_DEPTH; connection_depth++) {
	  if (connection_depth == 2)
	    continue;

	  checkAndSet(left_right_stability_mask, fold_line_index, stability_depth, connection_depth + 1, "connection depth increases through active fold lines");
	  model.addQConstr(left_right_stability_indicators[fold_line_index][stability_depth][connection_depth + 1] * fold_line_activity_indicators[fold_line_index] == left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] * fold_line_activity_indicators[fold_line_index], "connection depth increases through active fold lines");
        
	  checkAndSet(right_left_stability_mask, fold_line_index, stability_depth, connection_depth + 1, "connection depth increases through active fold lines");
	  model.addQConstr(right_left_stability_indicators[fold_line_index][stability_depth][connection_depth + 1] * fold_line_activity_indicators[fold_line_index] == right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] * fold_line_activity_indicators[fold_line_index], "connection depth increases through active fold lines");
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
      //if (optimized_flags[fold_line_index])      
      //continue;

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
	    if (fold_line_left_neighbors[fold_line_index].size() == 1)
              model.addConstr(left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] == left_neighbor_sum, "stability properties are the same for fold line pairs");
	    else
              model.addConstr(left_left_stability_indicators[fold_line_index][stability_depth][connection_depth] <= left_neighbor_sum, "stability properties are the same for fold line pairs");
          }
        
	  if (fold_line_right_neighbors.count(fold_line_index) > 0) {
	    GRBLinExpr right_neighbor_sum(0);
	    for (set<int>::const_iterator neighbor_it = fold_line_right_neighbors[fold_line_index].begin(); neighbor_it != fold_line_right_neighbors[fold_line_index].end(); neighbor_it++)
	      right_neighbor_sum += right_left_stability_indicators[*neighbor_it][stability_depth][connection_depth];

	    checkAndSet(right_right_stability_mask, fold_line_index, stability_depth, connection_depth, "stability properties are the same for fold line pairs");
	    if (fold_line_right_neighbors[fold_line_index].size() == 1)
	      model.addConstr(right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] == right_neighbor_sum, "stability properties are the same for fold line pairs");
	    else
              model.addConstr(right_right_stability_indicators[fold_line_index][stability_depth][connection_depth] <= right_neighbor_sum, "stability properties are the same for fold line pairs");
          }
        }
      }
    }

    //double-connected fold line constraints
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      //if (optimized_flags[fold_line_index])      
      //continue;

      if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
	continue;
      for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	GRBQuadExpr left_c_sum(0);
	GRBQuadExpr right_c_sum(0);
	for (int previous_stability_depth = 0; previous_stability_depth <= stability_depth; previous_stability_depth++) {
	  if (fold_line_left_paths.count(fold_line_index) > 0) {
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths.at(fold_line_index).end(); fold_line_it++) {
	      if (fold_line_right_paths.count(fold_line_it->first) > 0 && fold_line_right_paths.at(fold_line_it->first).count(fold_line_index) > 0)
                left_c_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      //if (fold_line_left_paths.count(fold_line_it->first) > 0 && fold_line_left_paths.at(fold_line_it->first).count(fold_line_index) > 0)
	      //right_c_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	    }
	  }
	  if (fold_line_right_paths.count(fold_line_index) > 0) {
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++) {
	      //if (fold_line_right_paths.count(fold_line_it->first) > 0 && fold_line_right_paths.at(fold_line_it->first).count(fold_line_index) > 0)
	      //left_c_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      if (fold_line_left_paths.count(fold_line_it->first) > 0 && fold_line_left_paths.at(fold_line_it->first).count(fold_line_index) > 0)
		right_c_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][2] * same_patch_indicators[fold_line_it->first][fold_line_index];
	    }
	  }
	}
	
	// if (fold_line_index == 28 && stability_depth == 0)
	//   model.addConstr(left_left_stability_indicators[fold_line_index][stability_depth][3] == 0, "double-connected");
	//   cout << left_c_sum << endl;
	//   exit(1);
	// }
	//   model.addQConstr(left_c_sum >= 2, "double-connected");
	
	checkAndSet(left_left_stability_mask, fold_line_index, stability_depth, 3, "double-connected");
	model.addQConstr((2 * left_left_stability_indicators[fold_line_index][stability_depth][3]) <= left_c_sum, "double-connected");

	checkAndSet(right_right_stability_mask, fold_line_index, stability_depth, 3, "double-connected");
	model.addQConstr(2 * right_right_stability_indicators[fold_line_index][stability_depth][3] <= right_c_sum, "double-connected");
      }
    }

    //stable fold line constraints
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      //if (optimized_flags[fold_line_index])      
      //continue;

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
	      if (fold_line_right_paths.count(fold_line_it->first) > 0 && fold_line_right_paths.at(fold_line_it->first).count(fold_line_index) > 0)
                left_s_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      else
		left_s_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
	    }
	  }
	  if (fold_line_right_paths.count(fold_line_index) > 0) {
	    for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++) {
	      if (fold_line_left_paths.count(fold_line_it->first) > 0 && fold_line_left_paths.at(fold_line_it->first).count(fold_line_index) > 0)
		right_s_sum += right_left_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
	      else
		right_s_sum += left_right_stability_indicators[fold_line_it->first][previous_stability_depth][0] * same_patch_indicators[fold_line_it->first][fold_line_index];
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
	if (fold_line_left_paths.count(fold_line_index) > 0 && fold_line_left_paths[fold_line_index].size() == 1)
          model.addQConstr(left_left_stability_indicators[fold_line_index][stability_depth][0] == left_s_sum, "stable");
	else
          model.addQConstr(left_left_stability_indicators[fold_line_index][stability_depth][0] <= left_s_sum, "stable");
      
	checkAndSet(right_right_stability_mask, fold_line_index, stability_depth, 0, "stable");
	if (fold_line_right_paths.count(fold_line_index) > 0 && fold_line_right_paths[fold_line_index].size() == 1)
	  model.addQConstr(right_right_stability_indicators[fold_line_index][stability_depth][0] == right_s_sum, "stable");
	else
          model.addQConstr(right_right_stability_indicators[fold_line_index][stability_depth][0] <= right_s_sum, "stable");

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
	model.addQConstr(left_right_stability_indicators[fold_line_index][stability_depth][0] * fold_line_activity_indicators[fold_line_index] <= stability_sum, "stable");
	
	checkAndSet(right_left_stability_mask, fold_line_index, stability_depth, 0, "stable");
	model.addQConstr(right_left_stability_indicators[fold_line_index][stability_depth][0] * fold_line_activity_indicators[fold_line_index] <= stability_sum, "stable");
      }
    }

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      //if (optimization_type == 'C' && optimized_flags[fold_line_index])
      //continue;

      vector<GRBLinExpr> left_right_stability_sum(MAX_CONNECTION_DEPTH + 1);
      vector<GRBLinExpr> right_left_stability_sum(MAX_CONNECTION_DEPTH + 1);
      {
	vector<GRBLinExpr> left_left_stability_sum(MAX_CONNECTION_DEPTH + 1);
        vector<GRBLinExpr> right_right_stability_sum(MAX_CONNECTION_DEPTH + 1);
        for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	  for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	    left_left_stability_sum[connection_depth] += left_left_stability_indicators[fold_line_index][stability_depth][connection_depth];
	    left_right_stability_sum[connection_depth] += left_right_stability_indicators[fold_line_index][stability_depth][connection_depth];
	    right_right_stability_sum[connection_depth] += right_right_stability_indicators[fold_line_index][stability_depth][connection_depth];
	    right_left_stability_sum[connection_depth] += right_left_stability_indicators[fold_line_index][stability_depth][connection_depth];
	  }
	}
	for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
	  model.addConstr(left_left_stability_sum[connection_depth] <= 1, "property sum <= 1");
	  model.addConstr(left_right_stability_sum[connection_depth] <= 1, "property sum <= 1");
	  model.addConstr(right_right_stability_sum[connection_depth] <= 1, "property sum <= 1");
	  model.addConstr(right_left_stability_sum[connection_depth] <= 1, "property sum <= 1");
	}
      }
      {
	vector<GRBLinExpr> left_left_connection_sum(MAX_STABILITY_DEPTH + 1);
        vector<GRBLinExpr> right_right_connection_sum(MAX_STABILITY_DEPTH + 1);
        vector<GRBLinExpr> left_right_connection_sum(MAX_STABILITY_DEPTH + 1);
        vector<GRBLinExpr> right_left_connection_sum(MAX_STABILITY_DEPTH + 1);
	for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
	  for (int connection_depth = 0; connection_depth <= MAX_CONNECTION_DEPTH; connection_depth++) {
            left_left_connection_sum[stability_depth] += left_left_stability_indicators[fold_line_index][stability_depth][connection_depth];
            left_right_connection_sum[stability_depth] += left_right_stability_indicators[fold_line_index][stability_depth][connection_depth];
            right_right_connection_sum[stability_depth] += right_right_stability_indicators[fold_line_index][stability_depth][connection_depth];
            right_left_connection_sum[stability_depth] += right_left_stability_indicators[fold_line_index][stability_depth][connection_depth];
          }
        }
	for (int stability_depth = 0; stability_depth <= MAX_STABILITY_DEPTH; stability_depth++) {
          model.addConstr(left_left_connection_sum[stability_depth] <= 1, "property sum <= 1");
          model.addConstr(left_right_connection_sum[stability_depth] <= 1, "property sum <= 1");
          model.addConstr(right_right_connection_sum[stability_depth] <= 1, "property sum <= 1");
          model.addConstr(right_left_connection_sum[stability_depth] <= 1, "property sum <= 1");
        }
      }

      
      if (left_background_fold_lines.count(fold_line_index) > 0 || right_background_fold_lines.count(fold_line_index) > 0)
	continue;
      //if (optimized_flags[fold_line_index])      
      //continue;
      model.addConstr(left_right_stability_sum[0] >= fold_line_activity_indicators[fold_line_index], "stable sum >= 1");
      model.addConstr(right_left_stability_sum[0] >= fold_line_activity_indicators[fold_line_index], "stable sum >= 1");
      //obj += -(left_stability_sum[0] + right_stability_sum[0]) * fold_line_activity_indicators[fold_line_index];
    }

    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      //if (optimized_flags[fold_line_index])      
      //continue;

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
      model.addConstr(left_right_stability_indicators[28][1][0] == 1);
      model.addConstr(right_left_stability_indicators[28][1][0] == 1);
      model.addConstr(left_right_stability_indicators[36][2][0] == 1);
      model.addConstr(left_right_stability_indicators[7][2][0] == 1);
      model.addConstr(left_right_stability_indicators[37][2][0] == 1);
      model.addConstr(left_left_stability_indicators[9][2][0] == 1);
      model.addConstr(left_right_stability_indicators[19][2][0] == 1);
      model.addConstr(left_right_stability_indicators[68][2][0] == 1);
      model.addConstr(left_right_stability_indicators[69][2][0] == 1);
      model.addConstr(right_right_stability_indicators[21][2][0] == 1);
      model.addConstr(right_left_stability_indicators[29][3][0] == 1);
      model.addConstr(left_left_stability_indicators[11][3][0] == 1);
      model.addConstr(right_right_stability_indicators[23][3][0] == 1);
      model.addConstr(right_left_stability_indicators[30][4][0] == 1);
      model.addConstr(right_right_stability_indicators[10][4][0] == 1);
      model.addConstr(left_left_stability_indicators[22][4][0] == 1);
      model.addConstr(right_left_stability_indicators[43][5][0] == 1);
      model.addConstr(right_left_stability_indicators[62][5][0] == 1);
      model.addConstr(right_left_stability_indicators[13][3][0] == 1);
      model.addConstr(left_right_stability_indicators[20][3][0] == 1);
      model.addConstr(right_left_stability_indicators[27][6][0] == 1);
      model.addConstr(right_left_stability_indicators[12][6][0] == 1);
      model.addConstr(right_left_stability_indicators[31][7][0] == 1);
    }
  }

  GRBLinExpr original_fold_line_sum(0);
  GRBLinExpr original_fold_line_weighted_sum(0);
  GRBLinExpr new_fold_line_sum(0);
  {
    vector<int> background_left_fold_lines = popup_graph.getBackgroundLeftFoldLines();
    vector<int> background_right_fold_lines = popup_graph.getBackgroundRightFoldLines();
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      //if (optimized_flags[fold_line_index])      
      //continue;
      if (fold_line_index >= popup_graph.getNumOriginalFoldLines()) {
	if (fold_line_index != popup_graph.getMiddleFoldLineIndex() && fold_line_index != popup_graph.getBorderFoldLineIndices().first && fold_line_index != popup_graph.getBorderFoldLineIndices().second)
	  new_fold_line_sum += fold_line_activity_indicators[fold_line_index];
      } else {
	//if (find(background_left_fold_lines.begin(), background_left_fold_lines.end(), fold_line_index) == background_left_fold_lines.end() && find(background_right_fold_lines.begin(), background_right_fold_lines.end(), fold_line_index) == background_right_fold_lines.end())
	original_fold_line_sum += fold_line_activity_indicators[fold_line_index];
	original_fold_line_weighted_sum += fold_line_activity_indicators[fold_line_index] * max(popup_graph.getFoldLineScore(fold_line_index) - 0.7, 0.0) * 4;
      }
    }
  }

  if (optimization_type != 'C') {
    for (vector<vector<int> >::const_iterator combination_it = excluded_fold_line_combinations.begin(); combination_it != excluded_fold_line_combinations.end(); combination_it++) {
      GRBLinExpr combination_sum(0);
      for (vector<int>::const_iterator fold_line_it = combination_it->begin(); fold_line_it != combination_it->end(); fold_line_it++)
	combination_sum += fold_line_activity_indicators[*fold_line_it];
      model.addConstr(combination_sum <= num_new_fold_lines_constraint - 1);
    }
  }
  // model.addConstr(fold_line_activity_indicators[38] + fold_line_activity_indicators[64] <= 1);
  // model.addConstr(fold_line_activity_indicators[41] + fold_line_activity_indicators[65] <= 1);
  // model.addConstr(fold_line_activity_indicators[35] + fold_line_activity_indicators[66] <= 1);
  // model.addConstr(fold_line_activity_indicators[71] + fold_line_activity_indicators[33] <= 1);
  // model.addConstr(fold_line_activity_indicators[56] + fold_line_activity_indicators[52] <= 1);
  // model.addConstr(fold_line_activity_indicators[46] + fold_line_activity_indicators[54] <= 1);
  // model.addConstr(fold_line_activity_indicators[44] + fold_line_activity_indicators[59] <= 1);
  // model.addConstr(fold_line_activity_indicators[34] + fold_line_activity_indicators[72] <= 1);
  // model.addConstr(fold_line_activity_indicators[63] + fold_line_activity_indicators[40] <= 1);
  // model.addConstr(fold_line_activity_indicators[50] + fold_line_activity_indicators[57] <= 1);
  //model.addConstr(obj >= 7);
  //if (optimization_type == 'C')
  //model.addConstr(fold_line_activity_indicators[17] == 0);
      
  if (optimization_type == 'T')  
    model.setObjective((new_fold_line_sum - original_fold_line_weighted_sum) * 10000 + position_obj, GRB_MINIMIZE);
  if (optimization_type == 'C')  
    model.setObjective((new_fold_line_sum - original_fold_line_sum * 2 - original_fold_line_weighted_sum) * 10000 + position_obj, GRB_MINIMIZE);
  model.addConstr(new_fold_line_sum >= num_new_fold_lines_constraint);
  
  //if (optimization_type == 'C')
  //model.addConstr(fold_line_activity_indicators[25] == 1);
  
  model.update();
  model.set(GRB_StringAttr_ModelName,"fold line assignment");
  model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
  //model.getEnv().set(GRB_DoubleParam_MIPGap, 0.005);
  //model.getEnv().set(GRB_DoubleParam_NodeLimit, 1000);
  model.getEnv().set(GRB_DoubleParam_TimeLimit, 300);
  //if (optimize_position == false && check_stability == false)
  //model.getEnv().set(GRB_IntParam_Presolve, 0);
  if (true)
    model.getEnv().set(GRB_IntParam_OutputFlag, 0);

  // GRBVar test_1 = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  // GRBVar test_2 = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  // GRBVar test_3 = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
  // model.update();
  // model.addQConstr(2 * test_3 <= test_1 * test_2);
  // model.addConstr(test_1 == 1);
  // model.addConstr(test_2 == 1);
  // model.addConstr(test_3 == 1);
  
  try {
    //model.set("OutputFlag", false);
    model.update();
    //if (check_stability)
    //model.write("Test/model.lp");
    model.optimize();
    //model.update();
    //model.write("Test/solution.sol");
    //if (model.get(GRB_IntAttr_IsMIP) == 0)
    //throw GRBException("Model is not a MIP");

    if (model.get(GRB_IntAttr_Status) != GRB_OPTIMAL) {
      if (optimization_type == 'C' && false) {
	//model.computeIIS();
	//model.write("Test/iis.ilp");
      }
      model.write("Test/model.lp");
      
      cout << "infeasible" << endl;

      popup_graph.setOptimizedFoldLineInfo(vector<int>(popup_graph.getNumFoldLines(), -1), vector<int>(popup_graph.getNumFoldLines(), -1), vector<int>(popup_graph.getNumFoldLines(), -1));
      return false;
    }
    
  }catch(GRBException e) {
    cout << "Error code = " << e.getErrorCode() << endl;
    cout << e.getMessage() << endl;
  }catch (...) {printf("Exception...\n");
    exit(1);
  }
  
  
  if (true) {
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      cout << fold_line_index << '\t' << fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_Xs[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_Ys[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << right_copy_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << denoted_fold_line_positions[fold_line_index] << endl;
      
      //pair<int, int> x_range = popup_graph.getFoldLineXRange(fold_line_index);
      //if (fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X) < x_range.first || fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X) > x_range.second)
      //cout << fold_line_index << '\t' << x_range.first << '\t' << x_range.second << endl;
      

      // for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    //   if (static_cast<int>(fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5) == 0)
    // 	cout << "inactive ";
      //continue;
    //cout << "fold line index: " << fold_line_index << endl;
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

    // for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
    //   int left_fold_line_index = fold_line_pair_it->first;
    //   int right_fold_line_index = fold_line_pair_it->second;
      
    //   if (right_copy_convexity_indicators[left_fold_line_index].get(GRB_DoubleAttr_X) == 1 && right_copy_Ys[left_fold_line_index].get(GRB_DoubleAttr_X) != fold_line_Ys[right_fold_line_index].get(GRB_DoubleAttr_X))
    //     cout << "postion inconsistency: " << left_fold_line_index << '\t' << right_fold_line_index << endl;      
    //   if (right_copy_convexity_indicators[left_fold_line_index].get(GRB_DoubleAttr_X) == 0 && right_copy_Xs[left_fold_line_index].get(GRB_DoubleAttr_X) != fold_line_Xs[right_fold_line_index].get(GRB_DoubleAttr_X))            
    //     cout << "postion inconsistency: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
    //   if (right_copy_convexity_indicators[left_fold_line_index].get(GRB_DoubleAttr_X) == 1 && right_copy_Xs[left_fold_line_index].get(GRB_DoubleAttr_X) > fold_line_Xs[right_fold_line_index].get(GRB_DoubleAttr_X))            
    //     cout << "postion conflict: " << left_fold_line_index << '\t' << right_fold_line_index << endl;      
    //   if (right_copy_convexity_indicators[left_fold_line_index].get(GRB_DoubleAttr_X) == 0 && right_copy_Ys[left_fold_line_index].get(GRB_DoubleAttr_X) > fold_line_Ys[right_fold_line_index].get(GRB_DoubleAttr_X))            
    //     cout << "postion conflict: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
    // }
      
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
      for (int other_fold_line_index = 0; other_fold_line_index < popup_graph.getNumFoldLines(); other_fold_line_index++)
	if (other_fold_line_index != fold_line_index && same_patch_indicators[fold_line_index][other_fold_line_index].get(GRB_DoubleAttr_X) == 1)
	  cout << "same patch fold line pairs: " << fold_line_index << '\t' << other_fold_line_index << endl;
  }
  //  popup_graph.checkFoldLineInfo();
  
  vector<int> optimized_fold_line_activities(popup_graph.getNumFoldLines(), -1);
  vector<int> optimized_fold_line_convexities(popup_graph.getNumFoldLines(), -1);
  vector<int> optimized_fold_line_positions(popup_graph.getNumFoldLines(), -1);
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    if (fold_line_index == popup_graph.getBorderFoldLineIndices().first) {
      optimized_fold_line_activities[fold_line_index] = 1;      
      optimized_fold_line_convexities[fold_line_index] = 0; 
      optimized_fold_line_positions[fold_line_index] = 0;      
      continue;
    }
    if (fold_line_index == popup_graph.getBorderFoldLineIndices().second) {
      optimized_fold_line_activities[fold_line_index] = 1;      
      optimized_fold_line_convexities[fold_line_index] = 0; 
      optimized_fold_line_positions[fold_line_index] = IMAGE_WIDTH - 1;
      continue;
    }

    optimized_fold_line_activities[fold_line_index] = static_cast<int>(fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5);
    optimized_fold_line_convexities[fold_line_index] = static_cast<int>(fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5);
    optimized_fold_line_positions[fold_line_index] = fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X);
  }

  if (optimization_type == 'C') {

    map<int, set<int> > patch_fold_lines = popup_graph.getPatchFoldLines();
    while (true) {
      bool has_change = false;
      for(int patch_index = 0; patch_index < popup_graph.getNumOriginalPatches(); patch_index++) {
	if (patch_index == popup_graph.getOriginalBackgroundPatchIndex())
	  continue;
      
	set<int> fold_lines = patch_fold_lines.at(patch_index);
	
	int num_active_fold_lines = 0;
	for (set<int>::const_iterator fold_line_it = fold_lines.begin(); fold_line_it != fold_lines.end(); fold_line_it++)		
          if (optimized_fold_line_activities[*fold_line_it] == 1)
	    num_active_fold_lines++;
	  
	if (num_active_fold_lines == 1) {
	  for (set<int>::const_iterator fold_line_it = fold_lines.begin(); fold_line_it != fold_lines.end(); fold_line_it++) {
	    if (denoted_fold_line_activities[*fold_line_it] != -1)                          	    
	      continue;	      
	    if (optimized_fold_line_activities[*fold_line_it] == 1) {
	      cout << "lonely fold line: " << *fold_line_it << endl;
	      optimized_fold_line_activities[*fold_line_it] = 0;
              has_change = true;
	    }
	  }
	}
      }
	   
      for (int fold_line_index = popup_graph.getNumOriginalFoldLines(); fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
        if (fold_line_index == popup_graph.getMiddleFoldLineIndex() || fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second)      
          continue;
	if (denoted_fold_line_activities[fold_line_index] != -1)                      	
	  continue;      
	if (optimized_fold_line_activities[fold_line_index] == 0) 
          continue;
        bool has_left_path = false;
	{
	  if (fold_line_left_paths.count(fold_line_index) > 0) {
		  for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths.at(fold_line_index).end(); fold_line_it++) {
		    if (optimized_fold_line_activities[fold_line_it->first] == 1) {
		      has_left_path = true;
		      break;
		    }
		  }
	  }
	}
	bool has_right_path = false;
        {
	  if (fold_line_right_paths.count(fold_line_index) > 0) {
		  for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++) {
		    if (optimized_fold_line_activities[fold_line_it->first] == 1) {
		      has_right_path = true;
		      break;
		    }
		  }
	  }
        }
	if (has_left_path == false || has_right_path == false) {
          cout << "lonely fold line: " << fold_line_index << endl;              
	  optimized_fold_line_activities[fold_line_index] = 0;            
	  has_change = true;
	}
      }

      if (has_change == false)
        break;
    }
  }

  // cout << "optimized info: " << endl;
  // cout << optimization_type << endl;
  // for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
  //   cout << fold_line_index << '\t' << optimized_fold_line_positions[fold_line_index] << '\t' << optimized_fold_line_convexities[fold_line_index] << endl;

  if (optimization_type != 'S')
    popup_graph.setOptimizedFoldLineInfo(optimized_fold_line_activities, optimized_fold_line_convexities, optimized_fold_line_positions);


  // for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs.begin(); fold_line_pair_it != fold_line_pairs.end(); fold_line_pair_it++) {
  //   int left_fold_line_index = fold_line_pair_it->first;
  //   int right_fold_line_index = fold_line_pair_it->second;
  //   cout << "fold line neighbors: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
  //   cout << right_copy_Xs[left_fold_line_index].get(GRB_DoubleAttr_X) - fold_line_Xs[right_fold_line_index].get(GRB_DoubleAttr_X) << endl;
  //   cout << right_copy_Ys[left_fold_line_index].get(GRB_DoubleAttr_X) - fold_line_Ys[right_fold_line_index].get(GRB_DoubleAttr_X) << endl;
  // }
  // //exit(1);
  // if (true)
  //   {
  //     vector<pair<int, int> > fold_line_pairs_passed = popup_graph.getFoldLinePairsPassed();
  //     for (vector<pair<int, int> >::const_iterator fold_line_pair_it = fold_line_pairs_passed.begin(); fold_line_pair_it != fold_line_pairs_passed.end(); fold_line_pair_it++) {
  //       int left_fold_line_index = fold_line_pair_it->first;
  //       int right_fold_line_index = fold_line_pair_it->second;
  //       cout << "fold line neighbors passed: " << left_fold_line_index << '\t' << right_fold_line_index << endl;
  //       // if (fold_line_pair_it - fold_line_pairs_passed.begin() == 1)
  //       //   break;
  //       if (right_fold_line_index >= popup_graph.getNumOriginalFoldLines()) {
  // 	  cout << fold_line_Xs[left_fold_line_index].get(GRB_DoubleAttr_X) - fold_line_Xs[right_fold_line_index].get(GRB_DoubleAttr_X) << endl;
  //         cout << fold_line_Ys[left_fold_line_index].get(GRB_DoubleAttr_X) - fold_line_Ys[right_fold_line_index].get(GRB_DoubleAttr_X) << endl;
  //       } else {
  // 	  cout << right_copy_Xs[left_fold_line_index].get(GRB_DoubleAttr_X) - right_copy_Xs[right_fold_line_index].get(GRB_DoubleAttr_X) << endl;
  //         cout << right_copy_Ys[left_fold_line_index].get(GRB_DoubleAttr_X) - right_copy_Ys[right_fold_line_index].get(GRB_DoubleAttr_X) << endl;
  //       }
  //     }
  //   }

  
  // vector<pair<int, int> > symmetric_fold_line_pairs = popup_graph.getSymmetricFoldLinePairs();
  // for (vector<pair<int, int> >::const_iterator symmetric_fold_line_pair_it = symmetric_fold_line_pairs.begin(); symmetric_fold_line_pair_it != symmetric_fold_line_pairs.end(); symmetric_fold_line_pair_it++) {
  //   cout << symmetric_fold_line_pair_it->first << '\t' << symmetric_fold_line_pair_it->second << '\t' << fold_line_activity_indicators[symmetric_fold_line_pair_it->first].get(GRB_DoubleAttr_X) << '\t' << fold_line_activity_indicators[symmetric_fold_line_pair_it->second].get(GRB_DoubleAttr_X) << '\t' << fold_line_convexity_indicators[symmetric_fold_line_pair_it->first].get(GRB_DoubleAttr_X) << '\t' << fold_line_convexity_indicators[symmetric_fold_line_pair_it->second].get(GRB_DoubleAttr_X) << endl;
  // }
  // exit(1);

  
  // double test = 0;
  // int fold_line_index = 42;
  // if (fold_line_left_paths.count(fold_line_index) > 0) {
  //   for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_left_paths.at(fold_line_index).begin(); fold_line_it != fold_line_left_paths.at(fold_line_index).end(); fold_line_it++) {
  //     test += left_right_stability_indicators[fold_line_it->first][0][2].get(GRB_DoubleAttr_X) * same_patch_indicators[fold_line_it->first][fold_line_index].get(GRB_DoubleAttr_X);
  //   }
  // }
  // if (fold_line_right_paths.count(fold_line_index) > 0) {
  //   for (map<int, vector<int> >::const_iterator fold_line_it = fold_line_right_paths.at(fold_line_index).begin(); fold_line_it != fold_line_right_paths.at(fold_line_index).end(); fold_line_it++) {
  //     test += left_right_stability_indicators[fold_line_it->first][0][2].get(GRB_DoubleAttr_X) * same_patch_indicators[fold_line_it->first][fold_line_index].get(GRB_DoubleAttr_X);
  //   }
  // }
  //  cout << "test: " << test << endl;
  //cout << "same: " << left_left_stability_indicators[fold_line_index][0][3].get(GRB_DoubleAttr_X) << endl;
  return true;
}
