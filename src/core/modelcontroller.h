#ifndef MODELCONTROLLER_H
#define MODELCONTROLLER_H
#include <QObject>

class Model;
class ModelController: public QObject
{
    Q_OBJECT
public:
    ModelController();
    ~ModelController();
    Model *model() const { return mModel; }
    // bool checkers...
    bool canCreate(); ///< return true if the model can be created (settings loaded and model does not exist)
    bool canDestroy(); ///< model may be destroyed
    bool canRun(); ///< model may be run
    bool isRunning(); ///< model is running
    bool isFinished(); ///< returns true if there is a valid model state, but the run is finished
    int currentYear() const; ///< return current year of the model
    int totalYears() const { return mYearsToRun - 1; } ///< returns total number of years to simulate
    // dynamic outputs (variable fields)
    void setupDynamicOutput(QString fieldList);
    QString dynamicOutput();
signals:
    void finished(QString errorMessage);
    void year(int year);
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
