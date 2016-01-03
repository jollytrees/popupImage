//
//  drawBoundaryLine.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/14/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "drawBoundaryLine.hpp"
#include "functionOutputMat.h"

#include <opencv2/imgproc/imgproc.hpp>

using namespace std;

bool drawBoundaryLine::execute(popupObject *obj)
{
    cv::Mat canvas(obj->initMatSize, CV_8UC3);
    canvas.setTo(black);
    
    //draw all patch
    for(int i=0; i < obj->classifiedPatches.size(); i++){
        
        vector<vector<cv::Point> > &contours = obj->classifiedPatches[i]->paths;
        
        /// draw contours
        for( int j = 0; j< contours.size(); j++ )
        {
            cv::drawContours( canvas, contours, j, blue, 2, 8 );
        }

        outputMat(i, "drawBoundaryLine", "boundaryLine", canvas);

    }
    
    //outputMat(0, "drawBoundaryLine", "boundaryLine", canvas);
    
    return true;
}
