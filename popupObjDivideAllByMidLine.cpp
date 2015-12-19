//
//  popupObjDivideAllByMidLine.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/8/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjDivideAllByMidLine.hpp"
#include "functionFindContour.h"

void merge_Mat(cv::Mat &dst, cv::Mat img1, cv::Mat img2){
    
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

void dividPatch(cv::Mat &input, cv::Mat &out1, cv::Mat &out2, int x){
    
    cv::Mat left, left_black, right, right_black;
    cv::Rect region_of_interest = cv::Rect(0, 0, x, input.rows);
    left = input(region_of_interest).clone();
    left_black = left.clone();
    left_black.setTo(cv::Scalar(0, 0 ,0));
    
    region_of_interest = cv::Rect(x, 0, input.cols - x, input.rows );
    right = input(region_of_interest).clone();
    right_black = right.clone();
    right_black.setTo(cv::Scalar(0, 0 ,0));
    
    merge_Mat(out1, left, right_black);
    merge_Mat(out2, left_black, right);
    
}

void pushPatches(std::vector< paths_type > &contours, std::vector< struct patches > &out_pchs, cv::Size &matSize){
    
    std::ostringstream oss;
    for(int i =0; i < contours.size(); i++){
        
        cv::Mat canvas(matSize, CV_8UC1);
        canvas.setTo(cv::Scalar(0));
        for(int j =0 ; j< contours[i].size(); j++){
            //draw contours
            if(j==0)
                cv::drawContours(canvas, contours[i], j, cv::Scalar(255), 3);
            else
                cv::drawContours(canvas, contours[i], j, cv::Scalar(0), 3);
        }
        struct patches pch;
        pch.pchMat = canvas.clone();
        pch.paths = contours[i];
        
        out_pchs.push_back(pch);
    }
}

bool popupObjDivideAllByMidLine::execute(popupObject *obj)
{
    
    for(int i =0 ;i < obj->initPatches.size(); i++){
        cv::Size matSize = obj->initMatSize;
        
        cv::Mat found_contour(matSize, CV_8UC3);
        found_contour.setTo(black);
        
        //draw contours
        for(int j = 0; j < obj->initPatches[i].paths.size(); j++){
            if(j==0)
                cv::drawContours(found_contour, obj->initPatches[i].paths, j, white, CV_FILLED);
            else
                cv::drawContours(found_contour, obj->initPatches[i].paths, j, black, CV_FILLED);
        }
        
        //divide patch by x
        cv::Mat out1, out2;
        dividPatch(found_contour, out1, out2, obj->centralXPosition);
        
        //after dividing, find new patches
        std::vector< paths_type > contours_1;
        std::vector< paths_type > contours_2;
        
        findContour(out1, contours_1);
        findContour(out2, contours_2);
        
        pushPatches(contours_1, obj->dividedPatches, obj->initMatSize);
        pushPatches(contours_2, obj->dividedPatches, obj->initMatSize);
        
    }

    return true;
}
