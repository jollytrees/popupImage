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


bool isActivePossibleBlob( cv::Mat greyMat, vector< foldLineType* > &patchLine, int patchIdx, int blobIdx, vector<int> &orifIdx){
    
    for(size_t j=0; j<patchLine.size(); j++){
        
        cv::Mat foldLineMat(greyMat.size(), CV_8UC1);
        foldLineMat.setTo(0);
        
        std::pair<cv::Point, cv::Point> line;

        if(patchLine[j]->isOriginalType){
            for(int k =0; k< patchLine[j]->extendLines.size(); k++){
                
                if(patchLine[j]->extendLines[k].patchIdx == patchIdx){
                    line = patchLine[j]->extendLines[k].line;
                }

            }
        }else{
            line = patchLine[j]->line;

        }
        
        
        cv::line(foldLineMat, line.first, line.second, 255, 8);
        
        cv::Mat lineCanvas = greyMat.clone();
        cv::line(lineCanvas,line.first, line.second, 125, 8);
        
        cv::Mat addImg(greyMat.size(), CV_8UC1);
        addImg = foldLineMat & greyMat;
        
        int count = cv::countNonZero(addImg);
        
        paths_type out_contour;
        findContourSimple(greyMat, out_contour);
        
        cv::Rect boundingRect = cv::boundingRect(out_contour[0]);
        cv::Point midTop = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.tl().y);
        cv::Point midBottom = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.br().y);

        if(count <= 0){
            std::ostringstream oss;
            oss << patchIdx <<"_" <<blobIdx<< "_" << j ;
            std::string str = "../FindPossibleActiveBolb/inActive_"+oss.str()+".png";
            imwrite(str, lineCanvas);
        }
        
        if(count>0){
            orifIdx.push_back(patchLine[j]->foldLineIdx);
            
            std::ostringstream oss;
            oss << patchIdx <<"_" <<blobIdx<< "_" << j ;
            std::string str = "../FindPossibleActiveBolb/active"+oss.str()+".png";
            imwrite(str, lineCanvas);
            
        }
        
    }
    if(orifIdx.size()>0) return true;

    return false;
    
}


static void findPossiblePatchesOfPatch(int patchIdx, popupObject *obj){
    
    if(obj->isBasePatch(patchIdx)){
       /* struct patches *pch = new struct patches();;
        pch->pchMat = obj->classifiedPatches[patchIdx]->pchMat;
        pch->paths = obj->classifiedPatches[patchIdx]->paths;
        obj->possiblePatchesOfPatch[patchIdx].push_back(pch);
        */
       // obj->possiblePatches.push_back(pch);
        
        
        struct patches *pch = new struct patches();;
        pch->pchMat = obj->classifiedPatches[patchIdx]->pchMat;
        pch->paths = obj->classifiedPatches[patchIdx]->paths;
        pch->oriPatchIdx = patchIdx;
        obj->possiblePatchesOfPatch[patchIdx].push_back(pch);
        obj->possiblePatches.push_back(pch);
        pch->patchIdx = obj->possiblePatches.size()-1;
        
        if(patchIdx==obj->floorPatch) obj->floorPatch =  obj->possiblePatches.size()-1;
        if(patchIdx==obj->backPatch) obj->backPatch =  obj->possiblePatches.size()-1;

        
        for(size_t i=0; i<obj->originalFoldLineOfPatch[patchIdx].size(); i++){
            int fIdx = obj->originalFoldLineOfPatch[patchIdx][i]->foldLineIdx;
            pch->addLine(obj->foldLine[fIdx]);
            obj->foldLine[fIdx]->connPatch.push_back(pch->patchIdx);
            obj->foldLine[fIdx]->possiblePatchIdx.push_back(make_pair(pch->patchIdx, pch->oriPatchIdx));
        }
        
        return;

    }
    
    //draw patch
    cv::Mat patchMat(obj->initMatSize, CV_8UC1);
    patchMat.setTo(0);
    cv::drawContours(patchMat, obj->classifiedPatches[patchIdx]->paths, 0, 255, CV_FILLED);
    
    //patch line -> for active blob
    vector< foldLineType* > patchLine;
    for(size_t i=0; i<obj->originalFoldLineOfPatch[patchIdx].size(); i++){
        patchLine.push_back(obj->originalFoldLineOfPatch[patchIdx][i]);
    }
    
    //draw inserted line of patches
    for(size_t i=0; i< obj->insertedLineOfPatch[patchIdx].size(); i++){
        cv::Point pf = obj->insertedLineOfPatch[patchIdx][i]->line.first;
        cv::Point ps = obj->insertedLineOfPatch[patchIdx][i]->line.second;
        cv::line(patchMat, pf, ps, 0);
        patchLine.push_back(obj->insertedLineOfPatch[patchIdx][i]);
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
        pch->oriPatchIdx = patchIdx;
        
        vector<int> fIdx;
        if(isActivePossibleBlob( blobMatBinary[i], patchLine, patchIdx, i, fIdx) && out_contour[0].size()>0){
            
            obj->possiblePatchesOfPatch[patchIdx].push_back(pch);
            obj->possiblePatches.push_back(pch);
            pch->patchIdx = obj->possiblePatches.size()-1;
            
            for(int k=0; k< fIdx.size();k++){
                pch->addLine(obj->foldLine[fIdx[k]]);
                obj->foldLine[fIdx[k]]->connPatch.push_back(pch->patchIdx);
                obj->foldLine[fIdx[k]]->possiblePatchIdx.push_back(make_pair(pch->patchIdx, pch->oriPatchIdx));
            }

        }else{
            std::ostringstream oss;
            oss << patchIdx <<"_" <<i <<"_p"<<pch->patchIdx;
            std::string str = "../FindPossibleActiveBolb/inactive"+oss.str()+".png";
            imwrite(str, blobMatBinary[i]);

            cout << "patch " << patchIdx << " has unvisible blobs"<<endl;
        }
    }
  
}

bool popupObjFindPossiblePatch::execute(popupObject *obj)
{

    obj->originalBackPatch = obj->backPatch;
    obj->originalFloorPatch = obj->floorPatch;
    
    obj->possiblePatchesOfPatch.resize(obj->classifiedPatches.size());

    for(size_t i=0; i<obj->classifiedPatches.size(); i++){
        findPossiblePatchesOfPatch(i, obj);
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
