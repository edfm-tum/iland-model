/** ModelController is a helper class used to
  control the flow of operations during a model run.
  Really useful???? or a dispatcher???
  */

#include "global.h"
#include "modelcontroller.h"
#include <QObject>

#include "model.h"
#include "helper.h"
#include "expression.h"
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
 return GlobalSettings::instance()->currentYear()>0;
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
        GlobalSettings::instance()->clearDebugLists();
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
        GlobalSettings::instance()->setCurrentYear(0);
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
    if (GlobalSettings::instance()->settings().paramValueBool("debug_clear"))
        GlobalSettings::instance()->clearDebugLists();  // clear debug data

    try {
        mModel->runYear();
        fetchDynamicOutput();
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
    mDynFieldList.clear();
    if (!fieldList.isEmpty()) {
        QRegExp rx("((?:\\[.+\\]|\\w+)\\.\\w+)");
        int pos=0;
        while ((pos = rx.indexIn(fieldList, pos)) != -1) {
            mDynFieldList.append(rx.cap(1));
            pos += rx.matchedLength();
        }

        //mDynFieldList = fieldList.split(QRegExp("(?:\\[.+\\]|\\w+)\\.\\w+"), QString::SkipEmptyParts);
        mDynFieldList.prepend("count");
        mDynFieldList.prepend("year"); // fixed fields.
    }
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
    DebugTimer t("dynamic output");
    QStringList var;
    QString lastVar = "";
    QVector<double> data;
    AllTreeIterator at(mModel);
    TreeWrapper tw;
    int var_index;
    StatData stat;
    double value;
    QStringList line;
    Expression custom_expr;
    bool simple_expression;
    foreach (QString field, mDynFieldList) {
        if (field=="count" || field=="year")
            continue;
        if (field.count()>0 && field.at(0)=='[') {
            QRegExp rex("\\[(.+)\\]\\.(\\w+)");
            rex.indexIn(field);
            var = rex.capturedTexts();
            var.pop_front(); // drop first element (contains the full string)
            simple_expression = false;
        } else {
            var = field.split(QRegExp("\\W+"), QString::SkipEmptyParts);
            simple_expression = true;
        }
        if (var.count()!=2)
                throw IException(QString("Invalid variable name for dynamic output:") + field);
        if (var.first()!=lastVar) {
            // load new field
            data.clear();
            at.reset(); var_index = 0;
            if (simple_expression) {
                var_index = tw.variableIndex(var.first());
                if (var_index<0)
                    throw IException(QString("Invalid variable name for dynamic output:") + var.first());

            } else {
                custom_expr.setExpression(var.first());
                custom_expr.setModelObject(&tw);
            }
            while (Tree *t = at.next()) {
                tw.setTree(t);
                if (simple_expression)
                    value = tw.value(var_index);
                else
                    value = custom_expr.execute();
                data.push_back(value);
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
    line.prepend( QString::number(data.size()) );
    line.prepend( QString::number(GlobalSettings::instance()->currentYear()) );
    mDynData.append(line.join(";"));
}
