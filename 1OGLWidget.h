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
class OGLWidget : public QOpenGLWidget
{
public:
    OGLWidget(QWidget *parent = 0);
    ~OGLWidget();
    
    void getPatch(popupObject *obj);
    
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
};

#endif // OGLWIDGET_H
