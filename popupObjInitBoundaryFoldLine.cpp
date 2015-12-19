//
//  popupObjInitBoundaryFoldLIne.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/9/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjInitBoundaryFoldLine.hpp"
#include "functionLineVecToLinePair.h"
#include "functionFindMatIntersection.h"
#include "functionDrawPatch.h"

bool is_midLine(cv::Mat &BoundaryLineMat, popupObject *obj){
    
    cv::Mat addMat(BoundaryLineMat.size(), CV_8UC1);
    cv::Mat midLine(BoundaryLineMat.size(), CV_8UC1);
    cv::line(midLine, cv::Point(obj->centralXPosition, 0), cv::Point(obj->centralXPosition, BoundaryLineMat.rows), 255, 10);
    
    addMat = BoundaryLineMat & midLine ;
    int count = cv::countNonZero(addMat);
    //std::cout << count << std::endl;
    if(count > 5){
        return true;
    }
    return false;
    
}

static void fit_right(cv::Mat &patchMat, foldLineType &rightLine,
                     cv::Rect &rect, popupObject *obj, int index){
    
    cv::Size matSize = patchMat.size();
    std::vector<cv::Vec4i> rightLineVec;
    
    int offset = 2;
    for (int i = rect.br().x;  i > rect.tl().x; i = i-offset){
        
        foldLineType line;
        cv::Point up = cv::Point(i, rect.tl().y-10);
        cv::Point down = cv::Point(i, rect.br().y+10);
        line.line = make_pair(up, down);
        
        if(isLineAndMatConn(line, patchMat ,rightLineVec)) break;
    }
    
    int count = countLineVec(rightLineVec, matSize);
    if(count == 1){
        lineVecToLinePair(rightLineVec, matSize, rightLine.line);
    }else{
        std::cout << "rightLine is empty or count > 1" << std::endl;
    }
}

static void fit_left(cv::Mat &patchMat, foldLineType &leftLine,
              cv::Rect &rect, popupObject *obj, int index){
    
    cv::Size matSize = patchMat.size();
    std::vector<cv::Vec4i> leftLineVec;
    
    int offset = 2;
    for (int i = rect.tl().x;  i < rect.br().x; i = i+offset){
        
        foldLineType line;
        cv::Point up = cv::Point(i, rect.tl().y-10);
        cv::Point down = cv::Point(i, rect.br().y+10);
        line.line = make_pair(up, down);
        
        if(isLineAndMatConn(line, patchMat ,leftLineVec)) break;
    }

    int count = countLineVec(leftLineVec, matSize);
    if(count == 1){
        lineVecToLinePair(leftLineVec, matSize, leftLine.line);
    }else{
        std::cout << "linesLeft is empty or count > 1" << std::endl;
    }
}


static void findBoundaryFoldLine(popupObject *obj,  int &index){
        
    cv::Rect boundingRectOnePatch = cv::boundingRect(obj->classifiedPatches[index]->paths[0]);
    
    obj->boundaryFoldLineOfPatch.resize(obj->classifiedPatches.size());
    
    //fit left line
    foldLineType leftLine;
    fit_left(obj->classifiedPatches[index]->pchMat, leftLine, boundingRectOnePatch,
             obj, index);
    
    foldLineType *lLine = new foldLineType(leftLine);
    obj->boundaryFoldLineOfPatch[index].push_back(lLine);
    
    //fit right line
    foldLineType rightLine;
    fit_right(obj->classifiedPatches[index]->pchMat, rightLine, boundingRectOnePatch,
              obj, index);
    
    foldLineType *rLine = new foldLineType(rightLine);
    obj->boundaryFoldLineOfPatch[index].push_back(rLine);
    
    cv::Mat canvas(obj->initMatSize, CV_8UC3);
    canvas.setTo(black);
    canvas = drawPatch(obj->classifiedPatches, obj->initMatSize);

    
    cv::line(canvas, leftLine.line.first, leftLine.line.second, blue, 3);
    cv::line(canvas, rightLine.line.first, rightLine.line.second, red, 3);
    
    ostringstream oss;
    oss.str("");
    oss<< index ;

    std::string str = "./foldline/"+oss.str()+".png";
    imwrite(str, canvas);
    

}


bool popupObjInitBoundaryFoldLine::execute(popupObject *obj)
{
    
    std::ostringstream oss;
    for(int i =0; i < obj->classifiedPatches.size(); i++){
        
        if(obj->isBasePatch(i)) continue;
        
        findBoundaryFoldLine(obj, i);
    }

    
    return true;
    
}