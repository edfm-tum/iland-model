/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
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

/** @class OutputManager
   Global container that handles data output.

  */

#include "global.h"
#include "outputmanager.h"
#include "debugtimer.h"
#include <QtCore>
#include <QtConcurrent>
#include "outputwriterthread.h"

// tree outputs
#include "treeout.h"
#include "standout.h"
#include "landscapeout.h"
#include "standdeadout.h"
#include "managementout.h"
#include "dynamicstandout.h"
#include "productionout.h"
#include "saplingout.h"
#include "carbonout.h"
#include "carbonflowout.h"
#include "soilinputout.h"
#include "waterout.h"
#include "svdout.h"
#include "svdindicatorout.h"
#include "devstageout.h"
#include "ecovizout.h"
#include "customaggout.h"
#include "understoryout.h"


// on creation of the output manager
// an instance of every iLand output
// must be added to the list of outputs.
OutputManager::OutputManager()
{
    mTransactionOpen = false;
    mThread = new OutputWriterThread();
    mGlobalBufferSize = 0;
    mGlobalBufferCap = 3276800; // default: 100MB (100 * 1024*1024 / 32)

    // add all the outputs
    mOutputs.append(new TreeOut);
    mOutputs.append(new TreeRemovedOut);
    mOutputs.append(new StandOut);
    mOutputs.append(new LandscapeOut);
    mOutputs.append(new LandscapeRemovedOut);
    mOutputs.append(new DynamicStandOut);
    mOutputs.append(new CustomAggOut);
    mOutputs.append(new ProductionOut);
    mOutputs.append(new StandDeadOut);
    mOutputs.append(new ManagementOut);
    mOutputs.append(new SaplingOut);
    mOutputs.append(new SaplingDetailsOut);
    mOutputs.append(new CarbonOut);
    mOutputs.append(new CarbonFlowOut);
    mOutputs.append(new SoilInputOut);
    mOutputs.append(new WaterOut);
    mOutputs.append(new SVDGPPOut);
    mOutputs.append(new SVDStateOut);
    mOutputs.append(new SVDIndicatorOut);
    mOutputs.append(new SVDUniqueStateOut);
    mOutputs.append(new DevStageOut);
    mOutputs.append(new EcoVizOut);
    mOutputs.append(new UnderstoryOut);
}

void OutputManager::addOutput(Output *output)
{
    mOutputs.append(output);
}

void OutputManager::removeOutput(const QString &tableName)
{
    Output *o = find(tableName);
    if (o) {
        mOutputs.removeAt(mOutputs.indexOf(o));
        delete o;
    }
}


OutputManager::~OutputManager()
{
    if (mThread) {
        mThread->stop();
        delete mThread;
    }
    qDeleteAll(mOutputs);
}

void OutputManager::setup()
{
    //close();
    qDebug() << "Setting up outputs...";
    bool buffered = GlobalSettings::instance()->settings().valueBool("system.settings.bufferOutput", false);
    if (buffered) {
        int buffer_size_mb = GlobalSettings::instance()->settings().valueInt("system.settings.bufferSize", 100);
        mGlobalBufferCap = buffer_size_mb * (1024*1024 / 32); // convert MB to QVariant count
        qDebug() << "Output buffering enabled. Global buffer size: " << buffer_size_mb << "MB (" << mGlobalBufferCap << " items). Starting background thread.";
        // If the thread is already running (e.g. from a previous run), stop and wait for it
        // to ensure all old data is written and we can cleanly switch the database file.
        if (mThread->isRunning()) {
            mThread->stop();
            mThread->wait();
        }
        mThread->setDatabaseName(GlobalSettings::instance()->dbout().databaseName());
        mThread->resetAbort(); // Reset the abort flag
        mThread->start();
    }

    // Ensure we are not in a transaction when creating tables (DDL)
    if (mTransactionOpen) {
        endTransaction();
    }

    QStringList output_names;
    XmlHelper &xml = const_cast<XmlHelper&>(GlobalSettings::instance()->settings());
    QString nodepath;
    foreach(Output *o, mOutputs) {
        nodepath = QString("output.%1").arg(o->tableName());
        xml.setCurrentNode(nodepath);
        output_names.push_back(o->tableName());
        o->setup();
        bool enabled = xml.valueBool(".enabled", false);
        bool file_mode = false;
        if (xml.hasNode(".mode"))
            file_mode = xml.value(".mode") == "file";
        if (file_mode)
            o->setMode(OutFile);
        o->setEnabled(enabled);
        o->setBuffered(buffered);
        if (enabled)
            o->open();
    }
    qDebug() << "processed" << output_names.size() << "outputs: " << output_names;
    qDebug() << "Setup of outputs completed.";
    
    // Force a commit of the schema changes so the background thread can see the new tables
    if (GlobalSettings::instance()->dbout().transaction()) {
        GlobalSettings::instance()->dbout().commit();
    }
    endTransaction(); // just to be sure
}

Output *OutputManager::find(const QString& tableName)
{
    foreach(Output* p,mOutputs)
        if (p->tableName()==tableName)
            return p;

    return nullptr;
}



void runOutput(Output *p)
{
    if (!p->isRowEmpty()) {
         qWarning() << "Output" << p->name() << "invalid (not at new row)!!!";
         return;
    }

    p->exec();
    p->flush();
}



void OutputManager::executeParallel(const QStringList &tableNames)
{
    DebugTimer t("OutputManager::executeParallel()");
    t.setSilent();

    QList<Output*> parallel_list;

    // Separate serial and parallel outputs
    foreach(const QString &name, tableNames) {
        Output *p = find(name);
        if (!p || !p->isEnabled()) continue;

        if (p->isBuffered()) {
            // Verify thread is running for buffered outputs
            if (!mThread->isRunning())
                 throw IException("OutputManager: Output Writer Thread is not running! Cannot save data.");

            parallel_list.append(p);
        } else {
             execute(name); // Execute non-buffered immediately (serial)
        }
    }

    // Execute parallel outputs
    if (!parallel_list.isEmpty()) {
        QTimer responsivenessTimer;
        // Only set up the timer if we are on the main GUI thread
        if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
            responsivenessTimer.setInterval(100); // Fire every 100ms

            // When the timer fires, process UI events.
            QObject::connect(&responsivenessTimer, &QTimer::timeout, []() {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            });

            responsivenessTimer.start();
        }

        QtConcurrent::blockingMap(parallel_list, runOutput);

        // Stop the timer once the blocking call is finished
        if (responsivenessTimer.isActive()) {
            responsivenessTimer.stop();
        }
    }
}



void OutputManager::save()
{
    endTransaction();
}

void OutputManager::close()
{
    qDebug() << "outputs closed";
    foreach(Output *p, mOutputs)
        p->close();
    if (mThread->isRunning())
        mThread->stop();
}

/** start a database transaction.
    do nothing if transaction is already open. */
void OutputManager::startTransaction()
{
    //return; // test without transactions
    if (!mTransactionOpen && GlobalSettings::instance()->dbout().isValid()) {
        if (GlobalSettings::instance()->dbout().transaction()) {
            qDebug() << "opening transaction";
            mTransactionOpen = true;
        }
    }
}
void OutputManager::endTransaction()
{
    //return; // test without transactions
    if (mTransactionOpen && GlobalSettings::instance()->dbout().isValid()) {
        if (GlobalSettings::instance()->dbout().commit()) {
            mTransactionOpen = false;
            qDebug() << "database transaction commited";
        }
    }
}

bool OutputManager::execute(const QString& tableName)
{
    DebugTimer t("OutputManager::execute()");
    t.setSilent();
    Output *p = find(tableName);
    if (p) {
        if (!p->isEnabled())
            return false;
        if(!p->isOpen())
            return false;
        if (!p->isRowEmpty()) {
            qWarning() << "Output" << p->name() << "invalid (not at new row)!!!";
            return false;
        }

        if (!p->isBuffered())
            startTransaction(); // just assure a transaction is open.... nothing happens if already inside a transaction
        
        p->exec();

        if (p->isBuffered()) {
            if (!mThread->isRunning())
                 throw IException("OutputManager: Output Writer Thread is not running! Cannot save data.");
            p->flush();
        }

        return true;
    }
    qDebug() << "output" << tableName << "not found!";
    return false; // no output found
}


QString OutputManager::wikiFormat()
{
    QString result;
    foreach(const Output *o, mOutputs)
        result+=o->wikiFormat() + "\n\n";
    return result;
}

