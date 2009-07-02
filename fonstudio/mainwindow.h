#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <vector>

#include "core/grid.h"
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
    int m_gfxtype;
    static QPlainTextEdit *mLogSpace;
    void loadPicusIniFile(const QString &fileName);
    // painter functions
    void paintFON(QPainter &painter, QRect rect);

private slots:
    void on_fonRun_clicked();
    void on_lrLightGrid_clicked();
    void on_lrCalcFullGrid_clicked();
    void on_lroTestHemi_clicked();
    void on_testLRO_clicked();
    void on_pbCreateLightroom_clicked();
    void on_actionFON_action_triggered();
    void on_actionEdit_XML_settings_triggered();
    void on_actionLightroom_triggered();
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
