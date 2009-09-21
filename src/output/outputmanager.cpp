#include "global.h"
#include "outputmanager.h"
#include <QtCore>

// tree outputs
#include "treeout.h"

OutputManager::OutputManager()
{
    // add all the outputs
    mOutputs.append(new TreeOut);
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
        p->startTransaction();
        p->exec();
        p->endTransaction();
        return true;
    }
    qDebug() << "output" << tableName << "not found!";
    return false; // no output found
}
