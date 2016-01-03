//
//  popupObjExtendFoldLineAndFindActiveBolb.cpp
//  Color_Segmentation
//
//  Created by jollytrees on 11/12/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#include "popupObjExtendFoldLineAndFindActiveBolb.hpp"
#include "functionFindBlob.h"
#include "functionFindContour.h"
static void  extendFoldline(popupObject *obj ,int pIdx1, int pIdx2,int lineIdx, cv::Mat canvas, vector< pair<cv::Point, cv::Point> > &patchExtendedLine){

    if(obj->isBasePatch(pIdx1)) return;

    foldLineType *line = obj->boundaryFoldLineConnGroupMap[pIdx1][pIdx2][lineIdx];
    cv::Point top, bottom;
    if(line->line.first.y < line->line.second.y){
        top = line->line.first;
        bottom = line->line.second;
    }else{
        top = line->line.second;
        bottom = line->line.first;
    }
    
    cv::line( canvas, line->line.first, line->line.second, black, 1, 8 );
    cv::Point extendTop, extendBottom;
        
    //draw line top
    for(int i=0; i<obj->initMatSize.height; i++){
        
        int newYPosition = top.y - i;
        if(newYPosition < 0) break;
            
        cv::Point testPoint = cv::Point(top.x, newYPosition);
        float dist = cv::pointPolygonTest( obj->classifiedPatches[pIdx1]->paths[0], testPoint, true );
            
        //break
        if(dist < 0) {
            cv::circle(canvas, testPoint, 0, black, 1 );
            extendTop = testPoint;
            
            if(canvas.at<cv::Vec3b>(testPoint.y,testPoint.x)[0] == 0&&
               canvas.at<cv::Vec3b>(testPoint.y,testPoint.x)[1] == 0&&
               canvas.at<cv::Vec3b>(testPoint.y,testPoint.x)[2] ==0){
                 break;
            }
            
           
        }else if(dist > 0){
            //in
            cv::circle(canvas, testPoint, 0, black, 1 );
                
        }else{
            cv::circle(canvas, testPoint, 0, black, 1 );
                
        }
            
    }
        
    //draw line down
    for(int i=0; i<obj->initMatSize.height; i++){
            
        int newYPosition = bottom.y + i;
        if(newYPosition > obj->initMatSize.height) break;
        cv::Point testPoint = cv::Point(top.x, newYPosition);
        float dist = cv::pointPolygonTest( obj->classifiedPatches[pIdx1]->paths[0], testPoint, true );
            
        if(dist < 0) {
                
            extendBottom = testPoint;
            cv::circle(canvas, testPoint, 0, black, 1 );
            if(canvas.at<cv::Vec3b>(testPoint.y,testPoint.x)[0] == 0&&
               canvas.at<cv::Vec3b>(testPoint.y,testPoint.x)[1] == 0&&
               canvas.at<cv::Vec3b>(testPoint.y,testPoint.x)[2] ==0){
                break;
            }
        
        }else if(dist > 0){
            cv::circle(canvas, testPoint, 0, black, 1 );
                
        }else{
            //in
            cv::circle(canvas, testPoint, 0, black, 1 );
                
        }
            
    }

    patchExtendedLine.push_back(make_pair(extendTop, extendBottom));

    
}

bool isActiveBlob( cv::Mat greyMat, vector< pair<cv::Point, cv::Point> > &patchExtendedLine, int patchIdx, int blobIdx, pair<int, int> &maxIdxs){
    
    int connectLeftSize = 0;
    int connectRightSize = 0;
    int maxLeftIdx = -1;
    int maxRightIdx = -1;
    int maxLeftArea = -1;
    int maxRightArea  = -1;
    for(size_t j=0; j<patchExtendedLine.size(); j++){
        
        cv::Mat foldLineMat(greyMat.size(), CV_8UC1);
        foldLineMat.setTo(0);
        cv::line(foldLineMat, patchExtendedLine[j].first, patchExtendedLine[j].second, 255, 2);

        cv::Mat lineCanvas = greyMat.clone();
        cv::line(lineCanvas, patchExtendedLine[j].first, patchExtendedLine[j].second, 125, 2);
        
        cv::Mat addImg(greyMat.size(), CV_8UC1);
        addImg = foldLineMat & greyMat;
        
        int count = cv::countNonZero(addImg);

        paths_type out_contour;
        findContourSimple(greyMat, out_contour);
        
        cv::Rect boundingRect = cv::boundingRect(out_contour[0]);
        cv::Point midTop = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.tl().y);
        cv::Point midBottom = cv::Point( (boundingRect.tl().x + boundingRect.br().x)/2, boundingRect.br().y);
        
        pair<cv::Point, cv::Point> midLinePair;
        midLinePair = make_pair(midTop, midBottom);

   
        if(count>0){
            
            if(patchExtendedLine[j].first.x > midTop.x){
                connectLeftSize++;
                if(count > maxLeftArea) maxLeftIdx = j;
            }else{
                connectRightSize++;
                if(count > maxRightArea) maxRightIdx = j;

            }
            
            std::ostringstream oss;
            oss << patchIdx <<"_" <<blobIdx<< "_" << j ;
            std::string str = "./ExtendFoldLineAndFindActiveBolb/active"+oss.str()+".png";
            imwrite(str, lineCanvas);
            
        }


    }
    
    if(connectLeftSize>0 && connectRightSize>0){
        //cout << "conn " << connectLeftSize <<" " << connectRightSize << endl;
        maxIdxs.first = maxLeftIdx;
        maxIdxs.second = maxRightIdx;

        return true;
    }
    

    
    return false;

}

bool popupObjExtendFoldLineAndFindActiveBolb::execute(popupObject *obj)
{
    for(size_t i=0; i< obj->boundaryFoldLineConnGroupMap.size(); i++){
        
        
        //extended line
        cv::Mat canvas(obj->initMatSize, CV_8UC3);
        canvas.setTo(black);
        //draw patch idx
        drawContours( canvas, obj->classifiedPatches[i]->paths, 0, white, CV_FILLED);
        vector< pair<cv::Point, cv::Point> > patchExtendedLine;
        vector< int > patchExtendedLineToFoldLine;

        
        for(size_t j=0; j<obj->originalFoldLineConnMap[i].size(); j++){
            for(size_t k=0; k<obj->originalFoldLineConnMap[i][j].size(); k++){
               // cout << i <<" " <<j << " extend " << obj->originalFoldLineConnMap[i][j][k]->foldLineIdx<<endl;;
                patchExtendedLineToFoldLine.push_back(obj->originalFoldLineConnMap[i][j][k]->foldLineIdx);
                extendFoldline(obj , (int)i, (int)j, (int)k, canvas, patchExtendedLine);

            }

        }
        std::ostringstream oss;
        oss << i;
        std::string str = "../ExtendFoldLineAndFindActiveBolb/"+oss.str()+".png";
        imwrite(str, canvas);
        
        //find blob
        cv::Mat greyMat;
        cv::cvtColor(canvas, greyMat, CV_BGR2GRAY);
        
        vector<cv::Mat> blobMat;
        ConnectedBlobs(greyMat, blobMat);
        
        vector<cv::Mat> activePatchBlobMat;
        vector<pair<int,int> > activePatchFoldLine;
        vector< pair<lineType, lineType> > activeExtentedLine;


        for(size_t j=0; j< blobMat.size(); j++){
        
            cv::Mat greyMat;
            cv::cvtColor(blobMat[j], greyMat, CV_BGR2GRAY);
            int count = cv::countNonZero(greyMat);
            oss.str("");
            oss << i << " " <<j;
            if(count > 0){
                pair<int, int> maxIdx;
                bool isActive = isActiveBlob( greyMat, patchExtendedLine, (int)i, (int)j, maxIdx);
                if(isActive){
                    
                    std::string str = "../ExtendFoldLineAndFindActiveBolb/blobMat"+oss.str()+".png";
                    imwrite(str, blobMat[j]);
                    int fidx1 = patchExtendedLineToFoldLine[maxIdx.first];
                    int fidx2 = patchExtendedLineToFoldLine[maxIdx.second];
                    //cout << fidx1 <<" " <<fidx2 << endl;

                    activePatchBlobMat.push_back(blobMat[j]);
                    activePatchFoldLine.push_back(make_pair(fidx1, fidx2));
                    lineType line1, line2;
                    line1.line = patchExtendedLine[maxIdx.first];
                    line2.line = patchExtendedLine[maxIdx.second];
                    activeExtentedLine.push_back(make_pair(line1, line2));
                    
                }
            }
            std::string str = "../ExtendFoldLineAndFindActiveBolb/allblobMat"+oss.str()+".png";
            imwrite(str, blobMat[j]);
        
        }
        
        obj->activeBlobMatOfPatch.push_back(activePatchBlobMat);
        obj->activeBlobFoldLineOfPatch.push_back(activePatchFoldLine);
        //obj->activeExtendedLineOfPatch.push_back(activeExtentedLine);

    }
    return true;
}
