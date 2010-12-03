#include "soil.h"
#include "exception.h"
/** @class Soil provides an implementation of the ICBM/2N soil carbon and nitrogen dynamics model.
  The ICBM/2N model was developed by Kätterer and Andren (2001) and used by others (e.g. Xenakis et al, 2008).
  See http://iland.boku.ac.at/soil+C+and+N+cycling for a model overview and the rationale of the model choice.

  */


// species specific soil paramters
//struct SoilParameters
//{
//    SoilParameters(): kyl(0.15), kyr(0.0807), ksw(0.015), cnFoliage(75.), cnFineroot(40.), cnWood(300.), halfLife(15.) {}
//    double kyl; // litter decomposition rate
//    double kyr; // downed woody debris (dwd) decomposition rate
//    double ksw; // standing woody debris (swd) decomposition rate
//    double cnFoliage; //  C/N foliage litter
//    double cnFineroot; // C/N ratio fine root
//    double cnWood; // C/N Wood: used for brances, stem and coarse root
//    double halfLife; // half-life period of the given species (for calculation of SWD->DWD transition)
//};


// site-specific parameters
// i.e. parameters that need to be specified in the environment file
// note that leaching is not actually influencing soil dynamics but reduces availability of N to plants by assuming that some N
// (proportional to its mineralization in the mineral soil horizon) is leached
// see separate wiki-page (http://iland.boku.ac.at/soil+parametrization+and+initialization)
// and R-script on parameter estimation and initialization
struct SoilParams {
    // ICBM/2N parameters
    SoilParams(): kyl(0.15), kyr(0.0807), ko(0.02), h(0.3), qb(5.), qh(25.), leaching(0.15), el(0.0577), er(0.073) {}
    double kyl; ///< litter decomposition rate
    double kyr; ///< downed woody debris (dwd) decomposition rate
    double ko; ///< decomposition rate for soil organic matter (i.e. the "old" pool sensu ICBM)
    double h; ///< humification rate
    double qb; ///< C/N ratio of soil microbes
    double qh; ///< C/N ratio of SOM
    double leaching; ///< how many percent of the mineralized nitrogen in O is not available for plants but is leached
    double el; ///< microbal efficiency in the labile pool, auxiliary parameter (see parameterization example)
    double er; ///< microbal efficiency in the refractory pool, auxiliary parameter (see parameterization example)
} soilpar;

//    ko=0.02 # decomposition rate for soil organic matter (i.e. the "old" pool sensu ICBM)
//    h=0.3 # humification rate
//    qh=25 # C/N ratio of SOM
//    leaching=0.15 # how many percent of the mineralized nitrogen in O is not available for plants but is leached
//    el=0.0577 # microbal efficiency in the labile pool, auxiliary parameter (see parameterization example)
//    er=0.0730 # microbal efficiency in the refractory pool, auxiliary parameter (see parameterization example)

Soil::Soil()
{
    mRE = 0.;
    mAvailableNitrogen = 0.;
}

void Soil::setSoilInput(const CNPool &labile_input_kg_ha, const CNPool &refractory_input_kg_ha)
{
    mInputLab = labile_input_kg_ha * 0.001; // transfer from kg/ha -> tons/ha
    mInputRef = refractory_input_kg_ha * 0.001;
}

/// Main calculation function
/// must be called after snag dyanmics (i.e. to ensure input fluxes are available)
void Soil::calculateYear()
{
    SoilParams &sp = soilpar;
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



