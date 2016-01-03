#include "ImageWidget.h"

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



ImageWidget::ImageWidget(const QImage *image) : image_(new QImage(*image))
{
    ori_width_ = image->width();
    zoom_scale_ = 1.0;
    last_position_ = QPoint(-1, -1);

    drawing_mode_ = 'N';
    showing_mode_ = 'S';

    num_segments_ = 0;

    setMouseTracking(true);
}

ImageWidget::ImageWidget()
{
    image_ = NULL;
    segmentation_image_ = NULL;

    ori_width_ = -1;
    zoom_scale_ = 1.0;
    last_position_ = QPoint(-1, -1);

    drawing_mode_ = 'N';
    showing_mode_ = 'N';

    setMouseTracking(true);
}

ImageWidget::~ImageWidget()
{
}

void ImageWidget::paintEvent(QPaintEvent *event)
{
    if (showing_mode_ == 'S') {

        QPainter widget_painter(this);
        const int IMAGE_SIZE = min(this->width(), this->height());
        widget_painter.drawImage(QRect(0, 0, this->width(), this->height()), *segmentation_image_);
//            widget_painter.drawImage(QRect(this->width() / 2 - IMAGE_SIZE / 2, this->height() / 2 - IMAGE_SIZE / 2, IMAGE_SIZE, IMAGE_SIZE), *image_);
//        if (showing_mode_ == 'C') {
//            widget_painter.setOpacity(0.5);
//            widget_painter.drawImage(QRect(this->width() / 2 - IMAGE_SIZE / 2, this->height() / 2 - IMAGE_SIZE / 2, IMAGE_SIZE, IMAGE_SIZE), *ori_slice_image_);
//            widget_painter.drawImage(QRect(this->width() / 2 - IMAGE_SIZE / 2, this->height() / 2 - IMAGE_SIZE / 2, IMAGE_SIZE, IMAGE_SIZE), *bone_slice_image_);
//        }


        widget_painter.setPen(QPen(Qt::white, 2));
        for (int i = 0; i < selected_region_boundary_points_.size(); i++)
            widget_painter.drawPoint(selected_region_boundary_points_[i]);

        widget_painter.setPen(QPen(Qt::red, 2));
        for (int i = 0; i < boundary_points_.size(); i++)
            widget_painter.drawPoint(boundary_points_[i]);

        widget_painter.setPen(QPen(Qt::red, 3));
        for (int i = 0; i < joint_lines_points_.size(); i++)
            widget_painter.drawPoint(joint_lines_points_[i]);
        widget_painter.setPen(QPen(Qt::red, 1));
        for (int i = 0; i + 1 < joint_lines_points_.size(); i++)
            widget_painter.drawLine(joint_lines_points_[i], joint_lines_points_[i + 1]);

        widget_painter.setPen(QPen(Qt::blue, 1));
        for (int i = 0; i < region_points_.size(); i++)
            widget_painter.drawPoint(region_points_[i]);
    }
    else if (showing_mode_ == 'O') {
        QPainter widget_painter(this);
        const int IMAGE_SIZE = min(this->width(), this->height());
        widget_painter.drawImage(QRect(0, 0, this->width(), this->height()), *image_);
    }
    else {
        QPainter widget_painter(this);
        widget_painter.background();
    }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (drawing_mode_ == 'N' || (event->buttons() & Qt::LeftButton) == 0 || event->pos() == last_position_
            || event->pos().x() < 0 || event->pos().x() >= this->width() || event->pos().y() < 0 || event->pos().y() >= this->height())
        return;
    if (drawing_mode_ == 'B')
        boundary_points_.push_back(event->pos());
    else if (drawing_mode_ == 'E') {
        QPoint point = event->pos();
        vector<QPoint> new_boundary_points;
        const int ERASER_SIZE = 3;
        for (int i = 0; i < boundary_points_.size(); i++)
            if (abs(boundary_points_[i].x() - point.x()) > ERASER_SIZE || abs(boundary_points_[i].y() - point.y()) > ERASER_SIZE)
                new_boundary_points.push_back(boundary_points_[i]);
        boundary_points_ = new_boundary_points;
        vector<QPoint> new_joint_line_points;
        for (int i = 0; i < joint_lines_points_.size(); i++)
            if (abs(joint_lines_points_[i].x() - point.x()) > ERASER_SIZE || abs(joint_lines_points_[i].y() - point.y()) > ERASER_SIZE)
                new_joint_line_points.push_back(joint_lines_points_[i]);
        joint_lines_points_ = new_joint_line_points;
        vector<QPoint> new_region_points;
        for (int i = 0; i < region_points_.size(); i++)
            if (abs(region_points_[i].x() - point.x()) > ERASER_SIZE || abs(region_points_[i].y() - point.y()) > ERASER_SIZE)
                new_region_points.push_back(region_points_[i]);
        region_points_ = new_region_points;
    }
    else if (drawing_mode_ == 'R') {
        QPoint point = event->pos();
        int x = point.x();
        int y = point.y();
        const int RANGE = 2;
        for (int delta_x = max(-RANGE, 0 - x); delta_x <= min(RANGE, this->width() - 1 - x); delta_x++)
            for (int delta_y = max(-RANGE, 0 - y); delta_y <= min(RANGE, this->height() - 1 - y); delta_y++)
                region_points_.push_back(QPoint(x + delta_x, y + delta_y));
    }
    last_position_ = event->pos();
    update();
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (selection_mode_ == 'O' || selection_mode_ == 'M') {
        QPoint point = event->pos();
        int point_index = readIndex(point);
//        cout << point_index % image_->width() << '\t' << point_index / image_->width() << endl;
        int segment_index = segmentation_[point_index];
        if (selected_segment_indices_.count(segment_index) > 0) {
            selected_segment_indices_.erase(segment_index);
            calcSelectedRegionBoundary();
            return;
        }

        if (selection_mode_ == 'O')
            selected_segment_indices_.clear();
        selected_segment_indices_.insert(segment_index);
        calcSelectedRegionBoundary();
    }
    if (drawing_mode_ == 'J') {
        joint_lines_points_.push_back(event->pos());
        update();
    }
}

void ImageWidget::erasePoints()
{
    selection_mode_ = 'N';
    drawing_mode_ = 'E';
    update();
}

vector<int> ImageWidget::convertPoints(const vector<QPoint> &drawed_points)
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

vector<int> ImageWidget::getBoundaryPoints()
{
    vector<int> points = convertPoints(boundary_points_);
    boundary_points_.clear();
    update();
    return points;
}

void ImageWidget::setShowingMode(const char showing_mode)
{
    showing_mode_ = showing_mode;
    update();
}

void ImageWidget::setImage(const QImage *image)
{
    if (image_ != NULL)
        delete image_;
    image_ = new QImage(*image);
}

void ImageWidget::setSegmentation(const vector<int> &segmentation)
{
    previous_segmentation_ = segmentation_;
    segmentation_ = segmentation;
    for (int i = 0; i < segmentation_.size(); i++)
        if (segmentation_[i] > num_segments_)
            num_segments_ = segmentation_[i];
    num_segments_++;
    showing_mode_ = 'S';
    calcSelectedRegionBoundary();
    drawSegmentation();
    update();
}

vector<int> ImageWidget::getSegmentation()
{
    return segmentation_;
}

vector<int> ImageWidget::getIndicatedPoints()
{
    return indicated_points_;
}

void ImageWidget::updateImages()
{
    update();
}

void ImageWidget::selectOneSegment()
{
    setSelectionMode('O');
}

void ImageWidget::selectMultipleSegments()
{
    setSelectionMode('M');
}

void ImageWidget::setSelectionMode(const char mode) {
    selection_mode_ = mode;
    drawing_mode_ = 'N';
    selected_segment_indices_.clear();
    selected_region_boundary_points_.clear();
    joint_lines_points_.clear();
    boundary_points_.clear();
    region_points_.clear();
    update();
}

int ImageWidget::readIndex(const QPoint point)
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

void ImageWidget::calcSelectedRegionBoundary()
{
    vector<int> points;
    const int WIDTH = image_->width();
    const int HEIGHT = image_->height();
    for (int x =  0; x < WIDTH; x++) {
        for (int y =  0; y < HEIGHT; y++) {
            int index = y * WIDTH + x;
            if (selected_segment_indices_.count(segmentation_[index]) == 0)
                continue;
            vector<int> neighbor_indices;
            if (x < WIDTH - 1)
                neighbor_indices.push_back(index + 1);
            if (x > 0)
                neighbor_indices.push_back(index - 1);
            if (y < HEIGHT - 1)
                neighbor_indices.push_back(index + WIDTH);
            if (y > 0)
                neighbor_indices.push_back(index - WIDTH);
            bool on_boundary = false;
            for (int i = 0; i < neighbor_indices.size(); i++) {
                int neighbor_index = neighbor_indices[i];
                if (neighbor_index >= 0 && neighbor_index < WIDTH * HEIGHT && selected_segment_indices_.count(segmentation_[neighbor_index]) == 0) {
                    on_boundary = true;
                    break;
                }
            }
            if (on_boundary)
                points.push_back(y * WIDTH + x);
        }
    }

    const int IMAGE_WIDTH = image_->width();
    const int IMAGE_HEIGHT = image_->height();
    const int WIDGET_WIDTH = this->width();
    const int WIDGET_HEIGHT = this->height();

    const double SCALE_FACTOR_X = 1.0 / IMAGE_WIDTH * WIDGET_WIDTH;
    const double SCALE_FACTOR_Y = 1.0 / IMAGE_HEIGHT * WIDGET_HEIGHT;

    selected_region_boundary_points_ = vector<QPoint>(points.size());
    for (int i = 0; i < points.size(); i++) {
        int image_x = points[i] % WIDTH;
        int image_y = points[i] / WIDTH;
        int widget_x = static_cast<int>(image_x * SCALE_FACTOR_X + 0.5);
        int widget_y = static_cast<int>(image_y * SCALE_FACTOR_Y + 0.5);
        selected_region_boundary_points_[i] = QPoint(widget_x, widget_y);
    }

    update();
}

void ImageWidget::mergeSegments()
{
    previous_segmentation_ = segmentation_;
    map<int, int> index_map;
    int new_index = 0;
    for (set<int>::const_iterator index_it = selected_segment_indices_.begin(); index_it != selected_segment_indices_.end(); index_it++)
        index_map[*index_it] = new_index;
    new_index++;
    for (int i = 0; i < previous_segmentation_.size(); i++) {
        int ori_index = previous_segmentation_[i];
        if (index_map.count(ori_index) == 0)
            index_map[ori_index] = new_index++;
        segmentation_[i] = index_map[ori_index];
    }
    num_segments_ = new_index;

    selected_segment_indices_.clear();
    selected_region_boundary_points_.clear();
    drawSegmentation();
}

void ImageWidget::createNewSegment()
{
    previous_segmentation_ = segmentation_;

    vector<int> new_segment_points;
    if (drawing_mode_ == 'R') {
        for (int i = 0; i + 1 < region_points_.size(); i++) {
            int index = readIndex(region_points_[i]);
            new_segment_points.push_back(index);
        }
        region_points_.clear();
    }
    else if (drawing_mode_ == 'J') {
        vector<int> contour_points;
        for (int i = 0; i + 1 < joint_lines_points_.size(); i++) {
            vector<int> points = calcIntermediatePoints(joint_lines_points_[i], joint_lines_points_[i + 1]);
            contour_points.insert(contour_points.end(), points.begin(), points.end());
        }
        if (joint_lines_points_.size() > 1) {
            vector<int> points = calcIntermediatePoints(joint_lines_points_[joint_lines_points_.size() - 1], joint_lines_points_[0]);
            contour_points.insert(contour_points.end(), points.begin(), points.end());
        }
        if (contour_points.size() == 0)
            return;

//        for (int i = 0; i < contour_points.size(); i++)
//            cout << contour_points[i] << endl;
//        return;
        const int WIDTH = image_->width();
        const int HEIGHT = image_->height();

        vector<bool> region_mask(segmentation_.size(), false);
        int sum_x = 0, sum_y = 0;
        for (int i = 0; i < contour_points.size(); i++) {
            int point = contour_points[i];
            region_mask[point] = true;
            sum_x += point % WIDTH;
            sum_y += point / WIDTH;
        }
        int center_x = sum_x / contour_points.size();
        int center_y = sum_y / contour_points.size();

        vector<int> border_points;
        border_points.push_back(center_y * image_->width() + center_x);
        while (border_points.size() > 0) {
            vector<int> new_border_points;
            for (int i = 0; i < border_points.size(); i++) {
                int point = border_points[i];
                int x = point % WIDTH;
                int y = point / WIDTH;
                if (region_mask[point] == true)
                    continue;
                region_mask[point] = true;
                new_segment_points.push_back(point);
                if (x > 0)
                    new_border_points.push_back(point - 1);
                if (x < WIDTH - 1)
                    new_border_points.push_back(point + 1);
                if (y > 0)
                    new_border_points.push_back(point - WIDTH);
                if (y < HEIGHT - 1)
                    new_border_points.push_back(point + WIDTH);
            }
            border_points = new_border_points;
        }
        new_segment_points.insert(new_segment_points.end(), contour_points.begin(), contour_points.end());
        joint_lines_points_.clear();
    }
    if (new_segment_points.size() == 0)
        return;

    int new_index = num_segments_;
    num_segments_++;
    for (int i = 0; i < new_segment_points.size(); i++)
        segmentation_[new_segment_points[i]] = new_index;

    indicated_points_.insert(indicated_points_.end(), new_segment_points.begin(), new_segment_points.end());

    drawSegmentation();
}

void ImageWidget::confirmSegmentPoints()
{
    if (selected_segment_indices_.size() != 1) {
        QMessageBox::information(this, "Sorry", "Please select one segment first.");
        return;
    }
    if (region_points_.size() == 0) {
        QMessageBox::information(this, "Sorry", "Please draw some region points for this segment.");
        return;
    }

    int segment_index = *selected_segment_indices_.begin();
    for (int i = 0; i < region_points_.size(); i++) {
        int point = readIndex(region_points_[i]);
        segmentation_[point] = segment_index;
        indicated_points_.push_back(point);
    }

    region_points_.clear();
    calcSelectedRegionBoundary();
    drawSegmentation();
}

void ImageWidget::drawJointLines()
{
    setDrawingMode('J');
}

void ImageWidget::drawBoundaryPoints()
{
    setDrawingMode('B');
}

void ImageWidget::drawRegionPoints()
{
    setDrawingMode('R');
}

void ImageWidget::setDrawingMode(const char mode)
{
    selection_mode_ = 'N';
    drawing_mode_ = mode;
    joint_lines_points_.clear();
    boundary_points_.clear();
    region_points_.clear();
    if (mode != 'R') {
        selected_segment_indices_.clear();
        selected_region_boundary_points_.clear();
    }
    update();
}

void ImageWidget::splitSegment()
{
    const int WIDTH = image_->width();
    const int HEIGHT = image_->height();

    vector<int> split_points;
    if (drawing_mode_ == 'J') {
        for (int i = 0; i + 1 < joint_lines_points_.size(); i++) {
            if (i == 0) {
                int point = readIndex(joint_lines_points_[i]);
                int x = point % WIDTH;
                int y = point / WIDTH;
                const int BORDER_SIZE = 3;
                if (x < BORDER_SIZE)
                    for (int border_x = 0; border_x < x; border_x++)
                        split_points.push_back(y * WIDTH + border_x);
                if (x >= WIDTH - BORDER_SIZE)
                    for (int border_x = x + 1; border_x < WIDTH; border_x++)
                        split_points.push_back(y * WIDTH + border_x);
                if (y < BORDER_SIZE)
                    for (int border_y = 0; border_y < y; border_y++)
                        split_points.push_back(border_y * WIDTH + x);
                if (y >= HEIGHT - BORDER_SIZE)
                    for (int border_y = y + 1; border_y < HEIGHT; border_y++)
                        split_points.push_back(border_y * WIDTH + x);
            }
            if (i + 2 == joint_lines_points_.size()) {
                int point = readIndex(joint_lines_points_[i + 1]);
                int x = point % WIDTH;
                int y = point / WIDTH;
                const int BORDER_SIZE = 3;
                if (x < BORDER_SIZE)
                    for (int border_x = 0; border_x < x; border_x++)
                        split_points.push_back(y * WIDTH + border_x);
                if (x >= WIDTH - BORDER_SIZE)
                    for (int border_x = x + 1; border_x < WIDTH; border_x++)
                        split_points.push_back(y * WIDTH + border_x);
                if (y < BORDER_SIZE)
                    for (int border_y = 0; border_y < y; border_y++)
                        split_points.push_back(border_y * WIDTH + x);
                if (y >= HEIGHT - BORDER_SIZE)
                    for (int border_y = y + 1; border_y < HEIGHT; border_y++)
                        split_points.push_back(border_y * WIDTH + x);
            }

            vector<int> points = calcIntermediatePoints(joint_lines_points_[i], joint_lines_points_[i + 1]);
            split_points.insert(split_points.end(), points.begin(), points.end());
        }
        joint_lines_points_.clear();
    }
    if (drawing_mode_ == 'B') {
        for (int i = 0; i + 1 < boundary_points_.size(); i++) {
            vector<int> points = calcIntermediatePoints(boundary_points_[i], boundary_points_[i + 1]);
//            for (int i = 0; i < points.size(); i++)
//                cout << points[i] % WIDTH << '\t' << points[i] / WIDTH << endl;
            split_points.insert(split_points.end(), points.begin(), points.end());
        }
        boundary_points_.clear();
    }
    if (split_points.size() == 0)
        return;

    vector<int> split_mask(segmentation_.size(), -1);
    map<int, int> segment_counter;
    for (int i = 0; i < split_points.size(); i++) {
        int point = split_points[i];
        int segment_index = segmentation_[point];
        segment_counter[segment_index]++;
        split_mask[point] = 0;
    }

    int split_index = -1;
    int max_count = 0;
    for (map<int, int>::const_iterator segment_it = segment_counter.begin(); segment_it != segment_counter.end(); segment_it++) {
        if (segment_it->second > max_count) {
            split_index = segment_it->first;
            max_count = segment_it->second;
        }
    }

    int point_on_one_side = -1;
    for (int i = 0; i < split_points.size(); i++) {
        int point = split_points[i];
        vector<int> neighbor_points;
        int x = point % WIDTH;
        int y = point / WIDTH;
        if (x < WIDTH - 1)
            neighbor_points.push_back(point + 1);
        if (x > 0)
            neighbor_points.push_back(point - 1);
        if (y < HEIGHT - 1)
            neighbor_points.push_back(point + WIDTH);
        if (y > 0)
            neighbor_points.push_back(point - WIDTH);

        for (int j = 0; j < neighbor_points.size(); j++) {
            int neighbor_point = neighbor_points[j];
            int segment_index = segmentation_[neighbor_point];
            if (split_mask[neighbor_point] != 0 && segment_index == split_index) {
                point_on_one_side = neighbor_point;
                break;
            }
        }
        if (point_on_one_side != -1)
            break;
    }

    vector<int> border_points;
    border_points.push_back(point_on_one_side);
    while (border_points.size() > 0) {
        vector<int> new_border_points;
        for (int i = 0; i < border_points.size(); i++) {
            if (segmentation_[border_points[i]] == split_index && split_mask[border_points[i]] == -1) {

                split_mask[border_points[i]] = 1;

                int x = border_points[i] % WIDTH;
                int y = border_points[i] / WIDTH;
                if (x < WIDTH - 1)
                    new_border_points.push_back(border_points[i] + 1);
                if (x > 0)
                    new_border_points.push_back(border_points[i] - 1);
                if (y < HEIGHT - 1)
                    new_border_points.push_back(border_points[i] + WIDTH);
                if (y > 0)
                    new_border_points.push_back(border_points[i] - WIDTH);
            }
        }
        border_points = new_border_points;
    }

    bool splitted = false;
    int max_index = 0;
    for (int i = 0; i < segmentation_.size(); i++) {
        int segment_index = segmentation_[i];
        if (segment_index == split_index && split_mask[i] == -1)
            splitted = true;
        if (segment_index > max_index)
            max_index = segment_index;
    }
    if (splitted == false)
        return;

    previous_segmentation_ = segmentation_;
    int new_index = max_index + 1;
    for (int i = 0; i < previous_segmentation_.size(); i++)
        if (split_mask[i] == 1)
            segmentation_[i] = new_index;

    num_segments_++;

    drawSegmentation();
}

vector<int> ImageWidget::calcIntermediatePoints(const QPoint start_point, const QPoint end_point)
{
    int start_point_index = readIndex(start_point);
    int end_point_index = readIndex(end_point);
    const int WIDTH = image_->width();
    const int HEIGHT = image_->height();

    int x_1 = start_point_index % WIDTH;
    int y_1 = start_point_index / WIDTH;
    int x_2 = end_point_index % WIDTH;
    int y_2 = end_point_index / WIDTH;

    vector<int> points;

    if (x_1 == x_2 && y_1 == y_2) {
        points.push_back(y_1 * WIDTH + x_1);
        return points;
    }
    if (abs(x_1 - x_2) > abs(y_1 - y_2)) {
         for (int x = min(x_1, x_2); x <= max(x_1, x_2); x++) {
             int y = y_1 + 1.0 * (x - x_1) / (x_2 - x_1) * (y_2 - y_1);
             points.push_back(y * WIDTH + x);
         }
    }
    else {
        for (int y = min(y_1, y_2); y <= max(y_1, y_2); y++) {
            int x = x_1 + static_cast<int>(1.0 * (y - y_1) / (y_2 - y_1) * (x_2 - x_1) + 0.5);
            points.push_back(y * WIDTH + x);
        }
    }
    return points;
}

void ImageWidget::drawSegmentation()
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

void ImageWidget::showOriginalImage()
{
    setShowingMode('O');
}

void ImageWidget::showSegmentationImage()
{
    setShowingMode('S');
}

void ImageWidget::undo()
{
    segmentation_ = previous_segmentation_;
    drawSegmentation();
}
