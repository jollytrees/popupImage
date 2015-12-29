//
//  popupObject.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/12/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include <stdio.h>
#include "popupObject.h"

using namespace std;
#include "functionInitPatch.h"
#include "functionFindContourByPath.h"
#include "functionDivideBasePatch.h"
#include "functionClickPatch.h"

#include <math.h>
#define NANOSVG_ALL_COLOR_KEYWORDS  // Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION      // Expands implementation
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#include <sstream>
#include <iostream>

void popupObject::assignDividedPatches(){
    divideBasePatch(this);
}

int popupObject::clickClassifiedPatch(cv::Point p){
    
    return clickPatch(classifiedPatches, p);
}

int popupObject::clickPossiblePatch(cv::Point p){
    
    return clickPatch(possiblePatches, p);
}

void popupObject::assignBasePatch(cv::Point p){
    
    assignedPatch = clickPatch(initPatches, p);
}

bool popupObject::isBasePatch(int idx){
    if(idx == floorPatch|| idx == backPatch) return true;
    return false;
}

bool popupObject::isOriginalBasePatch(int idx){
    if(idx == originalFloorPatch|| idx == originalBackPatch) return true;
    return false;
}

void popupObject::initPatch(std::string fileName){
    
    isShowPatches = false;
    isShowFoldlines = false;
    isShowOriginalPatches = false;
    scale = 1;

    std::ostringstream oss;
    
    //patches
    unsigned int r = 0;
    unsigned int g = 0;
    unsigned int b = 0;
    
    // Load
    struct NSVGimage* image;
    image = nsvgParseFromFile(fileName.c_str(), "px", 96);
    printf("size: %f x %f\n", image->width, image->height);
    
    initMatSize = cv::Size(image->width, image->height);
    
    cv::Mat contourImgB(initMatSize, CV_8UC1);
    cv::Mat allContourImg(initMatSize, CV_8UC3);
    allContourImg.setTo(white);
    
    // Use...
    int shapeSize = 0;
    
    for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {
        
        struct patches _pch;
        vector<vector<cv::Point> > _paths;
        
        //for output name
        shapeSize++;
        oss.str("");
        oss << shapeSize-1;
        
        //imageBez.setTo(cv::Scalar(255,255,255));
        contourImgB.setTo(0);
        b = (shape->fill.color >> 16)& 0xff;
        g = (shape->fill.color >> 8)& 0xff;
        r = (shape->fill.color >> 0)& 0xff;
        
        for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {
            vector<cv::Point> _contour;
            
            int size = (path->npts-2)/3;
            //for (int i = 0; i < path->npts-1; i += 3) {
            for(int i=size*3; i>=0; i-=3){
                float* bezier4points = &path->pts[i*2];
                drawBezierCurve(contourImgB, allContourImg, bezier4points , _contour);
            }
            
            _paths.push_back(_contour);
        }
        
        reverse(_paths.begin(), _paths.end());
        _pch.pchMat = contourImgB.clone();
        _pch.paths = findContourByPath( contourImgB.size(), _paths);
        struct patches *pch = new struct patches(_pch);;

        if(_pch.paths.size() > 0) initPatches.push_back(pch);
        
        string str = "./patches/ALL"+oss.str()+".png";
        cv::imwrite(str.c_str(),allContourImg);
        str = "./patches/label"+oss.str()+".png";
        cv::imwrite(str.c_str(), contourImgB);
        
    }//end patches

}