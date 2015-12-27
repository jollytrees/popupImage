#include "mainwindow.h"
#include <QApplication>
#include <QDir>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QDir bin(QCoreApplication::applicationDirPath());

    /* Set working directory */

  //#if defined(Q_WS_MAC)
    bin.cdUp();    /* Fix this on Mac because of the .app folder, */
    bin.cdUp();    /* which means that the actual executable is   */
//    bin.cdUp();    /* three levels deep. Grrr.                    */
  //#endif
    QDir::setCurrent(bin.absolutePath());

    MainWindow w;
    w.show();

    return a.exec();
}
