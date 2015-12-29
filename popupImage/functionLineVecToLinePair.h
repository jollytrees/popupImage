//
//  functionLineVecToLinePair.h
//  Color_Segmentation
//
//  Created by jollytrees on 11/11/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef functionLineVecToLinePair_h
#define functionLineVecToLinePair_h
#include "functionFindContour.h"
#include <time.h>
static bool findContourBoundingBox(cv::Mat &lineVecMat, Rect& rect){
    paths_type lineContourGroup;
    findContourSimple(lineVecMat, lineContourGroup);

    if(lineContourGroup.size() > 0){
    
        std::vector<cv::Point> lineContour;
        for(size_t i =0; i < lineContourGroup.size(); i++)
            for(size_t j=0; j< lineContourGroup[i].size(); j++)
                lineContour.push_back(lineContourGroup[i][j]);
        
        cv::Rect boundingRect = cv::boundingRect(lineContour);

        rect =  boundingRect;
    
        return true;
    }
    
    return false;
}

static int countLineVec(std::vector<cv::Vec4i> &linesVec, cv::Size matSize){

    cv::Mat lineVecMat( matSize, CV_8UC3);

    lineVecMat.setTo(black);
    for( size_t i = 0; i < linesVec.size(); i++ )
    {
        cv::line( lineVecMat, Point(linesVec[i][0], linesVec[i][1]),
                 Point(linesVec[i][2], linesVec[i][3]), white, 4, 10 );
    }
    
    paths_type lineContourGroup;
    findContourSimple(lineVecMat, lineContourGroup);
    return lineContourGroup.size();

}

static bool lineVecToLinePair
(std::vector<cv::Vec4i> &linesVec, cv::Size matSize,    std::pair<cv::Point, cv::Point> &line
){
    
    cv::Mat lineVecMat( matSize, CV_8UC3);
    
    lineVecMat.setTo(black);
    for( size_t i = 0; i < linesVec.size(); i++ )
    {
        cv::line( lineVecMat, Point(linesVec[i][0], linesVec[i][1]),
                 Point(linesVec[i][2], linesVec[i][3]), white, 4, 10 );
    }
    
    cv::Rect boundingRect;
    if(findContourBoundingBox(lineVecMat, boundingRect)){
        cv::Point _midTop = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.tl().y);
        cv::Point _midBottom = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.br().y);
        
        std::pair<cv::Point, cv::Point> linePair = std::make_pair(_midTop, _midBottom);
        
        line = linePair;
        return true;
    
    }
    
    return false;
    
    
}

#endif /* functionLineVecToLinePair_h */
