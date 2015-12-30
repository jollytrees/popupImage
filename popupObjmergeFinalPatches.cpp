//
//  popupObjmergeFinalPatches.cpp
//  popupImage
//
//  Created by jollytrees on 12/28/15.
//
//

#include "popupObjmergeFinalPatches.hpp"
#include "functionFindBlob.h"
#include "functionFindContour.h"
#include "functionFindMatIntersection.h"

bool popupObjmergeFinalPatches::execute(popupObject *obj)
{
    
    /*
    for(size_t fIdx=0; fIdx<obj->foldLine.size(); fIdx++){
        cout << fIdx << " " << obj->foldLine[fIdx]->originalConnPatch[0] << " " <<
        obj->foldLine[fIdx]->originalConnPatch[1]<<endl;
    }*/

    obj->mergedPatchesOfPatch.resize(obj->insertedLineOfPatch.size());
    for(size_t pIdx=0; pIdx< obj->insertedLineOfPatch.size(); pIdx++){
        
        //extended line
        cv::Mat canvas(obj->initMatSize, CV_8UC3);
        canvas.setTo(black);
        drawContours(canvas, obj->classifiedPatches[pIdx]->paths, 0, white, CV_FILLED);
        
        //draw active inserted line
        for(size_t fIdx=0; fIdx<obj->insertedLineOfPatch[pIdx].size(); fIdx++){
            int foldLineIdx = obj->insertedLineOfPatch[pIdx][fIdx]->foldLineIdx;
            cv::Point s = obj->foldLine[foldLineIdx]->line.first;
            cv::Point t = obj->foldLine[foldLineIdx]->line.second;
            cv::line(canvas, s, t, white);
        }
        cv::Mat greyMat;
        cv::cvtColor(canvas, greyMat, CV_BGR2GRAY);
        
        //find blob
        vector<cv::Mat> blobMat;
        ConnectedBlobs(greyMat, blobMat);
        
        //find contour of each blob and push into possiblePatches
        for(size_t i=0; i< blobMat.size(); i++){
            
            paths_type t_out_contour;
            findContourSimple(blobMat[i], t_out_contour);
            cv::Mat d = blobMat[i].clone();
            cv::drawContours(d, t_out_contour, 0, 255, 2);
            
            paths_type out_contour;
            findContourSimple(d, out_contour);
            
            struct patches *pch = new struct patches();;
            pch->pchMat = blobMat[i].clone();
            pch->paths = out_contour;
            pch->patchIdx = obj->mergedPatches.size()-1;
            if(out_contour[0].size()>0){
                obj->mergedPatches.push_back(pch);
                obj->mergedPatchesOfPatch[pIdx].push_back(pch);
            }else{
                cout << "patch" << pIdx << "has unvisible blobs"<<endl;
            }
        }
        
        //find connection of active inserted line
        for(size_t fIdx=0; fIdx<obj->insertedLineOfPatch[pIdx].size(); fIdx++){
            int foldLineIdx = obj->insertedLineOfPatch[pIdx][fIdx]->foldLineIdx;
            
            vector< pair<int,int> > countVec;
            for(int bIdx=0; bIdx < obj->mergedPatchesOfPatch[pIdx].size(); bIdx++ ){

                int count = isLineAndPatchConnInt(*obj->mergedPatchesOfPatch[pIdx][bIdx],
                                   *obj->foldLine[foldLineIdx], obj->initMatSize );
                countVec.push_back(make_pair(bIdx, count));
            }
            
            sort(countVec.begin(), countVec.end(), compareLongest);
            obj->foldLine[foldLineIdx]->finalConnPatch.push_back(obj->mergedPatchesOfPatch[pIdx][countVec[0].first]->patchIdx);
            obj->foldLine[foldLineIdx]->finalConnPatch.push_back(obj->mergedPatchesOfPatch[pIdx][countVec[1].first]->patchIdx);
        }
        

    }
    
    //cout <<"done"<<endl;
    
    return true;
}