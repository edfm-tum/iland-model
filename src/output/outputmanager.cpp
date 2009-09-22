/** @class OutputManager
   Global container that handles data output.

  */

#include "global.h"
#include "outputmanager.h"
#include <QtCore>

// tree outputs
#include "treeout.h"
#include "standout.h"
#include "dynamicstandout.h"



OutputManager::OutputManager()
{
    // add all the outputs
    mOutputs.append(new TreeOut);
    mOutputs.append(new StandOut);
    mOutputs.append(new DynamicStandOut);

}

OutputManager::~OutputManager()
{
    qDeleteAll(mOutputs);
}

void OutputManager::setup()
{
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
}

Output *OutputManager::find(const QString& tableName)
{
    foreach(Output* p,mOutputs)
        if (p->tableName()==tableName)
            return p;
    return NULL;
}

bool OutputManager::execute(const QString& tableName)
{
    Output *p = find(tableName);
    if (p) {
        if (!p->isEnabled())
            return false;
        if(!p->isOpen())
            return false;
        p->startTransaction();
        p->exec();
        p->endTransaction();
        return true;
    }
    qDebug() << "output" << tableName << "not found!";
    return false; // no output found
}
