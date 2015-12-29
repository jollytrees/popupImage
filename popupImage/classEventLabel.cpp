//
//  classEventLabel.cpp
//  popupImage
//
//  Created by jollytrees on 11/15/15.
//
//

#include "classEventLabel.hpp"


void EventLabel::mouseMoveEvent(QMouseEvent *event) {
    QString msg;
    msg.sprintf("<center><h1>Move: (%d, %d)</h1></center>",
                event->x(), event->y());
    this->setText(msg);
}

void EventLabel::mousePressEvent(QMouseEvent *event) {
    QString msg;
    msg.sprintf("<center><h1>Press: (%d, %d)</h1></center>",
                event->x(), event->y());
    this->setText(msg);
}

void EventLabel::mouseReleaseEvent(QMouseEvent *event) {
    QString msg;
    msg.sprintf("<center><h1>Release: (%d, %d)</h1></center>",
                event->x(), event->y());
    this->setText(msg);
}