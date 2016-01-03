//
//  popupObjMergeBoundaryLine.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/10/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjMergeBoundaryLine.hpp"
#include "functionFindMatIntersection.h"

static bool mergeBoundary(popupObject *obj, int ind1, int ind2){
    
    //reference
    std::vector<foldLineType*> &foldLineVecOfSamePatch =
    obj->boundaryFoldLineConnGroupMap[ind1][ind2];
    cv::Mat &interOfTwoPatches = obj->classifiedPatchIntersection[ind1][ind2];
    
    std::vector< std::pair<int, int> > intSecList; //intersection
    
    //1. count the insersection between two patch with input line
    for(int i=0; i< foldLineVecOfSamePatch.size(); i++ ){
        
        //draw input line mat
        cv::Mat inputLineMat(obj->initMatSize, CV_8UC1);
        inputLineMat.setTo(black);
        cv::line(inputLineMat, foldLineVecOfSamePatch[i]->line.first, foldLineVecOfSamePatch[i]->line.second, 255, 3);
        
        //count intersection
        cv::Mat outputMat(obj->initMatSize, CV_8UC1);
        int areaCount = findMatIntersectionRInt( interOfTwoPatches, inputLineMat, outputMat);
        
        if(areaCount>0) intSecList.push_back(std::make_pair(i, areaCount));
    }
    
    //2. find the two patches with max area
    if(intSecList.size()>0){
        sort(intSecList.begin(), intSecList.end(), compareLongest);
        foldLineVecOfSamePatch[intSecList[0].first]->isEmpty = false;
        obj->boundaryFoldLineConnMap[ind1][ind2] = foldLineVecOfSamePatch[intSecList[0].first];
        obj->boundaryFoldLineConnMap[ind2][ind1] = foldLineVecOfSamePatch[intSecList[0].first];
        
        obj->boundaryFoldLineConnMap[ind1][ind2]->isEmpty = false;
        obj->boundaryFoldLineConnMap[ind2][ind1]->isEmpty = false;
        obj->boundaryFoldLineConnMap[ind1][ind2]->isConnLine = false;
        obj->boundaryFoldLineConnMap[ind2][ind1]->isConnLine = false;
        
        
        return true;
    }else{
        
        return false;
    }

    
}

bool popupObjMergeBoundaryLine::execute(popupObject *obj)
{
    
    foldLineType *emptyLine = new foldLineType();
    emptyLine->isEmpty = true;
    //init
    obj->boundaryFoldLineConnMap.resize(obj->classifiedPatches.size());
    for(size_t i=0; i< obj->boundaryFoldLineConnMap.size(); i++)
        obj->boundaryFoldLineConnMap[i].resize(obj->classifiedPatches.size(), emptyLine);
    
    //upper triangle
    for(size_t i=0; i< obj->boundaryFoldLineConnGroupMap.size(); i++){
        for(size_t j=i+1; j< obj->boundaryFoldLineConnGroupMap[i].size(); j++){
            
            if(obj->boundaryFoldLineConnGroupMap[i][j].size()<2){
            
                if(obj->boundaryFoldLineConnGroupMap[i][j].size() == 1){
                    
                    obj->boundaryFoldLineConnMap[i][j] = obj->boundaryFoldLineConnGroupMap[i][j][0] ;
                    obj->boundaryFoldLineConnMap[i][j]->isEmpty = false;
                    obj->boundaryFoldLineConnMap[i][j]->isConnLine = false;
                    obj->boundaryFoldLineConnMap[j][i] = obj->boundaryFoldLineConnGroupMap[i][j][0] ;
                    obj->boundaryFoldLineConnMap[j][i]->isEmpty = false;
                    obj->boundaryFoldLineConnMap[j][i]->isConnLine = false;
                    //cout << i << " " << j <<endl;
                }
                continue;
            }
            
            
            if(mergeBoundary(obj, i, j)){
                
            }else{
                //std::cout <<i << " " << j <<"merge false" << std::endl;
            }
        }
    }

    for(size_t i=0; i< obj->boundaryFoldLineConnMap.size(); i++){
        int c=0;
        for(size_t j=0; j< obj->boundaryFoldLineConnMap[i].size(); j++){
            if(!obj->boundaryFoldLineConnMap[i][j]->isEmpty){
                c++;
            }
        }
        //cout << i << ":" << c<<endl;
        
    }
    return true;
}
