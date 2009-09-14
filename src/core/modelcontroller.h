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
    // dynamic outputs (variable fields)
    void setupDynamicOutput(QString fieldList);
    QString dynamicOutput();
public slots:
    void setFileName(QString initFileName); ///< set project file name
    void create(); ///< create the model
    void destroy(); ///< delete the model
    void run(); ///< run the model
    void runYear(); ///< runs a single time step
private:
    void fetchDynamicOutput();
    Model *mModel;
    QString mInitFile;
    QStringList mDynFieldList;
    QStringList mDynData;

};

#endif // MODELCONTROLLER_H
