#include "bite_global.h"
#include "bitescript.h"

#include "biteengine.h"
#include "biteagent.h"

#include "biteitem.h"
#include "bitedispersal.h"
#include "bitecolonization.h"

namespace BITE {


BiteScript::BiteScript()
{

}

void BiteScript::setup()
{
    // setup links to JS Object
    QJSEngine *engine = BiteEngine::instance()->scriptEngine();

    if (engine->globalObject().hasOwnProperty("BiteAgent"))
        return; // already done

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


QString BiteScript::JStoString(QJSValue value)
{
    if (value.isArray() || value.isObject()) {
        QJSValue fun = BiteEngine::instance()->scriptEngine()->evaluate("(function(a) { return JSON.stringify(a); })");
        QJSValue result = fun.call(QJSValueList() << value);
        return result.toString();
    } else
        return value.toString();

}

} // end namespace
