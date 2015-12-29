//
//  functionFindContourByPath.h
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#ifndef functionFindContourByPath_h
#define functionFindContourByPath_h

#include "functionFindContour.h"
static paths_type findContourByPath(cv::Size matSize, std::vector<std::vector<cv::Point> > &paths){
    
    cv::Mat canvas(matSize, CV_8UC3);
    canvas.setTo(black);
    
    for(int j = 0; j < paths.size(); j++){
        if(j==0)
            cv::drawContours(canvas, paths, j, white, CV_FILLED);
        else
            cv::drawContours(canvas, paths, j, black, CV_FILLED);
    }
    
    paths_type out_contour;
    findContourSimple(canvas, out_contour);
    
    
    return out_contour;
    
}

#endif /* functionFindContourByPath_h */
