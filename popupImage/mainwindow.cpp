#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
            
    ui->setupUi(this);
    ui->label_backdrop->installEventFilter(this);
    ui->label_resultLayout->installEventFilter(this);
    ui->label_pic1->installEventFilter(this);
    ui->label_pic2->installEventFilter(this);
    ui->label_pic3->installEventFilter(this);

    cv::Mat a(100,100,CV_8UC3);
        imwrite("pm1130.png", a);
        
    cv::Mat img1 = imread("../input/cute_fox.png");
    ui->label_pic1->setGeometry(QRect(50, 300, img1.cols, img1.rows));
    cv::cvtColor(img1, img1, CV_BGR2RGB);
    QImage image = mat2QImage(img1 , QImage::Format_RGB888);
    ui->label_pic1->setPixmap(QPixmap::fromImage(image));
    
    cv::Mat img2 = imread("../input/angrybirds05_illu.png");
    ui->label_pic2->setGeometry(QRect(300, 300, img2.cols, img2.rows));
    cv::cvtColor(img2, img2, CV_BGR2RGB);
    image = mat2QImage(img2 , QImage::Format_RGB888);
    ui->label_pic2->setPixmap(QPixmap::fromImage(image));

    cv::Mat img3 = imread("../input/bear_ill.png");
    cv::cvtColor(img3, img3, CV_BGR2RGB);
    ui->label_pic3->setGeometry(QRect(560, 300, img3.cols, img3.rows));
    image = mat2QImage(img3 , QImage::Format_RGB888);
    ui->label_pic3->setPixmap(QPixmap::fromImage(image));
    
    QObject::connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem *)),
                     this, SLOT(onItemClicked(QListWidgetItem *)));
    QObject::connect(ui->horizontalSlider, SIGNAL(valueChanged(int)),
                     this, SLOT(setScalse(int)));
        
    ui->tabWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow(){
    delete obj;
    delete ui;
}

void MainWindow::onItemClicked(QListWidgetItem *item){
    
    QVariant variant = item->data(Qt::UserRole);
    int data = variant.toInt();
    QString text = QString("%1:%2 is clicked.").arg(item->text()).arg(data);
    int idx = atoi(text.toStdString().c_str());

    cv::Mat resultsMat = drawPatch(obj->classifiedPatches, obj->initMatSize);
    if(!obj->isShowOriginalPatches) drawActiveFoldline(obj, resultsMat);
    else drawOriginalFoldline(obj, resultsMat);
    drawClickedItem(obj, resultsMat, idx, ui->label_resultLayout);
    ostringstream oss;
    if(obj->isShowOriginalPatches){
        oss << "f" << idx << " is connected to patch" << obj->foldLine[idx]->originalConnPatch[0]
        <<", patch" << obj->foldLine[idx]->originalConnPatch[1];
    }else{
        oss << "f" << idx << " is connected to patch" << obj->foldLine[idx]->connPatch[0]
        <<", patch" << obj->foldLine[idx]->connPatch[1];
    }

    QString qstr = QString::fromStdString(oss.str());
    ui->textBrowser->setText(qstr);
    
}

void MainWindow::createItems(){
    ui->listWidget->clear();
    ostringstream oss;
    for(size_t i=0; i< obj->activeFoldLine.size(); i++){
        oss.str("");
        oss << (int)i ;
        QString qstr = QString::fromStdString(oss.str());
        if(obj->isShowOriginalPatches && obj->foldLine[i]->isOriginalFoldLine){
            ui->listWidget->addItem(qstr);
        }else if(!obj->isShowOriginalPatches){
            ui->listWidget->addItem(qstr);
        }
    }
}

void MainWindow::setScalse(int scale){
    int pos = ui->horizontalSlider->sliderPosition();
    obj->scale = pos;
    QString text = QString::number(pos);
    ui->lineEdit->setText(text);
}

void MainWindow::showOriginalPatches(){
    obj->isShowOriginalPatches = true;
    showChanged(obj, ui->label_resultLayout);
    createItems();
}

void MainWindow::showFinalPatches(){
    obj->isShowOriginalPatches = false;
    showChanged(obj, ui->label_resultLayout);
    createItems();
}

void MainWindow::showPatches(){
    if(ui->check_labelPatches->isChecked()) obj->isShowPatches = true;
    else obj->isShowPatches = false;
    showChanged(obj, ui->label_resultLayout);
}
void MainWindow::showFoldLines(){
    if(ui->check_labelFoldline->isChecked()) obj->isShowFoldlines = true;
    else obj->isShowFoldlines = false;
    showChanged(obj, ui->label_resultLayout);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event){
    
    if(event->type() != QEvent::MouseButtonPress)
        return false;

    QMouseEvent *k = (QMouseEvent *)event;
    cv::Point p(k->x(), k->y());
    
    if(watched == ui->label_pic1){
        caseNum = 0;
        cout << caseNum <<endl;
        return true;
    }else if(watched == ui->label_pic2){
        caseNum = 1;
        return true;
    }else if(watched == ui->label_pic3){
        caseNum = 2;
        return true;
    }else if(watched == ui->label_backdrop){
        eventChooseBackdrop(obj, p, ui->label_backdrop);
        return true;
    }else if(watched == ui->label_resultLayout){
        drawResultLayout(obj, p, ui->label_resultLayout);
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void MainWindow::processPopup(){
    
    algoList.clear();
    
    //3. find the connection of divide patches
    algoList.push_back( new popupObjFindDividedPatchConnection );
    
    //4. classify the patches
    algoList.push_back( new popupObjClassifyPatches);
    
    //5. find the connection of classified patches
    algoList.push_back( new popupObjFindClassifiedPatchConnection);
    
    //6. find boundary fold lines of classified patches
    algoList.push_back( new popupObjInitBoundaryFoldLine);
    
    //7. build connection map of boundary fold lines
    algoList.push_back( new popupObjBuildBoundaryLineConnMap);

    //8. mergeBoundaryLine
    //if the elements is bigger than 2,
   // algoList.push_back( new popupObjMergeBoundaryLine);
    
    //9. addConnectionLine
    //if classified connection add fold line : classifiedConnMap
    algoList.push_back( new popupObjAddConnectionLineToMap);
    
    //10. extend the fold line
    algoList.push_back( new popupObjExtendFoldLineAndFindActiveBolb);
    
    //11. insert fold line
    algoList.push_back( new popupObjFindInsertedLine);
    
    //12. find possible patches
    algoList.push_back( new popupObjFindPossiblePatch);
    
    //13. find possible patches connection
    algoList.push_back( new popupObjFindPossiblePatchConnection);
    
    //14. find all path
    algoList.push_back( new popupObjGraphFindAllPath);
       
    //15. find fold line path
    algoList.push_back( new popupObjFindFoldLinePath);

    //16. find foldibility
    algoList.push_back( new popupObjOptimization);
    
    algoList.execute(obj, "main", true);

    //draw 3d scene
    ui->openGLWidget->setObj(obj);
    
    //set label_resultLayout
    cv::Mat possiblePatchMat = drawPatch(obj->possiblePatches, obj->initMatSize);
    QImage imagePossible = mat2QImage(possiblePatchMat , QImage::Format_RGB888);
    ui->label_resultLayout->setGeometry(QRect(60, 20, possiblePatchMat.cols, possiblePatchMat.rows));
    ui->label_resultLayout->setPixmap(QPixmap::fromImage(imagePossible));
    showChanged(obj, ui->label_resultLayout);

    //list widget
    createItems();
}

void MainWindow::chooseBasePatch(){
    //divide base patch
    obj->assignDividedPatches();
    processPopup();
    ui->tabWidget->setCurrentIndex(2);
}

void MainWindow::chooseInputImage(){
   
    string fileName[]={
        "../input/cute_fox.svg",
        "../input/angrybirds05_illu.svg",
        "../input/bear_ill.svg"
    };
    obj = new popupObject();
    obj->initPatch(fileName[caseNum]);
    cv::Mat initPatchMat = drawPatch(obj->initPatches, obj->initMatSize);
    
    //set Qimage
    QImage image = mat2QImage(initPatchMat , QImage::Format_RGB888);
    ui->label_backdrop->setGeometry(QRect(60, 20, initPatchMat.cols, initPatchMat.rows));
    ui->label_backdrop->setPixmap(QPixmap::fromImage(image));
    ui->tabWidget->setCurrentIndex(1);
    
}
