#include "understoreypft.h"
#include "understorey.h"
#include "csvfile.h"
#include "exception.h"

UnderstoreyPFT::UnderstoreyPFT() {}

void UnderstoreyState::setup(UnderstoreySetting s)
{
    mName = s.value("name").toString();
    UStateId id = s.value("id").toInt();
    if (id == 0)
        throw IException(s.error("id is 0 (use Ids > 0!)"));
    if (Understorey::instance().stateById(id) != nullptr)
        throw IException(s.error(QString("the 'id' is %1, which is not unique (used earlier)").arg(id)));
    mId = id;
    mNSlots = s.value("slots").toInt();
    if (mNSlots <= 0 || mNSlots>10)
        throw IException(s.error(QString("'slots' is %1, which is not valid (>0, <=10)")));

    mLAI = s.value("LAI").toDouble();
    mBiomass = s.value("biomass").toDouble();
    mCover = s.value("cover").toDouble();
    if (mCover <= 0. || mCover>1.)
        throw IException(s.error(QString("'cover' is %1, which is not valid (0..1)")));
    mHeight = s.value("height").toDouble();
    if (mHeight <= 0. || mHeight>4.)
        throw IException(s.error(QString("'height' is %1, which is not valid (0m..4m)")));

    // environmental responses
    QString resp = "lightResponse";
    try {
        mExprLight.setAndParse(s.value(resp).toString());
        mExprLight.linearize(0., 1.);
        // water
        resp = "waterResponse";
        mExprWater.setAndParse(s.value(resp).toString());
        mExprWater.linearize(0.,1.);
        // nutrients
        resp = "nutrientResponse";
        mExprNutrients.setAndParse(s.value(resp).toString());
        mExprNutrients.linearize(0., 1.);

    } catch (const IException &e) {
        throw IException( s.error(QString("'%1': Expression error: %2").arg(resp, e.message()))   );
    }
}

QString UnderstoreySetting::error(QString msg)
{
        return QString("Setup PFTs: Error in '%1' (line %2): %3")
            .arg(mFile->value(mRowNumber, "name").toString())
        .arg(mRowNumber)
        .arg(msg);
}

QVariant UnderstoreySetting::value(const QString &column_name)
{
    int i = mFile->columnIndex(column_name);
    if (i < 0)
        throw IException(error(QString("column '%1' not found!").arg(column_name) ));
    return mFile->value(mRowNumber, i);
}

bool UnderstoreySetting::hasColumn(const QString &column_name)
{
    return mFile->columnIndex(column_name) >= 0;
}
