#include "ImageSegmentationWidget.h"

//#include <Qt\qwidget.h>
#include <QPainter>>
#include <Qmessagebox>
#include <Qevent>
#include <Qmouseeventtransition>
#include <QWheelEvent>
#include <Qinputdialog>
#include <Qfiledialog>
#include <QGridLayout>
#include <QLabel>

#include <iostream>



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
    if (drawing_mode == 'I')
        user_indication_points_.push_back(event->pos());
    else if (drawing_mode == 'E') {
        QPoint point = event->pos();
        vector<QPoint> new_user_indication_points;
        const int ERASER_SIZE = 3;
        for (int i = 0; i < user_indication_points_.size(); i++)
            if (abs(user_indication_points_[i].x() - point.x()) > ERASER_SIZE || abs(user_indication_points_[i].y() - point.y()) > ERASER_SIZE)
                new_user_indication_points.push_back(user_indication_points_[i]);
        user_indication_points_ = new_user_indication_points;
    }
    last_position_ = event->pos();
    update();
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

void ImageSegmentationWidget::setSegmentation(const vector<int> &segmentation)
{
    segmentation_ = segmentation;
    for (int i = 0; i < segmentation_.size(); i++)
        if (segmentation_[i] > num_segments_)
            num_segments_ = segmentation_[i];
    num_segments_++;
    showing_mode_ = 'S';
    drawSegmentation();
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

void ImageSegmentationWidget::drawSegmentation()
{
    if (showing_mode_ == 'N')
        return;
    if (segmentation_image_ != NULL)
        delete segmentation_image_;
    segmentation_image_ = new QImage(*image_);

    const int WIDTH = image_->width();
    const int HEIGHT = image_->height();
    map<int, int> color_table;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int label = segmentation_[y * WIDTH + x];
            if (label == -1) {
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
            QRgb ori_color= image_->pixel(x, y);

            segmentation_image_->setPixel(x, y, qRgb(min(qRed(ori_color) * 0.6 + color / (256 * 256) * 0.4, 255.0),
                    min(qGreen(ori_color) * 0.6 + color % (256 * 256) / 256 * 0.4, 255.0),
                    min(qBlue(ori_color) * 0.6 + color % 256 * 0.4, 255.0)));
        }
    }
    update();
}

void ImageSegmentationWidget::showOriginalImage()
{
    showing_mode_ = 'O';
}

void ImageSegmentationWidget::showSegmentationImage()
{
    showing_mode_ = 'S';
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
