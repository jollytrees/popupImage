//
//  functionDivideBasePatch.h
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#ifndef functionDivideBasePatch_h
#define functionDivideBasePatch_h
#include "functionFindContour.h"
#include "functionDrawPatch.h"

static void merge_Mat(cv::Mat &dst, cv::Mat img1, cv::Mat img2){
    
    int rows1 = img1.rows;
    int cols1 = img1.cols;
    int rows2 = img2.rows;
    int cols2 = img2.cols;
    dst = cvCreateMat(rows1, cols1 + cols2, img1.type());
    cv::Mat tmp = dst(cv::Rect(0, 0, cols1, rows1));
    img1.copyTo(tmp);
    tmp = dst(cv::Rect(cols1, 0, cols2, rows2));
    img2.copyTo(tmp);
}

static void dividPatch(cv::Mat &input, cv::Mat &leftMat, cv::Mat &rightMat, int x){
    
    cv::Mat left, left_black, right, right_black;
    cv::Rect region_of_interest = cv::Rect(0, 0, x, input.rows);
    left = input(region_of_interest).clone();
    left_black = left.clone();
    left_black.setTo(black);
    
    region_of_interest = cv::Rect(x, 0, input.cols - x, input.rows );
    right = input(region_of_interest).clone();
    right_black = right.clone();
    right_black.setTo(black);
    
    merge_Mat(leftMat, left, right_black);
    merge_Mat(rightMat, left_black, right);
    
}

static void divideBasePatch(popupObject *obj)
{
    cv::Mat assignedPatchMat(obj->initMatSize, CV_8UC3);
    assignedPatchMat.setTo(black);
    //drawBasePatch
    fillSinglePatch(obj->initPatches[obj->assignedPatch], assignedPatchMat);
    
    //divide patch by x
    cv::Mat leftMat, rightMat;
    dividPatch(assignedPatchMat, leftMat, rightMat, obj->centralXPosition);

    //after dividing, find new patches
    paths_type  leftContours;
    paths_type  rightContours;
    
    //find contours of divided patches
    findContourSimple(leftMat, leftContours);
    findContourSimple(rightMat, rightContours);
    
    //copy initPatch to dividedPatch
    obj->dividedPatches = obj->initPatches;
    
    //remove assigned patch
    obj->dividedPatches.erase(obj->dividedPatches.begin()+ obj->assignedPatch);
    
    //add new patches
    struct patches leftPatch;
    struct patches rightPatch;
    cv::Mat leftBinaryMat(obj->initMatSize, CV_8UC1);
    leftBinaryMat.setTo(0);
    cv::Mat rightBinaryMat(obj->initMatSize, CV_8UC1);
    rightBinaryMat.setTo(0);
    drawBinaryPatchByPath(leftContours, leftBinaryMat);
    drawBinaryPatchByPath(rightContours, rightBinaryMat);
    
    leftPatch.pchMat = leftBinaryMat;
    leftPatch.paths = leftContours;
    
    rightPatch.pchMat = rightBinaryMat;
    rightPatch.paths = rightContours;
    
    //push new patches to dividedPatches
    struct patches *lPch = new struct patches(leftPatch);
    struct patches *rPch = new struct patches(rightPatch);

    obj->dividedPatches.push_back(lPch);
    obj->dividedPatches.push_back(rPch);
    
    //assign base patch
    obj->backPatch = obj->dividedPatches.size() - 1;
    obj->floorPatch = obj->dividedPatches.size() - 2;
}


#endif /* functionDivideBasePatch_h */
