#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QListWidget>
#include <QMouseEvent>

//popup file
#include "popupObject.h"
#include "popupObjAlgorithm.h"
#include "popupObjFindDividedPatchConnection.hpp"
#include "popupObjClassifyPatches.hpp"
#include "popupObjFindClassifiedPatchConnection.hpp"
#include "popupObjInitBoundaryFoldLine.hpp"
#include "popupObjBuildBoundaryLineConnMap.hpp"
#include "popupObjMergeBoundaryLine.hpp"
#include "popupObjAddConnectionLineToMap.hpp"
#include "popupObjExtendFoldLineAndFindActiveBolb.hpp"
#include "popupObjFindInsertedLine.hpp"
#include "popupObjFindPossiblePatch.hpp"
#include "popupObjFindPossiblePatchConnection.hpp"
#include "popupObjGraphFindAllPath.hpp"
#include "popupObjFindfoldLinePath.hpp"
#include "popupObjOptimization.hpp"
#include "popupObjmergeFinalPatches.hpp"
#include "popupObjFindFinalPath.hpp"

//draw fuction
#include "functionMat2QImage.h"
#include "functionDrawPatch.h"
#include "drawBoundaryLine.hpp"
#include "functionLabelContour.h"
#include "functionEventFilter.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    
private:

    Ui::MainWindow *ui;
    popupObject *obj;
    popupObjAlgorithmList algoList;
    int caseNum;
    void createItems();
    void processPopup();

    void createMenus();
    void loadImage();

private slots:
public slots:
    void chooseInputImage();
    void chooseBasePatch();
    void onItemClicked(QListWidgetItem * item);
    void showPatches();
    void showFoldLines();
    void showOriginalPatches();
    void showFinalPatches();
    void setScalse(int);
};

#endif // MAINWINDOW_H
