#ifndef fuctionFindMatIntersection_h
#define fuctionFindMatIntersection_h

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv/cv.h>

static void checkType(cv::Mat &input){
    if(input.type() == CV_8UC3) input.convertTo(input, CV_8UC1);
}

static int findMatIntersectionRInt( cv::Mat input1,  cv::Mat input2, cv::Mat &output){
    
    cv::Mat addImg(input1.size(), CV_8UC1);
    checkType(input1);
    checkType(input2);
    
    addImg = input1 & input2;
    
    int count = cv::countNonZero(addImg);
    
    //std::cout << count << std::endl;
    if(count>0){
        output = addImg.clone();
        return count;
    }
    
    return 0;
    
}

static int findMatIntersectionInt( cv::Mat input1,  cv::Mat input2, cv::Mat &output){
    
    cv::Mat addImg(input1.size(), CV_8UC1);
    checkType(input1);
    checkType(input2);
    
    addImg = input1 & input2;
    
    int count = cv::countNonZero(addImg);
    
    if(count>3){
        output = addImg.clone();
        return count;
    }
    
    return count;
    
}

static bool findMatIntersection( cv::Mat input1,  cv::Mat input2, cv::Mat &output){
    
    cv::Mat addImg(input1.size(), CV_8UC1);
    checkType(input1);
    checkType(input2);
    
    addImg = input1 & input2;
    
    int count = cv::countNonZero(addImg);
    
    if(count>3){
        output = addImg.clone();
        return true;
    }
    
    return false;
    
}

static bool isLineAndMatConn(foldLineType &line, cv::Mat &inputMat, std::vector<cv::Vec4i> &lineVec){
    cv::Mat lineImg(inputMat.size(), CV_8UC1);
    cv::Mat addImg(inputMat.size(), CV_8UC1);
    lineImg.setTo(0);
    addImg.setTo(0);
    
    //line
    cv::line(lineImg, line.line.first, line.line.second, 255, 3);
    
    if(findMatIntersection(lineImg, inputMat, addImg)){
        lineVec.clear();
        HoughLinesP( addImg, lineVec, 1, CV_PI/180, 3 );
        if(lineVec.size()>0)
            return true;
    }
    return false;
    
}

static int isLineAndPatchConnInt(struct patches &pch, foldLineType &line, cv::Size &matSize){
    cv::Mat lineImg(matSize, CV_8UC1);
    cv::Mat patchImg(matSize, CV_8UC1);
    cv::Mat addImg(matSize, CV_8UC1);
    lineImg.setTo(0);
    patchImg.setTo(0);
    addImg.setTo(0);
    
    //line
    cv::line(lineImg, line.line.first, line.line.second, 255, 8);
    
    //patch
    cv::drawContours(patchImg, pch.paths, 0, 255, CV_FILLED);
    
    
    int isConn = findMatIntersectionInt(lineImg, patchImg, addImg);
    //if(isConn) imshow("aa", addImg);
    
    return isConn;
    
}

static bool isLineAndPatchConn(struct patches &pch, foldLineType &line, cv::Size &matSize){
    cv::Mat lineImg(matSize, CV_8UC1);
    cv::Mat patchImg(matSize, CV_8UC1);
    cv::Mat addImg(matSize, CV_8UC1);
    lineImg.setTo(0);
    patchImg.setTo(0);
    addImg.setTo(0);
    
    //line
    cv::line(lineImg, line.line.first, line.line.second, 255, 8);
    
    //patch
    cv::drawContours(patchImg, pch.paths, 0, 255, CV_FILLED);
    
    
    bool isConn = findMatIntersection(lineImg, patchImg, addImg);
    //if(isConn) imshow("aa", addImg);
    
    return isConn;
    
}


#endif /* fuctionFindMatIntersection_h */