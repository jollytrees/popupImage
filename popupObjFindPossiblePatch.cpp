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


static int patchLineLeft(cv::Mat greyMat, std::vector<std::vector<cv::Point> > path, pair<cv::Point, cv::Point> &line, pair<int, int> &count){
    
    cv::Point up = line.first.y < line.second.y ? line.first :  line.second;
    int w = 10;
    int h = (int)sqrt(pow((double)line.first.y-line.second.y,2.0));
    cv::Rect rect_left = cv::Rect(up.x-w, up.y, w, h);
    cv::Rect rect_right = cv::Rect(up.x, up.y, w, h);
    
    cv::Mat ROI_left = greyMat(rect_left);
    cv::Mat ROI_right = greyMat(rect_right);
    
    count.first = cv::countNonZero(ROI_left);
    count.second = cv::countNonZero(ROI_right);
    
    if(count.first < count.second) return 0;
    else if(count.first == count.second) return 2;
    else if(count.first > count.second) return 1;
    
    return -1;
}

bool isActivePossibleBlob( cv::Mat greyMat, vector< pair<cv::Point, cv::Point> > &patchLine, int patchIdx, int blobIdx, pair<int, int> &maxIdxs,
                          vector< vector<int> >  &patchLineActive){
    
    int connectLeftSize = 0;
    int connectRightSize = 0;
    int maxLeftIdx = -1;
    int maxRightIdx = -1;
    int maxLeftArea = -1;
    int maxRightArea  = -1;
    for(size_t j=0; j<patchLine.size(); j++){
        
        cv::Mat foldLineMat(greyMat.size(), CV_8UC1);
        foldLineMat.setTo(0);
        cv::line(foldLineMat, patchLine[j].first, patchLine[j].second, 255, 8);
        
        cv::Mat lineCanvas = greyMat.clone();
        cv::line(lineCanvas, patchLine[j].first, patchLine[j].second, 125, 8);
        
        cv::Mat addImg(greyMat.size(), CV_8UC1);
        addImg = foldLineMat & greyMat;
        
        int count = cv::countNonZero(addImg);
        
        paths_type out_contour;
        findContourSimple(greyMat, out_contour);
        
        cv::Rect boundingRect = cv::boundingRect(out_contour[0]);
        cv::Point midTop = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.tl().y);
        cv::Point midBottom = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.br().y);
        
        pair<int, int> countArea;
        int isLeft = patchLineLeft(greyMat, out_contour, patchLine[j], countArea);
        

        if(count>0){

            if(isLeft==1){
                connectLeftSize++;
                if(count > maxLeftArea) maxLeftIdx = j;
            }else if(isLeft==0){
                connectRightSize++;
                if(count > maxRightArea) maxRightIdx = j;
            }else{
                connectLeftSize++;
                if(count > maxLeftArea) maxLeftIdx = j;
                connectRightSize++;
                if(count > maxRightArea) maxRightIdx = j;
                cout << "has equal size " <<"left: " << countArea.first <<" ,right: " <<countArea.second<< endl;
            }
            
            std::ostringstream oss;
            oss << patchIdx <<"_" <<blobIdx<< "_" << j ;
            std::string str = "../FindPossibleActiveBolb/active"+oss.str()+".png";
            imwrite(str, lineCanvas);
            
        }
        
    }
    
    if(connectLeftSize>0 && connectRightSize>0){
        patchLineActive[maxLeftIdx].push_back(blobIdx);
        patchLineActive[maxRightIdx].push_back(blobIdx);

        maxIdxs.first = maxLeftIdx;
        maxIdxs.second = maxRightIdx;
        
        return true;
    }
    
    if(connectLeftSize>1){
    
        return true;

    
    }else if(connectRightSize>1){
 
        return true;
    
    }
    return false;
    
}


static void findPossiblePatchesOfPatch(int patchIdx, popupObject *obj){
    
    if(obj->isBasePatch(patchIdx)){
        
        struct patches *pch = new struct patches();;
        pch->pchMat = obj->classifiedPatches[patchIdx]->pchMat;
        pch->paths = obj->classifiedPatches[patchIdx]->paths;
        obj->possiblePatchesOfPatch[patchIdx].push_back(pch);
        return;
    }
    
    //draw patch
    cv::Mat patchMat(obj->initMatSize, CV_8UC1);
    patchMat.setTo(0);
    cv::drawContours(patchMat, obj->classifiedPatches[patchIdx]->paths, 0, 255, CV_FILLED);
    
    //patch line -> for active blob
    vector< pair<cv::Point, cv::Point> > patchLine;
    for(size_t i=0; i<obj->originalFoldLineOfPatch[patchIdx].size(); i++){
        //cout << "isConnLine" << " " << obj->originalFoldLineOfPatch[patchIdx][i]->isConnLine << endl;
        patchLine.push_back(obj->originalFoldLineOfPatch[patchIdx][i]->line);
    }
    
    //draw inserted line of patches
    for(size_t i=0; i< obj->insertedLineOfPatch[patchIdx].size(); i++){
        cv::Point pf = obj->insertedLineOfPatch[patchIdx][i]->line.first;
        cv::Point ps = obj->insertedLineOfPatch[patchIdx][i]->line.second;
        patchLine.push_back(obj->insertedLineOfPatch[patchIdx][i]->line);
        cv::line(patchMat, pf, ps, 0);
    }
    
    vector< vector<int> >  patchLineActive;
    patchLineActive.resize(patchLine.size());
    
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
        
        pair<int, int> maxIdxs;
        

        
        if(isActivePossibleBlob( blobMatBinary[i], patchLine, patchIdx, i, maxIdxs, patchLineActive) && out_contour[0].size()>0){
        
            obj->possiblePatchesOfPatch[patchIdx].push_back(pch);

        }else{
            std::ostringstream oss;
            oss << patchIdx <<"_" <<i ;
            std::string str = "../FindPossibleActiveBolb/inactive"+oss.str()+".png";
            imwrite(str, blobMatBinary[i]);

            cout << "patch" << patchIdx << "has unvisible blobs"<<endl;
        }
    }
    
    
   /* if(patchIdx != 2)return;
    for(int i=0; i< patchLineActive.size(); i++){
        cout << "patchLineActive"<<" " <<i << " :";
        for(int j=0; j< patchLineActive[i].size(); j++){
            cout << patchLineActive[i][j] << " ";
        }cout << endl;
    
    }*/
    
}


bool popupObjFindPossiblePatch::execute(popupObject *obj)
{

    obj->possiblePatchesOfPatch.resize(obj->classifiedPatches.size());

    for(size_t i=0; i<obj->classifiedPatches.size(); i++){
        findPossiblePatchesOfPatch(i, obj);
    }
    return true;
}
