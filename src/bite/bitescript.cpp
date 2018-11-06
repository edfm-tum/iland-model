#include "bite_global.h"
#include "bitescript.h"

#include "biteengine.h"
#include "biteagent.h"

#include "biteitem.h"
#include "bitedispersal.h"
#include "bitecolonization.h"
#include "fmtreelist.h"
namespace BITE {


BiteScript::BiteScript(QObject *parent): QObject(parent)
{

}

void BiteScript::setup(BiteEngine *biteengine)
{
    mEngine = biteengine;
    // setup links to JS Object
    QJSEngine *engine = BiteEngine::instance()->scriptEngine();

    if (engine->globalObject().hasOwnProperty("BiteAgent"))
        return; // already done

    qRegisterMetaType<ABE::FMTreeList*>("ABE::FMTreeList*"); // register type, required to have that type as property
    qRegisterMetaType<BiteItem*>("BiteItem*"); // register type, required to have that type as property
    qRegisterMetaType<BiteCellScript*>("BiteCellScript*"); // register type, required to have that type as property
    // create this object
    QJSValue jsObj = engine->newQObject(this);
    engine->globalObject().setProperty("Bite", jsObj);

    // createable objects: BiteAgent

    QJSValue jsMetaObject = engine->newQMetaObject(&BiteAgent::staticMetaObject);
    engine->globalObject().setProperty("BiteAgent", jsMetaObject);

    // BiteItem (base class)
    jsMetaObject = engine->newQMetaObject(&BiteItem::staticMetaObject);
    engine->globalObject().setProperty("BiteItem", jsMetaObject);

    // derived classes (tbd)
    jsMetaObject = engine->newQMetaObject(&BiteDispersal::staticMetaObject);
    engine->globalObject().setProperty("BiteDispersal", jsMetaObject);

    jsMetaObject = engine->newQMetaObject(&BiteColonization::staticMetaObject);
    engine->globalObject().setProperty("BiteColonization", jsMetaObject);


}

QStringList BiteScript::agents()
{
    return mEngine->agentNames();
}


QString BiteScript::JStoString(QJSValue value)
{
    if (value.isArray() || value.isObject()) {
        QJSValue fun = BiteEngine::instance()->scriptEngine()->evaluate("(function(a) { return JSON.stringify(a); })");
        QJSValue result = fun.call(QJSValueList() << value);
        return result.toString();
    } else
        return value.toString();

}

BiteAgent *BiteScript::agent(QString agent_name)
{
    BiteAgent *ag = mEngine->agentByName(agent_name);
    if (!ag)
        throw IException("There is no Bite Agent with name: " + agent_name);
    return ag;
}

void BiteScript::log(QString msg)
{
    qCDebug(bite).noquote() << msg;
}

void BiteScript::log(QJSValue obj)
{
    QString msg = JStoString(obj);
    qCDebug(bite).noquote() <<  msg;

}

} // end namespace
