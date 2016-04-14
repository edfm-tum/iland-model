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
    //mGrid.setup(GlobalSettings::instance()->model()->grid()->metricRect(), GlobalSettings::instance()->model()->grid()->cellsize());
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();
    // mask out out-of-project areas
    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();
    for (int i=0; i<lif_grid->count(); ++i) {
        SaplingCell *s = cell(lif_grid->indexOf(i), false); // false: retrieve also invalid cells
        if (s) {
            if (!hg->valueAtIndex(lif_grid->index5(i)).isValid())
                s->state = SaplingCell::CellInvalid;
            else
                s->state = SaplingCell::CellFree;
        }

    }

}

void Saplings::establishment(const ResourceUnit *ru)
{
    HeightGrid *height_grid = GlobalSettings::instance()->model()->heightGrid();
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();

    QPoint imap = ru->cornerPointOffset(); // offset on LIF/saplings grid
    QPoint iseedmap = QPoint(imap.x()/10, imap.y()/10); // seed-map has 20m resolution, LIF 2m -> factor 10

    for (QList<ResourceUnitSpecies*>::const_iterator i=ru->ruSpecies().constBegin(); i!=ru->ruSpecies().constEnd(); ++i)
        (*i)->saplingStat().clearStatistics();

    double lif_corr[cPxPerHectare];
    for (int i=0;i<cPxPerHectare;++i)
        lif_corr[i]=-1.;

    int species_idx = irandom(0, ru->ruSpecies().size()-1);
    for (int s_idx = 0; s_idx<ru->ruSpecies().size(); ++s_idx) {

        // start from a random species (and cycle through the available species)
        species_idx = ++species_idx % ru->ruSpecies().size();

        ResourceUnitSpecies *rus = ru->ruSpecies()[species_idx];
        // check if there are seeds of the given species on the resource unit
        float seeds = 0.f;
        Grid<float> &seedmap =  const_cast<Grid<float>& >(rus->species()->seedDispersal()->seedMap());
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
        SaplingCell *sap_cells = ru->saplingCellArray();
        SaplingCell *s;
        int isc = 0; // index on 2m cell
        for (int iy=0; iy<cPxPerRU; ++iy) {
            //s = mGrid.ptr(imap.x(), imap.y()+iy); // ptr to the row
            s = &sap_cells[iy*cPxPerRU]; // pointer to a row
            isc = lif_grid->index(imap.x(), imap.y()+iy);

            for (int ix=0;ix<cPxPerRU; ++ix, ++s, ++isc) {
                if (s->state == SaplingCell::CellFree) {
                    // is a sapling of the current species already on the pixel?
                    // * test for sapling height already in cell state
                    // * test for grass-cover already in cell state
                    SaplingTree *stree=0;
                    SaplingTree *slot=s->saplings;
                    for (int i=0;i<NSAPCELLS;++i, ++slot) {
                        if (!stree && !slot->is_occupied())
                            stree=slot;
                        if (slot->species_index == species_idx) {
                            stree=0;
                            break;
                        }
                    }

                    if (stree) {
                        // grass cover?
                        float seed_map_value = seedmap[lif_grid->index10(isc)];
                        if (seed_map_value==0.f)
                            continue;
                        const HeightGridValue &hgv = (*height_grid)[lif_grid->index5(isc)];
                        float lif_value = (*lif_grid)[isc];

                        double &lif_corrected = lif_corr[iy*cPxPerRU+ix];
                        // calculate the LIFcorrected only once per pixel
                        if (lif_corrected<0.)
                            lif_corrected = rus->species()->speciesSet()->LRIcorrection(lif_value, 4. / hgv.height);

                        // check for the combination of seed availability and light on the forest floor
                        if (drandom() < seed_map_value*lif_corrected*abiotic_env ) {
                            // ok, lets add a sapling at the given position (age is incremented later)
                            stree->setSapling(0.05f, 0, species_idx);
                            s->checkState();
                            rus->saplingStat().mAdded++;

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

    QPoint imap = ru->cornerPointOffset();
    bool need_check=false;
    SaplingCell *sap_cells = ru->saplingCellArray();
    for (int iy=0; iy<cPxPerRU; ++iy) {
        //SaplingCell *s = mGrid.ptr(imap.x(), imap.y()+iy); // ptr to the row
        SaplingCell *s = &sap_cells[iy*cPxPerRU]; // ptr to row
        int isc = lif_grid->index(imap.x(), imap.y()+iy);

        for (int ix=0;ix<cPxPerRU; ++ix, ++s, ++isc) {
            if (s->state != SaplingCell::CellInvalid) {
                need_check=false;
                for (int i=0;i<NSAPCELLS;++i) {
                    if (s->saplings[i].is_occupied()) {
                        // growth of this sapling tree
                        const HeightGridValue &hgv = (*height_grid)[height_grid->index5(isc)];
                        float lif_value = (*lif_grid)[isc];

                        need_check |= growSapling(ru, *s, s->saplings[i], isc, hgv.height, lif_value);
                    }
                }
                if (need_check)
                    s->checkState();
            }
        }
    }




    // store statistics on saplings/regeneration
    for (QList<ResourceUnitSpecies*>::const_iterator i=ru->ruSpecies().constBegin(); i!=ru->ruSpecies().constEnd(); ++i) {
        (*i)->saplingStat().calculate((*i)->species(), const_cast<ResourceUnit*>(ru));
        (*i)->statistics().add(&((*i)->saplingStat()));
    }
}

SaplingCell *Saplings::cell(QPoint lif_coords, bool only_valid, ResourceUnit **rRUPtr)
{
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();

    // in this case, getting the actual cell is quite cumbersome: first, retrieve the resource unit, then the
    // cell based on the offset of the given coordiantes relative to the corner of the resource unit.
    ResourceUnit *ru = GlobalSettings::instance()->model()->ru(lif_grid->cellCenterPoint(lif_coords));
    if (rRUPtr)
        *rRUPtr = ru;

    if (ru) {
        QPoint local_coords = lif_coords - ru->cornerPointOffset();
        int idx = local_coords.y() * cPxPerRU + local_coords.x();
        DBGMODE( if (idx<0 || idx>=cPxPerHectare)
                 qDebug("invalid coords in Saplings::cell");
                    );
        SaplingCell *s=&ru->saplingCellArray()[idx];
        if (s && (!only_valid || s->state!=SaplingCell::CellInvalid))
            return s;
    }
    return 0;
}

void Saplings::clearSaplings(const QRectF &rectangle, const bool remove_biomass)
{
    GridRunner<float> runner(GlobalSettings::instance()->model()->grid(), rectangle);
    ResourceUnit *ru;
    while (runner.next()) {
        SaplingCell *s = cell(runner.currentIndex(), true, &ru);
        if (s) {

            for (int i=0;i<NSAPCELLS;++i)
                if (s->saplings[i].is_occupied()) {
                    if (remove_biomass) {
                        ResourceUnitSpecies *rus = ru->resourceUnitSpecies(s->saplings[i].species_index);
                        if (!rus && !rus->species()) {
                            qDebug() << "Saplings::clearSaplings(): invalid resource unit!!!";
                            return;
                        }
                        rus->saplingStat().addCarbonOfDeadSapling( s->saplings[i].height / rus->species()->saplingGrowthParameters().hdSapling * 100.f );
                    }
                    s->saplings[i].clear();
                }
            s->checkState();

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

bool Saplings::growSapling(const ResourceUnit *ru, SaplingCell &scell, SaplingTree &tree, int isc, float dom_height, float lif_value)
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

    rus->calculate(true); // calculate the 3pg module (this is done only if that did not happen up to now); true: call comes from regeneration
    double f_env_yr = rus->prod3PG().fEnvYear();

    double delta_h_factor = f_env_yr * lr; // relative growth

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
            rus->saplingStat().addCarbonOfDeadSapling( tree.height / species->saplingGrowthParameters().hdSapling * 100.f );
            tree.clear();
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

            bigtree.setPosition(GlobalSettings::instance()->model()->grid()->indexOf(isc));
            // add variation: add +/-10% to dbh and *independently* to height.
            bigtree.setDbh(static_cast<float>(dbh * nrandom(1. - mRecruitmentVariation, 1. + mRecruitmentVariation)));
            bigtree.setHeight(static_cast<float>(tree.height * nrandom(1. - mRecruitmentVariation, 1. + mRecruitmentVariation)));
            bigtree.setSpecies( const_cast<Species*>(species) );
            bigtree.setAge(tree.age,tree.height);
            bigtree.setRU(const_cast<ResourceUnit*>(ru));
            bigtree.setup();
            const Tree *t = &bigtree;
            const_cast<ResourceUnitSpecies*>(rus)->statistics().add(t, 0); // count the newly created trees already in the stats
        }
        // clear all regeneration from this pixel (including this tree)
        tree.clear(); // clear this tree (no carbon flow to the ground)
        for (int i=0;i<NSAPCELLS;++i) {
            if (scell.saplings[i].is_occupied()) {
                // add carbon to the ground
                rus->saplingStat().addCarbonOfDeadSapling( scell.saplings[i].height / species->saplingGrowthParameters().hdSapling * 100.f );
                scell.saplings[i].clear();
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
    mAdded=0;

}

void SaplingStat::calculate(const Species *species, ResourceUnit *ru)
{
    if (mLiving) {
        mAvgHeight /= double(mLiving);
        mAvgAge /= double(mLiving);
        mAvgDeltaHPot /= double(mLiving);
        mAvgHRealized /= double(mLiving);
    }

    // calculate carbon balance
    CNPair old_state = mCarbonLiving;
    mCarbonLiving.clear();

    CNPair dead_wood, dead_fine; // pools for mortality
    // average dbh
    if (mLiving>0) {
        // calculate the avg dbh and number of stems
        double avg_dbh = mAvgHeight / species->saplingGrowthParameters().hdSapling * 100.;
        double n = mLiving * species->saplingGrowthParameters().representedStemNumber( avg_dbh );
        // woody parts: stem, branchse and coarse roots
        double woody_bm = species->biomassWoody(avg_dbh) + species->biomassBranch(avg_dbh) + species->biomassRoot(avg_dbh);
        double foliage = species->biomassFoliage(avg_dbh);
        double fineroot = foliage*species->finerootFoliageRatio();

        mCarbonLiving.addBiomass( woody_bm*n, species->cnWood()  );
        mCarbonLiving.addBiomass( foliage*n, species->cnFoliage()  );
        mCarbonLiving.addBiomass( fineroot*n, species->cnFineroot()  );

        DBGMODE(
        if (isnan(mCarbonLiving.C))
            qDebug("carbon NaN in SaplingStat::calculate (living trees).");
                );

        // turnover
        if (ru->snag())
            ru->snag()->addTurnoverLitter(species, foliage*species->turnoverLeaf(), fineroot*species->turnoverRoot());

        // calculate the "mortality from competition", i.e. carbon that stems from reduction of stem numbers
        // from Reinekes formula.
        //
        if (avg_dbh>1.) {
            double avg_dbh_before = (mAvgHeight - mAvgHRealized) / species->saplingGrowthParameters().hdSapling * 100.;
            double n_before = mLiving * species->saplingGrowthParameters().representedStemNumber( qMax(1.,avg_dbh_before) );
            if (n<n_before) {
                dead_wood.addBiomass( woody_bm * (n_before-n), species->cnWood() );
                dead_fine.addBiomass( foliage * (n_before-n), species->cnFoliage()  );
                dead_fine.addBiomass( fineroot * (n_before-n), species->cnFineroot()  );
                DBGMODE(
                if (isnan(dead_fine.C))
                    qDebug("carbon NaN in SaplingStat::calculate (self thinning).");
                        );

            }
        }

    }
    if (mDied) {
        double avg_dbh_dead = mSumDbhDied / double(mDied);
        double n = mDied * species->saplingGrowthParameters().representedStemNumber( avg_dbh_dead );
        // woody parts: stem, branchse and coarse roots

        dead_wood.addBiomass( ( species->biomassWoody(avg_dbh_dead) + species->biomassBranch(avg_dbh_dead) + species->biomassRoot(avg_dbh_dead)) * n, species->cnWood()  );
        double foliage = species->biomassFoliage(avg_dbh_dead)*n;

        dead_fine.addBiomass( foliage, species->cnFoliage()  );
        dead_fine.addBiomass( foliage*species->finerootFoliageRatio(), species->cnFineroot()  );
        DBGMODE(
        if (isnan(dead_fine.C))
            qDebug("carbon NaN in SaplingStat::calculate (died trees).");
                );

    }
    if (!dead_wood.isEmpty() || !dead_fine.isEmpty())
        if (ru->snag())
            ru->snag()->addToSoil(species, dead_wood, dead_fine);

    // calculate net growth:
    // delta of stocks
    mCarbonGain = mCarbonLiving + dead_fine + dead_wood - old_state;
    if (mCarbonGain.C < 0)
        mCarbonGain.clear();


    GlobalSettings::instance()->systemStatistics()->saplingCount+=mLiving;
    GlobalSettings::instance()->systemStatistics()->newSaplings+=mAdded;

}

double SaplingStat::livingStemNumber(const Species *species, double &rAvgDbh, double &rAvgHeight, double &rAvgAge) const
{
     rAvgHeight = averageHeight();
     rAvgDbh = rAvgHeight / species->saplingGrowthParameters().hdSapling * 100.f;
     rAvgAge = averageAge();
     double n= species->saplingGrowthParameters().representedStemNumber(rAvgDbh);
     return n;
// *** old code (sapling.cpp) ***
//    double total = 0.;
//    double dbh_sum = 0.;
//    double h_sum = 0.;
//    double age_sum = 0.;
//    const SaplingGrowthParameters &p = mRUS->species()->saplingGrowthParameters();
//    for (QVector<SaplingTreeOld>::const_iterator it = mSaplingTrees.constBegin(); it!=mSaplingTrees.constEnd(); ++it) {
//        float dbh = it->height / p.hdSapling * 100.f;
//        if (dbh<1.) // minimum size: 1cm
//            continue;
//        double n = p.representedStemNumber(dbh); // one cohort on the pixel represents that number of trees
//        dbh_sum += n*dbh;
//        h_sum += n*it->height;
//        age_sum += n*it->age.age;
//        total += n;
//    }
//    if (total>0.) {
//        dbh_sum /= total;
//        h_sum /= total;
//        age_sum /= total;
//    }
//    rAvgDbh = dbh_sum;
//    rAvgHeight = h_sum;
//    rAvgAge = age_sum;
//    return total;
}
