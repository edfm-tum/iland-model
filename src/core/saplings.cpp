#include "global.h"
#include "saplings.h"

#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "establishment.h"
#include "species.h"
#include "seeddispersal.h"

double Saplings::mRecruitmentVariation = 0.1; // +/- 10%
double Saplings::mBrowsingPressure = 0.;


Saplings::Saplings()
{

}

void Saplings::setup()
{
    mGrid.setup(GlobalSettings::instance()->model()->grid()->metricRect(), GlobalSettings::instance()->model()->grid()->cellsize());

    // mask out out-of-project areas
    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();
    for (int i=0;i<mGrid.count();++i) {
        if (!hg->valueAtIndex(mGrid.index5(i)).isValid())
            mGrid[i].state = SaplingCell::CellInvalid;
        else
            mGrid[i].state = SaplingCell::CellFree;
    }


}

void Saplings::establishment(const ResourceUnit *ru)
{
    Grid<float> &seedmap =  const_cast<Grid<float>& > (ru->ruSpecies().first()->species()->seedDispersal()->seedMap() );
    HeightGrid *height_grid = GlobalSettings::instance()->model()->heightGrid();
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();

    QPoint iseedmap =  seedmap.indexAt(ru->boundingBox().topLeft()) ;
    QPoint imap =  mGrid.indexAt(ru->boundingBox().topLeft());

    int species_idx = irandom(0, ru->ruSpecies().size()-1);
    for (int s_idx = 0; s_idx<ru->ruSpecies().size(); ++s_idx) {

        // start from a random species (and cycle through the available species)
        species_idx = ++species_idx % ru->ruSpecies().size();

        ResourceUnitSpecies *rus = ru->ruSpecies()[species_idx];
        // check if there are seeds of the given species on the resource unit
        float seeds = 0.f;
        for (int iy=0;iy<5;++iy) {
            float *p = seedmap.ptr(iseedmap.x(), iseedmap.y());
            for (int ix=0;ix<5;++ix)
                seeds += *p++;
        }
        // if there are no seeds: no need to do more
        if (seeds==0.f)
            continue;

        // calculate the abiotic environment (TACA)
        rus->establishment().calculateAbioticEnvironment();
        double abiotic_env = rus->establishment().abioticEnvironment();
        if (abiotic_env==0.)
            continue;

        // loop over all 2m cells on this resource unit
        SaplingCell *s;
        int isc = 0; // index on 2m cell
        for (int iy=0; iy<cPxPerRU; ++iy) {
            s = mGrid.ptr(imap.x(), imap.y()+iy); // ptr to the row
            isc = mGrid.index(imap.x(), imap.y()+iy);

            for (int ix=0;ix<cPxPerRU; ++ix, ++s, ++isc, ++mTested) {
                if (s->state == SaplingCell::CellFree) {
                    bool viable = true;
                    // is a sapling of the current species already on the pixel?
                    // * test for sapling height already in cell state
                    // * test for grass-cover already in cell state
                    int i_occupied = -1;
                    for (int i=0;i<NSAPCELLS;++i) {
                        if (!s->saplings[i].is_occupied() && i_occupied<0)
                            i_occupied=i;
                        if (s->saplings[i].species_index == species_idx) {
                            viable = false;
                        }
                    }

                    if (viable && i_occupied>=0) {
                        // grass cover?
                        DBG_IF(i_occupied<0, "establishment", "invalid value i_occupied<0");
                        float seed_map_value = seedmap[seedmap.index10(isc)];
                        if (seed_map_value==0.f)
                            continue;
                        const HeightGridValue &hgv = (*height_grid)[height_grid->index5(isc)];
                        float lif_value = (*lif_grid)[isc];
                        double lif_corrected = rus->species()->speciesSet()->LRIcorrection(lif_value, 4. / hgv.height);
                        // check for the combination of seed availability and light on the forest floor
                         if (drandom() < seed_map_value*lif_corrected*abiotic_env ) {
                             // ok, lets add a sapling at the given position
                             s->saplings[i_occupied].setSapling(0.05f, 1, species_idx);
                             s->checkState();
                             mAdded++;

                         }

                    }

                }
            }
        }

    }

}

void Saplings::saplingGrowth(const ResourceUnit *ru)
{
    HeightGrid *height_grid = GlobalSettings::instance()->model()->heightGrid();
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();

    for (QList<ResourceUnitSpecies*>::const_iterator i=ru->ruSpecies().constBegin(); i!=ru->ruSpecies().constEnd(); ++i)
        (*i)->saplingStat().clearStatistics();

    QPoint imap =  mGrid.indexAt(ru->boundingBox().topLeft());
    bool need_check=false;
    for (int iy=0; iy<cPxPerRU; ++iy) {
        SaplingCell *s = mGrid.ptr(imap.x(), imap.y()+iy); // ptr to the row
        int isc = mGrid.index(imap.x(), imap.y()+iy);

        for (int ix=0;ix<cPxPerRU; ++ix, ++s, ++isc) {
            if (s->state != SaplingCell::CellInvalid) {
                need_check=false;
                for (int i=0;i<NSAPCELLS;++i) {
                    if (s->saplings[i].is_occupied()) {
                        // growth of this sapling tree
                        const HeightGridValue &hgv = (*height_grid)[height_grid->index5(isc)];
                        float lif_value = (*lif_grid)[isc];

                        need_check |= growSapling(ru, s->saplings[i], isc, hgv.height, lif_value);
                    }
                }
                if (need_check)
                    s->checkState();
            }
        }
    }

}

void Saplings::updateBrowsingPressure()
{
    if (GlobalSettings::instance()->settings().valueBool("model.settings.browsing.enabled"))
        Saplings::mBrowsingPressure = GlobalSettings::instance()->settings().valueDouble("model.settings.browsing.browsingPressure");
    else
        Saplings::mBrowsingPressure = 0.;
}

bool Saplings::growSapling(const ResourceUnit *ru, SaplingTree &tree, int isc, float dom_height, float lif_value)
{
    ResourceUnitSpecies *rus = const_cast<ResourceUnitSpecies*>(ru->ruSpecies()[tree.species_index]);
    const Species *species = rus->species();

    // (1) calculate height growth potential for the tree (uses linerization of expressions...)
    double h_pot = species->saplingGrowthParameters().heightGrowthPotential.calculate(tree.height);
    double delta_h_pot = h_pot - tree.height;

    // (2) reduce height growth potential with species growth response f_env_yr and with light state (i.e. LIF-value) of home-pixel.
    if (dom_height==0.f)
        throw IException(QString("growSapling: height grid at %1/%2 has value 0").arg(isc));

    double rel_height = tree.height / dom_height;

    double lif_corrected = species->speciesSet()->LRIcorrection(lif_value, rel_height); // correction based on height

    double lr = species->lightResponse(lif_corrected); // species specific light response (LUI, light utilization index)

    double delta_h_factor = rus->prod3PG().fEnvYear() * lr; // relative growth

    if (h_pot<0. || delta_h_pot<0. || lif_corrected<0. || lif_corrected>1. || delta_h_factor<0. || delta_h_factor>1. )
        qDebug() << "invalid values in Sapling::growSapling";

    // check browsing
    if (mBrowsingPressure>0. && tree.height<=2.f) {
        double p = rus->species()->saplingGrowthParameters().browsingProbability;
        // calculate modifed annual browsing probability via odds-ratios
        // odds = p/(1-p) -> odds_mod = odds * browsingPressure -> p_mod = odds_mod /( 1 + odds_mod) === p*pressure/(1-p+p*pressure)
        double p_browse = p*mBrowsingPressure / (1. - p + p*mBrowsingPressure);
        if (drandom() < p_browse) {
            delta_h_factor = 0.;
        }
    }

    // check mortality of saplings
    if (delta_h_factor < species->saplingGrowthParameters().stressThreshold) {
        tree.stress_years++;
        if (tree.stress_years > species->saplingGrowthParameters().maxStressYears) {
            // sapling dies...
            tree.clear();
            rus->saplingStat().addCarbonOfDeadSapling( tree.height / species->saplingGrowthParameters().hdSapling * 100.f );
            return true; // need cleanup
        }
    } else {
        tree.stress_years=0; // reset stress counter
    }
    DBG_IF(delta_h_pot*delta_h_factor < 0.f || delta_h_pot*delta_h_factor > 2., "Sapling::growSapling", "inplausible height growth.");

    // grow
    tree.height += delta_h_pot * delta_h_factor;
    tree.age++; // increase age of sapling by 1

    // recruitment?
    if (tree.height > 4.f) {
        rus->saplingStat().mRecruited++;

        float dbh = tree.height / species->saplingGrowthParameters().hdSapling * 100.f;
        // the number of trees to create (result is in trees per pixel)
        double n_trees = species->saplingGrowthParameters().representedStemNumber(dbh);
        int to_establish = static_cast<int>( n_trees );

        // if n_trees is not an integer, choose randomly if we should add a tree.
        // e.g.: n_trees = 2.3 -> add 2 trees with 70% probability, and add 3 trees with p=30%.
        if (drandom() < (n_trees-to_establish) || to_establish==0)
            to_establish++;

        // add a new tree
        for (int i=0;i<to_establish;i++) {
            Tree &bigtree = const_cast<ResourceUnit*>(ru)->newTree();

            bigtree.setPosition(mGrid.indexOf(isc));
            // add variation: add +/-10% to dbh and *independently* to height.
            bigtree.setDbh(dbh * nrandom(1. - mRecruitmentVariation, 1. + mRecruitmentVariation));
            bigtree.setHeight(tree.height * nrandom(1. - mRecruitmentVariation, 1. + mRecruitmentVariation));
            bigtree.setSpecies( const_cast<Species*>(species) );
            bigtree.setAge(tree.age,tree.height);
            bigtree.setRU(const_cast<ResourceUnit*>(ru));
            bigtree.setup();
            const Tree *t = &bigtree;
            const_cast<ResourceUnitSpecies*>(rus)->statistics().add(t, 0); // count the newly created trees already in the stats
        }
        // clear all regeneration from this pixel (including this tree)
        tree.clear(); // clear this tree (no carbon flow to the ground)
        SaplingCell &s=mGrid[isc];
        for (int i=0;i<NSAPCELLS;++i) {
            if (s.saplings[i].is_occupied()) {
                // add carbon to the ground
                rus->saplingStat().addCarbonOfDeadSapling( s.saplings[i].height / species->saplingGrowthParameters().hdSapling * 100.f );
                s.saplings[i].clear();
            }
        }
        return true; // need cleanup
    }
    // book keeping (only for survivors) for the sapling of the resource unit / species
    SaplingStat &ss = rus->saplingStat();
    ss.mLiving++;
    ss.mAvgHeight+=tree.height;
    ss.mAvgAge+=tree.age;
    ss.mAvgDeltaHPot+=delta_h_pot;
    ss.mAvgHRealized += delta_h_pot * delta_h_factor;
    return false;
}

void SaplingStat::clearStatistics()
{
    mRecruited=mDied=mLiving=0;
    mSumDbhDied=0.;
    mAvgHeight=0.;
    mAvgAge=0.;
    mAvgDeltaHPot=mAvgHRealized=0.;

}
