//
//  functionDrawPatch.h
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#ifndef functionDrawPatch_h
#define functionDrawPatch_h

#include <opencv/cv.h>

static void drawBinaryPatchByPath(paths_type &paths, cv::Mat &outMat){
    for(int i = 0; i < paths.size(); i++){
        if(i==0)
            cv::drawContours(outMat, paths, i, 255, 2);
        else
            cv::drawContours(outMat, paths, i, 255, 2);
    }
}

static void fillSinglePatch(struct patches *patch, cv::Mat &outMat){
    for(int i = 0; i < patch->paths.size(); i++){
        if(i==0)
            cv::drawContours(outMat, patch->paths, i, white, CV_FILLED);
        else
            cv::drawContours(outMat, patch->paths, i, black, CV_FILLED);
    }
}

static void drawSinglePatch(struct patches *patch, cv::Mat &outMat){
    for(int i = 0; i < patch->paths.size(); i++){
        if(i==0)
            cv::drawContours(outMat, patch->paths, i, white, 3);
        //else
            //cv::drawContours(outMat, patch.paths, i, white, 2);
    }
}

static cv::Mat drawPatch(std::vector< struct patches* > input_patches, cv::Size &matSize){
    cv::Mat canvas(matSize, CV_8UC3);
    canvas.setTo(black);
    for(int i =0 ;i < input_patches.size(); i++){
        drawSinglePatch(input_patches[i], canvas);
    }
    return canvas;
}

static cv::Mat drawPatchWOClearMat(std::vector< struct patches* > input_patches, cv::Mat &canvas){
   // canvas.setTo(black);
    for(int i =0 ;i < input_patches.size(); i++){
        
        drawSinglePatch(input_patches[i], canvas);
    }
    return canvas;
}

#endif /* functionDrawPatch_h */
