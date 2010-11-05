#include "sapling.h"
#include "model.h"
#include "species.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "tree.h"

/** @class Sapling
    Sapling stores saplings per species and resource unit and computes sapling growth (before recruitment).
    http://iland.boku.ac.at/sapling+growth+and+competition
    Saplings are established in a separate step (@sa Regeneration). If sapling reach a height of 4m, they are recruited and become "real" iLand-trees.
    Within the regeneration layer, a cohort-approach is applied.

  */

double Sapling::mRecruitmentVariation = 0.1; // +/- 10%

Sapling::Sapling()
{
    mRUS = 0;
    clearStatistics();
}

/// get the *represented* (Reineke's Law) number of trees (N/ha)
double Sapling::livingStemNumber(double &rAvgDbh, double &rAvgHeight, double &rAvgAge) const
{
    double total = 0.;
    double dbh_sum = 0.;
    double h_sum = 0.;
    double age_sum = 0.;
    const SaplingGrowthParameters &p = mRUS->species()->saplingGrowthParameters();
    for (QVector<SaplingTree>::const_iterator it = mSaplingTrees.constBegin(); it!=mSaplingTrees.constEnd(); ++it) {
        float dbh = it->height / p.hdSapling * 100.f;
        if (dbh<1.) // minimum size: 1cm
            continue;
        double n = p.representedStemNumber(dbh); // one cohort on the pixel represents that number of trees
        dbh_sum += n*dbh;
        h_sum += n*it->height;
        age_sum += n*it->age.age;
        total += n;
    }
    if (total>0.) {
        dbh_sum /= total;
        h_sum /= total;
        age_sum /= total;
    }
    rAvgDbh = dbh_sum;
    rAvgHeight = h_sum;
    rAvgAge = age_sum;
    return total;
}

/// maintenance function to clear dead/recruited saplings from storage
void Sapling::cleanupStorage()
{
    QVector<SaplingTree>::iterator forw=mSaplingTrees.begin();
    QVector<SaplingTree>::iterator back;

    // seek last valid
    for (back=mSaplingTrees.end()-1; back>=mSaplingTrees.begin(); --back)
        if ((*back).isValid())
            break;

    if (back<mSaplingTrees.begin()) {
        mSaplingTrees.clear(); // no valid trees available
        return;
    }

    while (forw < back) {
        if (!(*forw).isValid()) {
            *forw = *back; // copy (fill gap)
            while (back>forw) // seek next valid
                if ((*--back).isValid())
                    break;
        }
        ++forw;
    }
    if (back != mSaplingTrees.end()-1) {
        // free resources...
        mSaplingTrees.erase(back+1, mSaplingTrees.end());
    }
}

// not a very good way of checking if sapling is present
// maybe better: use also a (local) maximum sapling height grid
// maybe better: use a bitset:
// position: index of pixel on LIF (absolute index)
bool Sapling::hasSapling(const QPoint &position) const
{
    const QPoint &offset = mRUS->ru()->cornerPointOffset();
    int index = (position.x()- offset.x())*cPxPerRU + (position.y() - offset.y());
    return mSapBitset[index];
    /*
    float *target = GlobalSettings::instance()->model()->grid()->ptr(position.x(), position.y());
    QVector<SaplingTree>::const_iterator it;
    for (it = mSaplingTrees.constBegin(); it!=mSaplingTrees.constEnd(); ++it) {
        if (it->pixel==target)
            return true;
    }
    return false;
    */
}


void Sapling::addSapling(const QPoint &pos_lif)
{
    // adds a sapling...
    mSaplingTrees.push_back(SaplingTree());
    SaplingTree &t = mSaplingTrees.back();
    t.height = 0.05; // start with 5cm height
    Grid<float> &lif_map = *GlobalSettings::instance()->model()->grid();
    t.pixel = lif_map.ptr(pos_lif.x(), pos_lif.y());
    int index = (pos_lif.x() - mRUS->ru()->cornerPointOffset().x()) * cPxPerRU +(pos_lif.y() - mRUS->ru()->cornerPointOffset().y());
    mSapBitset.set(index,true); // set bit: now there is a sapling there
    mAdded++;
}

/// clear  saplings on a given position (after recruitment)
void Sapling::clearSaplings(const QPoint &position)
{
    float *target = GlobalSettings::instance()->model()->grid()->ptr(position.x(), position.y());
    QVector<SaplingTree>::const_iterator it;
    for (it = mSaplingTrees.constBegin(); it!=mSaplingTrees.constEnd(); ++it) {
        if (it->pixel==target) {
            // trick: use a const iterator to avoid a deep copy of the vector; then do an ugly const_cast to actually write the data
            const SaplingTree &t = *it;
            const_cast<SaplingTree&>(t).pixel=0;
        }
    }
    int index = (position.x() - mRUS->ru()->cornerPointOffset().x()) * cPxPerRU +(position.y() - mRUS->ru()->cornerPointOffset().y());
    mSapBitset.set(index,false); // clear bit: now there is no sapling on this position

}

/// growth function for an indivudal sapling.
/// returns true, if sapling survives, false if sapling dies or is recruited to iLand.
/// see also http://iland.boku.ac.at/recruitment
bool Sapling::growSapling(SaplingTree &tree, const double f_env_yr, Species* species)
{
    QPoint p=GlobalSettings::instance()->model()->grid()->indexOf(tree.pixel);

    // (1) calculate height growth potential for the tree (uses linerization of expressions...)
    double h_pot = species->saplingGrowthParameters().heightGrowthPotential.calculate(tree.height);
    double delta_h_pot = h_pot - tree.height;

    // (2) reduce height growth potential with species growth response f_env_yr and with light state (i.e. LIF-value) of home-pixel.
    double lif_value = *tree.pixel;
    double h_height_grid = GlobalSettings::instance()->model()->heightGrid()->valueAtIndex(p.x()/cPxPerHeight, p.y()/cPxPerHeight).height;
    if (h_height_grid==0)
        throw IException(QString("growSapling: height grid at %1/%2 has value 0").arg(p.x()).arg(p.y()));
    double rel_height = 4. / h_height_grid;

    double lif_corrected = mRUS->species()->speciesSet()->LRIcorrection(lif_value, rel_height); // correction based on height
    // Note: difference to trees: no "LRIcorrection"
    double lr = mRUS->species()->lightResponse(lif_corrected); // species specific light response (LUI, light utilization index)

    double delta_h_factor = f_env_yr * lr; // relative growth

    if (h_pot<0. || delta_h_pot<0. || lif_corrected<0. || lif_corrected>1. || delta_h_factor<0. || delta_h_factor>1. )
        qDebug() << "invalid values in Sapling::growSapling";

    // check mortality of saplings
    if (delta_h_factor < species->saplingGrowthParameters().stressThreshold) {
        tree.age.stress_years++;
        if (tree.age.stress_years > species->saplingGrowthParameters().maxStressYears) {
            // sapling dies...
            tree.pixel=0;
            mDied++;
            return false;
        }
    } else {
        tree.age.stress_years=0; // reset stress counter
    }
    DBG_IF(delta_h_pot*delta_h_factor < 0.f || delta_h_pot*delta_h_factor > 2., "Sapling::growSapling", "inplausible height growth.");

    // grow
    tree.height += delta_h_pot * delta_h_factor;
    tree.age.age++; // increase age of sapling by 1

    // recruitment?
    if (tree.height > 4.f) {
        mRecruited++;

        ResourceUnit *ru = const_cast<ResourceUnit*> (mRUS->ru());
        float dbh = tree.height / species->saplingGrowthParameters().hdSapling * 100.f;
        // the number of trees to create (result is in trees per pixel)
        double n_trees = species->saplingGrowthParameters().representedStemNumber(dbh);
        int to_establish = (int) n_trees;
        // if n_trees is not an integer, choose randomly if we should add a tree.
        // e.g.: n_trees = 2.3 -> add 2 trees with 70% probability, and add 3 trees with p=30%.
        if (drandom() < (n_trees-to_establish) || to_establish==0)
            to_establish++;

        // add a new tree
        for (int i=0;i<to_establish;i++) {
            Tree &bigtree = ru->newTree();
            bigtree.setPosition(p);
            // add variation: add +/-10% to dbh and *independently* to height.
            bigtree.setDbh(dbh * nrandom(1. - mRecruitmentVariation, 1. + mRecruitmentVariation));
            bigtree.setHeight(tree.height * nrandom(1. - mRecruitmentVariation, 1. + mRecruitmentVariation));
            bigtree.setSpecies( species );
            bigtree.setAge(tree.age.age,tree.height);
            bigtree.setRU(ru);
            bigtree.setup();
        }
        // clear all regeneration from this pixel (including this tree)
        ru->clearSaplings(p);
        return false;
    }
    // book keeping (only for survivors)
    mLiving++;
    mAvgHeight+=tree.height;
    mAvgAge+=tree.age.age;
    mAvgDeltaHPot+=delta_h_pot;
    mAvgHRealized += delta_h_pot * delta_h_factor;
    return true;

}

void Sapling::calculateGrowth()
{
    Q_ASSERT(mRUS);
    if (mLiving==0 && mAdded==0)
        return;

    clearStatistics();
    ResourceUnit *ru = const_cast<ResourceUnit*> (mRUS->ru() );
    Species *species = const_cast<Species*>(mRUS->species());

    // calculate necessary growth modifier (this is done only once per year)
    mRUS->calculate(true); // calculate the 3pg module (this is done only if that did not happen up to now); true: call comes from regeneration
    double f_env_yr = mRUS->prod3PG().fEnvYear();

    mLiving=0;
    QVector<SaplingTree>::const_iterator it;
    for (it = mSaplingTrees.constBegin(); it!=mSaplingTrees.constEnd(); ++it) {
        const SaplingTree &tree = *it;
        if (tree.height<0)
            qDebug() << "h<0";
        if (tree.isValid()) {
            // growing
            if (growSapling(const_cast<SaplingTree&>(tree), f_env_yr, species)) {
                // set the sapling height to the maximum value on the current pixel
                QPoint p=GlobalSettings::instance()->model()->grid()->indexOf(tree.pixel);
                ru->setMaxSaplingHeightAt(p,tree.height);
            }
        }
    }
    if (mLiving) {
        mAvgHeight /= double(mLiving);
        mAvgAge /= double(mLiving);
        mAvgDeltaHPot /= double(mLiving);
        mAvgHRealized /= double(mLiving);
    }

    if (mSaplingTrees.count() > mLiving*1.3)
        cleanupStorage();

    mRUS->statistics().add(this);
    //qDebug() << ru->index() << species->id()<< ": (living/avg.height):" <<  mLiving << mAvgHeight;
}

/// fill a grid with the maximum height of saplings per pixel.
/// this function is used for visualization only
void Sapling::fillHeightGrid(Grid<float> &grid) const
{
    QVector<SaplingTree>::const_iterator it;
    for (it = mSaplingTrees.begin(); it!=mSaplingTrees.end(); ++it) {
        if (it->isValid()) {
             QPoint p=GlobalSettings::instance()->model()->grid()->indexOf(it->pixel);
             if (grid.valueAtIndex(p)<it->height)
                 grid.valueAtIndex(p) = it->height;
        }
    }

}
