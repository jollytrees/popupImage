//
//  popupObjFindPossiblePatch.cpp
//  popupImage
//
//  Created by jollytrees on 11/17/15.
//
//

#include "popupObjFindPossiblePatch.hpp"
#include "functionFindBlob.h"
#include "functionFindContour.h"

static void findPossiblePatchesOfPatch(int patchIdx, popupObject *obj){
    
    //draw patch
    cv::Mat patchMat(obj->initMatSize, CV_8UC1);
    patchMat.setTo(0);
    cv::drawContours(patchMat, obj->classifiedPatches[patchIdx]->paths, 0, 255, CV_FILLED);
    
    //draw inserted line of patches
    for(size_t i=0; i< obj->insertedLineOfPatch[patchIdx].size(); i++){
        cv::Point pf = obj->insertedLineOfPatch[patchIdx][i]->line.first;
        cv::Point ps = obj->insertedLineOfPatch[patchIdx][i]->line.second;
        cv::line(patchMat, pf, ps, 0);
    }
    
    //find blobs
    vector<cv::Mat> blobMatBinary;
    ConnectedBlobsBinary(patchMat, blobMatBinary);

    //find contour of each blob and push into possiblePatches
    for(size_t i=0; i< blobMatBinary.size(); i++){

        paths_type t_out_contour;
        findContourSimple(blobMatBinary[i], t_out_contour);
        cv::Mat d = blobMatBinary[i].clone();
        cv::drawContours(d, t_out_contour, 0, 255, 1);

        paths_type out_contour;

        findContourSimple(d, out_contour);

        
        struct patches *pch = new struct patches();;
        pch->pchMat = blobMatBinary[i];
        pch->paths = out_contour;
        if(out_contour[0].size()>0){
            obj->possiblePatchesOfPatch[patchIdx].push_back(pch);
            
        }else{
            cout << "patch" << patchIdx << "has unvisible blobs"<<endl;
        }
    }
    
}


bool popupObjFindPossiblePatch::execute(popupObject *obj)
{

    obj->possiblePatchesOfPatch.resize(obj->classifiedPatches.size());

    for(size_t i=0; i<obj->classifiedPatches.size(); i++){
        findPossiblePatchesOfPatch(i, obj);
    }
    return true;
}
