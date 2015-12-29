//
//  popupObjFindPossiblePatchConnection.cpp
//  popupImage
//
//  Created by jollytrees on 11/17/15.
//
//

#include "popupObjFindPossiblePatchConnection.hpp"


static void convertInsertedLine(popupObject *obj){
    
    /*obj->originalFoldLineConnMap.resize(obj->boundaryFoldLineConnGroupMap.size());
    for(size_t i=0; i< obj->originalFoldLineConnMap.size(); i++){
        obj->originalFoldLineConnMap[i].resize(obj->boundaryFoldLineConnGroupMap.size());
    }
    
    obj->originalFoldLineOfPatch.resize(obj->boundaryFoldLineConnGroupMap.size());
    
    
    //upper triangle
    for(size_t i=0; i< obj->boundaryFoldLineConnGroupMap.size(); i++){
        
        for(size_t j=i+1; j<obj->boundaryFoldLineConnGroupMap[i].size(); j++){
            
            for(size_t k=0; k<obj->boundaryFoldLineConnGroupMap[i][j].size(); k++){
                foldLineType *line = new foldLineType(*obj->boundaryFoldLineConnGroupMap[i][j][k]);
                line->isOriginalFoldLine = true;
                line->originalConnPatch.push_back((int)i);
                line->originalConnPatch.push_back((int)j);
                    
                obj->foldLine.push_back(line);
                obj->foldLine.back()->foldLineIdx = obj->foldLine.size()-1;
                    
                obj->originalFoldLineConnMap[i][j].push_back(line);
                obj->originalFoldLineConnMap[j][i].push_back(line);
                
                obj->originalFoldLineOfPatch[i].push_back(line);
                obj->originalFoldLineOfPatch[j].push_back(line);
            }
        }
    }*/
    
    //push inserted line
    for(size_t i=0; i< obj->insertedLineOfPatch.size(); i++){
        for(size_t j=0; j<obj->insertedLineOfPatch[i].size(); j++){
            foldLineType *line = obj->insertedLineOfPatch[i][j];
            line->isOriginalFoldLine = false;
            line->originalConnPatch.push_back((int)i);
            obj->foldLine.push_back(line);
            obj->foldLine.back()->foldLineIdx = obj->foldLine.size()-1;

        }
    }
}

static void findConnFoldLine(size_t patchIdx, popupObject *obj){
    
    
    vector<vector<int> > possiblePatchInterLine(obj->possiblePatchesOfPatch[patchIdx].size());
   
    //find inner fold line conn of patchIdx
    for(size_t lineIdx=0; lineIdx< obj->foldLine.size(); lineIdx++){
        
        if(obj->foldLine[lineIdx]->isOriginalFoldLine==false && obj->foldLine[lineIdx]->originalConnPatch[0]== (int)patchIdx){

           
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
                if(obj->foldLine[lineIdx]->isOriginalFoldLine==true &&
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
    convertInsertedLine(obj);
    
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
       if(obj->foldLine[i]->connPatch.size() < 2) cout << "foldline "<< i << " conn size <2" << endl;
        obj->possibleFoldLineConnMap[obj->foldLine[i]->connPatch[0]][obj->foldLine[i]->connPatch[1]] = obj->foldLine[i];
        obj->possibleFoldLineConnMap[obj->foldLine[i]->connPatch[1]][obj->foldLine[i]->connPatch[0]] = obj->foldLine[i];
       // cout <<"ppf " <<obj->foldLine[i]->connPatch[0] << " : " << obj->foldLine[i]->connPatch[1] << " " << obj->foldLine[i]->foldLineIdx << endl;;
    }


    

   /* for(int patchIdx=0; patchIdx< obj->originalFoldLineOfPatch.size(); patchIdx++){
        
        cout << "idx: "<<patchIdx << " : ";
        for(size_t i=0; i< obj->originalFoldLineOfPatch[patchIdx].size(); i++){
            
            int idx = obj->originalFoldLineOfPatch[patchIdx][i]->foldLineIdx;
            // if(!obj->originalFoldLineOfPatch[patchIdx][i]->isConnLine){
            
            cout << idx << " ";
            //}
        }
        cout << endl;
    }*/
    

    
    
    return true;
}