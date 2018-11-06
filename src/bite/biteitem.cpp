#include "biteitem.h"
#include "biteengine.h"

namespace BITE {

BiteItem::BiteItem(QObject *parent) : QObject(parent), mAgent(nullptr)
{

}

BiteItem::BiteItem(QJSValue obj) : QObject(nullptr), mAgent(nullptr)
{
    mObj = obj;


}

void BiteItem::setup(BiteAgent *agent)
{
    mAgent = agent;
    try {
        mDescription = BiteEngine::valueFromJs(mObj, "description").toString();
        setRunCells(true); // default: run by cell


    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of Bite item '%1': %2").arg(mName).arg(e.message());
        qCInfo(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }
    qCDebug(biteSetup) << "*** Setup of a item complete ***";
}

QString BiteItem::info()
{
    return QString("*** base class BiteItem ****");
}

void BiteItem::run()
{
    qCDebug(bite) << " *** Execution of item: " << name();
}

void BiteItem::runCell(BiteCell *cell, ABE::FMTreeList *treelist)
{
    // do nothing
}

int BiteItem::cellSize() const
{
    Q_ASSERT(mAgent!=nullptr);
    return agent()->cellSize();
}

QStringList BiteItem::allowedProperties()
{
    QStringList l;
    l << "description";
    return l;
}

void BiteItem::checkProperties(QJSValue obj)
{
    QStringList allowed = allowedProperties();
    if (obj.isObject()) {
        QJSValueIterator it(obj);
        while (it.hasNext()) {
            it.next();
            if (!allowed.contains(it.name())) {
                qCDebug(biteSetup) << it.name() << "is not a valid property! Allowed are: " << allowed;
            }
        }
    }
}


}
