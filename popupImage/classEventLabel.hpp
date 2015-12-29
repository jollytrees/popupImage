//
//  classEventLabel.hpp
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#ifndef classEventLabel_hpp
#define classEventLabel_hpp

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMouseEvent>
#include <QLabel>
#include <stdio.h>
class EventLabel : public QLabel {
protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
};
#endif /* classEventLabel_hpp */
