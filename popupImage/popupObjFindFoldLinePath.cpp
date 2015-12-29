//
//  popupObjFindFoldLinePath.cpp
//  popupImage
//
//  Created by jollytrees on 11/24/15.
//
//

#include "popupObjFindFoldLinePath.hpp"
#include "functionFindAllPath.h"

bool popupObjFindFoldLinePath::execute(popupObject *obj)
{
    obj->foldLinePathsOfPatch.resize(obj->classifiedPatches.size());

    for(size_t pIdx =0; pIdx < obj->paths.size(); pIdx++){
        
        vector<vector<int> > tempPaths;
        vector<vector<int> > foldLinePath;
        tempPaths.clear();
        tempPaths.resize(obj->paths[pIdx].size ()-1);

        //path to fold line path
        for(size_t i=0; i< obj->paths[pIdx].size ()-1 ; i++){
            int idx1 = obj->paths[pIdx][i];
            int idx2 = obj->paths[pIdx][i+1];
            size_t foldLineSize = obj->originalFoldLineConnMap[idx1][idx2].size();
            for(size_t j=0; j< foldLineSize; j++){
                int fIdx = obj->originalFoldLineConnMap[idx1][idx2][j]->foldLineIdx;
                tempPaths[i].push_back(fIdx);
            }
        }
        
        clearGraph();
        
        int edgeSize = 0;
        for(size_t i=0; i< tempPaths.size () -1; i++){
            for(size_t j=0; j< tempPaths[i].size(); j++){
                for(size_t k=0; k< tempPaths[i+1].size(); k++){
                    edgeSize++;
                    createSingleEdgeM(tempPaths[i][j], tempPaths[i+1][k]);
                }
            }
        }

        PATHS.clear();
        foldLinePath.clear();
        for(size_t i=0; i< tempPaths.front().size(); i++){
            for(size_t j=0; j<tempPaths.back().size(); j++){
                
                findAllPaths(foldLinePath, tempPaths.front()[i], tempPaths.back()[j],  (int)obj->classifiedPatches.size(), edgeSize);
            }
        }
        
        for(size_t i=1; i< obj->paths[pIdx].size()-1 ; i++){
            int idx1 = obj->paths[pIdx][i];
            for(size_t j=0; j< foldLinePath.size(); j++){
                obj->foldLinePathsOfPatch[idx1].push_back(foldLinePath[j]);
            }
        }
        
    }
    
   /* for(size_t pIdx=0; pIdx<obj->foldLinePathsOfPatch.size(); pIdx++){
        cout << "patch:"<<(int)pIdx <<endl;
        for(size_t lIdx=0; lIdx< obj->foldLinePathsOfPatch[pIdx].size(); lIdx++){
            for(size_t i=0; i<obj->foldLinePathsOfPatch[pIdx][lIdx].size(); i++){
                cout << obj->foldLinePathsOfPatch[pIdx][lIdx][i] << " ";
            }
            cout << endl;
        }
        
    }*/
    

    return true;
}