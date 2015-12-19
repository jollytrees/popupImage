//
//  popupObjClassifyPatches.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/9/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjClassifyPatches.hpp"

bool popupObjClassifyPatches::execute(popupObject *obj)
{
    for(int i=0; i<obj->dividedPatches.size(); i++){
        
        if(obj->dividedConnSize[i] < 2)
            obj->islandPatches.push_back(obj->dividedPatches[i]);
        else{
            obj->classifiedPatches.push_back(obj->dividedPatches[i]);
            
            if(i == obj->backPatch){
                obj->backPatch = obj->classifiedPatches.size()-1;
            }else if(i == obj->floorPatch ){
                obj->floorPatch = obj->classifiedPatches.size()-1;
            }
            
        }
        
    
    }
    return true;

}