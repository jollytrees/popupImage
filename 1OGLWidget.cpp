//
//  oglwidget.cpp
//  popupImage
//
//  Created by jollytrees on 11/25/15.
//
//

#include "oglwidget.h"

OGLWidget::OGLWidget(QWidget *parent)
: QOpenGLWidget(parent)
{
    
}

OGLWidget::~OGLWidget()
{
    
}

void OGLWidget::initializeGL()
{
    glClearColor(0,0,0,1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void OGLWidget::getPatch(popupObject *obj){

    cout  << "int" << endl;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_POLYGON);
    glVertex2f(0.0, 0.0);
    glVertex2f(0.0, 1.0);
    glVertex2f(2.0, 1.0);
    glVertex2f(3.0, 1.5);
    glVertex2f(2.0, 0.0);
    glEnd();
}

void OGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glBegin(GL_TRIANGLES);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(-0.5, -0.5, 0);
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f( 0.5, -0.5, 0);
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f( 0.0,  0.5, 0);
    glEnd();
    
    glBegin(GL_POLYGON);
    glVertex2f(0.0, 0.0);
    glVertex2f(0.0, 1.0);
    glVertex2f(2.0, 1.0);
    glVertex2f(3.0, 1.5);
    glVertex2f(2.0, 0.0);
    glEnd();

    

}

void OGLWidget::resizeGL(int w, int h)
{
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (float)w/h, 0.01, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0,0,5,0,0,0,0,1,0);
}