//
//  functionInitPatch.h
//  Color_Segmentation
//
//  Created by jollytrees on 11/8/15.
//  Copyright © 2015 jollytrees. All rights reserved.
//

#ifndef functionInitPatch_h
#define functionInitPatch_h

static cv::Point2f pointAdd(cv::Point2f p, cv::Point2f q)
{
    p.x += q.x; p.y += q.y;
    return p;
}
static cv::Point2f pointTimes(float c, cv::Point2f p)
{
    p.x *= c; p.y *= c;
    return p;
}
static cv::Point2f Bernstein(float u, cv::Point2f *p)
{
    cv::Point2f a, b, c, d, r;
    
    a = pointTimes(pow(u,3), p[0]);
    b = pointTimes(3*pow(u,2)*(1-u), p[1]);
    c = pointTimes(3*u*pow((1-u),2), p[2]);
    d = pointTimes(pow((1-u),3), p[3]);
    r = pointAdd(pointAdd(a, b), pointAdd(c, d));
    
    return r;
}
static void DrawControlLine(cv::Point2f *p)
{
    CvPoint pc[4];
    for(int i=0;i<4;i++)
    {
        pc[i].x = (int)p[i].x;
        pc[i].y = (int)p[i].y;
    }
    
    //cvLine(image,pc[0],pc[1],CV_RGB(0,0,255),1,CV_AA,0);
    //cvLine(image,pc[2],pc[3],CV_RGB(0,0,255),1,CV_AA,0);
}

static void ecvDrawBezier(cv::Mat image, cv::Point2f* points, vector<cv::Point> &temp_contour)
{
    cv::Point2f pt_pre=cv::Point2f(points[0].x,points[0].y);
    cv::Point2f pt_now;
    int precision = 10;
    for (int i=0;i<=precision;i++)
    {
        float u = (float)i/precision;
        cv::Point2f newPt = Bernstein(u,points);
        
        pt_now.x = (int)newPt.x;
        pt_now.y = (int)newPt.y;
        
        if(i>0){
            cv::line(image,pt_now,pt_pre,CV_RGB(0,0,0),2,CV_AA, 0 );
            temp_contour.push_back(cv::Point(pt_pre.x, pt_pre.y));
        }
        pt_pre = pt_now;
    }
    //DrawControlLine(points);
}

static void ecvDrawBezierMat(cv::Mat label, cv::Point2f* points)
{
    cv::Point2f pt_pre=cv::Point2f(points[0].x,points[0].y);
    cv::Point2f pt_now;
    int precision = 10;
    for (int i=0;i<=precision;i++)
    {
        float u = (float)i/precision;
        cv::Point2f newPt = Bernstein(u,points);
        
        pt_now.x = (int)newPt.x;
        pt_now.y = (int)newPt.y;
        if(i>0)cv::line(label,pt_now,pt_pre,100,2);
        pt_pre = pt_now;
    }
    //DrawControlLine(points);
}

static void drawBezierCurve(cv::Mat label, cv::Mat image, float* p,  vector<cv::Point> &temp_contour){
    
    static cv::Point2f points[4];
    
    points[0].x = p[0];
    points[0].y = p[1];
    points[1].x = p[2];
    points[1].y = p[3];
    points[2].x = p[4];
    points[2].y = p[5];
    points[3].x = p[6];
    points[3].y = p[7];
    
    ecvDrawBezier(image, points, temp_contour);
    
    ecvDrawBezierMat(label, points);
    
}

static double coef_A(double p0, double p1, double p2, double p3){
    return  3 * p3 - 9 * p2 + 9 * p1 - 3 * p0;
}

static double coef_B(double p0, double p1, double p2){
    return  6 * p2 - 12 * p1 + 6 * p0;
}

static double coef_C(double p0, double p1){
    return  3 * p1 - 3 * p0;
}

static double Determinant(double a, double b, double c){
    return pow(b, 2.0) - 4*a*c;
}

static double _solve(double a, double b, double c, bool s){
    return (-b+(sqrt(Determinant(a, b, c))*((s)?1:-1)))/(2*a);
}

static void solve(double a, double b, double c, vector<double> &rs){
    double d = Determinant(a, b, c);
    if(d < 0)
        return;
    if(a == 0){
        rs.push_back(_solve(a, b, c, true));
    }else{
        rs.push_back(_solve(a, b, c, true));
        rs.push_back(_solve(a, b, c, false));
    }
}

static void drawBBox(cv::Mat label, cv::Mat image, float* p, vector<cv::Point2f> &rs, vector<cv::Point2f> &rp){
    static cv::Point2f points[4];
    
    points[0].x = p[0];
    points[0].y = p[1];
    points[1].x = p[2];
    points[1].y = p[3];
    points[2].x = p[4];
    points[2].y = p[5];
    points[3].x = p[6];
    points[3].y = p[7];
    
    double aX = coef_A( points[0].x, points[1].x, points[2].x, points[3].x);
    double bX = coef_B( points[0].x, points[1].x, points[2].x);
    double cX = coef_C( points[0].x, points[1].x);
    
    double aY = coef_A( points[0].y, points[1].y, points[2].y, points[3].y);
    double bY = coef_B( points[0].y, points[1].y, points[2].y);
    double cY = coef_C( points[0].y, points[1].y);
    
    vector<double> _resX;
    vector<double> _resY;
    vector<double> resX;
    vector<double> resY;
    solve(aX, bX, cX, _resX);
    for(int i=0; i<_resX.size(); i++){
        if (_resX[i]>=0 && _resX[i]<=1) {
            resX.push_back(_resX[i]);
        }
    }
    solve(aY, bY, cY, _resY);
    for(int i=0; i<_resY.size(); i++){
        if (_resY[i]>=0 && _resY[i]<=1) {
            resY.push_back(_resY[i]);
        }
    }
    
    vector<cv::Point2f> bbox;
    bbox.push_back(points[0]);
    bbox.push_back(points[3]);
    vector<double> times;
    std::vector<double>::iterator it;
    it = times.begin();
    if(resX.size()>0)times.insert(it, resX.begin(), resX.end());
    it = times.begin();
    if(resY.size()>0)times.insert(it, resY.begin(), resY.end());
    
    for(int i=0; i < times.size(); i++){
        cv::Point2f _p;
        _p =  Bernstein(times[i], points);
        bbox.push_back(_p);
    }
    
    vector<cv::Point2f> bbox_f;
    
    for(int i=0; i < bbox.size(); i++){
        cv::Point2f _p;
        _p.x = bbox[i].x;
        _p.y = bbox[i].y;
        bbox_f.push_back(_p);
    }
    vector<cv::Point2f>::iterator f_it;
    f_it = rp.begin();
    rp.insert(f_it, bbox_f.begin(), bbox_f.end());
    
    cv::Rect re = cv::boundingRect(bbox_f);
    rs.push_back(re.tl());
    rs.push_back(re.br());
}


#endif /* functionInitPatch_h */
