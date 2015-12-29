//
//  oglwidget.hpp
//  popupImage
//
//  Created by jollytrees on 11/25/15.
//
//

#ifndef OGLWIDGET_H
#define OGLWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <OpenGL/GL.h>
#include <OpenGL/GLU.h>
#include "popupObject.h"
#include <QMouseEvent>

class OGLWidget : public QOpenGLWidget
{
public:
    OGLWidget(QWidget *parent = 0);
    ~OGLWidget();
    popupObject *obj;
    bool isDraw;
    void setObj(popupObject *obj);
    float alpha;
    float beta;
    QPoint last_position;
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    //void keyPressEvent(QKeyEvent *event);
};

#endif // OGLWIDGET_H
