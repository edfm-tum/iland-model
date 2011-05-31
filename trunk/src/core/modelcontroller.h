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
    // simulation length
    int currentYear() const; ///< return current year of the model
    int totalYears() const { return mYearsToRun; } ///< returns total number of years to simulate
    // error handling
    void throwError(const QString msg);
    // dynamic outputs (variable fields)
    void setupDynamicOutput(QString fieldList);
    QString dynamicOutput();
    // some informational services
    QHash<QString, QString> availableSpecies();

    void saveScreenshot(QString file_name); ///< saves a screenshot of the central view widget to 'file_name'
    void paintGrid(const FloatGrid *grid, const QString &name, const GridViewType view_type, double min_value, double max_value);
    void paintMap(MapGrid *map, double min_value, double max_value);

    void addLayers(const LayeredGridBase *layers, const QString &name);
    void setViewport(QPointF center_point, double scale_px_per_m);
signals:
    void finished(QString errorMessage);
    void year(int year);
    void bufferLogs(bool do_buffer);
public slots:
    void setFileName(QString initFileName); ///< set project file name
    void create(); ///< create the model
    void destroy(); ///< delete the model
    void run(int years); ///< run the model
    bool runYear(); ///< runs a single time step
    bool pause(); ///< pause execution, and if paused, continue to run. returns state *after* change, i.e. true=now in paused mode
    void cancel(); ///< cancel execution of the model
private slots:
    void runloop();
private:
    void fetchDynamicOutput();
    MainWindow *mViewerWindow;
    Model *mModel;
    bool mPaused;
    bool mRunning;
    bool mFinished;
    bool mCanceled;
    int mYearsToRun;
    QString mInitFile;
    QStringList mDynFieldList;
    QStringList mDynData;

};

#endif // MODELCONTROLLER_H
