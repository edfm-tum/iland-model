#include <QtGui/QApplication>
#include "mainwindow.h"
#include "../3rdparty/MersenneTwister.h"
#include "globalsettings.h"
MTRand _mtrand;
MTRand &mtRand()
{
    return _mtrand;
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
