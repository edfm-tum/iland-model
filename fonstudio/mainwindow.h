#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
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
    static QPlainTextEdit* logSpace();
    ~MainWindow();

private:
    void stampTrees();
    double retrieveFon();
    void addTrees(const double dbh, const int count);
    Ui::MainWindowClass *ui;
    FloatGrid* mGrid;
    std::vector<Tree> mTrees;
    Stamp mStamp;
    static QPlainTextEdit *mLogSpace;
    void loadPicusIniFile(const QString &fileName);

private slots:
    void on_calcMatrix_clicked();
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
