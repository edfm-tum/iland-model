#include "svdout.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"
#include "climate.h"
#include "svdstate.h"

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

    mSpeciesList << "piab" << "abal" << "lade" << "pisy" << "fasy" << "quro" << "acps" << "bepe";
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
    setName("Forest states", "svdstate");
    setDescription("Forest state (for SVD)");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::id()
              << OutputColumn("stateId", "unique state Id within one iLand simulation", OutInteger)
              << OutputColumn("composition", "species composition state", OutString)
              << OutputColumn("structure", "dominant height class", OutDouble)
              << OutputColumn("function", "leaf area index", OutDouble)
              << OutputColumn("previousStateId", "unique state Id that the RU was before the current state", OutInteger)
              << OutputColumn("previousTime", "number of years that the resource unit was in the previous state", OutInteger);

}

void SVDStateOut::exec()
{
    if (!GlobalSettings::instance()->model()->svdStates())
        return;

    SVDStates *svd = GlobalSettings::instance()->model()->svdStates();

    QList<ResourceUnit*>::const_iterator it;
    Model *m = GlobalSettings::instance()->model();
    bool all_states = currentYear()==1;
    for (it=m->ruList().constBegin(); it!=m->ruList().constEnd(); ++it) {
        if ((*it)->id()==-1)
            continue; // do not include if out of project area

        const SVDState &s = svd->state((*it)->svdStateId());
        if (all_states || (*it)->svdStateTime()==1) {
            // write output only at the beginning or when states change
            *this << currentYear() << (*it)->index() << (*it)->id();
            *this << s.Id;
            *this << s.compositionString();
            *this << s.structure;
            *this << s.function;
            *this << (*it)->svdPreviousStateId();
            *this << (*it)->svdPreviousTime();

            writeRow();
        }

    }
}

void SVDStateOut::setup()
{

}
