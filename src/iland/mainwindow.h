#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <vector>

#include "grid.h"
#include "tree.h"
#include "helper.h"

#include "modelcontroller.h"

class Model;

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
    Ui::MainWindowClass *ui;
    ModelController mRemoteControl;
    // setup
    void setupModel();
    void readwriteCycle();
    void addTrees(const double dbh, const int count);


    int m_gfxtype;

    static QPlainTextEdit *mLogSpace;
    void loadPicusIniFile(const QString &fileName);
    // painter functions
    void paintFON(QPainter &painter, QRect rect);
    Viewport vp;
    QString dumpTreelist();
    void applyCycles(int cycle_count=1);

private slots:

    void on_actionFON_grid_triggered();
    void on_actionTreelist_triggered();
    void on_openFile_clicked();
    void on_pbSetAsDebug_clicked();


    void on_actionEdit_XML_settings_triggered();

    void on_saveFile_clicked();
    void repaintArea(QPainter &painter);
    void mouseClick(const QPoint& pos);
    void mouseMove(const QPoint& pos);
    void mouseDrag(const QPoint& from, const QPoint &to, const Qt::MouseButton button);
    void mouseWheel(const QPoint& pos, int steps);
    void on_visFon_toggled();
    void on_visDomGrid_toggled();
    void on_visImpact_toggled();
};

#endif // MAINWINDOW_H
