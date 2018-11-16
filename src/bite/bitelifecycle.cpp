#include "bitelifecycle.h"
#include "biteengine.h"

namespace BITE {



BiteLifeCycle::BiteLifeCycle(QJSValue obj): BiteItem(obj)
{

}

void BiteLifeCycle::setup(BiteAgent *parent_agent)
{
    BiteItem::setup(parent_agent);
    try {

        checkProperties(mObj);

        QJSValue volt = BiteEngine::valueFromJs(mObj, "voltinism", "", "'voltinism' is a required property");
        mVoltinism.setup(volt, DynamicExpression::CellWrap, parent_agent);

        QJSValue spread_filter = BiteEngine::valueFromJs(mObj, "spreadFilter", "", "'spreadFilter' is a required property");
        mSpreadFilter.setup(spread_filter, DynamicExpression::CellWrap, parent_agent);

        mSpreadDelay = BiteEngine::valueFromJs(mObj, "spreadDelay", "0", "'spreadDelay' is a required property").toInt();
        mDieAfterDispersal = BiteEngine::valueFromJs(mObj, "dieAfterDispersal", "", "'dieAfterDispersal' is a required property").toBool();

        QJSValue spread_freq = BiteEngine::valueFromJs(mObj, "spreadInterval", "1", "'spreadInterval' is a required property");
        mSpreadInterval.setup(spread_freq, DynamicExpression::CellWrap, parent_agent);

        mThis = BiteEngine::instance()->scriptEngine()->newQObject(this);
        BiteAgent::setCPPOwnership(this);

        mEvents.setup(mObj, QStringList() <<  "onSetup", agent());
        QJSValueList eparam = QJSValueList() << thisJSObj();
        mEvents.run("onSetup", nullptr, &eparam);




    } catch (const IException &e) {
        QString error = QString("An error occured in the setup of BiteLifeCylcle item '%1': %2").arg(name()).arg(e.message());
        qCInfo(biteSetup) << error;
        BiteEngine::instance()->error(error);

    }

}

QString BiteLifeCycle::info()
{
    QString res = QString("Type: BiteLifeCycle\nDesc: %1").arg(description());
    return res;

}

void BiteLifeCycle::notify(BiteCell *cell, BiteCell::ENotification what)
{
    switch (what) {

    default: break; // ignore other notifications
    }
}

int BiteLifeCycle::numberAnnualCycles(BiteCell *cell)
{
    double vol_res = mVoltinism.evaluate(cell);
    if (agent()->verbose()) qCDebug(bite) << "LifeCycle: voltinism:" << vol_res;
    return static_cast<int>(vol_res);
}

bool BiteLifeCycle::shouldSpread(BiteCell *cell)
{
    if (!cell->isActive())
        return false;

    if (mSpreadDelay > cell->yearsLiving()) {
        if (agent()->verbose()) qCDebug(bite) << "Not spreading (initial delay)";
        return false;
    }

    if (mSpreadFilter.isValid()) {
        double res = mSpreadFilter.evaluate(cell);
        // the result of the filter is interpreted probabilistically
        if (drandom() < res) {
            if (agent()->verbose()) qCDebug(bite) << "Spreading, p:" << res;
            return true;
        }
    }

    double val = mSpreadInterval.evaluate(cell);
    if (BiteEngine::instance()->currentYear() - cell->yearLastSpread() >= val) {
        if (agent()->verbose()) qCDebug(bite) << "Spreading (Interval)";
        return true;
    }

    return false;

}

QStringList BiteLifeCycle::allowedProperties()
{
    QStringList l = BiteItem::allowedProperties();
    l << "dieAfterDispersal" << "spreadFilter" << "spreadDelay" << "spreadInterval" << "voltinism";
    return l;
}


} // end namespace
