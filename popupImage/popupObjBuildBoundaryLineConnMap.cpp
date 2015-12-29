//
//  popupObjBuildConnAndMergeLine.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/10/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjBuildBoundaryLineConnMap.hpp"
#include "functionFindMatIntersection.h"
#include "functionOutputMat.h"

static bool searchTheMaxOverlapPatch(popupObject *obj, foldLineType *inputLine, int idx1, int idx2){
    
    //draw input line mat
    cv::Mat inputLineMat(obj->initMatSize, CV_8UC1);
    inputLineMat.setTo(black);
    cv::line(inputLineMat, inputLine->line.first, inputLine->line.second, 255, 10);
    std::vector< std::pair<int, int> > intSecList; //intersection
    intSecList.clear();
    
    
    //1. count the intersection between patch with input line
    for(int i=0; i< obj->classifiedPatches.size(); i++ ){
        
        if(i==idx1) continue;
        //count intersection
        cv::Mat outputMat(obj->initMatSize, CV_8UC1);
        int areaCount = findMatIntersectionRInt( inputLineMat,  obj->classifiedPatches[i]->pchMat, outputMat);
        
        if(areaCount>0){
        
            ostringstream oss;
            oss.str("");
            oss << idx1 << "_" << i ;
            intSecList.push_back(std::make_pair(i, areaCount));
            std::string str = "./popupObjBuildBoundaryLineConnMap/" +oss.str()+".png";
            imwrite(str, outputMat);
        }
        
    }
   
    sort(intSecList.begin(), intSecList.end(), compareLongest);
    
    //2. find the patches with max area
    if(intSecList.size()>0){
        
       /* if(intSecList.size() > 1){
        
            for(int j=0; j< intSecList.size(); j++){
                if(!obj->isBasePatch(intSecList[j].first)){
                    cout << idx1 << " " << intSecList[j].first <<" " << j << " " << idx2 <<endl;
                    obj->boundaryFoldLineConnGroupMap[intSecList[j].first][idx1].push_back(inputLine);
                    obj->boundaryFoldLineConnGroupMap[idx1][intSecList[j].first].push_back(inputLine);
                }
            }

            
        }else{*/
        
            obj->boundaryFoldLineConnGroupMap[intSecList[0].first][idx1].push_back(inputLine);
            obj->boundaryFoldLineConnGroupMap[idx1][intSecList[0].first].push_back(inputLine);
       // }
                
        return true;
    }else{
        cout << idx1<<" " << idx2 << " error" << endl;
    
    }
    
    //std::cout << "searchTheMaxOverlapPatch size is less than 2" << std::endl;
    return false;

}

bool popupObjBuildBoundaryLineConnMap::execute(popupObject *obj)
{
    
    std::ostringstream oss;
    
    //init
    obj->boundaryFoldLineConnGroupMap.resize(obj->classifiedPatches.size());
    for(size_t i=0; i< obj->boundaryFoldLineConnGroupMap.size(); i++)
        obj->boundaryFoldLineConnGroupMap[i].resize(obj->classifiedPatches.size());
    
    //each patch
    for(size_t i=0; i< obj->boundaryFoldLineOfPatch.size(); i++){
    
        //each line of one patch
        for(size_t j=0; j< obj->boundaryFoldLineOfPatch[i].size(); j++){
            
            searchTheMaxOverlapPatch(obj, obj->boundaryFoldLineOfPatch[i][j], i, j);
            
        }
    }
    
    return true;
}