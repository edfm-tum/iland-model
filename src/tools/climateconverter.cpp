#include "climateconverter.h"
#include <QtScript>

Q_SCRIPT_DECLARE_QMETAOBJECT(ClimateConverter, QObject*)

void ClimateConverter::addToScriptEngine(QScriptEngine &engine)
{
    // about this kind of scripting magic see: http://qt.nokia.com/developer/faqs/faq.2007-06-25.9557303148
    QScriptValue cc_class = engine.scriptValueFromQMetaObject<ClimateConverter>();
    // the script name for the object is "ClimateConverter".
    engine.globalObject().setProperty("ClimateConverter", cc_class);
}

ClimateConverter::ClimateConverter(QObject *parent)
{

    bindExpression(mExpYear, 0);
    bindExpression(mExpMonth, 1);
    bindExpression(mExpDay, 2);

    bindExpression(mExpTemp, 3);
    bindExpression(mExpPrec, 4);
    bindExpression(mExpRad, 5);
    bindExpression(mExpVpd, 6);

}

void ClimateConverter::bindExpression(Expression &expr, int index)
{
    for (int i=0;i<10;i++)
        mVars[index*10 + i] = expr.addVar( QString("c%1").arg(i) );
}

void ClimateConverter::run()
{
    mExpYear.setExpression(mYear);
    mExpMonth.setExpression(mMonth);
    mExpDay.setExpression(mDay);

    mExpTemp.setExpression(mTemp);
    mExpPrec.setExpression(mPrec);
    mExpRad.setExpression(mRad);
    mExpVpd.setExpression(mVpd);

    for (int i=0;i<10;i++) {
        for (int j=0;j<6;j++) {
            *(mVars[j*10 + i]) = 1.2;
        }
    }
    int year = (int)mExpYear.execute();
    int month = (int)mExpMonth.execute();
    int day = (int)mExpDay.execute();
    double temp = mExpTemp.execute();
    double prec = mExpPrec.execute();
    double rad = mExpRad.execute();
    double vpd = mExpVpd.execute();
    qDebug() << year << month << day << temp << prec << rad << vpd;
}
