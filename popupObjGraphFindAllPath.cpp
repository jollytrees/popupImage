//
//  popupObjGraphFindAllPath.cpp
//  popupImage
//
//  Created by jollytrees on 11/22/15.
//
//

#include "popupObjGraphFindAllPath.hpp"
#include "functionFindAllPath.h"

bool popupObjGraphFindAllPath::execute(popupObject *obj)
{
    GRAPH.clear();
    int originalSize = 0;
    //putEdges
    for(int i=0; i< obj->foldLine.size(); i++){
        if(!obj->foldLine[i]->isOriginalType) continue;
        originalSize ++ ;
        createEdgeM(obj->foldLine[i]->originalConnPatch[0], obj->foldLine[i]->originalConnPatch[1]);
    }
    
    //find all path
    findAllPaths(obj->paths, obj->originalFloorPatch, obj->originalBackPatch,  obj->classifiedPatches.size(), originalSize);

    return true;
}