//
//  popupObjectFindInsertedLine.cpp
//  popupImage
//
//  Created by jollytrees on 11/16/15.
//
//

#include "popupObjFindInsertedLine.hpp"
#include "functionFindContour.h"
#include "functionLineVecToLinePair.h"
#include "functionFindBlob.h"
#include "functionDrawPatch.h"

static bool returnInterLine(pair<cv::Point, cv::Point> linePair, cv::Mat greyBlobMat, pair<cv::Point, cv::Point> & returnLine){

    cv::Mat lineMat(greyBlobMat.size(), CV_8UC3);
    lineMat.setTo(0);
    cv::line(lineMat, linePair.first, linePair.second, white, 2);
    cv::Mat greyLineMat;
    cv::cvtColor(lineMat, greyLineMat, CV_BGR2GRAY);
    
    cv::Mat addImg(lineMat.size(), CV_8UC1);
    
    addImg = greyLineMat & greyBlobMat;
    
    foldLineType insertedLine;
    std::vector<cv::Vec4i> insertedLineVec;
    HoughLinesP( addImg, insertedLineVec, 1, CV_PI/180, 10, 5, 8 );
    
    if(insertedLineVec.size()>0){
        if(lineVecToLinePair(insertedLineVec, lineMat.size(), insertedLine.line)){
            returnLine = insertedLine.line;
            return true;
        }
    }
    std::cout << "insertedLineVec is empty" << std::endl;
    return false;

}


static bool isActiveLine(pair<cv::Point, cv::Point> linePair,  cv::Mat greyBlobMat){
    cv::Mat lineMat = greyBlobMat.clone();
    cv::line(lineMat, linePair.first, linePair.second, 0, 1);
    
    int count = countBlobs(lineMat);
    
    if(count == 2) return true;

    return false;
}

static bool testLine(pair<cv::Point, cv::Point> linePair, cv::Mat greyBlobMat, pair<cv::Point, cv::Point> &returnLine){

    pair<cv::Point, cv::Point> interLine;
    if(isActiveLine(linePair, greyBlobMat)){
        if(returnInterLine(linePair, greyBlobMat, interLine)){
            returnLine = interLine;
            return true;
        }
    }else{
    }
    return false;
}


static bool searchRightLine(pair<cv::Point, cv::Point> linePair, cv::Rect rect,  cv::Mat greyBlobMat, pair<cv::Point, cv::Point> &returnLine, int &step){
    
    int offset = 3;
    //--->
    for (int i = linePair.first.x +3 ;  i < rect.br().x; i = i+offset){
        
        pair<cv::Point, cv::Point> newLine;
        cv::Point up = cv::Point(i, rect.tl().y);
        cv::Point down = cv::Point(i, rect.br().y);
        newLine = make_pair(up, down);
    
        pair<cv::Point, cv::Point> rightLine;
        if(testLine(newLine, greyBlobMat, rightLine)){
            returnLine = rightLine;
            step = i - (linePair.first.x +3);
            return true;
        }
    }
    
    return false;
}

static bool searchLeftLine(pair<cv::Point, cv::Point> linePair, cv::Rect rect,  cv::Mat greyBlobMat, pair<cv::Point, cv::Point> &returnLine, int &step){
    
    int offset = 3;
    //<----
    for (int i = linePair.first.x -3 ;  i > rect.tl().x; i = i-offset){
        
        pair<cv::Point, cv::Point> newLine;
        cv::Point up = cv::Point(i, rect.tl().y);
        cv::Point down = cv::Point(i, rect.br().y);
        newLine = make_pair(up, down);
        
        pair<cv::Point, cv::Point> LeftLine;
        if(testLine(newLine, greyBlobMat, LeftLine)){
            returnLine = LeftLine;
            step = (linePair.first.x -3) - i;
            return true;
        }
    }
    
    return false;
}

static bool findActiveInsertedLine(pair<cv::Point, cv::Point> midLinePair, cv::Rect boundingRect,  cv::Mat greyBlobMat, pair<cv::Point, cv::Point> &returnLine){
    
    //test mid
    pair<cv::Point, cv::Point> midLine;
    if(testLine(midLinePair, greyBlobMat, midLine)){
        returnLine = midLine;
        return true;
    }
    //test left
    pair<cv::Point, cv::Point> leftLine;
    int stepLeft = 0;
    searchRightLine(midLinePair, boundingRect, greyBlobMat, leftLine, stepLeft);
    
    //test right
    pair<cv::Point, cv::Point> rightLine;
    int stepRight = 0;
    searchRightLine(midLinePair, boundingRect, greyBlobMat, rightLine, stepRight);
    
    if(stepLeft!=0 && stepRight!=0){
        stepLeft < stepRight ? returnLine = leftLine : returnLine = rightLine;

        return true;
    }
    
    return false;
}



static void findInsertedLine(int patchIdx, popupObject *obj){

    obj->insertedLineOfPatch.resize(obj->activeBlobOfPatch.size());

    for(int i=0; i< obj->activeBlobOfPatch[patchIdx].size(); i++){
        cv::Mat greyBlobMat;
        cv::cvtColor(obj->activeBlobOfPatch[patchIdx][i].blobMat, greyBlobMat, CV_BGR2GRAY);
        
        paths_type out_contour;
        findContourSimple(obj->activeBlobOfPatch[patchIdx][i].blobMat, out_contour);
        
        cv::Rect boundingRect = cv::boundingRect(out_contour[0]);
        cv::Point midTop = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.tl().y);
        cv::Point midBottom = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.br().y);
        
        pair<cv::Point, cv::Point> midLinePair;
        midLinePair = make_pair(midTop, midBottom);

        //find active line
        pair<cv::Point, cv::Point> activeLine;
        if(findActiveInsertedLine(midLinePair, boundingRect, greyBlobMat, activeLine)){
            
            foldLineType *line = new foldLineType();
            line->line = activeLine;
            line->setInsertedType();
            line->originalConnPatch.push_back(patchIdx);
            //line->connOriLeftFoldLine = obj->activeBlobOfPatch[patchIdx][i].leftLineIdx;
            //line->connOriRightFoldLine = obj->activeBlobOfPatch[patchIdx][i].rightLineIdx;
            line->oriPatchIdx = patchIdx;

            obj->insertedLineOfPatch[patchIdx].push_back(line);
            obj->foldLine.push_back(line);
            obj->foldLine.back()->foldLineIdx = obj->foldLine.size()-1;
            
            ostringstream oss;
            oss << patchIdx << "_" << i;
            std::string str = "../popupObjFindInsertedLine/blobMat"+oss.str()+".png";
            imwrite(str, greyBlobMat);
            
        }else{
            cout << patchIdx << " " << i<<" find inerted line : not active" << endl;
        }
    }
}

bool popupObjFindInsertedLine::execute(popupObject *obj)
{
    for(int i=0; i< obj->activeBlobOfPatch.size(); i++){
        findInsertedLine(i, obj);
    }
    

    
    return true;
}