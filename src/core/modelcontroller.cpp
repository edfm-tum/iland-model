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

/** ModelController is a helper class used to control the flow of operations during a model run.
  The ModelController encapsulates the Model class and is the main control unit. It is used by the
  iLand GUI as well as the command line version (ilandc).

  */

#include "global.h"
#include "modelcontroller.h"
#include <QObject>

#include "model.h"
#include "debugtimer.h"
#include "helper.h"
#include "expression.h"
#include "expressionwrapper.h"
#include "../output/outputmanager.h"

#include "species.h"
#include "speciesset.h"
#include "mapgrid.h"
#include "statdata.h"

#ifdef ILAND_GUI
#include "mainwindow.h" // for the debug message buffering
#endif

ModelController::ModelController()
{
    mModel = NULL;
    mPaused = false;
    mRunning = false;
    mHasError = false;
    mYearsToRun = 0;
    mViewerWindow = 0;
    mDynamicOutputEnabled = false;
}

ModelController::~ModelController()
{
    destroy();
}

void ModelController::connectSignals()
{
    if (!mViewerWindow)
        return;
#ifdef ILAND_GUI
    connect(this,SIGNAL(bufferLogs(bool)), mViewerWindow, SLOT(bufferedLog(bool)));
#endif
}

/// prepare a list of all (active) species
QHash<QString, QString> ModelController::availableSpecies()
{
    QHash<QString, QString> list;
    if (mModel) {
        SpeciesSet *set = mModel->speciesSet();
        if (!set)
            throw IException("there are 0 or more than one species sets.");
        foreach (const Species *s, set->activeSpecies()) {
            list[s->id()] = s->name();
        }
    }
    return list;
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
    return mRunning;
}

bool ModelController::isFinished()
{
    if (!mModel)
        return false;
    return canRun() && !isRunning()  && mFinished;
}

bool ModelController::isPaused()
{
    return mPaused;
}

int ModelController::currentYear() const
{
    return GlobalSettings::instance()->currentYear();
}

void ModelController::setFileName(QString initFileName)
{
    mInitFile = initFileName;
    try {
        GlobalSettings::instance()->loadProjectFile(mInitFile);
    } catch(const IException &e) {
        QString error_msg = e.message();
        Helper::msg(error_msg);
        mHasError = true;
        mLastError = error_msg;
        qDebug() << error_msg;
    }
}

void ModelController::create()
{
    if (!canCreate())
        return;
    emit bufferLogs(true);

    try {
        mHasError = false;
        DebugTimer::clearAllTimers();
        mModel = new Model();
        mModel->loadProject();
        if (!mModel->isSetup()) {
            mHasError = true;
            mLastError = "An error occured during the loading of the project. Please check the logs.";
            return;
        }

        // reset clock...
        GlobalSettings::instance()->setCurrentYear(1); // reset clock
        // initialization of trees, output on startup
        mModel->beforeRun();
    } catch(const IException &e) {
        QString error_msg = e.message();
        Helper::msg(error_msg);
        mLastError = error_msg;
        mHasError = true;
        qDebug() << error_msg;
    }
    emit bufferLogs(false);

    qDebug() << "Model created.";
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

void ModelController::runloop()
{
    static QTime sLastTime = QTime::currentTime();
#ifdef ILAND_GUI
 //   QApplication::processEvents();
#else
 //   QCoreApplication::processEvents();
#endif
    if (mPaused)
        return;
    bool doStop = false;
    mHasError = false;
    if (GlobalSettings::instance()->currentYear()<=1) {
        sLastTime = QTime::currentTime(); // reset clock at the beginning of the simulation
    }

    if (!mCanceled && GlobalSettings::instance()->currentYear() < mYearsToRun) {
        emit bufferLogs(true);

        mHasError = runYear(); // do the work!

        mRunning = true;
        emit year(GlobalSettings::instance()->currentYear());
        if (!mHasError) {
            int elapsed = sLastTime.msecsTo(QTime::currentTime());
            int time=0;
            if (currentYear()%50==0 && elapsed>10000)
                time = 100; // a 100ms pause...
            if (currentYear()%100==0 && elapsed>10000) {
                time = 500; // a 500ms pause...
            }
            if (time>0) {
                sLastTime = QTime::currentTime(); // reset clock
                qDebug() << "--- little break ---- (after " << elapsed << "ms).";
                //QTimer::singleShot(time,this, SLOT(runloop()));
            }

        } else {
           doStop = true; // an error occured
           mLastError = "An error occured while running the model. Please check the logs.";
           mHasError = true;
        }

    } else {
        doStop = true; // all years simulated
    }

    if (doStop || mCanceled) {
                // finished
        internalStop();
    }

#ifdef ILAND_GUI
    QApplication::processEvents();
#else
    QCoreApplication::processEvents();
#endif
}

bool ModelController::internalRun()
{
    // main loop

    while (mRunning && !mPaused &&  !mFinished) {
        runloop(); // start the running loop
    }
    return isFinished();
}

void ModelController::internalStop()
{
    if (mRunning) {
        GlobalSettings::instance()->outputManager()->save();
        DebugTimer::printAllTimers();
        mFinished = true;
    }
    mRunning = false;
    emit bufferLogs(false); // stop buffering
    emit finished(QString());
    emit stateChanged();

}

void ModelController::run(int years)
{
    if (!canRun())
        return;
    emit bufferLogs(true); // start buffering

    DebugTimer many_runs(QString("Timer for %1 runs").arg(years));
    mPaused = false;
    mFinished = false;
    mCanceled = false;
    mYearsToRun = years;
    //GlobalSettings::instance()->setCurrentYear(1); // reset clock

    DebugTimer::clearAllTimers();

    mRunning = true;
    emit stateChanged();

    qDebug() << "ModelControler: runloop started.";
    internalRun();
    emit stateChanged();
}

bool ModelController::runYear()
{
    if (!canRun()) return false;
    DebugTimer t("ModelController:runYear");
    qDebug() << "ModelController: run year" << currentYear();

    if (GlobalSettings::instance()->settings().paramValueBool("debug_clear"))
        GlobalSettings::instance()->clearDebugLists();  // clear debug data
    bool err=false;
    try {
        emit bufferLogs(true);
        mModel->runYear();
        fetchDynamicOutput();
    } catch(const IException &e) {
        QString error_msg = e.message();
        Helper::msg(error_msg);
        qDebug() << error_msg;
        err=true;
    }
    emit bufferLogs(false);
    return err;
}

bool ModelController::pause()
{
    if(!isRunning())
        return mPaused;

    if (mPaused) {
        // currently in pause - mode -> continue
        mPaused = false;

    } else {
        // currently running -> set to pause mode
        GlobalSettings::instance()->outputManager()->save();
        mPaused = true;
        emit bufferLogs(false);
    }
    emit stateChanged();
    return mPaused;
}

bool ModelController::continueRun()
{
    mRunning = true;
    emit stateChanged();
    return internalRun();
}

void ModelController::cancel()
{
    mCanceled = true;
    internalStop();
    emit stateChanged();
}

QMutex error_mutex;
void ModelController::throwError(const QString msg)
{
    QMutexLocker lock(&error_mutex); // serialize access
    qDebug() << "ModelController: throwError reached:";
    qDebug() << msg;
    mLastError = msg;
    mHasError = true;
    emit bufferLogs(false);
    emit bufferLogs(true); // start buffering again

    throw IException(msg); // raise error again

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
    mDynamicOutputEnabled = true;
}

QString ModelController::dynamicOutput()
{
    return mDynData.join("\n");
}

const QStringList aggList = QStringList() << "mean" << "sum" << "min" << "max" << "p25" << "p50" << "p75" << "p5"<< "p10" << "p90" << "p95";
void ModelController::fetchDynamicOutput()
{
    if (!mDynamicOutputEnabled || mDynFieldList.isEmpty())
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
            case 7: value = stat.percentile(5); break;
            case 8: value = stat.percentile(10); break;
            case 9: value = stat.percentile(90); break;
            case 10: value = stat.percentile(95); break;
            default: throw IException(QString("Invalid aggregate expression for dynamic output: %1\nallowed:%2")
                                  .arg(var[1]).arg(aggList.join(" ")));
        }
        line+=QString::number(value);
    }
    line.prepend( QString::number(data.size()) );
    line.prepend( QString::number(GlobalSettings::instance()->currentYear()) );
    mDynData.append(line.join(";"));
}

void ModelController::saveScreenshot(QString file_name)
{
#ifdef ILAND_GUI
    if (!mViewerWindow)
        return;
    QImage img = mViewerWindow->screenshot();
    img.save(file_name);
#endif
}

void ModelController::paintMap(MapGrid *map, double min_value, double max_value)
{
#ifdef ILAND_GUI
    if (mViewerWindow) {
        mViewerWindow->paintGrid(map, "", GridViewRainbow, min_value, max_value);
        qDebug() << "painted map grid" << map->name() << "min-value (blue):" << min_value << "max-value(red):" << max_value;
    }
#else
    Q_UNUSED(map);Q_UNUSED(min_value);Q_UNUSED(max_value);
#endif
}

void ModelController::addGrid(const FloatGrid *grid, const QString &name, const GridViewType view_type, double min_value, double max_value)
{
#ifdef ILAND_GUI

    if (mViewerWindow) {
        mViewerWindow->paintGrid(grid, name, view_type, min_value, max_value);
        qDebug() << "painted grid min-value (blue):" << min_value << "max-value(red):" << max_value;
    }
#else
    Q_UNUSED(grid); Q_UNUSED(name); Q_UNUSED(view_type); Q_UNUSED(min_value);Q_UNUSED(max_value);
#endif
}

void ModelController::addLayers(const LayeredGridBase *layers, const QString &name)
{
#ifdef ILAND_GUI
    if (mViewerWindow)
        mViewerWindow->addLayers(layers, name);
    //qDebug() << layers->names();
#else
    Q_UNUSED(layers); Q_UNUSED(name);
#endif
}

void ModelController::setViewport(QPointF center_point, double scale_px_per_m)
{
#ifdef ILAND_GUI
    if (mViewerWindow)
        mViewerWindow->setViewport(center_point, scale_px_per_m);
#else
    Q_UNUSED(center_point);Q_UNUSED(scale_px_per_m);
#endif
}

void ModelController::repaint()
{
#ifdef ILAND_GUI
    if (mViewerWindow)
        mViewerWindow->repaint();
#endif
}



