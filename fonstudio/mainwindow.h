#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <vector>

#include "../core/grid.h"
#include "tree.h"
#include "stamp.h"



namespace Ui
{
    class MainWindowClass;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindowClass *ui;
    FloatGrid* mGrid;
    std::vector<Tree> Trees;
    Stamp mStamp;

private slots:
    void on_pbAddTrees_clicked();
    void on_lCalcResult_linkActivated(QString link);
    void on_pbRetrieve_clicked();
    void on_calcFormula_clicked();
    void on_applyXML_clicked();
    void on_stampTrees_clicked();
    void on_saveFile_clicked();
    void repaintArea(QPainter &painter);
    void mouseClick(const QPoint& pos);
};

#endif // MAINWINDOW_H
