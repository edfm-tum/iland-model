/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <vector>

#include "grid.h"
#include "tree.h"
#include "helper.h"

#include "modelcontroller.h"

class Model;
class Tree;
class ResourceUnit;
class MapGrid;
class LayeredGridBase;

namespace Ui
{
    class MainWindowClass;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    // logging
    static QPlainTextEdit* logSpace() { return mLogSpace; }
    static QTextStream* logStream() { return mLogStream;}
    ~MainWindow();
public slots:
    void repaint(); ///< force a repaint of the main drawing area
    void yearSimulated(int year);
    void modelFinished(QString errorMessage);
    void bufferedLog(bool bufferLog);
    QImage screenshot(); ///< craete a screenshot of the main painting area
    void paintGrid(MapGrid *map_grid,
                   const QString &name=QString(),
                   const GridViewType view_type=GridViewRainbow,
                   double min_val=0., double max_val=1.);
    void paintGrid(const FloatGrid *grid,
                   const QString &name=QString(),
                   const GridViewType view_type=GridViewRainbow,
                   double min_val=0., double max_val=1.);
    void addLayers(const LayeredGridBase *layer, const QString &name);
    void setViewport(QPointF center_point, double scale_px_per_m); /// set the viewport (like interactive with mouse)

protected:
     void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindowClass *ui;
    ModelController mRemoteControl;
    QLabel *mStatusLabel;
    // setup
    void labelMessage(const QString message) { if (mStatusLabel) mStatusLabel->setText(message);}
    void setupModel();
    void readwriteCycle();
    // paint
    void updatePaintGridList();
    /// PaintObject stores what kind of object to paint during next repaint
    struct PaintObject {
        PaintObject(): what(PaintNothing), map_grid(0), float_grid(0), layered(0), layer_id(0), min_value(0.), max_value(1.), cur_min_value(0.), cur_max_value(1.), auto_range(false) {}
        enum { PaintNothing, PaintMapGrid, PaintFloatGrid, PaintLayers, PaintLIF, PaintTrees, PaintHeightGrid, PaintResourceUnits, PaintRegeneration } what;
        MapGrid *map_grid;
        const FloatGrid *float_grid;
        const LayeredGridBase *layered;
        int layer_id;
        GridViewType view_type;
        double min_value;
        double max_value;
        double cur_min_value, cur_max_value;
        bool auto_range;
        static QColor background_color;
        // other options
        bool clip_to_stands;
        bool species_colors;
    } mPaintNext;
    QHash<QString, PaintObject> mPaintList;
    void paintGrid(QPainter &painter, PaintObject &object);

    static QPlainTextEdit *mLogSpace;
    static QTextStream *mLogStream;
    void loadPicusIniFile(const QString &fileName);
    // painter functions
    void paintFON(QPainter &painter, QRect rect); ///< general paint function (GUI driven)
    void paintMapGrid(QPainter &painter, MapGrid *map_grid, const FloatGrid *float_grid, const GridViewType view_type, double min_val=0., double max_val=1.); ///< paint a map grid (controller driver)
    Viewport vp;
    QString dumpTreelist();
    void applyCycles(int cycle_count=1);
    void checkModelState();
    void showTreeDetails(Tree* tree);
    void showResourceUnitDetails(const ResourceUnit *ru);
    void readSettings();
    void writeSettings();
    // debug data and outputs
    void saveDebugOutputs();
    void setupFileLogging(const bool do_start);
    void batchLog(const QString s); ///< logging function for batch mode
    // visualization helper grid
    Grid<float> mRegenerationGrid;

private slots:
    void automaticRun(); ///< automatically start a simulation...

    void on_actionWarning_triggered() { on_actionDebug_triggered(); }
    void on_actionError_triggered() { on_actionDebug_triggered(); }
    void on_actionInfo_triggered() { on_actionDebug_triggered(); }
    void on_actionDebug_triggered();
    void on_actionSnag_Dynamics_triggered();
    void on_action_debugEstablishment_triggered();
    void on_selectJavaScript_clicked();
    void on_actionClearDebugOutput_triggered();
    void on_pbLogFilterClear_clicked();
    void on_pbFilterExecute_clicked();
    void on_pbLogClearText_clicked();
    void on_pbLogToClipboard_clicked();
    void on_actionDaily_responses_Output_triggered();
    void on_actionAbout_triggered();
    void on_actionOnline_ressources_triggered();
    void on_actionTimers_triggered();
    void on_actionOutput_table_description_triggered();
    void on_actionWater_Output_triggered();
    void on_actionStop_triggered();
    void on_actionPause_triggered();
    void on_scriptCommand_returnPressed();
    void on_reloadJavaScript_clicked();
    void on_actionShow_Debug_Messages_triggered(bool checked);
    void on_actionDynamic_Output_triggered();
    void on_pbExecExpression_clicked();
    void on_pbCalculateExpression_clicked();
    void on_actionReload_triggered();
    void on_actionRun_one_year_triggered();
    void on_actionTree_NPP_triggered();
    void on_actionSelect_Data_Types_triggered();
    void on_actionTree_Growth_triggered();
    void on_actionTree_Partition_triggered();
    void on_actionModelRun_triggered();
    void on_actionModelDestroy_triggered();
    void on_actionModelCreate_triggered();
    void on_actionFON_grid_triggered();
    void on_actionTreelist_triggered();
    void on_actionImageToClipboard_triggered();
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
    void on_visImpact_clicked() { on_visFon_toggled(); } // force repeaint
    void on_actionPerformance_triggered();
    void on_scriptCommandHistory_currentIndexChanged(int index);
    void on_paintGridBox_currentIndexChanged(int index);
};

#endif // MAINWINDOW_H
