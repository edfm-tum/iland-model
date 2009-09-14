/** ModelController is a helper class used to
  control the flow of operations during a model run.
  Really useful???? or a dispatcher???
  */

#include "global.h"
#include "modelcontroller.h"
#include <QObject>

#include "model.h"
#include "helper.h"
#include "expressionwrapper.h"

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
    GlobalSettings::instance()->clearDebugLists();  // clear debug data

    try {
        mModel->runYear();
        dynamicOutput();
    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
}


//////////////////////////////////////
// dynamic outut
//////////////////////////////////////
//////////////////////////////////////
void ModelController::setupDynamicOutput(QString fieldList)
{
    mDynFieldList = fieldList.split(",");
    mDynData.clear();
    mDynData.append(mDynFieldList.join(";"));
}
QString ModelController::dynamicOutput()
{
    return mDynData.join("\n");
}

const QStringList aggList = QStringList() << "mean" << "sum" << "min" << "max" << "p25" << "p50" << "p75";
void ModelController::fetchDynamicOutput()
{
    if (mDynFieldList.isEmpty())
        return;
    QStringList var;
    QString lastVar = "";
    QVector<double> data;
    AllTreeIterator at(mModel);
    TreeWrapper tw;
    int var_index;
    StatData stat;
    double value;
    QStringList line;
    foreach (QString field, mDynFieldList) {
        var = field.split(QRegExp("\\W+"), QString::SkipEmptyParts);
        if (var.count()!=2)
                throw IException(QString("Invalid variable name for dynamic output:") + field);
        if (var.first()!=lastVar) {
            // load new field
            data.clear();
            at.reset();
            var_index = tw.variableIndex(var.first());
            if (var_index<0) {
                throw IException(QString("Invalid variable name for dynamic output:") + var.first());
            }
            while (Tree *t = at.next()) {
                tw.setTree(t);
                data.push_back(tw.value(var_index));
            }
            stat.setData(data);
        }
        // fetch data
        var_index = aggList.indexOf(var[1]);
        switch (var_index) {
            case 0: value = stat.mean(); break;
            case 1: value = stat.sum(); break;
            case 2: value = stat.min(); break;
            case 3: value = stat.max(); break;
            case 4: value = stat.percentile25(); break;
            case 5: value = stat.median(); break;
            case 6: value = stat.percentile75(); break;
            default: throw IException(QString("Invalid aggregate expression for dynamic output: %1\nallowed:%2")
                                  .arg(var[1]).arg(aggList.join(" ")));
        }
        line+=QString::number(value);
    }
    mDynFieldList.append(line.join(";"));
}
