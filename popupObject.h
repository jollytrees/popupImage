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

using namespace std;

//pch
struct patches{
    std::vector<std::vector<cv::Point> > paths;
    cv::Mat pchMat;
    int patchIdx;
};

typedef std::vector<std::vector<cv::Point> > paths_type;

static bool compareLongest( std::pair<int,int> x, std::pair<int,int> y ) {return x.second > y.second;}

static cv::Scalar green = cv::Scalar(0, 255 , 0);
static cv::Scalar red = cv::Scalar(0, 0 , 255);
static cv::Scalar blue = cv::Scalar(255, 0 , 0);
static cv::Scalar purple = cv::Scalar(255, 0 , 255);
static cv::Scalar yellow = cv::Scalar(0, 255 , 255);
static cv::Scalar blueGreen = cv::Scalar(255, 255 , 0);
static cv::Scalar white = cv::Scalar(255, 255, 255);
static cv::Scalar black = cv::Scalar(0, 0, 0);

//pch
class foldLineType{
public:
    std::pair<cv::Point, cv::Point> line;
    bool isCentralLine;
    bool isEmpty;
    bool isConnLine;
    bool isOriginalFoldLine;
    vector<int> originalConnPatch;
    vector<int> finalConnPatch;
    vector<int> connPatch;
    vector<int> connOriFoldLine;
    int foldLineIdx;
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
    //std::vector<std::vector<foldLineType*>  > boundaryFoldLineConnMap;
    
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

    
};


#endif /* popupObject_h */
