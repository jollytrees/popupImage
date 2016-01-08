//
//  popupObjFindPossiblePatchConnection.cpp
//  popupImage
//
//  Created by jollytrees on 11/17/15.
//
//

#include "popupObjFindPossiblePatchConnection.hpp"

static void findConnFoldLine(size_t patchIdx, popupObject *obj){
    
    
    vector<vector<int> > possiblePatchInterLine(obj->possiblePatchesOfPatch[patchIdx].size());
   
    //find inner fold line conn of patchIdx
    for(size_t lineIdx=0; lineIdx< obj->foldLine.size(); lineIdx++){
        
        if(obj->foldLine[lineIdx]->isOriginalType==false && obj->foldLine[lineIdx]->originalConnPatch[0]== (int)patchIdx){

           
            for(size_t pPchIdx=0; pPchIdx< obj->possiblePatchesOfPatch[patchIdx].size(); pPchIdx++){
                
                if(isLineAndPatchConn(*obj->possiblePatchesOfPatch[patchIdx][pPchIdx],
                                      *obj->foldLine[lineIdx], obj->initMatSize) ){
                    possiblePatchInterLine[pPchIdx].push_back((int)lineIdx);
                }
            }
        }
    }
    
    for(size_t pPchIdx=0; pPchIdx< possiblePatchInterLine.size(); pPchIdx++){
        
        obj->possiblePatches.push_back(obj->possiblePatchesOfPatch[patchIdx][pPchIdx]);
        if(patchIdx==obj->floorPatch) obj->floorPatch =  obj->possiblePatches.size()-1;
        if(patchIdx==obj->backPatch) obj->backPatch =  obj->possiblePatches.size()-1;
        
        int allPossibleIdx = obj->possiblePatches.size()-1;
    
        //one inserted line, more original line
        if(possiblePatchInterLine[pPchIdx].size() >= 0 ){
            
            for(size_t j=0; j< possiblePatchInterLine[pPchIdx].size(); j++){
                
                int insertLineIdx = possiblePatchInterLine[pPchIdx][j];
                obj->foldLine[insertLineIdx]->connPatch.push_back(allPossibleIdx);
            }

            for(size_t lineIdx=0; lineIdx< obj->foldLine.size(); lineIdx++){
                if(obj->foldLine[lineIdx]->isOriginalType==true &&
                   (obj->foldLine[lineIdx]->originalConnPatch[0]==(int)patchIdx || obj->foldLine[lineIdx]->originalConnPatch[1]==(int)patchIdx)){
                    
                    if(isLineAndPatchConn(*obj->possiblePatchesOfPatch[patchIdx][pPchIdx],
                                          *obj->foldLine[lineIdx], obj->initMatSize) ){
                        obj->foldLine[lineIdx]->connPatch.push_back(allPossibleIdx);
                    }
                }
            }
        }else{
            cout << "find possible fold line conn error" <<endl;
        }
    }
}


bool popupObjFindPossiblePatchConnection::execute(popupObject *obj)
{
    
    obj->originalBackPatch = obj->backPatch;
    obj->originalFloorPatch = obj->floorPatch;
    
    for(size_t i=0; i< obj->possiblePatchesOfPatch.size(); i++){
        findConnFoldLine(i, obj);
    }

    obj->possibleFoldLineConnMap.resize(obj->possiblePatches.size());
    for(size_t i=0; i< obj->possiblePatches.size(); i++){
        obj->possibleFoldLineConnMap[i].resize(obj->possiblePatches.size(), NULL);
    }
    
    for(size_t i=0; i< obj->foldLine.size(); i++){
        if(obj->foldLine[i]->connPatch.size() != 2) {
            cout << "foldline "<< i << " conn size!=2, size="<< obj->foldLine[i]->connPatch.size() << endl;
            for(int j=0; j< obj->foldLine[i]->connPatch.size() ; j++){
                cout << obj->foldLine[i]->connPatch[j] << " ";
            }
            cout << endl;
        }
        
        obj->possibleFoldLineConnMap[obj->foldLine[i]->connPatch[0]][obj->foldLine[i]->connPatch[1]] = obj->foldLine[i];
        obj->possibleFoldLineConnMap[obj->foldLine[i]->connPatch[1]][obj->foldLine[i]->connPatch[0]] = obj->foldLine[i];
    }

    return true;
}