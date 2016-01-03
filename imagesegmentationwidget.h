#ifndef IMAGESEGMENTATIONWIDGET_H
#define IMAGESEGMENTATIONWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QPoint>

#include <vector>
#include <set>


using namespace std;

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    ImageWidget(const QImage *image);
    ImageWidget();
    ~ImageWidget();


    void setImage(const QImage *image);
    void setSegmentation(const vector<int> &segmentation);
    void setShowingMode(const char showing_mode);
    void updateImages();
    vector<int> getBoundaryPoints();
    vector<int> getSegmentation();
    vector<int> getIndicatedPoints();

private:
    QImage *image_;
    QImage *segmentation_image_;
    vector<int> segmentation_;
    vector<int> previous_segmentation_;

    int num_segments_;

    char showing_mode_;
    char selection_mode_;
    char drawing_mode_;

    int ori_width_;
    float zoom_scale_;
    QPoint last_position_;

    vector<QPoint> boundary_points_;
    vector<QPoint> region_points_;
    vector<QPoint> selected_region_boundary_points_;

    set<int> selected_segment_indices_;

    vector<QPoint> joint_lines_points_;

    vector<int> indicated_points_;


    void setDrawingMode(const char mode);
    void setSelectionMode(const char mode);

    vector<int> convertPoints(const vector<QPoint> &drawed_points);
    int readIndex(const QPoint point);
    vector<int> calcIntermediatePoints(const QPoint start_point, const QPoint end_point);

    void calcSelectedRegionBoundary();
    void drawSegmentation();


protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);


public slots:
    void selectOneSegment();
    void selectMultipleSegments();

    void drawJointLines();
    void drawBoundaryPoints();
    void drawRegionPoints();
    void erasePoints();

    void mergeSegments();
    void splitSegment();
    void createNewSegment();
    void confirmSegmentPoints();

    void showOriginalImage();
    void showSegmentationImage();

    void undo();
private slots:
};

#endif // IMAGESEGMENTATIONWIDGET_H
