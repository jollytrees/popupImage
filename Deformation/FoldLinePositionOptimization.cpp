#include "FoldLineOptimization.h"

#include <stdio.h>
#include <stdlib.h>
#include <gurobi_c++.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;


bool optimizeFoldLinePositions(Popup::PopupGraph &popup_graph)
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

  {
    vector<int> optimized_fold_line_xs;
    vector<bool> optimized_fold_line_convexities;
    popup_graph.getOptimizedFoldLineInfo(optimized_fold_line_xs, optimized_fold_line_convexities);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
      if (optimized_fold_line_xs[fold_line_index] >= 0) {
	cout << fold_line_index << endl;
	model.addConstr(fold_line_activity_indicators[fold_line_index] == 1);
	model.addConstr(fold_line_convexity_indicators[fold_line_index] == static_cast<int>(optimized_fold_line_convexities[fold_line_index]));
      }
      else if (fold_line_index != popup_graph.getBorderFoldLineIndices().first && fold_line_index != popup_graph.getBorderFoldLineIndices().second && fold_line_index != popup_graph.getMiddleFoldLineIndex())
	model.addConstr(fold_line_activity_indicators[fold_line_index] == 0);
    }
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
    
    GRBQuadExpr position_obj(0);
    vector<int> desirable_fold_line_positions;
    popup_graph.getDesirableFoldLinePositions(desirable_fold_line_positions);
    for (int fold_line_index = 0; fold_line_index < popup_graph.getNumOriginalFoldLines(); fold_line_index++)
      position_obj += (fold_line_positions[fold_line_index] - desirable_fold_line_positions[fold_line_index]) * (fold_line_positions[fold_line_index] - desirable_fold_line_positions[fold_line_index]);
    model.setObjective(position_obj, 1);
    
    //for (int fold_line_index = 0; fold_line_index < popup_graph.num_fold_lines - 2; fold_line_index++)
    //model.setPWLObj(fold_line_positions[fold_line_index], fold_line_positions_costs_pair[fold_line_index].first.size(), &fold_line_positions_costs_pair[fold_line_index].first[0], &fold_line_positions_costs_pair[fold_line_index].second[0]);
  }
  
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

  
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++)
    cout << fold_line_index << '\t' << fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_convexity_indicators[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << '\t' << fold_line_Xs[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_Ys[fold_line_index].get(GRB_DoubleAttr_X) << '\t' << fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X) << endl;

  
  vector<int> optimized_fold_line_xs(popup_graph.getNumFoldLines(), -1);
  for (int fold_line_index = 0; fold_line_index < popup_graph.getNumFoldLines(); fold_line_index++) {
    if (fold_line_index == popup_graph.getBorderFoldLineIndices().first || fold_line_index == popup_graph.getBorderFoldLineIndices().second) {
      optimized_fold_line_xs[fold_line_index] = -1;
      continue;
    }
    if (static_cast<int>(fold_line_activity_indicators[fold_line_index].get(GRB_DoubleAttr_X) + 0.5) == 1)
      optimized_fold_line_xs[fold_line_index] = fold_line_positions[fold_line_index].get(GRB_DoubleAttr_X);      
  }
  popup_graph.setOptimizedFoldLineInfo(optimized_fold_line_xs);
  
  return true;
}
