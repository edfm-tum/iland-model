/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
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

#include "ui/settingsdialog.h"
#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif
#include <vector>

#include "grid.h"
#include "tree.h"

#include "modelcontroller.h"
#include "paintarea.h"
#include "viewport.h"

#include "ui/linkxmlqt.h"

class QQuickView;
class Model;
class Tree;
class ResourceUnit;
class MapGrid;
class LayeredGridBase;
class Colors;

struct metadata {
    QStringList elements;
    QStringList inputType;
    QStringList defaultValue;
    QStringList labelName;
    QStringList toolTip;
} ;

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
    Ui::MainWindowClass *uiclass() {return ui; }
    Colors *ruler() { return mRulerColors; }

public slots:
    void repaint(); ///< force a repaint of the main drawing area
    void yearSimulated(int year);
    void modelFinished(QString errorMessage);
    void checkModelState();
    void bufferedLog(bool bufferLog);
    QImage screenshot(); ///< create a screenshot of the main painting area
    void paintGrid(MapGrid *map_grid,
                   const QString &name=QString(),
                   const GridViewType view_type=GridViewRainbow,
                   double min_val=0., double max_val=1.);
    void paintGrid(const FloatGrid *grid,
                   const QString &name=QString(),
                   const GridViewType view_type=GridViewRainbow,
                   double min_val=0., double max_val=1.);
    void paintGrid(const Grid<double> *grid,
                   const QString &name=QString(),
                   const GridViewType view_type=GridViewRainbow,
                   double min_val=0., double max_val=1.);

    void addLayers(const LayeredGridBase *layer, const QString &name);

    void addPaintLayer(Grid<double> *dbl_grid, MapGrid* mapgrid, const QString name, GridViewType view_type=GridViewRainbow);
    void removePaintLayer(Grid<double> *dbl_grid, MapGrid* mapgrid);

    void addPaintLayers(QObject *handler, const QStringList names, const QVector<GridViewType> view_types=QVector<GridViewType>());
    void removePaintLayers(QObject *handler);

    void removeLayers(const LayeredGridBase *layer);

    // set the next object to paint by name
    void setPaintGrid(const QString grid_name);

    void setViewport(QPointF center_point, double scale_px_per_m); /// set the viewport (like interactive with mouse)
    void setUIshortcuts(QVariantMap shortcuts);

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindowClass *ui;
    ModelController mRemoteControl;

    SettingsDialog *ui_settingsDialog;

    LinkXmlQt *mLinkxqt;
    SettingMetaData *mSettingMetaData;

    QStringList mMetaKeys;
    QStringList mMetaValues;
    metadata mMeta;

    QLabel *mStatusLabel;
    QQuickView *mRuler;
    Colors *mRulerColors;
    QString mLastPaintError;
    // setup

    void labelMessage(const QString message) { if (mStatusLabel) mStatusLabel->setText(message);}
    void setupModel();
    void readwriteCycle();
    // paint
    void updatePaintGridList();
    bool mDoRepaint;
    /// PaintObject stores what kind of object to paint during next repaint
    PaintObject mPaintNext;
    QMap<QString, PaintObject> mPaintList;
    void paintGrid(QPainter &painter, PaintObject &object);

    static QPlainTextEdit *mLogSpace;
    static QTextStream *mLogStream;
    void loadPicusIniFile(const QString &fileName);
    // painter functions
    void paintFON(QPainter &painter, QRect rect); ///< general paint function (GUI driven)
    void paintMapGrid(QPainter &painter,
                      MapGrid *map_grid, const FloatGrid *float_grid, const Grid<double> *double_grid,
                      const GridViewType view_type,
                      double min_val=0., double max_val=1.,
                      bool shading=false); ///< paint a map grid (controller driver)
    Viewport vp;
    QString dumpTreelist();
    void applyCycles(int cycle_count=1);

    void showTreeDetails(Tree* tree);
    void showResourceUnitDetails(const ResourceUnit *ru);
    bool showABEDetails(const QPointF &coord);
    void showRegenDetails(const QPointF &coord);

    void readSettings();
    void writeSettings();
    // logging and outputs
    void setupFileLogging(const bool do_start);
    void batchLog(const QString s); ///< logging function for batch mode
    // visualization helper grid
    Grid<float> mRegenerationGrid;
    //recent file menu
    void recentFileMenu();
    QList<QString> mRecentFileList;

    //Dialog
    void processMetaData(metadata& meta);

private slots:
    //void openSystemSettingsDialog();

    void automaticRun(); ///< automatically start a simulation...
    void updateLabel(); ///< update UI labels during run
    void checkExpressionError(); ///< show error message in case an error occured previously

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
    void on_reloadJavaScript_clicked();
    void on_actionDynamic_Output_triggered();
    void on_pbExecExpression_clicked();
    void on_pbCalculateExpression_clicked();
    void on_actionReload_triggered();
    void on_actionRun_one_year_triggered();
    void on_actionTree_NPP_triggered();
    void on_actionSelect_Data_Types_triggered();
    void on_actionTree_Growth_triggered();
    void on_actionTree_Partition_triggered();
    void on_action_debugSapling_triggered();
    void on_actionModelRun_triggered();
    void on_actionModelDestroy_triggered();
    void on_actionModelCreate_triggered();
    void on_actionFON_grid_triggered();
    void on_actionTreelist_triggered();
    void on_actionImageToClipboard_triggered();
    void on_initFileName_editingFinished();
    void on_openFile_clicked();

    void on_actionSettingsDialog_triggered();

    void repaintArea(QPainter &painter);
    void mouseClick(const QPoint& pos);
    void mouseMove(const QPoint& pos);
    void mouseDrag(const QPoint& from, const QPoint &to, const Qt::MouseButton button);
    void mouseWheel(const QPoint& pos, int steps);
    void executeJS(QString code);
    void on_visFon_toggled();
    void on_visDomGrid_toggled();
    void on_visImpact_toggled();
    void on_visImpact_clicked() { on_visFon_toggled(); } // force repeaint
    void on_visSeeds_clicked() { on_visFon_toggled(); } // force repeaint
    void on_visRegeneration_clicked() { on_visFon_toggled(); } // force repeaint
    void on_visRegenNew_clicked()    { on_visFon_toggled();    } // force repaint
    void on_visResourceUnits_clicked()  { on_visFon_toggled();    } // force repaint
    void on_visOtherGrid_clicked()  { on_visFon_toggled();    } // force repaint
    void on_visShading_clicked() {on_visFon_toggled();   } // force repain
    void on_visSnags_clicked() { on_visFon_toggled(); }     // force repaint
    void on_actionPerformance_triggered();
    void on_actionTest_triggered();
    void on_pbReloadQml_clicked();
    void on_actionExit_triggered();
    void on_actionOpen_triggered();
    //recent file menu
    void menuRecent_Files();
    void on_lJSShortcuts_linkActivated(const QString &link);

    void on_actionShow_full_extent_triggered();
    void on_actionRepaint_triggered();
    void on_checkXMLFile_clicked();
    void on_actionSave_regeneration_grid_triggered();
    void on_pbLoadTree_clicked();
    void on_otherGridTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_dataTree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_speciesFilterBox_currentIndexChanged(int index);
    void on_visRUSpeciesColor_stateChanged(int arg1) {Q_UNUSED(arg1); on_visFon_toggled();  } // force repaint
    void on_actionExpression_plotter_triggered();
    void on_actionUpdate_XML_file_triggered();
};

#endif // MAINWINDOW_H
