#ifndef IMAGESEGMENTATIONWIDGET_H
#define IMAGESEGMENTATIONWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QPoint>

#include <vector>

class ImageSegmentationWidget : public QWidget
{
    Q_OBJECT
/*
public:
     ImageSegmentationWidget(const QImage &image);
     ImageSegmentationWidget(QWidget *parent = 0);
     ~ImageSegmentationWidget();

    void setImage(const QImage &image);
    void setSegmentation(const std::vector<int> &segmentation, const int IMAGE_WIDTH, const int IMAGE_HEIGHT);
    void updateImages();
    std::vector<int> getSegmentation();
    void segmentImage();
    void clearUserIndication();

private:
    QImage *image_;
    QImage *segmentation_image_;
    std::vector<int> segmentation_;

    int num_segments_;

    char showing_mode_;

    int ori_width_;
    float zoom_scale_;
    QPoint last_position_;

    std::vector<QPoint> user_indication_points_;
    std::vector<int> indicated_points_;



    std::vector<int> convertPoints(const std::vector<QPoint> &drawed_points);
    int readIndex(const QPoint point);

    void drawSegmentation(const int IMAGE_WIDTH, const int IMAGE_HEIGHT);

    std::vector<QPoint> calcIntermediatePoints(const QPoint &point_1, const QPoint &point_2);

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);


public slots:

    void showOriginalImage();
    void showSegmentationImage();

    void restart();
    void refine();
private slots:
*/
};

#endif // IMAGESEGMENTATIONWIDGET_H
