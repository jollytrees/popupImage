//
//  functionFindConnection.h
//  Color_Segmentation
//
//  Created by jollytrees on 11/9/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef functionFindConnection_h
#define functionFindConnection_h
#include "functionFindMatIntersection.h"

static void findConnection(const std::vector< struct patches* > &patches,
                    std::vector<std::vector<int> > &connMap,
                    std::vector<int> &connSize,
                    std::vector<std::vector<cv::Mat> > &patchIntersection){
    
    std::ostringstream oss;
    
    size_t patchSize = patches.size();
    connMap.resize(patchSize);
    connSize.resize(patchSize, 0);
    patchIntersection.resize(patchSize);
    
    for(size_t i =0; i < patchSize; i++){
        connMap[i].resize(patchSize,0);
        patchIntersection[i].resize(patchSize);
    }
    //find connected
    for(size_t i = 0; i < patchSize; i++){
        for(size_t j=i+1; j< patchSize; j++){
            cv::Mat addImg;
            
            if(findMatIntersection(patches[i]->pchMat,
                                   patches[j]->pchMat, addImg))
                
            {
                connSize[i]++;
                connSize[j]++;
                connMap[i][j] = 1;
                connMap[j][i] = 1;
                patchIntersection[i][j] = addImg.clone();
                patchIntersection[j][i] = addImg.clone();
                
            }
            
        }
    }


}

#endif /* functionFindConnection_h */
