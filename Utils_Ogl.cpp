//
//  Utils_Ogl.cpp
//  ivModeling
//
//  Created by sulaispig on 7/4/14.
//  Copyright (c) 2014 nthu. All rights reserved.
//

#include "Utils_Ogl.h"
#include <OpenGL/GL.h>
#include <OpenGL/GLU.h>

//draw x,y,z axis
void Utils_Ogl::drawAxis()
{
    glPushAttrib( GL_CURRENT_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glBegin(GL_LINES);
    glColor4f(1.0f,0.0f,0.0f,1.0f);
    glVertex3d(0.0,0.0,0.0);
    glVertex3d(1.0,0.0,0.0);
    glColor4f(0.0f,1.0f,0.0f,1.0f);
    glVertex3d(0.0,0.0,0.0);
    glVertex3d(0.0,1.0,0.0);
    glColor4f(0.0f,0.0f,1.0f,1.0f);
    glVertex3d(0.0,0.0,0.0);
    glVertex3d(0.0,0.0,1.0);
    glEnd();
    
    glBegin(GL_POINTS);
    glColor4f(0.0f,0.0f,0.0f,1.0f);
    glVertex3d(0.0,0.0,0.0);
    
    glColor4f(1.0f,0.0f,0.0f,1.0f);
    glVertex3d(1.0,0.0,0.0);
    
    glColor4f(0.0f,1.0f,0.0f,1.0f);
    glVertex3d(0.0,1.0,0.0);
    
    glColor4f(0.0f,0.0f,1.0f,1.0f);
    glVertex3d(0.0,0.0,1.0);
    
    glEnd();
    
    
    glPopAttrib();
}

