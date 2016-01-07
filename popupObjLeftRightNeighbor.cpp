//
//  popupObjLeftRightNeighbor.cpp
//  GitPopupImage
//
//  Created by jollytrees on 12/30/15.
//
//

#include "popupObjLeftRightNeighbor.hpp"
#include "functionFindBlob.h"
#include "functionFindContour.h"
#include "functionFindMatIntersection.h"

/*
class lineStruct{
public:
    int foldLineIdx;
    bool isCuttedLine;
    pair<cv::Point, cv::Point> line;
    
};*/

static bool isActiveCuttedBlob( cv::Mat greyMat, vector< foldLineType* > &patchLine, int patchIdx, int blobIdx, pair<int, int> &maxIdxs){
    
    int connectLeftSize = 0;
    int connectRightSize = 0;
    int maxLeftIdx = -1;
    int maxRightIdx = -1;
    int maxLeftArea = -1;
    int maxRightArea  = -1;
    for(size_t j=0; j<patchLine.size(); j++){
        
        cv::Mat foldLineMat(greyMat.size(), CV_8UC1);
        foldLineMat.setTo(0);
        cv::line(foldLineMat, patchLine[j]->line.first, patchLine[j]->line.second, 255, 2);
        
        //for draw
        cv::Mat lineCanvas = greyMat.clone();
        cv::line(lineCanvas, patchLine[j]->line.first, patchLine[j]->line.second, 125, 2);
        
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
            if(patchLine[j]->line.first.x > midTop.x){
                connectLeftSize++;
                if(count > maxLeftArea) maxLeftIdx = j;
            }else{
                connectRightSize++;
                if(count > maxRightArea) maxRightIdx = j;
            }
            cv::cvtColor(lineCanvas, lineCanvas, CV_GRAY2BGR);
            std::ostringstream oss;
            oss << "_"<< patchIdx <<"_" <<blobIdx<< "_" << j ;
            std::string str = "../popupObjLeftRightNeighbor/active"+oss.str()+".png";
            for(size_t k=0; k<patchLine.size(); k++){
                cv::line(lineCanvas, patchLine[k]->line.first, patchLine[k]->line.second, red, 1);
            }
            imwrite(str, lineCanvas);
        }
    }
    
    if(connectLeftSize>0 && connectRightSize>0){
       // patchLineActive[maxLeftIdx].push_back(blobIdx);
      //  patchLineActive[maxRightIdx].push_back(blobIdx);
        maxIdxs.first = maxLeftIdx;
        maxIdxs.second = maxRightIdx;
        return true;
    }
    return false;
}

class blobStruct{
public:
    int patchesIdx;
    bool isLeft;
    cv::Mat blobMat;
    paths_type path;
    bool isActive;
    int newPatchIdx;
};


static bool searchTheMaxOverlapBlob(popupObject *obj, vector<blobStruct*> blobs, foldLineType *inputLine, int &maxIdx){
    
    //draw input line mat
    cv::Mat inputLineMat(obj->initMatSize, CV_8UC1);
    inputLineMat.setTo(black);
    cv::line(inputLineMat, inputLine->line.first, inputLine->line.second, 255, 10);
    std::vector< std::pair<int, int> > intSecList; //intersection
    intSecList.clear();
    
    //1. count the intersection between patch with input line
    for(int i=0; i< blobs.size(); i++ ){
        
        //count intersection
        cv::Mat outputMat(obj->initMatSize, CV_8UC1);
        
        cv::Mat greyMat;
        cv::cvtColor(blobs[i]->blobMat, greyMat, CV_BGR2GRAY);
        
        int areaCount = findMatIntersectionRInt( inputLineMat,  greyMat, outputMat);

        if(areaCount>0){
            intSecList.push_back(std::make_pair(i, areaCount));
        }
    }
    
    //2. find the patches with max area
    sort(intSecList.begin(), intSecList.end(), compareLongest);
    if(intSecList.size()>0){
        int maxIdx = intSecList[0].first;
        inputLine->connPatch.push_back(blobs[maxIdx]->newPatchIdx);
        return true;
    }
    return false;
    
}


static bool inLineLeft(cv::Mat &bMat, pair<cv::Point, cv::Point> &line){

    cv::Mat output = bMat.clone();
    cvtColor( bMat, output, CV_BGR2GRAY );
    
    cv::Point up = line.first.y < line.second.y ? line.first :  line.second;
    int w = 3;
    int h = (int)sqrt(pow((double)line.first.y-line.second.y,2.0));

    cv::Rect rect_left = cv::Rect(up.x-w, up.y, w, h);
    cv::Rect rect_right = cv::Rect(up.x, up.y, w, h);

    
    cv::Mat ROI_left = output(rect_left);
    cv::Mat ROI_right = output(rect_right);

    int count_left = cv::countNonZero(ROI_left);
    int count_right = cv::countNonZero(ROI_right);
    
    if(count_right < count_left) return true;
    return false;

}

bool popupObjLeftRightNeighbor::execute(popupObject *obj)
{
    for(int lIdx= 0; lIdx < obj->foldLine.size(); lIdx++){
        int pIdx1 = obj->foldLine[lIdx]->connPatch[0];
        int pIdx2 = obj->foldLine[lIdx]->connPatch[1];
        obj->foldLine[lIdx]->xPosition = obj->foldLine[lIdx]->line.first.x;
    }
    
    for(int pIdx =0; pIdx < obj->possiblePatches.size(); pIdx++){
        //cout << "----pch" << pIdx<< " size = " << obj->possiblePatches[pIdx]->foldLine.size() << endl;
        if ( obj->possiblePatches[pIdx]->foldLine.size()==0) {
            cout << "pch" << pIdx<<  "size = 0" << endl;
        }
       //  obj->possiblePatches[pIdx]->printFoldLine();
    }
    
    for(int lIdx= 0; lIdx < obj->foldLine.size(); lIdx++){
        
        
        if(!obj->foldLine[lIdx]->isOriginalType) continue;
        if(!obj->foldLine[lIdx]->isConnLine) continue;
        
        cout << "lIdx " <<lIdx << endl;

        //is connection line
        //find connected patch
        int oriPIdx1 = obj->foldLine[lIdx]->originalConnPatch[0];
        int oriPIdx2 = obj->foldLine[lIdx]->originalConnPatch[1];
        int pIdx1 = obj->foldLine[lIdx]->connPatch[0];
        int pIdx2 = obj->foldLine[lIdx]->connPatch[1];
        
        cout << lIdx << " :ori  " << oriPIdx1 << "," << oriPIdx2 << endl;

        cout << lIdx << " : " << pIdx1 << "," << pIdx2 << endl;
        
        cout << obj->floorPatch <<" "<<obj->backPatch<< obj->possiblePatches.size()-1 << endl;
        
        // is not connected to background
        if(obj->isBasePatch(pIdx1)) continue;
        if(obj->isBasePatch(pIdx2)) continue;

        cv::Mat canvas1(obj->initMatSize, CV_8UC1);
        canvas1.setTo(0);
        cv::Mat canvas2(obj->initMatSize, CV_8UC1);
        canvas2.setTo(0);

        cv::drawContours(canvas1, obj->possiblePatches[pIdx1]->paths, 0, 255, CV_FILLED);
        cv::drawContours(canvas2, obj->possiblePatches[pIdx2]->paths, 0, 255, CV_FILLED);
        
        cout << "t" << endl;
        cv::Mat canvas3(obj->initMatSize, CV_8UC3);
        canvas3.setTo(black);
        cv::Mat canvas4(obj->initMatSize, CV_8UC3);
        canvas4.setTo(black);
        cv::drawContours(canvas3, obj->possiblePatches[pIdx1]->paths, 0, white, CV_FILLED);
        cv::drawContours(canvas4, obj->possiblePatches[pIdx2]->paths, 0, white, CV_FILLED);
        cout << "t1" << endl;

       
        //get extended line
        int exIdx1 = obj->foldLine[lIdx]->returnExtendLineIdx(oriPIdx1);
        int exIdx2 = obj->foldLine[lIdx]->returnExtendLineIdx(oriPIdx2);
        
        cout << exIdx1 << " " << exIdx2 <<" "<<obj->foldLine[lIdx]->extendLines.size()<< endl;
        
        cv::Point p1 = obj->foldLine[lIdx]->extendLines[exIdx1].line.first;
        cv::Point p2 = obj->foldLine[lIdx]->extendLines[exIdx1].line.second;
        pair<cv::Point, cv::Point> cuttedLine1 = make_pair(p1, p2);
        cv::line(canvas1, p1, p2, 0);
        cv::line(canvas3, p1, p2, red);


        cv::Point p3 = obj->foldLine[lIdx]->extendLines[exIdx2].line.first;
        cv::Point p4 = obj->foldLine[lIdx]->extendLines[exIdx2].line.second;
        pair<cv::Point, cv::Point> cuttedLine2 = make_pair(p3, p4);
        cv::line(canvas2, p3, p4, 0);
        cv::line(canvas4, p3, p4, red);
        cout << "t2" << endl;

        
        //find new blob
        vector<cv::Mat> blobMat1, blobMat2;
        vector<blobStruct*> blobs;

        // 0|1
        // 2|3
        ConnectedBlobs(canvas1, blobMat1);
        int mapActive[4] = {0};
        for(int j=0; j< blobMat1.size(); j++){
            blobStruct *a = new blobStruct();
            a->blobMat = blobMat1[j].clone();
            a->isLeft = inLineLeft(blobMat1[j], cuttedLine1);
            a->patchesIdx = pIdx1;
            blobs.push_back(a);
            if (a->isLeft) {
                mapActive[0] = 1;
            }else{
                mapActive[1] = 1;
            }
        }
        
        ConnectedBlobs(canvas2, blobMat2);
        for(int j=0; j< blobMat2.size(); j++){
            blobStruct *a = new blobStruct();
            a->blobMat = blobMat2[j].clone();
            a->isLeft = inLineLeft(blobMat2[j], cuttedLine2);
            a->patchesIdx = pIdx2;
            blobs.push_back(a);
            if (a->isLeft) {
                mapActive[2] = 1;
            }else{
                mapActive[3] = 1;
            }
        }
        
        cout << "t3" << endl;

        //find
        if (blobs.size() == 3) {
            if (mapActive[1]==0 || mapActive[2]==0) {
                obj->foldLine[lIdx]->leftIdx = pIdx1;
                obj->foldLine[lIdx]->rightIdx = pIdx2;
            }else if(mapActive[0]==0 || mapActive[3]==0){
                obj->foldLine[lIdx]->leftIdx = pIdx2;
                obj->foldLine[lIdx]->rightIdx = pIdx1;
            }

            continue;
        }else if(blobs.size() < 3){
            cout << lIdx << " has error blob size: " << blobs.size() << endl;
            cout << "p" << pIdx1<<" " <<mapActive[0] <<" "<< mapActive[1]<<endl;
            cout << "p" << pIdx2<<" " <<mapActive[2] <<" "<< mapActive[3]<<endl;
            imshow("pIdx1", canvas1);
            imshow("pIdx2", canvas2);
            imshow("pIdx11", canvas3);
            imshow("pIdx22", canvas4);

            continue;
        }

        cout << "4" << endl;

        
        foldLineType *line1 = new foldLineType();
        line1->isCuttedLine = true;
        line1->line = cuttedLine1;
        
        foldLineType *line2 = new foldLineType();
        line2->isCuttedLine = true;
        line2->line = cuttedLine2;
        
        //put all line
        vector< foldLineType* > cuttedLine;
        cuttedLine.push_back(line1);
        cuttedLine.push_back(line2);
        
        vector< foldLineType* > cuttedLineWoExtendLine;
        vector<int > cuttedLinePatchMap;
        
        for(int j=0; j<obj->possiblePatches[pIdx1]->foldLine.size(); j++){

            if(obj->possiblePatches[pIdx1]->foldLine[j]->foldLineIdx == lIdx) continue;
            foldLineType *line = new foldLineType();
            line->isCuttedLine = false;
            line->line = obj->possiblePatches[pIdx1]->foldLine[j]->line;
            line->foldLineIdx = obj->possiblePatches[pIdx1]->foldLine[j]->foldLineIdx;
            line->connPatch = obj->possiblePatches[pIdx1]->foldLine[j]->connPatch;
            cuttedLine.push_back(line);
            cuttedLineWoExtendLine.push_back(line);
            cuttedLinePatchMap.push_back(pIdx1);
        }

        for(int j=0; j<obj->possiblePatches[pIdx2]->foldLine.size(); j++){

            if(obj->possiblePatches[pIdx2]->foldLine[j]->foldLineIdx == lIdx) continue;
            foldLineType *line = new foldLineType();
            line->isCuttedLine = false;
            line->line = obj->possiblePatches[pIdx2]->foldLine[j]->line;
            line->foldLineIdx = obj->possiblePatches[pIdx2]->foldLine[j]->foldLineIdx;
            line->connPatch = obj->possiblePatches[pIdx2]->foldLine[j]->connPatch;

            cuttedLine.push_back(line);
            cuttedLineWoExtendLine.push_back(line);
            cuttedLinePatchMap.push_back(pIdx2);
        }

        
        int mapActiveBlob[4] = {0};
        //check 4 blobs whether correct
        bool isActivecut = true;
        int activeSize = 0;
        for(size_t j=0; j< blobs.size(); j++){
            
            cv::Mat greyMat;
            cv::cvtColor(blobs[j]->blobMat, greyMat, CV_BGR2GRAY);
            int count = cv::countNonZero(greyMat);
            if(count > 0){
                pair<int, int> maxIdx;
                bool isActive = isActiveCuttedBlob( greyMat, cuttedLine, (int)lIdx, (int)j, maxIdx);
                blobs[j]->isActive = isActive;
                if(!isActive){
                    isActivecut = false;
                }else{
                    if(blobs[j]->patchesIdx == pIdx1){
                        if(blobs[j]->isLeft) mapActiveBlob[0] = 1;
                        else mapActiveBlob[1] = 1;
                    }else{
                        if(blobs[j]->isLeft) mapActiveBlob[2] = 1;
                        else mapActiveBlob[3] = 1;
                    }
                    activeSize++;
                }
            }
        }
        
        if(isActivecut) cout << "f" <<lIdx << "  is active cut" << endl;
        else {
            if(activeSize<3){
                cout << lIdx << " error active blob size: "<<activeSize << endl;

                cout << "p" << pIdx1<<" " <<mapActiveBlob[0] <<" "<< mapActiveBlob[1]<<endl;
                cout << "p" << pIdx2<<" " <<mapActiveBlob[2] <<" "<< mapActiveBlob[3]<<endl;
                for(int k=0; k<cuttedLineWoExtendLine.size(); k++){
                    cv::line(canvas3, cuttedLineWoExtendLine[k]->line.first, cuttedLineWoExtendLine[k]->line.second, red);
                    cv::line(canvas4, cuttedLineWoExtendLine[k]->line.first, cuttedLineWoExtendLine[k]->line.second, red);
                    cout << "f"<<cuttedLineWoExtendLine[k]->foldLineIdx <<" " ;

                }
                cout << endl;
                imshow("pIdx1", canvas1);
                imshow("pIdx2", canvas2);
                imshow("pIdx11", canvas3);
                imshow("pIdx22", canvas4);


            }
            if (mapActiveBlob[1]==0 || mapActiveBlob[2]==0) {
                obj->foldLine[lIdx]->leftIdx = pIdx1;
                obj->foldLine[lIdx]->rightIdx = pIdx2;
            }else if(mapActiveBlob[0]==0 || mapActiveBlob[3]==0){
                obj->foldLine[lIdx]->leftIdx = pIdx2;
                obj->foldLine[lIdx]->rightIdx = pIdx1;
            }

            continue;
        };

        foldLineType* l1 = new foldLineType();
        l1->line = line1->line;
        l1->isCentralLine = false;
        l1->isConnLine = false;
        l1->isOriginalType = false;
        l1->isCuttedLine = true;
        l1->xPosition = line1->line.first.x;
        
        foldLineType* l2 = new foldLineType();
        l2->line = line2->line;
        l2->isCentralLine = false;
        l2->isConnLine = false;
        l2->isOriginalType = false;
        l2->isCuttedLine = true;
        l2->xPosition = line2->line.first.x;

        obj->foldLine.push_back(l1);
        l1->foldLineIdx = obj->foldLine.size()-1;
        //cout << "add foldLine " << l1->foldLineIdx << endl;

        obj->foldLine.push_back(l2);
        l2->foldLineIdx = obj->foldLine.size()-1;
        //cout << "add foldLine " << l2->foldLineIdx << endl;

        
        //change the data or new data
        for(int j=0; j<blobs.size(); j++){
        
            patches* pch = new patches();
            cv::Mat output;
            cvtColor( blobs[j]->blobMat, output, CV_BGR2GRAY );
            findContourSimple(output, pch->paths);
            pch->pchMat = blobs[j]->blobMat;
                        
            if(blobs[j]->isLeft){
                obj->possiblePatches.push_back(pch);
                pch->patchIdx = obj->possiblePatches.size()-1;

                //cout << "add patch" <<pch->patchIdx << endl;
            }else{
                obj->possiblePatches[blobs[j]->patchesIdx] = pch;
                pch->patchIdx = blobs[j]->patchesIdx;
            }
            blobs[j]->newPatchIdx = pch->patchIdx;
            
            if(blobs[j]->patchesIdx ==pIdx1){
                l1->connPatch.push_back(pch->patchIdx);
                obj->possiblePatches[pch->patchIdx]->addLine(l1);
                if(blobs[j]->isLeft) l1->leftIdx =pch->patchIdx;
                else l1->rightIdx =pch->patchIdx;
            }else{
                l2->connPatch.push_back(pch->patchIdx);
                obj->possiblePatches[pch->patchIdx]->addLine(l2);
                if(blobs[j]->isLeft) l2->leftIdx =pch->patchIdx;
                else l2->rightIdx =pch->patchIdx;
            }
        }
        
        obj->foldLine[lIdx]->isCuttedLine = false;
        obj->foldLine[lIdx]->connPatch.clear();
        
        foldLineType* lineClone = new foldLineType();
        lineClone->line = obj->foldLine[lIdx]->line;
        lineClone->isCentralLine = obj->foldLine[lIdx]->isCentralLine;
        lineClone->isConnLine = obj->foldLine[lIdx]->isConnLine;
        lineClone->isOriginalType = false;
        lineClone->isCuttedLine = false;
        lineClone->isCloneLine = true;
        lineClone->xPosition = obj->foldLine[lIdx]->line.first.x;
        
        obj->foldLine.push_back(lineClone);
        lineClone->foldLineIdx = obj->foldLine.size()-1;
        //cout << "add foldLine " << lineClone->foldLineIdx << endl;
        
        
        // find the add fold line connection
        for(int j=0; j<blobs.size(); j++){
            if(blobs[j]->isLeft){
                //upper left
                if(blobs[j]->patchesIdx == pIdx1){
                    
                    lineClone->connPatch.push_back(blobs[j]->newPatchIdx);
                    obj->possiblePatches[blobs[j]->newPatchIdx]->addLine(lineClone);
                    lineClone->leftIdx = blobs[j]->newPatchIdx;
                }else{ //bottom left
                    obj->foldLine[lIdx]->connPatch.push_back(blobs[j]->newPatchIdx);
                    obj->possiblePatches[blobs[j]->newPatchIdx]->addLine(obj->foldLine[lIdx]);
                    obj->foldLine[lIdx]->leftIdx = blobs[j]->newPatchIdx;
                }
                
            }else{
                if(blobs[j]->patchesIdx == pIdx1){
                    //upper right
                    obj->foldLine[lIdx]->connPatch.push_back(blobs[j]->newPatchIdx);
                    obj->possiblePatches[blobs[j]->newPatchIdx]->addLine(obj->foldLine[lIdx]);
                    obj->foldLine[lIdx]->rightIdx = blobs[j]->newPatchIdx;
                }else{
                    //bottom right
                    lineClone->connPatch.push_back(blobs[j]->newPatchIdx);
                    obj->possiblePatches[blobs[j]->newPatchIdx]->addLine(lineClone);
                    lineClone->rightIdx = blobs[j]->newPatchIdx;
                    
                }
            }
        }

        // find new patch fold line
        for(int j=0; j<cuttedLineWoExtendLine.size(); j++){
            
            foldLineType* lineT = cuttedLineWoExtendLine[j];
            if(lineT->foldLineIdx == lIdx) continue;
            lineT->removeConnPatch(cuttedLinePatchMap[j]);
            obj->possiblePatches[cuttedLinePatchMap[j]]->removeLine(lIdx);
            int maxIdx;
            bool isfindMax = searchTheMaxOverlapBlob(obj, blobs, lineT, maxIdx);
            obj->foldLine[lineT->foldLineIdx]->connPatch = lineT->connPatch;
            obj->possiblePatches[maxIdx]->addLine(obj->foldLine[lineT->foldLineIdx]);
        }

    }
    return true;
}