#include "svdout.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"
#include "climate.h"
#include "svdstate.h"
#include "debugtimer.h"
#include "soil.h"

SVDGPPOut::SVDGPPOut()
{
    setName("Compact GPP potential per RU", "svdgpp");
    setDescription("GPP potential (as conditioned by climate/site) per species and m2.");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("climate_year", "year of the climate table", OutInteger)
              << OutputColumn("gpp_s1", "gpp of species 1", OutDouble)
              << OutputColumn("gpp_s2", "gpp of species 2", OutDouble)
              << OutputColumn("gpp_s3", "gpp of species 3", OutDouble)
              << OutputColumn("gpp_s4", "gpp of species 4", OutDouble)
              << OutputColumn("gpp_s5", "gpp of species 5", OutDouble)
              << OutputColumn("gpp_s6", "gpp of species 6", OutDouble)
              << OutputColumn("gpp_s7", "gpp of species 7", OutDouble)
              << OutputColumn("gpp_s8", "gpp of species 8", OutDouble)
              << OutputColumn("gpp_s9", "gpp of species 9", OutDouble)
              << OutputColumn("gpp_s10", "gpp of species 10", OutDouble);

    mSpeciesList << "piab" << "abal" << "lade" << "pisy" << "fasy" << "quro" << "bepe" << "quil" << "pipi" << "piha";
    for (int i=0;i<10;++i) {
        mSpeciesIndex[i]=-1;
    }



}

void SVDGPPOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    if (mSpeciesIndex[0]==-1)
        for (int i=0;i<mSpeciesList.count();++i) {
            const Species *s=m->speciesSet()->species(mSpeciesList[i]);
            if (!s) throw IException(QString("Setup of SVDGPPOut: species '%1' is not valid/active!").arg(mSpeciesList[i]));
            mSpeciesIndex[i] = s->index();
        }

    QList<ResourceUnit*>::const_iterator it;
    for (it=m->ruList().constBegin(); it!=m->ruList().constEnd(); ++it) {
        const ResourceUnit *ru = *it;
        if (ru->id()==-1)
            continue; // do not include if out of project area
        *this << currentYear() << ru->index() << ru->id();
        // climate year:
        *this << ru->climate()->climateDataYear();
        for (int i=0;i<10;++i) {
            *this << (mSpeciesIndex[i]>-1 ? ru->resourceUnitSpecies(mSpeciesIndex[i])->prod3PG().GPPperArea() : 0.);
        }
        writeRow();

    }
}

void SVDGPPOut::setup()
{

}

/*  ***********************************************************************  */
/*  **********************  SVD State output ******************************  */
/*  ***********************************************************************  */

SVDStateOut::SVDStateOut()
{
    setName("Forest state transitions", "svdstate");
    setDescription("Forest state (for SVD). The output contains fixed columns (see below) " \
                   "and adds two extra columns for every active tree species. Those species columns " \
                   "hold the species share [0..1] for the local and the mid-range-neighborhood. Former have " \
                   "'l_' and latter 'm_' as prefix (e.g. 'l_piab', 'm_piab'). Note that the sum of all shares is <=1, but " \
                   "can be lower than 1. See also the 'svduniquestate' output.");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("stateId", "unique state Id within one iLand simulation", OutInteger)
              << OutputColumn("previousStateId", "unique state Id that the RU was before the current state", OutInteger)
              << OutputColumn("previousTime", "number of years that the resource unit was in the previous state", OutInteger);

}


// neighborhood analysis per resource unit
static int svd_evals=0;
void nc_calculateSVDNeighbors(ResourceUnit *unit)
{
    // evaluate immediately after state change (which sets the time to 1),
    // and at least every 10 yrs.
    if (GlobalSettings::instance()->currentYear()==0 ||  unit->svdStateTime() % 10 == 1)  {
        GlobalSettings::instance()->model()->svdStates()->evaluateNeighborhood(unit);
        svd_evals++;
    }
}



void SVDStateOut::exec()
{
    if (!GlobalSettings::instance()->model()->svdStates()) {
        qWarning() << "'svdstate' output enabled, but not the SVD state subsystem ('model.settings.svdStates.enabled'). No output written.";
        return;
    }

    SVDStates *svd = GlobalSettings::instance()->model()->svdStates();
    // run the analysis of species composition in the neighborhood in parallel
    { DebugTimer dt("SVDStateNeighbors");
    int old_val=svd_evals;
    GlobalSettings::instance()->model()->executePerResourceUnit(nc_calculateSVDNeighbors);
    qDebug() << "SVDStateOut: evaluate neighbors. total count:" << svd_evals-old_val;
    }

    bool start_year = GlobalSettings::instance()->currentYear() == 0;

    QList<ResourceUnit*>::const_iterator it;
    Model *m = GlobalSettings::instance()->model();
    for (it=m->ruList().constBegin(); it!=m->ruList().constEnd(); ++it) {
        if ((*it)->id()==-1)
            continue; // do not include if out of project area

        const SVDState &s = svd->state((*it)->svdStateId());
        if ( (*it)->svdStateTime() % 10==1 || start_year) {
            // write output only at the beginning or when states change
            *this << currentYear() << (*it)->index() << (*it)->id();
            *this << s.Id;
            if ((*it)->svdStateTime()==1) {
                // a state change!
                *this << (*it)->svdPreviousStateId();
                *this << (*it)->svdPreviousTime();
            } else {
                // stay in the state:
                *this << s.Id; // use the current id as 'previous state id'
                *this << (*it)->svdStateTime(); // use the current residence time
            }
            // the values for the neighborhood(s): pairs for local/mid-range neighbors:
            QVector<float> &local = *(*it)->mSVDState.localComposition;
            QVector<float> &mid = *(*it)->mSVDState.midDistanceComposition;
            for (int s=0;s<local.count();++s) {
                *this << local[s] << mid[s];
            }

            writeRow();
        }

    }
}

void SVDStateOut::setup()
{
    // clear extra columns: everything after previousTime
    clearColumnsAfter("previousTime");

    // add columns for all active species:
    int n=0;
    for (QList<Species*>::const_iterator i = GlobalSettings::instance()->model()->speciesSet()->activeSpecies().constBegin();
         i!= GlobalSettings::instance()->model()->speciesSet()->activeSpecies().constEnd(); ++i) {
        columns() << OutputColumn(QString("l_%1").arg((*i)->id()), QString(), OutDouble);
        columns() << OutputColumn(QString("m_%1").arg((*i)->id()), QString(), OutDouble);
        n++;
    }
    qDebug() << "SVDStateOutput: added extra columns for" << n << "species to the output dynamically.";
}


/*  ***********************************************************************  */
/*  ******************  SVD Unique indicator output ***********************  */
/*  ***********************************************************************  */

SVDUniqueStateOut::SVDUniqueStateOut()
{
    setName("Unique forest states", "svduniquestate");
    setDescription("List of forest states for the current simulation (for SVD). Each state is defined by a unique numerical Id ('stateId') " \
                   "which is used as a key in the 'svdstate' output. " );
    columns() << OutputColumn("stateId", "unique state Id within one iLand simulation", OutInteger)
              << OutputColumn("composition", "species composition state", OutString)
              << OutputColumn("structure", "dominant height class (class index) ", OutInteger)
              << OutputColumn("functioning", "leaf area index (class index)", OutInteger)
              << OutputColumn("description", "Verbose description of the state", OutString);

}

void SVDUniqueStateOut::exec()
{

    if (!GlobalSettings::instance()->model()->svdStates()) {
        qWarning() << "'svduniquestate' output enabled, but not the SVD state subsystem ('model.settings.svdStates.enabled'). No output written.";
        return;
    }

    if (!mCondition.isEmpty())
        if (!mCondition.calculate(GlobalSettings::instance()->currentYear()))
            return;

    // clear the table before writing
    truncateTable();

    SVDStates *svd = GlobalSettings::instance()->model()->svdStates();
    for (int i=0;i<svd->count();++i) {

        const SVDState &s = svd->state(i);
        *this << s.Id;
        *this << s.compositionString();
        *this << s.structure;
        *this << s.function;
        *this << s.stateLabel();
        writeRow();
    }

}

void SVDUniqueStateOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);

}


/*  ***********************************************************************  */
/*  *********************  SVD Indicator output ***************************  */
/*  ***********************************************************************  */

// list of available indicators:
static const QStringList svd_indicators = QStringList() << "shannonIndex" << "abovegroundCarbon" << "totalCarbon" <<
                                                     "volume" << "crownCover" << "LAI" << "basalArea" << "stemDensity" << "saplingDensity";


SVDIndicatorOut::SVDIndicatorOut()
{
    setName("SVD forest indicator data", "svdindicator");
    setDescription("Indicator data per resource unit as used by SVD.\n " \
                   "The selection of indicators is triggered by keys in the project file (sub section 'indicators').\n " \
                   "!!! indicators\n\n" \
                   "The following columns are supported:\n\n" \
                   "||__key__|__description__\n" \
                   "shannonIndex|shannon index (exponential) on the RU (based on basal area of trees >4m)\n" \
                   "abovegroundCarbon|living aboveground carbon (tC/ha) on the RU (trees + regen)\n" \
                   "totalCarbon|all C on the RU (tC/ha), including soil, lying and standing deadwood\n" \
                   "volume|tree volume (trees>4m) m3/ha\n" \
                   "crownCover|fraction of crown cover (0..1) (see saveCrownCoverGrid() in SpatialAnalysis - not yet implemented)\n" \
                   "LAI|leaf area index (trees>4m) m2/m2\n" \
                   "basalArea|basal area (trees>4m) m2/ha\n" \
                   "stemDensity|trees per ha (trees>4m) ha-1\n" \
                    "saplingDensity|density of saplings (represented trees>1.3m) ha-1||\n\n" \
                   "!!! species proportions\n" \
                   "A special case is the setting 'speciesProportions': this is a list of species (Ids) separated with a comma or white space. When present, the output will " \
                   " include for each species the relative proportion calculated based on basal area (for trees >4m). \n" \
                   " \n" \
                   "!!! disturbance history\n" \
                   "The setting 'disturbanceHistory' indicates if (value != 0) and how many (value>0, maximum=3) disturbance events should be recorded and added to the " \
                   "output. Each __event__ is defined by three columns. 'tsd_x' is number of years since disturbance, 'type_x' encodes the disturbance " \
                   "agent (see below), and 'addinfo_x' is agent-specific additional information (see below), with 'x' the number of event (1,2,3).\n\n" \
                   "||__value__|__type__|__additional info__\n" \
                   "0|fire|proportion of area burned per ha (0..1) \n" \
                   "1|(spruce) bark beetle|NA\n" \
                   "2|wind|NA \n" \
                   "3|BITE|NA \n" \
                   "4|ABE|NA \n" \
                   "5|base management|NA|| \n\n" \
                   "!!! example \n\n" \
                   "An example for the project file node:\n" \
                   "<indicators>\n<shannonIndex>true</shannonIndex>\n<abovegroundCarbon>false</abovegroundCarbon>\n ... \n" \
                   "<speciesProportions>Pico,Abal</speciesProportions>\n" \
                   "<disturbanceHistory>2 </disturbanceHistory>\n</indicators>\n"
                   ); // //dtFire, dtBarkBeetle, dtWind, dtBite, dtAbe, dtManagement
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("stateId", "current state of the resource unit (see 'svdstate' output)", OutInteger)
              << OutputColumn("time", "number of years the resource unit is already in the state 'stateId' (see 'svdstate' output)", OutInteger);


}

void SVDIndicatorOut::setup()
{
    // clear extra columns:
    clearColumnsAfter("time");

    // use a condition for to control execuation for the current year
    XmlHelper indicators(settings().node(".indicators"));
    // look for all defined indicators in the XML structure
    for (int i=0;i<svd_indicators.size();++i) {
        if (indicators.valueBool(QString(".%1").arg(svd_indicators[i]))) {
            // set active
            mIndicators[i] = true;
            // add to output table
            columns() << OutputColumn(svd_indicators[i], QString(), OutDouble);
        }
    }
    // special case for species proportions
    QString specieslist = indicators.value(".speciesProportions");
    if (!specieslist.isEmpty()) {
        // extract list of species, create columns and store the name for later use
        QStringList species_list = specieslist.split(QRegularExpression("([^\\.\\w]+)"));
        mSpecies.clear();
        for (int i=0;i<species_list.size();++i) {
            mSpecies.push_back( QPair<QString, int>(species_list[i], -1));
            columns() << OutputColumn(QString("prop_%1").arg(species_list[i]), QString(), OutDouble);
        }
        qDebug() << "SVDIndicatorOut: setup relative species proportions for" << species_list.count() << "species.";

    }
    // species case disturbance history
    mNDisturbanceHistory = indicators.valueInt(".disturbanceHistory", 0);
    if (mNDisturbanceHistory > 0) {
        // add columns: for each event we need 3 columns
        for (int i=0;i<mNDisturbanceHistory;++i) {
            // time since disturbance, type, additional info
            columns() << OutputColumn(QString("tsd_%1").arg(i+1), QString(), OutInteger);
            columns() << OutputColumn(QString("type_%1").arg(i+1), QString(), OutInteger);
            columns() << OutputColumn(QString("addinfo_%1").arg(i+1), QString(), OutDouble);
        }
    }

    qDebug() << "SVDIndicatorOut: setup indicators: " << mIndicators.count() << "active. Details: " <<  QString::fromStdString(mIndicators.to_string());

}

double SVDIndicatorOut::calcShannonIndex(const ResourceUnit *ru)
{
    // calculate shannon index from the given data [I did this already for PICUS...]:
    // see also ARANGE project D2.2, 4.2.2
    double total_ba = ru->statistics().basalArea();
    if (total_ba==0.)
        return 0.;

    // loop over each species:
    double shannon = 0.;
    for (QList<ResourceUnitSpecies*>::const_iterator it=ru->ruSpecies().constBegin(); it!=ru->ruSpecies().constEnd();++it){
        double ba = (*it)->statistics().basalArea();
        if (ba>0.)
            shannon += ba/total_ba * log(ba/total_ba);
    }

    // 'true diversity' is the exponent of the shannon index:
    double exp_shannon = exp( -shannon );
    return exp_shannon;

}

double SVDIndicatorOut::calcCrownCover(const ResourceUnit *ru)
{
    return 0.; // TODO: implement
}

double SVDIndicatorOut::calcTotalCarbon(const ResourceUnit *ru)
{
    double total_carbon = ru->statistics().totalCarbon() / 1000.; // aboveground, kg C/ha -> tC/ha
    double area_factor = ru->stockableArea() / cRUArea; // conversion factor from real area to per ha values
    total_carbon +=  ru->snag()->totalCarbon() / 1000. / area_factor; // kgC/RU -> tC/ha
    total_carbon += ru->soil()->totalCarbon(); // t/ha
    return total_carbon;
}

void SVDIndicatorOut::addSpeciesProportions(const ResourceUnit *ru)
{
    if (mSpecies.isEmpty())
        return;

     // do only once:
    if (mSpecies[0].second == -1) {
        for (int i=0;i<mSpecies.size();++i) {
            Species *s = Globals->model()->speciesSet()->species(mSpecies[i].first);
            if (!s)
                throw IException(QString("Setup SVDIndicatorOut: Species '%1' is not available!").arg(mSpecies[i].first));
            mSpecies[i].second = s->index(); // save index for later use
        }
    }

    double total_ba = std::max(ru->statistics().basalArea(), 0.00001);
    for (int i=0;i<mSpecies.size();++i) {
        double prop = ru->ruSpecies()[mSpecies[i].second]->constStatistics().basalArea() / total_ba;
        // add to the output
        *this << prop;
    }
}

void SVDIndicatorOut::addDisturbanceHistory(const ResourceUnit *ru)
{
    if (mNDisturbanceHistory == 0) return;

    for (int i=0;i<mNDisturbanceHistory;++i) {
        if (i < ru->mSVDState.disturbanceEvents->size()) {
            const ResourceUnit::RUSVDState::SVDDisturbanceEvent &e =  (*ru->mSVDState.disturbanceEvents)[i];
            *this << Globals->currentYear() - e.year; // time since disturbance
            *this << e.source; // type of disturbance
            *this << e.info; // some additional information
        } else {
            *this << 0 << 0 << 0; // no data available
        }
    }
}

void SVDIndicatorOut::exec()
{
    if (!GlobalSettings::instance()->model()->svdStates()) {
        qWarning() << "Output SVDIndicatorOut cannot be used, because it requires the 'svdstate' output (and the SVD subsystem ('model.settings.svdStates.enabled')). Output disabled.";

        throw IException("Setup of SVDIndcatorOut: SVD states are required for this output ('model.settings.svdStates.enabled').");
    }


    QList<ResourceUnit*>::const_iterator it;
    Model *m = GlobalSettings::instance()->model();
    for (it=m->ruList().constBegin(); it!=m->ruList().constEnd(); ++it) {
        if ((*it)->id()==-1)
            continue; // do not include if out of project area

        *this << currentYear() << (*it)->index() << (*it)->id();
        *this << (*it)->mSVDState.stateId
              << (*it)->mSVDState.time;

        // process indicators:
        // Note: sequence important: see string list svd_indicators !
        if (mIndicators.test(EshannonIndex))
            *this << calcShannonIndex(*it);
        if (mIndicators.test(EabovegroundCarbon))
            *this << (*it)->statistics().totalCarbon() / 1000.; // trees + regen, t C/ha
        if (mIndicators.test(EtotalCarbon))
            *this << calcTotalCarbon(*it); //
        if (mIndicators.test(Evolume))
            *this << (*it)->statistics().volume();
        if (mIndicators.test(EcrownCover))
            *this << calcCrownCover(*it);
        if (mIndicators.test(ELAI))
            *this << (*it)->statistics().leafAreaIndex(); // LAI trees > 4m
        if (mIndicators.test(EbasalArea))
            *this << (*it)->statistics().basalArea();
        if (mIndicators.test(EstemDensity))
            *this << (*it)->statistics().nStem();
        if (mIndicators.test(EsaplingDensity))
            *this << (*it)->statistics().saplingCount();

        addSpeciesProportions(*it);

        addDisturbanceHistory(*it);

        writeRow();
    }


}



