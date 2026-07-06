#include "understorypft.h"
#include "understory.h"
#include "csvfile.h"
#include "exception.h"
#include "resourceunit.h"


void UnderstoryState::setup(UnderstorySetting s, int index)
{
    mId = index;


    QString pft_id = s.value("pftId").toString();
    mPFT = Understory::instance().pftByName(pft_id);
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

QString UnderstorySetting::error(QString msg)
{
        return QString("Setup PFTs: Error in '%1' (line %2): %3")
            .arg(mFile->value(mRowNumber, "pftId").toString())
        .arg(mRowNumber+1) // count also header
        .arg(msg);
}

QVariant UnderstorySetting::value(const QString &column_name)
{
    int i = mFile->columnIndex(column_name);
    if (i < 0)
        throw IException(error(QString("column '%1' not found!").arg(column_name) ));
    return mFile->value(mRowNumber, i);
}

bool UnderstorySetting::hasColumn(const QString &column_name)
{
    return mFile->columnIndex(column_name) >= 0;
}

void UnderstoryPFT::setup(UnderstorySetting s, int index)
{
    mIndex = index;

    // environmental responses
    QString resp;
    QString expr;
    bool ok;
    try {
        // load properties
        mName = s.value("pftId").toString();
        mBaseEstablishmentProb = s.value("estBaseProb").toDouble(&ok);
        if (!ok || mBaseEstablishmentProb<0) throw IException("invalid value for 'estBaseProb'!");

        mBaseMortalityProb = s.value("mortalityBaseProb").toDouble(&ok);
        if (!ok || mBaseMortalityProb<0) throw IException("invalid value for 'mortalityBaseProb'!");

        mOptimalGrowth = s.value("optimalGrowth").toDouble(&ok);
        if (!ok || mOptimalGrowth<=0) throw IException("invalid value for 'optimalGrowth'!");

        mPDecline = s.value("pDecline").toDouble(&ok);
        if (!ok || mPDecline<0) throw IException("invalid value for 'pDecline'!");

        mEffectiveLAIfactor = s.value("effectiveLAIfactor").toDouble(&ok);
        if (!ok || mEffectiveLAIfactor<0 || mEffectiveLAIfactor>1) throw IException("invalid value for 'effectiveLAI'!");

        mPsiMin = s.value("psiMin").toDouble(&ok);
        mPsiMin = -fabs(mPsiMin); // make sure it is negative
        if (!ok || mPsiMin < -10) throw IException("invalid value for 'psiMin'!");

        mTurnoverRate = s.value("turnoverRate").toDouble(&ok);
        if (!ok || mTurnoverRate<0. || mTurnoverRate>1.) throw IException("invalid value for 'turnoverRate' (0..1)");
        mBelowgroundFrac = s.value("belowgroundFrac").toDouble(&ok);
        if (!ok || mBelowgroundFrac<0. || mBelowgroundFrac>1.) throw IException("invalid value for 'belowgroundFrac' (0..1)");
        mLitterDecompRate = s.value("litterDecompRate").toDouble(&ok);
        if (!ok || mLitterDecompRate<0. || mLitterDecompRate>1.) throw IException("invalid value for 'litterDecompRate' (0..1)");

        // response functions
        resp = "lightResponse";
        expr = s.value(resp).toString();
        if (expr.isEmpty()) {
            expr = QString("min(0.05 + 1.5/(1 + ((lr - (0.15*LightEIV-0.4))/(0.8-0.05*LightEIV))^2),1)");
        }
        if (expr.contains("LightEIV")) {
            double value = s.value("LightEIV").toDouble(&ok);
            if (!ok || value < 1. || value > 9.) throw IException("LightEIV required, but not a number or out of range (1..9)!");
            expr.replace("LightEIV", s.value("LightEIV").toString());
        }

        mExprLight.setAndParse(expr);
        mExprLight.linearize(0., 1.);
        // water
        resp = "waterResponse";
        expr = s.value(resp).toString();
        if (expr.isEmpty()) {
            expr = QString("min(0.05 + 1.3/(1 + ((swpgs + 14 + (10-MoistureEIV)*0.5)/3)^2),1)");
        }
        if (expr.contains("MoistureEIV")) {
            double value = s.value("MoistureEIV").toDouble(&ok);
            if (!ok || value < 1. || value > 9.) throw IException("MoistureEIV required, but not a number or out of range (1..9)!");
            expr.replace("MoistureEIV", s.value("MoistureEIV").toString());
        }

        mExprWater.setAndParse(expr);
        mExprWater.linearize(0.,1.);

        // nutrients
        resp = "nutrientResponse";
        expr = s.value(resp).toString();
        if (expr.isEmpty()) {
            expr = QString("min(0.05 + 1.5/(1 + ((PlantAvailN - (5*NutrientEIV+40))/20)^2),1)");
        }
        if (expr.contains("NutrientEIV")) {
            double value = s.value("NutrientEIV").toDouble(&ok);
            if (!ok || value < 1. || value > 9.) throw IException("NutrientEIV required, but not a number or out of range (1..9)!");
            expr.replace("NutrientEIV", s.value("NutrientEIV").toString());
        }

        mExprNutrients.setAndParse(expr);
        mExprNutrients.linearize(0., 1.);

        // temperature responose
        resp = "tempResponse";
        expr = s.value(resp).toString();
        if (expr.isEmpty()) {
            expr = QString("min(0.05 + 1.2/(1 + ((meanTemp - (TemperatureEIV*1.5))/3)^2),1)");
        }
        if (expr.contains("TemperatureEIV")) {
            double value = s.value("TemperatureEIV").toDouble(&ok);
            if (!ok || value < 1. || value > 9.) throw IException("TemperatureEIV required, but not a number or out of range (1..9)!");
            expr.replace("TemperatureEIV", s.value("TemperatureEIV").toString());
        }

        mExprTemp.setAndParse(expr);
        mExprTemp.linearize(0., 1.);

        // expression for stress
        resp = "stressFunction";
        expr = s.value(resp).toString();
        if (expr.isEmpty()) {
            expr = QString("0.5*exp(-x/stressSensitivity)");
        }
        if (expr.contains("stressSensitivity")) {
            double value = s.value("stressSensitivity").toDouble(&ok);
            if (!ok || value < 0. || value > 1.) throw IException("stressSensitivity required, but not a number or out of range (0..1)!");
            expr.replace("stressSensitivity", s.value("stressSensitivity").toString());
        }

        mExprStress.setAndParse(expr);
        mExprStress.linearize(0., 1.);

        // total response for establishment
        resp = "establishmentResponse";
        expr = s.value(resp).toString();
        if (!expr.isEmpty()) {
            // variable sequence: light, water, temp, nitrogen
            mExprEstResponse.addVar("rlight");
            mExprEstResponse.addVar("rwater");
            mExprEstResponse.addVar("rtemp");
            mExprEstResponse.addVar("rnitrogen");
            mExprEstResponse.setExpression(expr);
            mExprEstResponse.parse();
        }

        // total response for state transition / growth

        resp = "growthResponse";
        expr = s.value(resp).toString();
        if (!expr.isEmpty()) {
            // variable sequence: light, water, temp, nitrogen
            mExprGrowthResponse.addVar("rlight");
            mExprGrowthResponse.addVar("rwater");
            mExprGrowthResponse.addVar("rtemp");
            mExprGrowthResponse.addVar("rnitrogen");
            mExprGrowthResponse.setExpression(expr);
            mExprGrowthResponse.parse();
        }



    } catch (const IException &e) {
        throw IException( s.error(QString("'%1' for PFT '%3': Expression error: %2").arg(resp, e.message(), name()))   );
    }
    if (logLevelDebug())
        qDebug().noquote() << dump(); // use noquote() to get newlines etc
}

UStateId UnderstoryPFT::stateTransition(const UnderstoryPlant &plant,
                                        UnderstoryCellParams &ucp,
                                        UnderstoryRU &us_ru) const
{
    double light_available = ucp.lightProfile->relativeLightAt(ucp.height, ucp.cell_index);
    ucp.lightResponse = mExprLight.calculate(light_available);

    // calculate environmental responses
    calculateEnvironment(ucp, true);

    /// calculate total responese based on user defined function (default: multiplication of all responses)
    double total_response = calculateResponse(ucp, true);


    // **********************************************************
    // translate environmental response to
    // transition probabilites
    // **********************************************************
    double p_previous, p_next, p_mort;
    responseToTransitionProb(total_response, p_previous, p_next, p_mort);

    // **********************************************************
    // the actual decision:
    // draw a random number and determine the next state probabilistically
    // **********************************************************
    const auto *state = Understory::instance().state(plant.stateId());
    double r = drandom();
    UStateId next_state = plant.stateId(); // default: no change
    if (r < p_mort) {
        // mortality
        us_ru.statsPlantDied(&plant);
        next_state = std::numeric_limits<UStateId>::max();
    } else if (r < p_previous + p_mort && !state->isFirstState()) {
        // decline to previous state
        us_ru.statsPlantTransition(&plant, false);
        next_state = plant.stateId() - 1;
    } else if (r > 1. - p_next && !state->isFinalState()) {
        // growth to next state
        us_ru.statsPlantTransition(&plant, true);
        next_state =  plant.stateId() + 1;
    }

    if (GlobalSettings::instance()->isDebugEnabled(GlobalSettings::dUnderstory)) {
        DebugList &out = GlobalSettings::instance()->debugList(us_ru.ru()->index(), GlobalSettings::dUnderstory );
        out << us_ru.ru()->index() << ucp.cell_index <<
            state->pft()->name() << state->id() << ucp.lightResponse << ucp.nitrogenResponse <<
            ucp.waterResponse << ucp.tempResponse << total_response <<
            p_mort << p_previous << p_next << next_state;
        //"ruindex", "cellindex",
        //    "pft", "stateId", "lightResponse", "nitrogenResponse", "waterResponse", "tempResponse", "totalResponse",
        //    "pMortality", "pDecline", "pGrowth", "nextStateId"};

    }


    return next_state;
}

bool UnderstoryPFT::establishment(UnderstoryCellParams &ucp,
                                   double n_represented) const
{
    calculateEnvironment(ucp);

    double light_response = mExprLight.calculate(ucp.ground_light);
    ucp.lightResponse = light_response;

    // calculate total response based on user-defined-function
    double total_response = calculateResponse(ucp, false);

    if (total_response == 0.)
        return false;

    // the test for establishment represented more than one cell, update the prob accordingly
    double p_adjusted = 1. - std::pow(1. - total_response, n_represented);
    if (drandom() < p_adjusted) {
        // establish PFT on the cell
        return true;
    }

    return false;
}

void UnderstoryPFT::calculateEnvironment(UnderstoryCellParams &ucp, bool always_calc) const
{
    if (always_calc || !ucp.PFTcalc) {
        ucp.nitrogenResponse = mExprNutrients.calculate(ucp.availableNitrogen);
        ucp.waterResponse = mExprWater.calculate(ucp.psiGrowingSeason);
        ucp.tempResponse = mExprTemp.calculate(ucp.meanTemperature);
        ucp.PFTcalc = true;
    }
}

double UnderstoryPFT::calculateResponse(const UnderstoryCellParams &ucp, bool calc_growth_response) const
{
    double local_vars[4];
    // variable sequence: light, water, temp, nitrogen
    local_vars[0] = ucp.lightResponse;
    local_vars[1] = ucp.waterResponse;
    local_vars[2] = ucp.tempResponse;
    local_vars[3] = ucp.nitrogenResponse;

    if (calc_growth_response) {
        // growth response
        // default: all factors multiplied
        if (mExprGrowthResponse.isEmpty())
            return ucp.lightResponse * ucp.waterResponse * ucp.tempResponse * ucp.nitrogenResponse;
        double response = mExprGrowthResponse.execute(local_vars);
        return limit(response, 0., 1.);

    } else {
        // establishment response
        // if function empty: response = light_response
        if (mExprEstResponse.isEmpty())
            return ucp.lightResponse;
        double response = mExprEstResponse.execute(local_vars);
        return limit(response, 0., 1.);
    }

}

QString UnderstoryPFT::dump()
{
    QString result;
    QTextStream str(&result);
    str << "PFT:" << name() << " index:" << index() << Qt::endl;
    str << "base establishment probability:" << mBaseEstablishmentProb << Qt::endl;
    str << "base mortality probability:" << mBaseMortalityProb << Qt::endl;
    str << "Response Light:" << mExprLight.expression()<< Qt::endl;
    str << "Response Nutrient:" << mExprNutrients.expression()<< Qt::endl;
    str << "Response Water:" << mExprWater.expression()<< Qt::endl;
    str << "Response Temperature:" << mExprTemp.expression()<< Qt::endl;
    str << "Stress function:" << mExprStress.expression()<< Qt::endl;

    return result;
}

void UnderstoryPFT::responseToTransitionProb(const double response, double &rPrevious, double &rNext, double &rMort) const
{
    // linear functions approach (KB): optimal growth: time to reach maximum state
    // rationale: prob of state change per year = rNext
    // expected value for the number of years for one step = 1/rNext, for N Steps: NStates * 1/rNext
    // ->  optimalGrowth = NStates * 1/rNext
    rNext = mNStates / mOptimalGrowth * response;

    // stress is a function of environment
    const double stress_factor = mExprStress.calculate(response);

    // mortality is based only on an annual prob. of mortality and stress
    rMort = mBaseMortalityProb + stress_factor - (mBaseMortalityProb*stress_factor);

    // decline is a fixed response
    rPrevious = mPDecline;



}
