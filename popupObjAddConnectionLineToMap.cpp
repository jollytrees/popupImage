//
//  popupObjAddConnectionLineToMap.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/10/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//
#include "popupObjAddConnectionLineToMap.hpp"
#include "functionLineVecToLinePair.h"
#include "functionDrawPatch.h"

void fitMaxLine(cv::Mat &patchMat, std::vector<cv::Vec4i> &maxLineVec,
                cv::Rect &rect, popupObject *obj, int idx1, int idx2){

    cv::Size matSize = patchMat.size();
    int offset = 5;
    int maxSize = 0;
    cv::Mat mat_line(matSize, CV_8UC1);
    cv::Mat addMat(matSize, CV_8UC1);
    std::vector<cv::Vec4i> LineVec;

    for (int i = rect.tl().x;  i < rect.br().x; i = i+offset){

        LineVec.clear();
        mat_line.setTo(0);
        cv::Point up = cv::Point(i, rect.tl().y-5);
        cv::Point down = cv::Point(i, rect.br().y+5);
        cv::line(mat_line, up, down, 255, 5);
   
        addMat.setTo(0);
        addMat = mat_line & patchMat ;  
        
        int count = cv::countNonZero(addMat);
        //int threshold, double minLineLength=0, double maxLineGap=0
        HoughLinesP( addMat, LineVec, 1, CV_PI/180, 3 );

        if(count > maxSize && count > 0){
            maxLineVec = LineVec;
            maxSize = count;
        }
    }
}

void addConnectionLine(popupObject *obj, int idx1, int idx2){
    
    cv::Mat patchMat = obj->classifiedPatchIntersection[idx1][idx2];
    cv::Rect boundingRectOfInterPatch;
    findContourBoundingBox(patchMat, boundingRectOfInterPatch);

    std::vector<cv::Vec4i> maxLineVec;
    
    fitMaxLine(patchMat, maxLineVec, boundingRectOfInterPatch, obj, idx1, idx2);
    
    foldLineType maxLine;
    if(lineVecToLinePair(maxLineVec, patchMat.size(), maxLine.line)){
        maxLine.isCentralLine = false;
        maxLine.isConnLine = true;
        maxLine.isEmpty = false;
        maxLine.setOriginalType();
        foldLineType *foldLine = new foldLineType(maxLine);;
        
        obj->boundaryFoldLineConnGroupMap[idx1][idx2].push_back(foldLine);
        obj->boundaryFoldLineConnGroupMap[idx2][idx1].push_back(foldLine);
    }else{
    
        cout << "find connection line error! " << maxLineVec.size()<< endl;
    }
}

bool popupObjAddConnectionLineToMap::execute(popupObject *obj)
{
    //exclude the floor and back patch
    for(int i=0; i< obj->boundaryFoldLineConnGroupMap.size(); i++){
        for(int j=i+1; j< obj->boundaryFoldLineConnGroupMap[i].size(); j++){
        
            if(obj->isBasePatch(i)) continue;
            if(obj->boundaryFoldLineConnGroupMap[i][j].size()==0 && obj->classifiedConnMap[i][j]==1) {
                addConnectionLine(obj, i ,j);
                cout << "add" << endl;
            }
        }
    }
    
    obj->originalFoldLineConnMap.resize(obj->boundaryFoldLineConnGroupMap.size());
    for(size_t i=0; i< obj->originalFoldLineConnMap.size(); i++){
        obj->originalFoldLineConnMap[i].resize(obj->boundaryFoldLineConnGroupMap.size());
    }
    
    obj->originalFoldLineOfPatch.resize(obj->boundaryFoldLineConnGroupMap.size());
    
    
    //upper triangle
    for(size_t i=0; i< obj->boundaryFoldLineConnGroupMap.size(); i++){
        
        for(size_t j=i+1; j<obj->boundaryFoldLineConnGroupMap[i].size(); j++){
            
            for(size_t k=0; k<obj->boundaryFoldLineConnGroupMap[i][j].size(); k++){
                foldLineType *line = new foldLineType(*obj->boundaryFoldLineConnGroupMap[i][j][k]);
                line->setOriginalType();
                line->originalConnPatch.push_back((int)i);
                line->originalConnPatch.push_back((int)j);
                
                
                obj->foldLine.push_back(line);
                obj->foldLine.back()->foldLineIdx = obj->foldLine.size()-1;
                cout << i <<" " <<  j  << " "<< obj->foldLine.back()->foldLineIdx<< " " << boolalpha << line->isConnLine << endl;;
                
                obj->originalFoldLineConnMap[i][j].push_back(line);
                obj->originalFoldLineConnMap[j][i].push_back(line);
                
                obj->originalFoldLineOfPatch[i].push_back(line);
                obj->originalFoldLineOfPatch[j].push_back(line);
            }
        }
    }
    
    
    
    return true;
}