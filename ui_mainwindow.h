/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>
#include <QtWidgets/QBoxLayout>
#include <oglwidget.h>
#include <imagesegmentationwidget.h>


QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QTabWidget *tabWidget;
    QWidget *tab;
    QPushButton *pushButton;
    QGroupBox *segmentation_button_group;
    QVBoxLayout *segmentation_button_layout;
    QPushButton *start_segmentation_button;
    QPushButton *refine_segmentation_button;
    QPushButton *clear_segmentation_button;
    QPushButton *confirm_segmentation_button;
    QLabel *label_pic1;
    QLabel *label_pic2;
    QLabel *label_pic3;
    QWidget *tab_2;
    QLabel *label_backdrop;
    QPushButton *pushButton_2;
    QWidget *tab_5;
    QLabel *label_resultLayout;
    QListWidget *listWidget;
    QListWidget *listWidget_patch;
    QLabel *label;
    QGroupBox *groupBox;
    QCheckBox *check_labelFoldline;
    QCheckBox *check_labelPatches;
    QTextBrowser *textBrowser;
    QGroupBox *groupBox_2;
    QRadioButton *radio_original;
    QRadioButton *radio_final;
    QWidget *tab_7;
    QWidget *tab_3;
    //ImageSegmentationWidget *image_segmentation_widget;
    OGLWidget *openGLWidget;
    QSlider *horizontalSlider;
    QLineEdit *lineEdit;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(980, 946);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName(QStringLiteral("tabWidget"));
        tabWidget->setGeometry(QRect(10, 30, 951, 851));
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        pushButton = new QPushButton(tab);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(700, 10, 113, 32));
        label_pic1 = new QLabel(tab);
        label_pic1->setObjectName(QStringLiteral("label_pic1"));
        label_pic1->setGeometry(QRect(80, 200, 100, 100));
        label_pic2 = new QLabel(tab);
        label_pic2->setObjectName(QStringLiteral("label_pic2"));
        label_pic2->setGeometry(QRect(610, 500, 100, 100));
        label_pic3 = new QLabel(tab);
        label_pic3->setObjectName(QStringLiteral("label_pic3"));
        label_pic3->setGeometry(QRect(350, 200, 100, 100));
        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        label_backdrop = new QLabel(tab_2);
        label_backdrop->setObjectName(QStringLiteral("label_backdrop"));
        label_backdrop->setGeometry(QRect(330, 250, 371, 421));
        pushButton_2 = new QPushButton(tab_2);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
        pushButton_2->setGeometry(QRect(700, 10, 113, 32));
        tabWidget->addTab(tab_2, QString());
        tab_5 = new QWidget();
        tab_5->setObjectName(QStringLiteral("tab_5"));
        label_resultLayout = new QLabel(tab_5);
        label_resultLayout->setObjectName(QStringLiteral("label_resultLayout"));
        label_resultLayout->setGeometry(QRect(180, 100, 511, 631));
        listWidget = new QListWidget(tab_5);
        listWidget->setObjectName(QStringLiteral("listWidget"));
        listWidget->setGeometry(QRect(700, 260, 81, 301));
        listWidget_patch = new QListWidget(tab_5);
        listWidget_patch->setObjectName(QStringLiteral("listWidget"));
        listWidget_patch->setGeometry(QRect(800, 260, 81, 301));
        label = new QLabel(tab_5);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(700, 240, 59, 16));
        groupBox = new QGroupBox(tab_5);
        groupBox->setObjectName(QStringLiteral("groupBox"));
        groupBox->setGeometry(QRect(700, 100, 171, 80));
        check_labelFoldline = new QCheckBox(groupBox);
        check_labelFoldline->setObjectName(QStringLiteral("check_labelFoldline"));
        check_labelFoldline->setGeometry(QRect(0, 30, 161, 20));
        check_labelPatches = new QCheckBox(groupBox);
        check_labelPatches->setObjectName(QStringLiteral("check_labelPatches"));
        check_labelPatches->setGeometry(QRect(0, 50, 151, 20));
        textBrowser = new QTextBrowser(tab_5);
        textBrowser->setObjectName(QStringLiteral("textBrowser"));
        textBrowser->setGeometry(QRect(700, 190, 171, 41));
        groupBox_2 = new QGroupBox(tab_5);
        groupBox_2->setObjectName(QStringLiteral("groupBox_2"));
        groupBox_2->setGeometry(QRect(700, 10, 171, 80));
        radio_original = new QRadioButton(groupBox_2);
        radio_original->setObjectName(QStringLiteral("radio_original"));
        radio_original->setGeometry(QRect(10, 30, 161, 20));
        radio_final = new QRadioButton(groupBox_2);
        radio_final->setObjectName(QStringLiteral("radio_final"));
        radio_final->setGeometry(QRect(10, 50, 151, 20));
        radio_final->setChecked(true);
        tabWidget->addTab(tab_5, QString());
        tabWidget->setTabText(tabWidget->indexOf(tab_5), QStringLiteral("result"));
        tab_7 = new QWidget();
        tab_7->setObjectName(QStringLiteral("tab_7"));
        openGLWidget = new OGLWidget(tab_7);
        openGLWidget->setObjectName(QStringLiteral("openGLWidget"));
        openGLWidget->setGeometry(QRect(50, 70, 731, 711));
        horizontalSlider = new QSlider(tab_7);
        horizontalSlider->setObjectName(QStringLiteral("horizontalSlider"));
        horizontalSlider->setGeometry(QRect(790, 140, 141, 22));
        horizontalSlider->setMinimum(1);
        horizontalSlider->setMaximum(10);
        horizontalSlider->setOrientation(Qt::Horizontal);
        lineEdit = new QLineEdit(tab_7);
        lineEdit->setObjectName(QStringLiteral("lineEdit"));
        lineEdit->setGeometry(QRect(800, 100, 113, 21));
        tabWidget->addTab(tab_7, QString());

        tab_3 = new QWidget();
        tab_3->setObjectName(QStringLiteral("tab_3"));
        //image_segmentation_widget = new ImageSegmentationWidget(tab_3);
        //image_segmentation_widget->setObjectName(QStringLiteral("image_segmentation_widget"));
        //image_segmentation_widget->setGeometry(QRect(50, 70, 731, 600));
        tabWidget->addTab(tab_3, QString());
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QStringLiteral("Image Segmentation"));
        segmentation_button_group = new QGroupBox(tab_3);
        segmentation_button_group->setGeometry(QRect(800, 100, 100, 200));
        start_segmentation_button = new QPushButton(segmentation_button_group);
        start_segmentation_button->setText("Segment");
        refine_segmentation_button = new QPushButton(segmentation_button_group);
        refine_segmentation_button->setText("Refine");
        clear_segmentation_button = new QPushButton(segmentation_button_group);
        clear_segmentation_button->setText("Clear");
        confirm_segmentation_button = new QPushButton(segmentation_button_group);
        confirm_segmentation_button->setText("Confirm");
        segmentation_button_layout = new QVBoxLayout(tab_3);
        segmentation_button_layout->addWidget(start_segmentation_button);
        segmentation_button_layout->addWidget(refine_segmentation_button);
        segmentation_button_layout->addWidget(clear_segmentation_button);
        segmentation_button_layout->addWidget(confirm_segmentation_button);
        segmentation_button_group->setLayout(segmentation_button_layout);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 980, 22));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);
        QObject::connect(pushButton, SIGNAL(clicked()), MainWindow, SLOT(chooseInputImage()));
        QObject::connect(pushButton_2, SIGNAL(clicked()), MainWindow, SLOT(chooseBasePatch()));
        QObject::connect(check_labelFoldline, SIGNAL(clicked()), MainWindow, SLOT(showFoldLines()));
        QObject::connect(check_labelPatches, SIGNAL(clicked()), MainWindow, SLOT(showPatches()));
        QObject::connect(radio_original, SIGNAL(clicked()), MainWindow, SLOT(showOriginalPatches()));
        QObject::connect(radio_final, SIGNAL(clicked()), MainWindow, SLOT(showFinalPatches()));
        QObject::connect(horizontalSlider, SIGNAL(valueChanged(int)), MainWindow, SLOT(setScale()));

        QObject::connect(start_segmentation_button, SIGNAL(clicked()), MainWindow, SLOT(startSegmentation()));
        QObject::connect(refine_segmentation_button, SIGNAL(clicked()), MainWindow, SLOT(refineSegmentation()));
        QObject::connect(clear_segmentation_button, SIGNAL(clicked()), MainWindow, SLOT(clearSegmentation()));
        QObject::connect(confirm_segmentation_button, SIGNAL(clicked()), MainWindow, SLOT(confirmSegmentation()));

        tabWidget->setCurrentIndex(3);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        pushButton->setText(QApplication::translate("MainWindow", "choose", 0));
        label_pic1->setText(QApplication::translate("MainWindow", "TextLabel", 0));
        label_pic2->setText(QApplication::translate("MainWindow", "TextLabel", 0));
        label_pic3->setText(QApplication::translate("MainWindow", "TextLabel", 0));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("MainWindow", "input", 0));
        label_backdrop->setText(QApplication::translate("MainWindow", "TextLabel", 0));
        pushButton_2->setText(QApplication::translate("MainWindow", "next", 0));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("MainWindow", "choose backdrop", 0));
        label_resultLayout->setText(QApplication::translate("MainWindow", "TextLabel", 0));
        label->setText(QApplication::translate("MainWindow", "fold lines", 0));
        groupBox->setTitle(QApplication::translate("MainWindow", "labels", 0));
        check_labelFoldline->setText(QApplication::translate("MainWindow", "show fold line index", 0));
        check_labelPatches->setText(QApplication::translate("MainWindow", "show patche index", 0));
        groupBox_2->setTitle(QApplication::translate("MainWindow", "Patches", 0));
        radio_original->setText(QApplication::translate("MainWindow", "show original patches", 0));
        radio_final->setText(QApplication::translate("MainWindow", "show final patches", 0));
        lineEdit->setText(QApplication::translate("MainWindow", "1", 0));
        tabWidget->setTabText(tabWidget->indexOf(tab_7), QApplication::translate("MainWindow", "3d scene", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
