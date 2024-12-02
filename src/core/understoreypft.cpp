#include "understoreypft.h"
#include "understorey.h"
#include "csvfile.h"
#include "exception.h"


void UnderstoreyState::setup(UnderstoreySetting s, int index)
{
    mId = index;


    QString pft_id = s.value("pftId").toString();
    mPFT = Understorey::instance().pftByName(pft_id);
    if (!mPFT)
        throw IException(s.error(QString("the pftID is '%1', but that is not an available PFT.").arg(pft_id)));

    mSizeClass = s.value("size").toInt();
    // todo: check uniqueness!

    mName = QString("%1_%2").arg(pft_id).arg(mSizeClass);

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

void UnderstoreyPFT::setup(UnderstoreySetting s, int index)
{
    mIndex = index;
    mName = s.value("pftId").toString();

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

UStateId UnderstoreyPFT::stateTransition(const UnderstoreyPlant &plant,
                                         UnderstoreyCellParams &ucp,
                                         UnderstoreyRUStats &rustats) const
{
    double light_response = mExprLight.calculate(ucp.lif_corr);
    double nitrogen_response = mExprNutrients.calculate(ucp.availableNitrogen);
    double water_response = mExprWater.calculate(ucp.SWCgrowingSeason);

    // fake - should be some fancy function :=)
    double total_response = light_response * nitrogen_response  * water_response;

    // probability of going to next/previous state (fake!)
    double p_previous = total_response < 0.3 ? 0.2 : 0.1;
    double p_next = total_response > 0.6 ? 0.3 : 0.1;
    double p_mort = p_previous; // prob of mortality of PFT

    // draw a random number and determine the next state probabilistically
    const auto *state = Understorey::instance().state(plant.stateId());
    double r = drandom();
    if (r < p_mort) {
        // mortality
        rustats.died++;
        return std::numeric_limits<UStateId>::max();
    }
    if (r < p_previous + p_mort && !state->isFirstState()) {
        // decline to previous state
        rustats.transitionDown++;
        return plant.stateId() - 1;
    }
    if (r > 1. - p_next && !state->isFinalState()) {
        // growth to next state
        rustats.transitionUp++;
        return plant.stateId() + 1;
    }
    // no change
    return plant.stateId();
}

bool UnderstoreyPFT::establishment(UnderstoreyCellParams &ucp,
                                   UnderstoreyRUStats &rustats) const
{
    if (!ucp.PFTcalc) {
        ucp.nitrogenResponse = mExprNutrients.calculate(ucp.availableNitrogen);
        ucp.waterResponse = mExprWater.calculate(ucp.SWCgrowingSeason);
        ucp.PFTcalc = true;
    }

    double light_response = mExprLight.calculate(ucp.lif_corr);

    // fake - should be some fancy function :=)
    double total_response = light_response * ucp.nitrogenResponse  * ucp.waterResponse;

    if (drandom() < total_response) {
        // establish PFT on the cell
        rustats.established++;
        return true;
    }

    return false;
}
