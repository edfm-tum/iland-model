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
    SoilParams(): kyl(0.15), kyr(0.0807), ko(0.02), h(0.3), qb(5.), qh(25.), leaching(0.15), el(0.0577), er(0.073), is_setup(false) {}
    double kyl; ///< litter decomposition rate
    double kyr; ///< downed woody debris (dwd) decomposition rate
    double ko; ///< decomposition rate for soil organic matter (i.e. the "old" pool sensu ICBM)
    double h; ///< humification rate
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
    if (mParams->is_setup || !GlobalSettings::instance()->model())
        return;
    XmlHelper xml(GlobalSettings::instance()->settings().node("model.settings.soil"));
    mParams->kyl = xml.valueDouble("kyl", 0.15);
    mParams->kyr = xml.valueDouble("kyr", 0.0807);
    mParams->ko = xml.valueDouble("ko", 0.02);
    mParams->h =  xml.valueDouble("h", 0.3);
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
    fetchParameters();
}

/// setup initial content of the soil pool (call before model start)
void Soil::setInitialState(const CNPool &young_labile_kg_ha, const CNPool &young_refractory_kg_ha, const CNPool &SOM_kg_ha)
{
    mYL = young_labile_kg_ha*0.001;
    mYR = young_refractory_kg_ha*0.001;
    mSOM = SOM_kg_ha*0.001;

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
}


/// Main calculation function
/// must be called after snag dyanmics (i.e. to ensure input fluxes are available)
void Soil::calculateYear()
{
    SoilParams &sp = *mParams;
    // checks
    if (mRE==0) {
        throw IException("Soil::calculate(): Invalid value for 're' (0.)");
    }
    const double t = 1.; // timestep (annual)
    // auxiliary calculations
    CNPool total_in = mInputLab + mInputRef;
    double ylss = mInputLab.C / (sp.kyl * mRE); // Yl stedy state C
    double cl = sp.el * (1. - sp.h)/sp.qb - sp.h*(1.-sp.el)/sp.qh; // eta l in the paper
    double ynlss = 0.;
    if (!mInputLab.isEmpty())
        ynlss = mInputLab.C / (sp.kyl*mRE*(1.-sp.h)) * ((1.-sp.el)/mInputLab.CN() + cl); // Yl steady state N

    double yrss = mInputRef.C / (sp.kyr * mRE); // Yr steady state C
    double cr = sp.er * (1. - sp.h)/sp.qb - sp.h*(1.-sp.er)/sp.qh; // eta r in the paper
    double ynrss = 0.;
    if (!mInputRef.isEmpty())
        ynrss = mInputRef.C / (sp.kyr*mRE*(1.-sp.h)) * ((1.-sp.er)/mInputRef.CN() + cr); // Yr steady state N

    double oss = sp.h*total_in.C / (sp.ko*mRE); // O steady state C
    double onss = sp.h*total_in.C / (sp.qh*sp.ko*mRE); // O steady state N

    double al = sp.h*(sp.kyl*mRE* mYL.C - mInputLab.C) / ((sp.ko-sp.kyl)*mRE);
    double ar = sp.h*(sp.kyr*mRE* mYR.C - mInputRef.C) / ((sp.ko-sp.kyr)*mRE);

    // update of state variables
    // precalculations
    double lfactor = exp(-sp.kyl*mRE*t);
    double rfactor = exp(-sp.kyr*mRE*t);
    // young labile pool
    CNPool yl=mYL;
    mYL.C = ylss + (yl.C-ylss)*lfactor;
    mYL.N = ynlss + (yl.N-ynlss-cl/(sp.el-sp.h)*(yl.C-ylss))*exp(-sp.kyl*mRE*(1.-sp.h)*t/(1.-sp.el))+cl/(sp.el-sp.h)*(yl.C-ylss)*lfactor;
    // young ref. pool
    CNPool yr=mYR;
    mYR.C = yrss + (yr.C-yrss)*rfactor;
    mYR.N = ynrss + (yr.N-ynrss-cr/(sp.er-sp.h)*(yr.C-yrss))*exp(-sp.kyr*mRE*(1.-sp.h)*t/(1.-sp.er))+cr/(sp.er-sp.h)*(yr.C-yrss)*rfactor;
    // SOM pool (old)
    CNPool o = mSOM;
    mSOM.C = oss + (o.C -oss - al - ar)*exp(-sp.ko*mRE*t) + al*lfactor + ar*rfactor;
    mSOM.N = onss + (o.N - onss -(al+ar)/sp.qh)*exp(-sp.ko*mRE*t) + al/sp.qh * lfactor + ar/sp.qh * rfactor;

    // calculate plant available nitrogen
    double nav = sp.kyl*mRE*(1.-sp.h)/(1.-sp.el) * (mYL.N - sp.el*mYL.C/sp.qb); // N from labile...
    nav += sp.kyr*mRE*(1-sp.h)/(1.-sp.er)* (mYR.N - sp.er*mYR.C/sp.qb); // + N from refractory...
    nav += sp.ko*mRE*mSOM.N*(1.-sp.leaching); // + N from SOM pool (reduced by leaching (leaching modeled only from slow SOM Pool))
    mAvailableNitrogen = nav * 1000.; // from t/ha -> kg/ha

    // stedy state for n-available
    //    double navss = sp.kyl*mRE*(1.-sp.h)/(1.-sp.el)*(ynlss-sp.el*ylss/sp.qb); // available nitrogen (steady state)
    //    navss += sp.kyr*mRE*(1.-sp.h)/(1.-sp.er)*(ynrss - sp.er*yrss/sp.qb);
    //    navss += sp.ko*mRE*onss*(1.-sp.leaching);

}

QList<QVariant> Soil::debugList()
{
    QList<QVariant> list;
    // (1) inputs of the year
    list << mInputLab.C << mInputLab.N << mInputRef.C << mInputRef.N << mRE;
    // (2) states
    list << mYL.C << mYL.N << mYR.C << mYR.N << mSOM.C << mSOM.N;
    // (3) nav
    list << mAvailableNitrogen;
    return list;
}







