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
static bool  extendFoldline(popupObject *obj ,int pIdx1, int pIdx2,int lineIdx, cv::Mat canvas, vector< pair<cv::Point, cv::Point> > &patchExtendedLine){

    if(obj->isBasePatch(pIdx1)) return false;

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

    return true;
}

bool isActiveBlob( cv::Mat greyMat, vector< pair<cv::Point, cv::Point> > &patchExtendedLine, int patchIdx, int blobIdx,
                  vector<int> &leftIdx, vector<int> &rightIdx){
    
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
                leftIdx.push_back(j);
                if(count > maxLeftArea) maxLeftIdx = j;
            }else{
                connectRightSize++;
                rightIdx.push_back(j);
                if(count > maxRightArea) maxRightIdx = j;

            }
            
            std::ostringstream oss;
            oss << patchIdx <<"_" <<blobIdx<< "_" << j ;
            std::string str = "./ExtendFoldLineAndFindActiveBolb/active"+oss.str()+".png";
            imwrite(str, lineCanvas);
            
        }


    }
    
    if(connectLeftSize>0 && connectRightSize>0){
        cout << "conn " << connectLeftSize <<" " << connectRightSize << endl;
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
                
                bool isAddFoldLine = extendFoldline(obj , (int)i, (int)j, (int)k, canvas, patchExtendedLine);
                
                if(!isAddFoldLine) continue;
                
                extendLine etLine;
                etLine.line = patchExtendedLine.back();
                etLine.patchIdx = i;
                etLine.foldLineIdx = obj->originalFoldLineConnMap[i][j][k]->foldLineIdx;
                
                obj->originalFoldLineConnMap[i][j][k]->extendLines.push_back(etLine);
                
               // cout << "foldLine " <<etLine.foldLineIdx << " " << obj->originalFoldLineConnMap[i][j][k]->extendLines.size() << endl;

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
        
        vector<activeBlob> activePatchBlob;
        activePatchBlob.clear();

        for(size_t j=0; j< blobMat.size(); j++){
        
            cv::Mat greyMat;
            cv::cvtColor(blobMat[j], greyMat, CV_BGR2GRAY);
            int count = cv::countNonZero(greyMat);
            oss.str("");
            oss << i << " " <<j;
            if(count > 0){
                vector<int> leftIdx;
                vector<int> rightIdx;
                bool isActive = isActiveBlob( greyMat, patchExtendedLine, (int)i, (int)j, leftIdx, rightIdx);
                if(isActive){
                    
                    std::string str = "../ExtendFoldLineAndFindActiveBolb/blobMat"+oss.str()+".png";
                    imwrite(str, blobMat[j]);
                    
                    activeBlob blob (blobMat[j]);
                    activePatchBlob.push_back(blob);
                    
                    for(int idx=0; idx<leftIdx.size(); idx++){
                        int fidx = patchExtendedLineToFoldLine[leftIdx[idx]];
                        blob.leftLineIdx.push_back(fidx);
                    }
                    for(int idx=0; idx<rightIdx.size(); idx++){
                        int fidx = patchExtendedLineToFoldLine[rightIdx[idx]];
                        blob.leftLineIdx.push_back(fidx);
                    }
                    
                }
                else{
                    std::string str = "../ExtendFoldLineAndFindActiveBolb/inactiveBlob"+oss.str()+".png";
                    imwrite(str, blobMat[j]);
                }
            }
            std::string str = "../ExtendFoldLineAndFindActiveBolb/allblobMat"+oss.str()+".png";
            imwrite(str, blobMat[j]);
        
        }
        
        obj->activeBlobOfPatch.push_back(activePatchBlob);

    }
    return true;
}
