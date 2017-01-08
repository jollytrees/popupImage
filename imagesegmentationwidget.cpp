#include "imagesegmentationwidget.h"

//#include <Qt\qwidget.h>
#include <QPainter>
#include <Qmessagebox>
#include <Qevent>
#include <Qmouseeventtransition>
#include <QWheelEvent>
#include <Qinputdialog>
#include <Qfiledialog>
#include <QGridLayout>
#include <QLabel>

#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/imgcodecs/imgcodecs.hpp>

using namespace std;
using namespace cv;

 ImageSegmentationWidget::ImageSegmentationWidget(const QImage &image) : image_(new QImage(image))
{
    ori_width_ = image_->width();
    zoom_scale_ = 1.0;
    last_position_ = QPoint(-1, -1);

    showing_mode_ = 'O';

    num_segments_ = 0;

    setMouseTracking(true);
}

  ImageSegmentationWidget::ImageSegmentationWidget(QWidget *parent) : QWidget(parent)
{
    image_ = NULL;
    segmentation_image_ = NULL;

    ori_width_ = -1;
    zoom_scale_ = 1.0;
    last_position_ = QPoint(-1, -1);

    showing_mode_ = 'N';

    setMouseTracking(true);
}

 ImageSegmentationWidget::~ImageSegmentationWidget()
{
}

void ImageSegmentationWidget::setImage(const QImage &image)
{
    if (image_ != NULL)
        delete image_;
    image_ = new QImage(image);
    ori_width_ = image_->width();

    showing_mode_ = 'O';
    update();
}

void ImageSegmentationWidget::paintEvent(QPaintEvent *event)
{
    if (showing_mode_ == 'S') {

        QPainter widget_painter(this);
        widget_painter.drawImage(QRect(0, 0, this->width(), this->height()), *segmentation_image_);

    }
    else if (showing_mode_ == 'O') {
        QPainter widget_painter(this);
        const int IMAGE_SIZE = min(this->width(), this->height());
        widget_painter.drawImage(QRect(0, 0, this->width(), this->height()), *image_);
        widget_painter.setPen(QPen(Qt::white, 2));
        for (int i = 0; i < user_indication_points_.size(); i++)
            widget_painter.drawPoint(user_indication_points_[i]);
    }
    else {
        QPainter widget_painter(this);
        widget_painter.background();
    }
}

void ImageSegmentationWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & (Qt::LeftButton | Qt::RightButton)) == 0 || event->pos() == last_position_
            || event->pos().x() < 0 || event->pos().x() >= this->width() || event->pos().y() < 0 || event->pos().y() >= this->height())
        return;
    char drawing_mode;
    if ((event->buttons() & Qt::LeftButton) != 0)
        drawing_mode = 'I';
    else
        drawing_mode = 'E';
    if (drawing_mode == 'I') {
        if (last_position_.x() < 0 || last_position_.y() < 0)
            user_indication_points_.push_back(event->pos());
        else {
            vector<QPoint> stroke_points = calcIntermediatePoints(last_position_, event->pos());
            user_indication_points_.insert(user_indication_points_.end(), stroke_points.begin(), stroke_points.end());
        }
    } else if (drawing_mode == 'E') {
        QPoint point = event->pos();
        vector<QPoint> new_user_indication_points;
        const int ERASER_SIZE = 5;
        for (int i = 0; i < user_indication_points_.size(); i++)
            if (abs(user_indication_points_[i].x() - point.x()) > ERASER_SIZE || abs(user_indication_points_[i].y() - point.y()) > ERASER_SIZE)
                new_user_indication_points.push_back(user_indication_points_[i]);
        user_indication_points_ = new_user_indication_points;
    }
    last_position_ = event->pos();
    update();
}

void ImageSegmentationWidget::mouseReleaseEvent(QMouseEvent *)
{
    last_position_ = QPoint(-1, -1);
}

vector<int> ImageSegmentationWidget::convertPoints(const vector<QPoint> &drawed_points)
{
    vector<int> points(drawed_points.size());

    const int IMAGE_WIDTH = image_->width();
    const int IMAGE_HEIGHT = image_->height();
    const int WIDGET_WIDTH = this->width();
    const int WIDGET_HEIGHT = this->height();
//    const int WIDGET_SIZE = min(this->width(), this->height());
//    const int START_X = this->width() / 2 - IMAGE_SIZE / 2;
//    const int START_Y = this->height() / 2 - IMAGE_SIZE / 2;
    const int START_X = 0;
    const int START_Y = 0;
    for (int i = 0; i < drawed_points.size(); i++) {
        QPoint point = drawed_points[i];
        const double SCALE_FACTOR_X = 1.0 * IMAGE_WIDTH / WIDGET_WIDTH;
        const double SCALE_FACTOR_Y = 1.0 * IMAGE_HEIGHT / WIDGET_HEIGHT;
        int image_x = static_cast<int>((point.x() - START_X) * SCALE_FACTOR_X + 0.5);
        int image_y = static_cast<int>((point.y() - START_Y) * SCALE_FACTOR_Y + 0.5);

        points[i] = image_y * IMAGE_WIDTH + image_x;
    }
    return points;
}

void ImageSegmentationWidget::setSegmentation(const vector<int> &segmentation, const int IMAGE_WIDTH, const int IMAGE_HEIGHT)
{
    segmentation_ = segmentation;
    for (int i = 0; i < segmentation_.size(); i++)
        if (segmentation_[i] > num_segments_)
            num_segments_ = segmentation_[i];
    num_segments_++;
    showing_mode_ = 'S';
    drawSegmentation(IMAGE_WIDTH, IMAGE_HEIGHT);
    update();
}

vector<int> ImageSegmentationWidget::getSegmentation()
{
    return segmentation_;
}

void ImageSegmentationWidget::updateImages()
{
    update();
}

int ImageSegmentationWidget::readIndex(const QPoint point)
{
    const int IMAGE_WIDTH = image_->width();
    const int IMAGE_HEIGHT = image_->height();
    const int WIDGET_WIDTH = this->width();
    const int WIDGET_HEIGHT = this->height();

    const int START_X = 0;
    const int START_Y = 0;
    const double SCALE_FACTOR_X = 1.0 * (IMAGE_WIDTH - 1) / (WIDGET_WIDTH - 1);
    const double SCALE_FACTOR_Y = 1.0 * (IMAGE_HEIGHT - 1) / (WIDGET_HEIGHT - 1);
    int image_x = static_cast<int>((point.x() - START_X) * SCALE_FACTOR_X + 0.5);
    int image_y = static_cast<int>((point.y() - START_Y) * SCALE_FACTOR_Y + 0.5);
    image_x = min(image_x, IMAGE_WIDTH - 1);
    image_y = min(image_y, IMAGE_HEIGHT - 1);

    return image_y * IMAGE_WIDTH + image_x;
}

void ImageSegmentationWidget::drawSegmentation(const int IMAGE_WIDTH, const int IMAGE_HEIGHT)
{
    if (showing_mode_ == 'N')
        return;
    if (segmentation_image_ != NULL)
        delete segmentation_image_;
    segmentation_image_ = new QImage(IMAGE_WIDTH, IMAGE_HEIGHT, QImage::Format_RGB888);

    map<int, int> color_table;
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            int label = segmentation_[y * IMAGE_WIDTH + x];
            if (label == -1) {
                segmentation_image_->setPixel(x, y, qRgb(0, 0, 0));
//                segmentation_image_->setPixel(x, y, qRgb(255, 255, 255));
                continue;
            }
            int color;
            if (color_table.count(label) > 0)
                color = color_table[label];
            else {
                color = rand() % (256 * 256 * 256);
                color_table[label] = color;
            }
            segmentation_image_->setPixel(x, y, qRgb(color / (256 * 256),
                    color % (256 * 256) / 256,
                    color % 256));

//            QRgb ori_color= image_->pixel(x, y);
//            segmentation_image_->setPixel(x, y, qRgb(min(qRed(ori_color) * 0.6 + color / (256 * 256) * 0.4, 255.0),
//                    min(qGreen(ori_color) * 0.6 + color % (256 * 256) / 256 * 0.4, 255.0),
//                    min(qBlue(ori_color) * 0.6 + color % 256 * 0.4, 255.0)));
        }
    }
    update();
}

void ImageSegmentationWidget::showOriginalImage()
{
    showing_mode_ = 'O';
    update();
}

void ImageSegmentationWidget::showSegmentationImage()
{
    showing_mode_ = 'S';
    update();
}

void ImageSegmentationWidget::restart()
{
    user_indication_points_.clear();
    showing_mode_ = 'O';
    update();
}

void ImageSegmentationWidget::refine()
{
    user_indication_points_.clear();
    showing_mode_ = 'O';
    update();
}

vector<QPoint> ImageSegmentationWidget::calcIntermediatePoints(const QPoint &point_1, const QPoint &point_2)
{
    vector<QPoint> points;
    int x_1 = point_1.x();
    int y_1 = point_1.y();
    int x_2 = point_2.x();
    int y_2 = point_2.y();
    if (x_1 == x_2 && y_1 == y_2) {
        return points;
    }
    if (abs(x_1 - x_2) > abs(y_1 - y_2)) {
         for (int x = min(x_1, x_2); x <= max(x_1, x_2); x++) {
             int y = y_1 + static_cast<int>(1.0 * (x - x_1) / (x_2 - x_1) * (y_2 - y_1) + 0.5);
             if (x == x_1 && y == y_1)
                 continue;
             points.push_back(QPoint(x, y));
         }
    }
    else {
        for (int y = min(y_1, y_2); y <= max(y_1, y_2); y++) {
            int x = x_1 + static_cast<int>(1.0 * (y - y_1) / (y_2 - y_1) * (x_2 - x_1) + 0.5);
            if (x == x_1 && y == y_1)
                continue;
            points.push_back(QPoint(x, y));
        }
    }
    return points;
}

void ImageSegmentationWidget::segmentImage()
{
    Mat ori_image(image_->height(), image_->width(), CV_8UC3);
    for (int y = 0; y < image_->height(); y++) {
        for (int x = 0; x < image_->width(); x++) {
            QColor color = image_->pixel(x, y);
            Vec3b color_vec;
            color_vec[0] = color.blue();
            color_vec[1] = color.green();
            color_vec[2] = color.red();
            ori_image.at<Vec3b>(y, x) = color_vec;
        }
    }

    Mat markerMask = Mat::zeros(image_->height(), image_->width(), CV_8UC1);
    vector<int> user_indication_pixels = convertPoints(user_indication_points_);
    for (vector<int>::const_iterator pixel_it = user_indication_pixels.begin(); pixel_it != user_indication_pixels.end(); pixel_it++)
        markerMask.at<uchar>(*pixel_it / image_->width(), *pixel_it % image_->width()) = 255;

//    Mat markers(markerMask.size(), CV_32S);
//    markerMask.convertTo(markers, CV_32S);


    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    findContours(markerMask, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

    if( contours.empty() )
        return;
    Mat markers(markerMask.size(), CV_32S);
    markers = Scalar::all(0);
    int compCount = 0;
    for(int idx = 0; idx >= 0; idx = hierarchy[idx][0], compCount++ )
        drawContours(markers, contours, idx, Scalar::all(compCount), -1, 8, hierarchy, INT_MAX);
    if( compCount == 0 )
        return;

    watershed(ori_image, markers);

    segmentation_.assign(image_->height() * image_->width(), 0);
    for (int y = 0; y < image_->height(); y++) {
        for (int x = 0; x < image_->width(); x++) {
            segmentation_[y * image_->width() + x] = markers.at<int>(y, x);
        }
    }

    for (int pixel = 0; pixel < image_->width() * image_->height(); pixel++)
        if (pixel % image_->width() == 0 || pixel % image_->width() == image_->width() - 1 || pixel / image_->width() == 0 || pixel / image_->width() == image_->height() - 1)
            segmentation_[pixel] = segmentation_[image_->width() + 1];
    while (true) {
        bool has_boundary_pixels = false;
        for (int pixel = 0; pixel < image_->width() * image_->height(); pixel++) {
            if (segmentation_[pixel] != -1)
                continue;
            has_boundary_pixels = true;
            vector<int> neighbor_pixels;
            if (pixel % image_->width() < image_->width() / 2 && pixel % image_->width() > 0)
                neighbor_pixels.push_back(pixel - 1);
            if (pixel % image_->width() >= image_->width() / 2 && pixel % image_->width() < image_->width() - 1)
                neighbor_pixels.push_back(pixel + 1);
            for (vector<int>::const_iterator neighbor_pixel_it = neighbor_pixels.begin(); neighbor_pixel_it != neighbor_pixels.end(); neighbor_pixel_it++)
                if (segmentation_[*neighbor_pixel_it] != -1)
                    segmentation_[pixel] = segmentation_[*neighbor_pixel_it];
        }
        if (has_boundary_pixels == false)
            break;
    }
}

void ImageSegmentationWidget::clearUserIndication()
{
    user_indication_points_.clear();
}
