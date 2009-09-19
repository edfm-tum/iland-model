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
        nodepath = QString("output.%1").arg(o->name());
        xml.setCurrentNode(nodepath);
        qDebug() << "setup of output" << o->name();
        o->setup();
    }
}
