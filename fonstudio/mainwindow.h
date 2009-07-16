#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <vector>

#include "core/grid.h"
#include "tree.h"
#include "imagestamp.h"
#include "tools/helper.h"


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
    void addTrees(const double dbh, const int count);
    Ui::MainWindowClass *ui;
    FloatGrid* mGrid; // light grid
    FloatGrid *mDomGrid; // dominance grid
    std::vector<Tree> mTrees;
    ImageStamp mStamp;
    int m_gfxtype;
    float m_pixelpercell;
    static QPlainTextEdit *mLogSpace;
    void loadPicusIniFile(const QString &fileName);
    // painter functions
    void paintFON(QPainter &painter, QRect rect);
    Viewport vp;
    QString dumpTreelist();

private slots:
    void on_actionFON_grid_triggered();
    void on_actionTreelist_triggered();
    void on_openFile_clicked();
    void on_pbSetAsDebug_clicked();
    void on_lrReadStamps_clicked();
    void on_treeChange_clicked();
    void on_lrLoadStamps_clicked();
    void on_lrProcess_clicked();
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
    void on_calcFormula_clicked();
    void on_applyXML_clicked();
    void on_stampTrees_clicked();
    void on_saveFile_clicked();
    void repaintArea(QPainter &painter);
    void mouseClick(const QPoint& pos);
    void mouseMove(const QPoint& pos);
    void mouseDrag(const QPoint& from, const QPoint &to);
    void on_visFon_toggled();
    void on_visDomGrid_toggled();
    void on_visImpact_toggled();
};

#endif // MAINWINDOW_H
