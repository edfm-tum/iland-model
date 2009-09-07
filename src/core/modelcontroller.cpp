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
    mRunYears = 0;
}

ModelController::~ModelController()
{
    destroy();
}


const bool ModelController::canCreate()
{
    if (mModel)
        return false;
    return true;
}

const bool ModelController::canDestroy()
{
    return mModel != NULL;
}

const bool ModelController::canRun()
{
    if (mModel && mModel->isSetup())
        return true;
    return false;
}

const bool ModelController::isRunning()
{
 return mRunYears>0;
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
    mRunYears = 0;
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
        mRunYears = 0;
        qDebug() << "ModelController: Model destroyed.";
    }
}
void ModelController::run()
{
    if (!canRun()) return;
    try {
        mModel->runYear();
        mRunYears ++;
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
        GlobalSettings::instance()->clearDebugLists();  // clear debug data
        mModel->runYear();
        mRunYears ++;
    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
}


