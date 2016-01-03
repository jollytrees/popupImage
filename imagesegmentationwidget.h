#ifndef IMAGESEGMENTATIONWIDGET_H
#define IMAGESEGMENTATIONWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QPoint>

#include <vector>
#include <set>


using namespace std;

class ImageSegmentationWidget : public QWidget
{
    Q_OBJECT

public:
    ImageSegmentationWidget(const QImage &image);
    ImageSegmentationWidget(QWidget *parent = 0);
    ~ImageSegmentationWidget();

    void setImage(const QImage &image);
    void setSegmentation(const vector<int> &segmentation);
    void updateImages();
    vector<int> getSegmentation();

private:
    QImage *image_;
    QImage *segmentation_image_;
    vector<int> segmentation_;

    int num_segments_;

    char showing_mode_;

    int ori_width_;
    float zoom_scale_;
    QPoint last_position_;

    vector<QPoint> user_indication_points_;
    vector<int> indicated_points_;



    vector<int> convertPoints(const vector<QPoint> &drawed_points);
    int readIndex(const QPoint point);

    void drawSegmentation();


protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);


public slots:

    void showOriginalImage();
    void showSegmentationImage();

    void restart();
    void refine();
private slots:
};

#endif // IMAGESEGMENTATIONWIDGET_H
