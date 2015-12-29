//
//  functionFindContour.h
//  Color_Segmentation
//
//  Created by jollytrees on 11/8/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef functionFindContour_h
#define functionFindContour_h
#include "popupObject.h"

using namespace cv;

static void uniqueLength(std::vector<cv::Point> &input_contour, std::vector<cv::Point> &output_contour)
{
    int unique_length = 4; ///Define the sample distance of original contour.
    std::vector<cv::Point> _output_contour;
    _output_contour.push_back(input_contour[0]);
    int pre_point = 0;
    int now_point = 1;
    while(now_point < input_contour.size())
    {
        cv::Point pre = cv::Point(input_contour[pre_point].x, input_contour[pre_point].y);
        cv::Point now = cv::Point(input_contour[now_point].x, input_contour[now_point].y);
        cv::Point vec = pre - now;
        double vec_norm = cv::norm(vec);
        if (vec_norm > unique_length)
        {
            _output_contour.push_back(input_contour[now_point]);
            pre_point = now_point;
            now_point = now_point + 1;
        }
        else
        {
            now_point++;
        }
    }
    output_contour = _output_contour;
}

static void findInner(vector<vector<int> > &overlapping, paths_type &contours, cv::Mat &src ){
    
    overlapping.resize(contours.size());
    for(int i =0; i < contours.size(); i++){
        overlapping[i].resize(contours.size());
    }
    
    std::ostringstream oss;
    
    /*
     for(int i =0; i < contours.size(); i++){
     oss.str("");
     oss<<i;
     cv::Mat matC(src.rows ,src.cols, CV_8UC1);
     matC.setTo(0);
     cv::drawContours(matC, contours, i, 100, CV_FILLED);
     
     
     string str = "./patches/c" + oss.str();
     imwrite(str + ".png", matC);
     
     }*/
    
    for(int i =0; i < contours.size(); i++){
        
        for( int j=i+1; j< contours.size(); j++){
            
            cv::Mat addImg(src.rows ,src.cols, CV_8UC1);
            cv::Mat matI(src.rows ,src.cols, CV_8UC1);
            cv::Mat matJ(src.rows ,src.cols, CV_8UC1);
            addImg.setTo(0);
            matI.setTo(0);
            matJ.setTo(0);
            cv::drawContours(matI, contours, i, 100, CV_FILLED);
            cv::drawContours(matJ, contours, j, 100, CV_FILLED);
            
            addImg = matI & matJ ;
            
            int count = cv::countNonZero(addImg);
            if(count!=0){
                overlapping[i][j] = 1;
                overlapping[j][i] = 1;
            }else{
                overlapping[i][j] = 0;
                overlapping[j][i] = 0;
            }
            
        }
    }
    
    
    
}

static void testIncludeSize(paths_type &contours, paths_type &return_contours){

    
    paths_type out_contour;
    
    vector<vector< int > > elemSize(contours.size());
    
    for(int i=0; i< elemSize.size(); i++){
        
        for( int j=0; j < elemSize.size(); j++){
        
            if(i==j) continue;
            int dist = pointPolygonTest( contours[j], contours[i][0], true );
            
            if(dist>=0){
                elemSize[j].push_back(i);
            
            }
        
        
        }
       
    
    }
    
    std::vector<std::pair<int, int> > contourLength;

    for( int i =0; i < elemSize.size(); i++){
        contourLength.push_back(std::make_pair(i, elemSize[i].size()));
    }
    
    sort(contourLength.begin(), contourLength.end(), compareLongest);
    
    paths_type contours_out;
    for(int i=0; i< contourLength.size(); i++){
        contours_out.push_back(contours[contourLength[i].first]);
    }
    
    
    return_contours = contours_out;


}

static void findContourSimple(cv::Mat src, paths_type &out_contour){
    
    out_contour.clear();
    cv::Mat src_gray;
    
    /// Convert image to gray and blur it
    if(src.type()==CV_8UC3) cvtColor( src, src_gray, CV_BGR2GRAY );
    else src_gray = src.clone();
    //blur( src_gray, src_gray, cv::Size(3,3) );
    
    paths_type contours; /* temporary results from the cv::findContours */
    cv::Mat oMat = src_gray.clone();
    
    /* find contours */
    cv::findContours(src_gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE); ///Finds contours in a binary image.
    
    for( size_t i =0; i < contours.size(); i++)
        uniqueLength(contours[i], contours[i]);
    
    testIncludeSize(contours, out_contour);
    
}


static void findContour(cv::Mat src, vector< paths_type > &out_contour){
    
    out_contour.clear();
    cv::Mat src_gray;
    
    /// Convert image to gray and blur it
    if(src.type()==CV_8UC3) cvtColor( src, src_gray, CV_BGR2GRAY );
    //blur( src_gray, src_gray, cv::Size(3,3) );
    
    paths_type contours; /* temporary results from the cv::findContours */
    cv::Mat oMat = src_gray.clone();
    
    /* find contours */
    cv::findContours(src_gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE); ///Finds contours in a binary image.
    
    /* find Inner contour */
    vector<vector<int> > overlapping;
    findInner(overlapping, contours, src);
    vector<int> useMap(overlapping.size(), 0);
    
    for( int i =0; i < contours.size(); i++)
        uniqueLength(contours[i], contours[i]);
    
    for (int i =0; i < overlapping.size(); i++)
    {
        if(useMap[i]==1) continue;
        int count = 0;
        paths_type group_path;
        for( int j=i+1; j< overlapping.size(); j++){
            
            if(useMap[j]==1) continue;
            if(overlapping[i][j]==1){
                
                useMap[i] = useMap[j] = 1;
                if(contours[i].size() > contours[j].size()){
                    group_path.push_back(contours[i]);
                    group_path.push_back(contours[j]);
                }else{
                    group_path.push_back(contours[j]);
                    group_path.push_back(contours[i]);
                }
                count++;
                
            }
        }
        
        if(count==0){
            group_path.push_back(contours[i]);
        }
        
        out_contour.push_back(group_path);
        
    }
}


#endif /* functionFindContour_h */
