/** @class OutputManager
   Global container that handles data output.

  */

#include "global.h"
#include "outputmanager.h"
#include "helper.h"
#include <QtCore>

// tree outputs
#include "treeout.h"
#include "standout.h"
#include "standdeadout.h"
#include "managementout.h"
#include "dynamicstandout.h"
#include "productionout.h"
#include "saplingout.h"



OutputManager::OutputManager()
{
    mTransactionOpen = false;
    // add all the outputs
    mOutputs.append(new TreeOut);
    mOutputs.append(new StandOut);
    mOutputs.append(new DynamicStandOut);
    mOutputs.append(new ProductionOut);
    mOutputs.append(new StandDeadOut);
    mOutputs.append(new ManagementOut);
    mOutputs.append(new SaplingOut);

}

OutputManager::~OutputManager()
{
    qDeleteAll(mOutputs);
}

void OutputManager::setup()
{
    close();
    XmlHelper &xml = const_cast<XmlHelper&>(GlobalSettings::instance()->settings());
    QString nodepath;
    foreach(Output *o, mOutputs) {
        nodepath = QString("output.%1").arg(o->tableName());
        xml.setCurrentNode(nodepath);
        qDebug() << "setup of output" << o->name();
        o->setup();
        bool enabled = xml.valueBool(".enabled", false);
        o->setEnabled(enabled);
        if (enabled)
            o->open();
    }
    endTransaction(); // just to be sure
}

Output *OutputManager::find(const QString& tableName)
{
    foreach(Output* p,mOutputs)
        if (p->tableName()==tableName)
            return p;
    return NULL;
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
}

/** start a database transaction.
    do nothing if transaction is already open. */
void OutputManager::startTransaction()
{
    if (!mTransactionOpen && GlobalSettings::instance()->dbout().isValid()) {
        if (GlobalSettings::instance()->dbout().transaction()) {
            qDebug() << "opening transaction";
            mTransactionOpen = true;
        }
    }
}
void OutputManager::endTransaction()
{
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
        if (!p->onNewRow()) {
            qWarning() << "Output" << p->name() << "invalid (not at new row)!!!";
            return false;
        }

        startTransaction(); // just assure a transaction is open.... nothing happens if already inside a transaction
        p->exec();

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
