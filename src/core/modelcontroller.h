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

#ifndef MODELCONTROLLER_H
#define MODELCONTROLLER_H
#include <QObject>
#include <QHash>
#include "grid.h"
#include "layeredgrid.h"
class Model;
class MainWindow;
class MapGrid;

class ModelController: public QObject
{
    Q_OBJECT
public:
    ModelController();
    ~ModelController();
    void setMainWindow(MainWindow *mw) { mViewerWindow = mw; }
    void connectSignals(); // connect signal/slots to the main window if available
    Model *model() const { return mModel; }
    // bool checkers...
    bool canCreate(); ///< return true if the model can be created (settings loaded and model does not exist)
    bool canDestroy(); ///< model may be destroyed
    bool canRun(); ///< model may be run
    bool isRunning(); ///< model is running
    bool isFinished(); ///< returns true if there is a valid model state, but the run is finished
    bool isPaused(); ///< returns true if the model is currently paused
    bool hasError() { return mHasError; } ///< returns true if an error occured during the last operation
    QString lastError() { return mLastError; } ///< error message of the last received error
    // simulation length
    int currentYear() const; ///< return current year of the model
    int totalYears() const { return mYearsToRun; } ///< returns total number of years to simulate
    // error handling
    void throwError(const QString msg);
    // dynamic outputs (variable fields)
    void setDynamicOutputEnabled(bool enabled) { mDynamicOutputEnabled = enabled; }
    void setupDynamicOutput(QString fieldList);
    QString dynamicOutput();
    // some informational services
    QHash<QString, QString> availableSpecies();

    void saveScreenshot(QString file_name); ///< saves a screenshot of the central view widget to 'file_name'
    void addGrid(const FloatGrid *grid, const QString &name, const GridViewType view_type, double min_value, double max_value);
    void paintMap(MapGrid *map, double min_value, double max_value);

    void addLayers(const LayeredGridBase *layers, const QString &name);
    void removeLayers(const LayeredGridBase *layers);
    void setViewport(QPointF center_point, double scale_px_per_m);
signals:
    void finished(QString errorMessage); ///< model has finished run (errorMessage is empty in case of success)
    void year(int year); ///< signal indicating a year of the simulation has been processed
    void bufferLogs(bool do_buffer); ///< signal indicating that logs should be buffered (true, model run mode) or that buffering should stop (false) for "interactive" mode
    void stateChanged(); ///< is emitted when model started/stopped/paused
public slots:
    void setFileName(QString initFileName); ///< set project file name
    void create(); ///< create the model
    void destroy(); ///< delete the model
    void run(int years); ///< run the model
    bool runYear(); ///< runs a single time step
    bool pause(); ///< pause execution, and if paused, continue to run. returns state *after* change, i.e. true=now in paused mode
    bool continueRun(); ///< continues execution if simulation was paused
    void cancel(); ///< cancel execution of the model
    void repaint(); ///< force a repaint of the main drawing window
private slots:
    void runloop();
private:
    bool internalRun(); ///< runs the main loop
    void internalStop(); ///< save outputs, stop the model execution
    void fetchDynamicOutput(); ///< execute the dynamic output and fetch data
    MainWindow *mViewerWindow;
    Model *mModel;
    bool mPaused;
    bool mRunning;
    bool mFinished;
    bool mCanceled;
    bool mHasError;
    QString mLastError;
    int mYearsToRun;
    QString mInitFile;
    bool mDynamicOutputEnabled;
    QStringList mDynFieldList;
    QStringList mDynData;

};

#endif // MODELCONTROLLER_H
