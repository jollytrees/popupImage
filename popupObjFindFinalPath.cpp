//
//  popupObjFindFinalPath.cpp
//  GitPopupImage
//
//  Created by jollytrees on 12/30/15.
//
//

#include "popupObjFindFinalPath.hpp"
#include "functionFindAllPath.h"

bool popupObjFindFinalPath::execute(popupObject *obj)
{
    GRAPH.clear();
    int originalSize = 0;
    
    //putEdges
    for(int i=0; i< obj->foldLine.size(); i++){
        if(obj->foldLine[i]->isOriginalFoldLine && obj->activeFoldLine[i]==0.0) continue;
        originalSize ++ ;
        createEdgeM(obj->foldLine[i]->connPatch[0], obj->foldLine[i]->connPatch[1]);
    }
    
    
    vector<vector<int> > Paths;

    //find all path
    for(int i=0; i< obj->possiblePatches.size(); i++){
        PATHS.clear();
        cout << obj->floorPatch << " " << i << endl;
        findAllPaths(Paths, obj->floorPatch, i,  obj->possiblePatches.size(), originalSize);
        obj->finalPaths.push_back(Paths[0]);
        
        for(int j=0; j<Paths[0].size(); j++){
        
            cout << Paths[0][j] << " ";
        }
        cout << endl;
    }
    
    return true;
}
