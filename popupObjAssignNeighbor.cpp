//
//  popupObjAsignNeighbor.cpp
//  GitPopupImage
//
//  Created by jollytrees on 1/5/16.
//
//

#include "popupObjAssignNeighbor.hpp"

int ccc = 0;

static int patchInLineLeft(popupObject *obj, std::vector<std::vector<cv::Point> > path, pair<cv::Point, cv::Point> &line, pair<int, int> &count){
    
    cv::Mat canvas(obj->initMatSize, CV_8UC1);
    canvas.setTo(0);
    cv::drawContours(canvas, path, 0, 255, CV_FILLED);
    
    cv::Point up = line.first.y < line.second.y ? line.first :  line.second;
    int w = 5;
    int h = (int)sqrt(pow((double)line.first.y-line.second.y,2.0));
    cv::Rect rect_left = cv::Rect(up.x-w, up.y, w, h);
    cv::Rect rect_right = cv::Rect(up.x, up.y, w, h);
    
    cv::Mat ROI_left = canvas(rect_left);
    cv::Mat ROI_right = canvas(rect_right);
    
    count.first = cv::countNonZero(ROI_left);
    count.second = cv::countNonZero(ROI_right);
    
    if(count.first == count.second){
        stringstream ss;
        ss << ccc;
        string str = ss.str()+"Canvas.png";
        imwrite(str, canvas);
        str = ss.str()+"L.png";
        imwrite(str, ROI_left);
        str = ss.str()+"R.png";
        imwrite(str, ROI_right);
        cv::Mat c1 = canvas.clone();
        rectangle(c1, rect_left , 124);
        str = ss.str()+"LD.png";
        imwrite(str, c1);
        str = ss.str()+"RD.png";
        c1 = canvas.clone();
        rectangle(c1, rect_right , 124);
        imwrite(str, c1);

        ccc++;
    }
    
    
    if(count.first < count.second) return 0;
    else if(count.first == count.second) return 2;
    else if(count.first > count.second) return 1;

    return -1;
}

static string coutLeft(int v){
    if(v==0) return "right";
    if(v==1) return "left";
    if(v==2) return "equal";
    if(v==-1) return "error";
}
bool popupObjAssignNeighbor::execute(popupObject *obj)
{
    
    //patch : clear and update patchIdx
    for(size_t pIdx =0; pIdx < obj->possiblePatches.size(); pIdx++){
        obj->possiblePatches[pIdx]->foldLine.clear();
        obj->possiblePatches[pIdx]->patchIdx = pIdx;
    }
    
    //update foldLine position and update foldLines of patch
    for(size_t lIdx= 0; lIdx < obj->foldLine.size(); lIdx++){
        int pIdx1 = obj->foldLine[lIdx]->connPatch[0];
        int pIdx2 = obj->foldLine[lIdx]->connPatch[1];
        obj->foldLine[lIdx]->xPosition = obj->foldLine[lIdx]->line.first.x;
        
        obj->possiblePatches[pIdx1]->addLine(obj->foldLine[lIdx]);
        obj->possiblePatches[pIdx2]->addLine(obj->foldLine[lIdx]);
    }

    //find line left and right
    for(size_t fIdx =0; fIdx< obj->foldLine.size(); fIdx++){
        if(obj->foldLine[fIdx]->leftIdx!=-1 && obj->foldLine[fIdx]->rightIdx!=-1) continue;
        int pIdx1 = obj->foldLine[fIdx]->connPatch[0];
        int pIdx2 = obj->foldLine[fIdx]->connPatch[1];
        
        
        if((obj->possiblePatches[pIdx1]->isLeftLine(fIdx) && !obj->isBasePatch(pIdx1))
           || (obj->possiblePatches[pIdx2]->isRightLine(fIdx) &&!obj->isBasePatch(pIdx2))){
            obj->foldLine[fIdx]->leftIdx = pIdx2;
            obj->foldLine[fIdx]->rightIdx = pIdx1;

            continue;
        }else if((obj->possiblePatches[pIdx1]->isRightLine(fIdx) && !obj->isBasePatch(pIdx1))
            || (obj->possiblePatches[pIdx2]->isLeftLine(fIdx) &&!obj->isBasePatch(pIdx2))){
            obj->foldLine[fIdx]->leftIdx = pIdx1;
            obj->foldLine[fIdx]->rightIdx = pIdx2;

            continue;
        }
        
        pair<int, int> count_1;
        pair<int, int> count_2;
        
        int isLeft1 = patchInLineLeft(obj,obj->possiblePatches[pIdx1]->paths, obj->foldLine[fIdx]->line,count_1);
        int isLeft2 = patchInLineLeft(obj,obj->possiblePatches[pIdx2]->paths, obj->foldLine[fIdx]->line,count_2);
        
       /* if(isLeft2 == isLeft1){
            cout << "left and right error-------" << endl;
            cout << "p"<<pIdx1 <<" " << std::boolalpha <<isLeft1 <<" p" << pIdx2 <<" " << std::boolalpha <<isLeft2<< endl;
            cout << count_1.first <<" "<<count_1.second<< endl;
            cout << count_2.first <<" "<<count_2.second<< endl;
            cout << "f"<<fIdx << endl;
            if(obj->foldLine[fIdx]->isOriginalFoldLine) cout <<"is original fold line" << endl;
            cout << "---------------------------" << endl;
        }*/
        
      /*  cout << fIdx << "  "<<pIdx1<<" " << isLeft1 << " "<<pIdx2<<" " << isLeft2 << endl;
        cout << count_1.first <<" "<<count_1.second<< endl;
        cout << count_2.first <<" "<<count_2.second<< endl;*/
        
        if(isLeft1==1 && isLeft2==0){
            obj->foldLine[fIdx]->leftIdx = pIdx1;
            obj->foldLine[fIdx]->rightIdx = pIdx2;
        }else if(isLeft1==0 && isLeft2==1){
            obj->foldLine[fIdx]->leftIdx = pIdx2;
            obj->foldLine[fIdx]->rightIdx = pIdx1;
        }else{
            cout << "isLeftOrRight Error" << endl;
            cout << "f"<<fIdx << "  "<<pIdx1<<" " << coutLeft(isLeft1) << " "<<pIdx2<<" " << coutLeft(isLeft2) << endl;
            cout << "lr1 size "<< count_1.first <<" "<<count_1.second<< endl;
            cout << "lr2 size "<< count_2.first <<" "<<count_2.second<< endl;
            obj->foldLine[fIdx]->leftIdx = pIdx1;
            obj->foldLine[fIdx]->rightIdx = pIdx2;
        }
       
    }
    
    
    //set line left and right
    for(size_t fIdx =0; fIdx< obj->foldLine.size(); fIdx++){
        int left_Idx = obj->foldLine[fIdx]->leftIdx;
        int right_Idx = obj->foldLine[fIdx]->rightIdx;

        if(left_Idx==-1 || right_Idx==-1) {
            cout << "left or right idx not found!" << endl;
            cout << "f" << fIdx <<" Lp="<<left_Idx << " Rp=" << right_Idx << endl;

            continue;
        }
        
        obj->possiblePatches[left_Idx]->rightPatchIdx.push_back(right_Idx);
        obj->possiblePatches[right_Idx]->leftPatchIdx.push_back(left_Idx);
        
    }
    
    obj->orientation.resize(obj->foldLine.size(), 0);
    obj->activeFoldLine.resize(obj->foldLine.size(), 0);
    
    return true;
}