#include "soil.h"
#include "globalsettings.h"
#include "xmlhelper.h" // for load settings
#include "exception.h"
/** @class Soil provides an implementation of the ICBM/2N soil carbon and nitrogen dynamics model.
  The ICBM/2N model was developed by K�tterer and Andren (2001) and used by others (e.g. Xenakis et al, 2008).
  See http://iland.boku.ac.at/soil+C+and+N+cycling for a model overview and the rationale of the model choice.

  */


// site-specific parameters
// i.e. parameters that need to be specified in the environment file
// note that leaching is not actually influencing soil dynamics but reduces availability of N to plants by assuming that some N
// (proportional to its mineralization in the mineral soil horizon) is leached
// see separate wiki-page (http://iland.boku.ac.at/soil+parametrization+and+initialization)
// and R-script on parameter estimation and initialization
struct SoilParams {
    // ICBM/2N parameters
    SoilParams(): qb(5.), qh(25.), leaching(0.15), el(0.0577), er(0.073), is_setup(false) {}
    double qb; ///< C/N ratio of soil microbes
    double qh; ///< C/N ratio of SOM
    double leaching; ///< how many percent of the mineralized nitrogen in O is not available for plants but is leached
    double el; ///< microbal efficiency in the labile pool, auxiliary parameter (see parameterization example)
    double er; ///< microbal efficiency in the refractory pool, auxiliary parameter (see parameterization example)
    bool is_setup;
} global_soilpar;
SoilParams *Soil::mParams = &global_soilpar; // save a ptr to the single value container as a static class variable

void Soil::fetchParameters()
{
    XmlHelper xml_site(GlobalSettings::instance()->settings().node("model.site"));
    mKo = xml_site.valueDouble("somDecompRate", 0.02);
    mH =  xml_site.valueDouble("soilHumificationRate", 0.3);

    if (mParams->is_setup || !GlobalSettings::instance()->model())
        return;
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.settings.soil"));
    mParams->qb =  xml.valueDouble("qb", 5.);
    mParams->qh =  xml.valueDouble("qh", 25.);
    mParams->leaching =  xml.valueDouble("leaching", 0.15);
    mParams->el =  xml.valueDouble("el", 0.0577);
    mParams->er = xml.valueDouble("er", 0.073);

    mParams->is_setup = true;
}


Soil::Soil()
{
    mRE = 0.;
    mAvailableNitrogen = 0.;
    mKyl = 0.;
    mKyr = 0.;
    mH = 0.;
    mKo = 0.;
    fetchParameters();
}

/// setup initial content of the soil pool (call before model start)
void Soil::setInitialState(const CNPool &young_labile_kg_ha, const CNPool &young_refractory_kg_ha, const CNPair &SOM_kg_ha)
{
    mYL = young_labile_kg_ha*0.001; // pool sizes are stored in t/ha
    mYR = young_refractory_kg_ha*0.001;
    mSOM = SOM_kg_ha*0.001;

    mKyl = young_labile_kg_ha.parameter();
    mKyr = young_refractory_kg_ha.parameter();

    if (mKyl<=0. || mKyr<=0.)
        throw IException(QString("setup of Soil: kyl or kyr invalid: kyl: %1 kyr: %2").arg(mKyl).arg(mKyr));
    if (!mYL.isValid())
        throw IException(QString("setup of Soil: yl-pool invalid: c: %1 n: %2").arg(mYL.C).arg(mYL.N));
    if (!mYL.isValid())
        throw IException(QString("setup of Soil: yr-pool invalid: c: %1 n: %2").arg(mYR.C).arg(mYR.N));
    if (!mYL.isValid())
        throw IException(QString("setup of Soil: som-pool invalid: c: %1 n: %2").arg(mSOM.C).arg(mSOM.N));
}

/// set soil inputs of current year (litter and deadwood)
void Soil::setSoilInput(const CNPool &labile_input_kg_ha, const CNPool &refractory_input_kg_ha)
{
    mInputLab = labile_input_kg_ha * 0.001; // transfer from kg/ha -> tons/ha
    mInputRef = refractory_input_kg_ha * 0.001;
    // calculate the decomposition rates
    mKyl = mYL.parameter(mInputLab);
    mKyr = mYR.parameter(mInputRef);

}


/// Main calculation function
/// must be called after snag dyanmics (i.e. to ensure input fluxes are available)
void Soil::calculateYear()
{
    SoilParams &sp = *mParams;
    // checks
    if (mRE==0) {
        throw IException("Soil::calculateYear(): Invalid value for 're' (0.)");
    }
    const double t = 1.; // timestep (annual)
    // auxiliary calculations
    CNPair total_in = mInputLab + mInputRef;
    double ylss = mInputLab.C / (mKyl * mRE); // Yl stedy state C
    double cl = sp.el * (1. - mH)/sp.qb - mH*(1.-sp.el)/sp.qh; // eta l in the paper
    double ynlss = 0.;
    if (!mInputLab.isEmpty())
        ynlss = mInputLab.C / (mKyl*mRE*(1.-mH)) * ((1.-sp.el)/mInputLab.CN() + cl); // Yl steady state N

    double yrss = mInputRef.C / (mKyr * mRE); // Yr steady state C
    double cr = sp.er * (1. - mH)/sp.qb - mH*(1.-sp.er)/sp.qh; // eta r in the paper
    double ynrss = 0.;
    if (!mInputRef.isEmpty())
        ynrss = mInputRef.C / (mKyr*mRE*(1.-mH)) * ((1.-sp.er)/mInputRef.CN() + cr); // Yr steady state N

    double oss = mH*total_in.C / (mKo*mRE); // O steady state C
    double onss = mH*total_in.C / (sp.qh*mKo*mRE); // O steady state N

    double al = mH*(mKyl*mRE* mYL.C - mInputLab.C) / ((mKo-mKyl)*mRE);
    double ar = mH*(mKyr*mRE* mYR.C - mInputRef.C) / ((mKo-mKyr)*mRE);

    // update of state variables
    // precalculations
    double lfactor = exp(-mKyl*mRE*t);
    double rfactor = exp(-mKyr*mRE*t);
    // young labile pool
    CNPair yl=mYL;
    mYL.C = ylss + (yl.C-ylss)*lfactor;
    mYL.N = ynlss + (yl.N-ynlss-cl/(sp.el-mH)*(yl.C-ylss))*exp(-mKyl*mRE*(1.-mH)*t/(1.-sp.el))+cl/(sp.el-mH)*(yl.C-ylss)*lfactor;
    mYL.setParameter( mKyl ); // update decomposition rate
    // young ref. pool
    CNPair yr=mYR;
    mYR.C = yrss + (yr.C-yrss)*rfactor;
    mYR.N = ynrss + (yr.N-ynrss-cr/(sp.er-mH)*(yr.C-yrss))*exp(-mKyr*mRE*(1.-mH)*t/(1.-sp.er))+cr/(sp.er-mH)*(yr.C-yrss)*rfactor;
    mYR.setParameter( mKyr ); // update decomposition rate
    // SOM pool (old)
    CNPair o = mSOM;
    mSOM.C = oss + (o.C -oss - al - ar)*exp(-mKo*mRE*t) + al*lfactor + ar*rfactor;
    mSOM.N = onss + (o.N - onss -(al+ar)/sp.qh)*exp(-mKo*mRE*t) + al/sp.qh * lfactor + ar/sp.qh * rfactor;



    // calculate plant available nitrogen
    double nav = mKyl*mRE*(1.-mH)/(1.-sp.el) * (mYL.N - sp.el*mYL.C/sp.qb); // N from labile...
    nav += mKyr*mRE*(1-mH)/(1.-sp.er)* (mYR.N - sp.er*mYR.C/sp.qb); // + N from refractory...
    nav += mKo*mRE*mSOM.N*(1.-sp.leaching); // + N from SOM pool (reduced by leaching (leaching modeled only from slow SOM Pool))
    mAvailableNitrogen = nav * 1000.; // from t/ha -> kg/ha

    if (mAvailableNitrogen<0.)
        mAvailableNitrogen = 0.;

    // stedy state for n-available
    //    double navss = mKyl*mRE*(1.-mH)/(1.-sp.el)*(ynlss-sp.el*ylss/sp.qb); // available nitrogen (steady state)
    //    navss += mKyr*mRE*(1.-mH)/(1.-sp.er)*(ynrss - sp.er*yrss/sp.qb);
    //    navss += mKo*mRE*onss*(1.-sp.leaching);

}

QList<QVariant> Soil::debugList()
{
    QList<QVariant> list;
    // (1) inputs of the year
    list << mInputLab.C << mInputLab.N << mInputLab.parameter() << mInputRef.C << mInputRef.N << mInputRef.parameter() << mRE;
    // (2) states
    list << mKyl << mKyr << mYL.C << mYL.N << mYR.C << mYR.N << mSOM.C << mSOM.N;
    // (3) nav
    list << mAvailableNitrogen;
    return list;
}







