//
//  functionEvenFilter.h
//  popupImage
//
//  Created by jollytrees on 12/13/15.
//
//
#include "popupObject.h"
#include <Qlabel>
#ifndef functionEventFilter_h
#define functionEventFilter_h


static void eventChooseBackdrop(popupObject *obj, cv::Point p, QLabel *label_backdrop){

    obj->assignedPatch = -1;
    obj->assignBasePatch(p);
    
    if(obj->assignedPatch < 0) return;
    
    cv::Mat initPatchMat = drawPatch(obj->initPatches, obj->initMatSize);
    cv::drawContours(initPatchMat, obj->initPatches[obj->assignedPatch]->paths, 0, yellow, 3);
    
    QImage image = mat2QImage(initPatchMat , QImage::Format_RGB888);
    label_backdrop->setPixmap(QPixmap::fromImage(image));

}

static void drawClickedItem(popupObject *obj, cv::Mat &resultsMat, int idx, QLabel *label_resultLayout){
    
    for(size_t i=0; i< obj->foldLine.size(); i++){
        
        if(idx!=i) continue;
        ostringstream oss;
        oss.str("");
        oss<< "f" << i;
        int x = obj->X[i] + obj->Y[i];
        
        cv::Point op1 = obj->foldLine[i]->line.first;
        cv::Point op2 = obj->foldLine[i]->line.second;

        cv::Point p = cv::Point(obj->foldLine[i]->line.first.x,(obj->foldLine[i]->line.first.y + obj->foldLine[i]->line.second.y)/2 );
        cv::line(resultsMat, op1, op2, purple, 3);
        putText(resultsMat, oss.str(),p, FONT_HERSHEY_SIMPLEX, 0.5, purple, 1);
    }
    
    if(obj->isShowPatches && !obj->isShowOriginalPatches) LabelPatchContourPoint(obj->possiblePatches, obj->initMatSize, resultsMat);
    if(obj->isShowPatches && obj->isShowOriginalPatches) LabelPatchContourPoint(obj->classifiedPatches, obj->initMatSize, resultsMat);

    QImage image = mat2QImage(resultsMat , QImage::Format_RGB888);
    label_resultLayout->setPixmap(QPixmap::fromImage(image));
    
}

static void drawActiveFoldline(popupObject *obj, cv::Mat &resultsMat){
    
    for(size_t i=0; i< obj->possiblePatches.size(); i++){
        if(obj->orientation[i]!=0.0){
            cv::drawContours(resultsMat, obj->possiblePatches[i]->paths, 0, blueGreen, CV_FILLED);
        }
    }
    for(size_t i=0; i< obj->foldLine.size(); i++){
        ostringstream oss;
        oss.str("");
        oss<< "f" << i;
        
        int x = obj->X[i] + obj->Y[i];
        cv::Point op1 = obj->foldLine[i]->line.first;
        cv::Point op2 = obj->foldLine[i]->line.second;
        
        if(obj->activeFoldLine[i]!=0){
            cv::Point p = cv::Point(obj->foldLine[i]->line.first.x,(obj->foldLine[i]->line.first.y + obj->foldLine[i]->line.second.y)/2 );
            cv::line(resultsMat, op1, op2, red, 2);
            
            if(obj->isShowFoldlines) putText(resultsMat, oss.str(),p, FONT_HERSHEY_SIMPLEX, 0.5, purple, 1);
        }else{
            cv::Point p = cv::Point(obj->foldLine[i]->line.first.x,(obj->foldLine[i]->line.first.y + obj->foldLine[i]->line.second.y)/2 );
            cv::line(resultsMat, op1, op2, blue, 1);
            if(obj->isShowFoldlines) putText(resultsMat, oss.str(), p, FONT_HERSHEY_SIMPLEX, 0.5, purple, 1);
        }
    }
    if(obj->isShowPatches) LabelPatchContourPoint(obj->possiblePatches, obj->initMatSize, resultsMat);
    cv::cvtColor(resultsMat, resultsMat, CV_BGR2RGB);
   
    
}

static void drawOriginalFoldline(popupObject *obj, cv::Mat &resultsMat){
    
    for(size_t i=0; i< obj->foldLine.size(); i++){
        
        if(!obj->foldLine[i]->isOriginalFoldLine) continue;
        ostringstream oss;
        oss.str("");
        oss<< "f" << i;
        
        cv::Point op1 = obj->foldLine[i]->line.first;
        cv::Point op2 = obj->foldLine[i]->line.second;
        
        cv::Point p = cv::Point(obj->foldLine[i]->line.first.x,(obj->foldLine[i]->line.first.y + obj->foldLine[i]->line.second.y)/2 );
        cv::line(resultsMat, op1, op2, red, 2);
        if(obj->isShowFoldlines) putText(resultsMat, oss.str(), p, FONT_HERSHEY_SIMPLEX, 0.5, purple, 1);

    }
    
    if(obj->isShowPatches) LabelPatchContourPoint(obj->classifiedPatches, obj->initMatSize, resultsMat);
    cv::cvtColor(resultsMat, resultsMat, CV_BGR2RGB);
}

static void showChanged(popupObject *obj, QLabel *label_resultLayout){
    
    cv::Mat resultsMat = drawPatch(obj->classifiedPatches, obj->initMatSize);
    if(obj->isShowOriginalPatches) drawOriginalFoldline(obj, resultsMat);
    else drawActiveFoldline(obj, resultsMat);
    QImage image = mat2QImage(resultsMat , QImage::Format_RGB888);
    label_resultLayout->setPixmap(QPixmap::fromImage(image));
}

static void drawResultLayout(popupObject *obj, cv::Point p, QLabel *label_resultLayout){
    
    int clickPatch = obj->clickPossiblePatch(p);
    cv::Mat resultsMat = drawPatch(obj->classifiedPatches, obj->initMatSize);

    if(clickPatch < 0){
        QImage image = mat2QImage(resultsMat , QImage::Format_RGB888);
        label_resultLayout->setPixmap(QPixmap::fromImage(image));
        return;
    }
    
    drawPatchWOClearMat(obj->classifiedPatches, resultsMat);
    drawActiveFoldline(obj, resultsMat);
    QImage image = mat2QImage(resultsMat , QImage::Format_RGB888);
    label_resultLayout->setPixmap(QPixmap::fromImage(image));
}


#endif /* functionEvenFilter_h */
