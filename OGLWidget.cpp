//
//  oglwidget.cpp
//  popupImage
//
//  Created by jollytrees on 11/25/15.
//
//

#include "oglwidget.h"
#include "simple_svg_1.0.0.hpp"
#include "polygonTriangulate.hpp"
#include "OGLWidget.h"

using namespace svg;


static void drawAxis()
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

//draw XY plane
static void drawGridXY(int xw, int yw)
{
    glPushAttrib( GL_POLYGON_BIT | GL_CURRENT_BIT );
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
    
    for(int x=-xw;x<xw;x++)
    {
        float xc1 = (float)x/xw;
        float xc2 = (float)(x+1)/xw;
        
        for(int y=-yw;y<yw;y++)
        {
            float yc1 = (float)y/yw;
            float yc2 = (float)(y+1)/yw;
            
            glVertex3f(xc1,yc1,0.0f);
            glVertex3f(xc2,yc1,0.0f);
            glVertex3f(xc2,yc2,0.0f);
            glVertex3f(xc1,yc2,0.0f);
        }
    }
    
    glEnd();
    glPopAttrib();
}

static void renderColoredGridXY(int xw, int yw, float alpha, float r, float g, float b, float diff=1.0f)
{
    glPushAttrib( GL_CURRENT_BIT );
    glBegin(GL_QUADS);
    
    for(int x=-xw;x<xw;x++)
    {
        float xc1 = (float)x/xw;
        float xc2 = (float)(x+1)/xw;
        
        for(int y=-yw;y<yw;y++)
        {
            //cornflower color (0.392, 0.584, 0.929)
            float c = 1.0f + diff * (std::abs(x+y)%2);
            glColor4f( r/c, g/c, b/c, alpha );
            
            float yc1 = (float)y/yw;
            float yc2 = (float)(y+1)/yw;
            
            glVertex3f(xc1,yc1,0.0f);
            glVertex3f(xc2,yc1,0.0f);
            glVertex3f(xc2,yc2,0.0f);
            glVertex3f(xc1,yc2,0.0f);
        }
    }
    
    glEnd();
    
    glColor4f( r/1.5f, g/1.5f, b/1.5f, alpha );
    drawGridXY(xw,yw);
    
    glPopAttrib();
}


OGLWidget::OGLWidget(QWidget *parent)
: QGLWidget(parent)
{
    isDraw = false;

}

OGLWidget::~OGLWidget()
{
    
}

void OGLWidget::initializeGL()
{
    alpha = 0.0f;
    beta = M_PI/4.0f;
    glClearColor(0,0,0,1);
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_LIGHT0);
//    glEnable(GL_LIGHTING);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void OGLWidget::setObj(popupObject *obj){

    this->obj = obj;
    Dimensions dimensions(obj->initMatSize.width, obj->initMatSize.height);
    Document doc("my_svg.svg", Layout(dimensions, Layout::TopLeft));
    
    for(int i=0; i< obj->possiblePatches.size(); i++){
        for(int j=0; j< obj->possiblePatches[i]->paths.size();j++){
            Polygon plygn(Color(200, 160, 220), Stroke(.5, Color(150, 160, 200)));

            for(int k=0; k< obj->possiblePatches[i]->paths[j].size(); k++){
                int x = obj->possiblePatches[i]->paths[j][k].x;
                int y = obj->possiblePatches[i]->paths[j][k].y;
                plygn << Point(x, y);
            }
            
            doc << plygn;
        }
    }
    
    doc.save();

    
    isDraw = true;
}

void OGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    glPushMatrix();

    float smRadius = std::cos(beta);
    GLdouble eyeX = smRadius * std::cos(alpha);
    GLdouble eyeY = smRadius * std::sin(alpha);
    GLdouble eyeZ = std::sin(beta);;
    gluLookAt( eyeX, eyeY, eyeZ, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 );
   // glScalef(obj->scale*0.5+1, obj->scale*0.5+1, obj->scale*0.5+1);
    //glScalef(0.5,0.5,0.5);================================================================================================================================================
    glScalef(0.5,0.5,0.5);
    drawAxis();
    glPushMatrix();
    glTranslatef(0, 0, -0.01);
    renderColoredGridXY(10, 10, .5f, 0.392f, 0.584f, 0.929f);
    glPopMatrix();
    glTranslatef( -0.5f, -0.5f, 0.0f );
    
    glPushAttrib(GL_COLOR);
    
    // Create a pretty complicated little contour by pushing them onto
    // an stl vector.
    for(int pIdx=0; isDraw && pIdx<obj->possiblePatches.size(); pIdx++){

        if(obj->positionLineIdxOfPatch[pIdx].size()==0) continue;
        
        int fIdx = obj->positionLineToFoldLine[obj->positionLineIdxOfPatch[pIdx].front().first];
        int oriX = obj->foldLine[fIdx]->line.first.x;
        int positioinIdx = obj->positionLineIdxOfPatch[pIdx].front().first;
        
        Vector2dVector a;
        for(size_t i=0; i< obj->possiblePatches[pIdx]->paths[0].size(); i++){
            cv::Point p = obj->possiblePatches[pIdx]->paths[0][i];
            a.push_back( Vector2d(p.x, p.y));
            glColor3f(0.0, 1.0, 0.0);

           
            /*if(obj->orientation[pIdx]&& i< obj->possiblePatches[pIdx]->paths[0].size()-1){
                cv::Point p2 = obj->possiblePatches[pIdx]->paths[0][i+1];

                glBegin(GL_LINES);
                glVertex3f( obj->X[positioinIdx]/obj->initMatSize.width, 1-p.y/obj->initMatSize.width, (p.x-obj->X[positioinIdx])/obj->initMatSize.width );
                glVertex3f( obj->X[positioinIdx]/obj->initMatSize.width, 1-p2.y/obj->initMatSize.width, (p2.x-obj->X[positioinIdx])/obj->initMatSize.width );
                glEnd();
            }*/
        }
        
        
        // allocate an STL vector to hold the answer.
        Vector2dVector result;
        
        //  Invoke the triangulator to triangulate this polygon.
        Triangulate::Process(a,result);
        
        
        // print out the results.
        int tcount = result.size()/3;
        for (int i=0; i<tcount; i++)
        {
            const Vector2d &p1 = result[i*3+0];
            const Vector2d &p2 = result[i*3+1];
            const Vector2d &p3 = result[i*3+2];
           // printf("Triangle %d => (%0.0f,%0.0f) (%0.0f,%0.0f) (%0.0f,%0.0f)\n",i+1,p1.GetX(),p1.GetY(),p2.GetX(),p2.GetY(),p3.GetX(),p3.GetY());
            glPushMatrix();
            glColor3f(1.0, 0.0, 0.0);

            
            //cout << "pid " << pIdx << " X=" << obj->X[positioinIdx] << "Y=" << obj->Y[positioinIdx] << "poIdx" << positioinIdx<<endl;
            
            if(obj->orientation[pIdx]){
               // glRotatef(-90, 0, 1, 0);
                
                glColor3f(1.0, 1.0, 0.0);
               // cout <<p.x-obj->Y[positioinIdx] << " " <<obj->Y[positioinIdx] << " x="<<obj->X[positioinIdx] << "y=" << obj->Y[positioinIdx];

            glBegin(GL_TRIANGLES);
           // glVertex3f(p1.GetX()/obj->initMatSize.width, 1-p1.GetY()/obj->initMatSize.width, obj->Y[idx]/obj->initMatSize.width);
           // glVertex3f( p2.GetX()/obj->initMatSize.width, 1-p2.GetY()/obj->initMatSize.width, obj->Y[idx]/obj->initMatSize.width);
           // glVertex3f( p3.GetX()/obj->initMatSize.width,  1-p3.GetY()/obj->initMatSize.width, obj->Y[idx]/obj->initMatSize.width);
                glVertex3f( obj->X[positioinIdx]/obj->initMatSize.width, 1-p1.GetY()/obj->initMatSize.width, (p1.GetX()-obj->X[positioinIdx])/obj->initMatSize.width );
                glVertex3f( obj->X[positioinIdx]/obj->initMatSize.width, 1-p2.GetY()/obj->initMatSize.width, (p2.GetX()-obj->X[positioinIdx])/obj->initMatSize.width);
                glVertex3f( obj->X[positioinIdx]/obj->initMatSize.width, 1-p3.GetY()/obj->initMatSize.width, (p3.GetX()-obj->X[positioinIdx])/obj->initMatSize.width);
                
                glEnd();
                
            }else{
                
                glBegin(GL_TRIANGLES);
                glVertex3f( (p1.GetX()-obj->Y[positioinIdx])/obj->initMatSize.width, 1-p1.GetY()/obj->initMatSize.width, obj->Y[positioinIdx]/obj->initMatSize.width );
                glVertex3f( (p2.GetX()-obj->Y[positioinIdx])/obj->initMatSize.width, 1-p2.GetY()/obj->initMatSize.width, obj->Y[positioinIdx]/obj->initMatSize.width);
                glVertex3f( (p3.GetX()-obj->Y[positioinIdx])/obj->initMatSize.width, 1-p3.GetY()/obj->initMatSize.width, obj->Y[positioinIdx]/obj->initMatSize.width);                glEnd();
            
            }
            
            glPopMatrix();
            
            
            
        }
        
        glEnd();

    }
    
    
    glPopAttrib();
    glPopMatrix();
    update();

}

/*
void OGLWidget::mousePressEvent(QMouseEvent *event) {
    
}*
/*void OGLWidget::mouseMoveEvent(QMouseEvent *event) {
    rot_x = event->x - old_rot_x;
    rot_y = y - old_rot_y;
    glutPostRedisplay();
}*/
/*
void OGLWidget::keyPressEvent(QKeyEvent* event) {
    switch(event->key()) {
        case Qt::Key_Escape:
            close();
            break;
        default:
            event->ignore();
            break;
    }
}
*/

void OGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    
    QPoint dp = event->pos() - last_position;
    last_position = event->pos();
    
    float speed = .5f;
    this->alpha -= dp.x() * M_PI * speed / 180.0f;
    this->beta  += dp.y() * M_PI * speed / 180.0f;
    
    if( this->alpha > 2*M_PI )
        this->alpha = 0.0f;
    if( this->alpha < 0.0f )
        this->alpha = 2*M_PI;
    
    float range = 10.0f*M_PI/180.0f;
    if( this->beta > M_PI/2 - range )
        this->beta = M_PI/2 - range;
    if( this->beta < range )
        this->beta = range;
    
    //abc
    //cout << this->alpha << " ab " << this->beta << endl;
}


void OGLWidget::mousePressEvent(QMouseEvent *event)
{
    last_position = event->pos();
    
}

void OGLWidget::resizeGL(int w, int h)
{
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90, (float)w/h, 0.0001f, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //gluLookAt(3,0,3,0,0,0,0,1,0);
}
