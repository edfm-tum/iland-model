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
    const bool canCreate(); ///< return true if the model can be created (settings loaded and model does not exist)
    const bool canDestroy(); ///< model may be destroyed
    const bool canRun(); ///< model may be run
    const bool isRunning(); ///< model is runni
public slots:
    void setFileName(QString initFileName); ///< set project file name
    void create(); ///< create the model
    void destroy(); ///< delete the model
    void run(); ///< run the model
    void runYear(); ///< runs a single time step
private:
    Model *mModel;
    QString mInitFile;
};

#endif // MODELCONTROLLER_H
