//
//  popupObject.h
//  Color_Segmentation
//
//  Created by jollytrees on 11/8/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef popupObject_h
#define popupObject_h
#include <iostream>
#include <vector>
#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv/cv.h>

using namespace std;
typedef std::vector<std::vector<cv::Point> > paths_type;

class extendLine{
public:
    std::pair<cv::Point, cv::Point> line;
    int foldLineIdx;
    int patchIdx;
};


//pch
class foldLineType{
public:
    foldLineType(){
        leftIdx = -1;
        rightIdx = -1;
        isCuttedLine = false;
    }
    std::pair<cv::Point, cv::Point> line;
    bool isCentralLine;
    bool isEmpty;
    bool isConnLine;
    bool isOriginalFoldLine;
    bool isCuttedLine;
    vector<int> originalConnPatch;
    vector<int> finalConnPatch;
    vector<int> connPatch;
    vector<int> connOriFoldLine;
    int foldLineIdx;
    int xPosition;
    int leftIdx;
    int rightIdx;
    
    vector<extendLine> extendLines;
    
    void removeConnPatch(int pIdx){
        for(int i=0; i<connPatch.size(); i++){
            if(connPatch[i]== pIdx){
                connPatch.erase(connPatch.begin()+i);
            }
        }
        
    }
    
    void printConnPatch(){
        for(int i=0; i<connPatch.size(); i++){
            cout << connPatch[i] << " ";
        }
        cout << endl;
    }
    
    int returnExtendLineIdx(int patchIdx){
        for(int i=0; i< extendLines.size(); i++){
            if(extendLines[i].patchIdx == patchIdx){
                return i;
            }
        }
        return -1;
        
    }
    
};

static bool compareLongest( std::pair<int,int> x, std::pair<int,int> y ) {return x.second > y.second;}
static bool compareIncrement( foldLineType* x, foldLineType* y ) {return x->xPosition < y->xPosition;}

static cv::Scalar green = cv::Scalar(0, 255 , 0);
static cv::Scalar red = cv::Scalar(0, 0 , 255);
static cv::Scalar blue = cv::Scalar(255, 0 , 0);
static cv::Scalar purple = cv::Scalar(255, 0 , 255);
static cv::Scalar yellow = cv::Scalar(0, 255 , 255);
static cv::Scalar blueGreen = cv::Scalar(255, 255 , 0);
static cv::Scalar white = cv::Scalar(255, 255, 255);
static cv::Scalar black = cv::Scalar(0, 0, 0);
static cv::Scalar orange = cv::Scalar(0, 128, 255);
static cv::Scalar lemonYellow= cv::Scalar(173, 240, 255);


//pch
class patches{
public:
    std::vector<std::vector<cv::Point> > paths;
    cv::Mat pchMat;
    int patchIdx;
    
    vector<foldLineType*> foldLine;

    vector<int> leftPatchIdx;
    vector<int> rightPatchIdx;

    void addLine( foldLineType* line){
        foldLine.push_back(line);
        sort(foldLine.begin(), foldLine.end(), compareIncrement);
    }
    
    void removeLine(int pIdx){
        for(int i=0; i<foldLine.size(); i++){
            if(foldLine[i]->foldLineIdx== pIdx){
                foldLine.erase(foldLine.begin()+i);
            }
        }
        
    }
    
    bool isBoundaryLine(int idx){
        for(vector<foldLineType*>::iterator it = foldLine.begin(); it!=foldLine.end(); ++it){
            if( (*it)->foldLineIdx==idx ){
                if(it==foldLine.begin() || it==foldLine.end()-1){
                    return true;
                }
                return false;
            }
        }
    }
    bool isLeftLine(int idx){
        for(vector<foldLineType*>::iterator it = foldLine.begin(); it!=foldLine.end(); ++it){
            if( (*it)->foldLineIdx==idx ){
                if(it==foldLine.begin()){
                    return true;
                }
                return false;
            }
        }
    }
    bool isRightLine(int idx){
        for(vector<foldLineType*>::iterator it = foldLine.begin(); it!=foldLine.end(); ++it){
            if( (*it)->foldLineIdx==idx ){
                if(it==foldLine.end()-1){
                    return true;
                }
                return false;
            }
        }
    }
    
    void printFoldLine(){
        cout << "patch " << patchIdx <<" foldLines" << endl;
        for(vector<foldLineType*>::iterator it = foldLine.begin(); it!=foldLine.end(); ++it){
            cout << (*it)->foldLineIdx <<" ";
        }
        cout << endl;
    }
    
    void printLeftPatch(){
        cout << "patch " << patchIdx <<" left patches" << endl;
        for(vector<int>::iterator it = leftPatchIdx.begin(); it!=leftPatchIdx.end(); ++it){
            cout << (*it) <<" ";
        }
        cout << endl;
    }
    
    void printRightPatch(){
        cout << "patch " << patchIdx <<" right patches" << endl;
        for(vector<int>::iterator it = rightPatchIdx.begin(); it!=rightPatchIdx.end(); ++it){
            cout << (*it) <<" ";
        }
        cout << endl;
    }
};

class lineType{
public:
    std::pair<cv::Point, cv::Point> line;
};


class popupObject
{
    
public:
    //constructor
    popupObject(){}
    
    //back floor patches
    int backPatch;
    int floorPatch;
    int assignedPatch;
    int originalFloorPatch;
    int originalBackPatch;
    
    //show
    bool isShowPatches;
    bool isShowFoldlines;
    bool isShowOriginalPatches;
    
    //
    int scale;
    
    //InitPatch
    std::vector< struct patches* > initPatches;
    cv::Size initMatSize;
    int centralXPosition = 300;
    
    //divided patch
    std::vector< struct patches* > dividedPatches;
    
    //3. find the connection of divided patches
    std::vector<std::vector<int> > dividedConnMap;
    std::vector<int> dividedConnSize;
    std::vector<std::vector<cv::Mat> > dividedPatchIntersection;
    
    //4. classify patches
    std::vector< struct patches* > islandPatches;
    std::vector< struct patches* > classifiedPatches;
    
    //5. find the connection of classified patched
    std::vector<std::vector<int> > classifiedConnMap;
    std::vector<int> classifiedConnSize;
    std::vector<std::vector<cv::Mat> > classifiedPatchIntersection;
    
    //6. initialize boundary fold lines
    std::vector<std::vector<foldLineType* > > boundaryFoldLineOfPatch;
    
    //7. build connection map of boundary fold line
    std::vector<std::vector<std::vector<foldLineType*> > > boundaryFoldLineConnGroupMap;
    
    //8. after merge
    std::vector<std::vector<foldLineType*>  > boundaryFoldLineConnMap;
    
    //active blob
    vector<vector<cv::Mat> >activeBlobMatOfPatch;
    vector<vector<pair<int,int> > > activeBlobFoldLineOfPatch;
    
    //inserted line
    std::vector<std::vector<foldLineType*>  > insertedLineOfPatch;
    
    //divide patch by inserted line
    vector<std::vector< struct patches* > > possiblePatchesOfPatch;
    std::vector< struct patches* > possiblePatches;
    
    //convert : before connection
    std::vector<foldLineType*  > foldLine;
    std::vector<std::vector<foldLineType* > > originalFoldLineOfPatch;
    std::vector<std::vector<std::vector<foldLineType* > > > originalFoldLineConnMap;
    
    //find connection of possible patches
    std::vector<std::vector<foldLineType*>  > possibleFoldLineConnMap;
    
    //find all path
    vector<vector<int> > paths;
    
    //find fold line path
    vector<vector<int> > foldLinePaths;
    vector<vector<vector<int> > > foldLinePathsOfPatch;
    
    //find foldibility
    vector<double> X;
    vector<double> Y;
    vector<double> oriX;
    vector<vector<pair<int,int> > > positionLineIdxOfPatch;
    vector<vector<int> > foldLineToPositionLine;
    map<int, int> positionLineToFoldLine;
    vector<vector<int> > neighborsOfOriginalPatch;
    vector<vector<int> > neighborsOfPossiblePatch;
    
    bool isBasePatch(int idx);
    bool isOriginalBasePatch(int idx);
    void initPatch(std::string fileName);
    void assignBasePatch(cv::Point p);
    int clickClassifiedPatch(cv::Point p);
    void assignDividedPatches();
    int clickPossiblePatch(cv::Point p);
    
    vector<double> activeFoldLine;
    vector<double> orientation;
    
    //merge patches
    std::vector< struct patches* > mergedPatches;
    std::vector< std::vector< struct patches*> > mergedPatchesOfPatch;
    
    //find final path
    vector<vector<int> > finalPaths;
    
    int isLeftOrRightNeighbor(int p1, int p2);
    
    
};


#endif /* popupObject_h */
