/** ModelController is a helper class used to
  control the flow of operations during a model run.
  Really useful???? or a dispatcher???
  */

#include "global.h"
#include "modelcontroller.h"
#include <QObject>

#include "model.h"
#include "helper.h"

ModelController::ModelController()
{
    mModel = NULL;
}

ModelController::~ModelController()
{
    destroy();
}


bool ModelController::canCreate()
{
    if (mModel)
        return false;
    return true;
}

bool ModelController::canDestroy()
{
    return mModel != NULL;
}

bool ModelController::canRun()
{
    if (mModel && mModel->isSetup())
        return true;
    return false;
}

bool ModelController::isRunning()
{
 return GlobalSettings::instance()->runYear()>0;
}


void ModelController::setFileName(QString initFileName)
{
    mInitFile = initFileName;
    try {
        GlobalSettings::instance()->loadProjectFile(mInitFile);
    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
}

void ModelController::create()
{
    if (!canCreate())
        return;
    try {
    mModel = new Model();
    mModel->loadProject();
    GlobalSettings::instance()->clearDebugLists();  // clear debug data

    if (mModel->isSetup())
        mModel->beforeRun();

    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
}

void ModelController::destroy()
{
    if (canDestroy()) {
        delete mModel;
        mModel = 0;
        GlobalSettings::instance()->setRunYear(0);
        qDebug() << "ModelController: Model destroyed.";
    }
}
void ModelController::run()
{
    if (!canRun()) return;
    try {
        mModel->runYear();
    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
}

void ModelController::runYear()
{
    if (!canRun()) return;
    try {
        mModel->runYear();
    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
}


