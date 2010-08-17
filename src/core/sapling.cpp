#include "sapling.h"
#include "model.h"
#include "species.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "tree.h"

/** @class Sapling
    Sapling stores saplings and computes sapling growth (before recruitment).
    http://iland.boku.ac.at/sapling+growth+and+competition
    Saplings are established in a separate step (@sa Regeneration). If sapling reach a height of 4m, they are recruited and become "real" iLand-trees.
    Within the regeneration layer, a cohort-approach is applied.

  */


Sapling::Sapling()
{
    mRUS = 0;
    clearStatistics();
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

void Sapling::addSapling(const QPoint &pos_lif)
{
    // adds a sapling...
    mSaplingTrees.push_back(SaplingTree());
    SaplingTree &t = mSaplingTrees.back();
    t.height = 0.05; // start with 5cm height
    Grid<float> &lif_map = *GlobalSettings::instance()->model()->grid();
    t.pixel = lif_map.ptr(pos_lif.x(), pos_lif.y());
    mAdded++;
}

/// growth function for an indivudal sapling.
/// returns true, if sapling survives, false if sapling dies or is recruited to iLand.
/// see also http://iland.boku.ac.at/recruitment
bool Sapling::growSapling(SaplingTree &tree, const double f_env_yr, Species* species)
{
    // (1) calculate height growth potential for the tree (uses linerization of expressions...)
    double h_pot = species->saplingGrowthParameters().heightGrowthPotential.calculate(tree.height);
    double delta_h_pot = h_pot - tree.height;

    // (2) reduce height growth potential with species growth response f_env_yr and with light state (i.e. LIF-value) of home-pixel.
    double lif_value = *tree.pixel;
    double delta_h_factor = f_env_yr * lif_value; // relative growth

    if (h_pot<0. || delta_h_pot<0. || lif_value<0. || lif_value>1. || delta_h_factor<0. || delta_h_factor>1. )
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
        tree.pixel=0;
        QPoint p=GlobalSettings::instance()->model()->grid()->indexOf(tree.pixel);
        ResourceUnit *ru = const_cast<ResourceUnit*> (mRUS->ru());
        float dbh = tree.height / species->saplingGrowthParameters().hdSapling * 100.f;
        // add a new tree
        Tree &bigtree = ru->newTree();
        bigtree.setPosition(p);

        bigtree.setDbh(dbh);
        bigtree.setHeight(tree.height);
        bigtree.setSpecies( species );
        bigtree.setAge(tree.age.age,tree.height);
        bigtree.setRU(ru);
        bigtree.setup();
        return false;
    }
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
    QVector<SaplingTree>::iterator it;
    for (it = mSaplingTrees.begin(); it!=mSaplingTrees.end(); ++it) {
        SaplingTree &tree = *it;
        if (tree.height<0)
            qDebug() << "h<0";
        if (tree.isValid()) {
            // growing
            if (growSapling(tree, f_env_yr, species)) {
                // book keeping (only for survivors)
                mLiving++;
                QPoint p=GlobalSettings::instance()->model()->grid()->indexOf(tree.pixel);
                float h = ru->saplingHeightAt(p);
                if (tree.height>h) {
                    ru->setSaplingHeightAt(p,tree.height);
                    mAvgHeight+=tree.height;
                    mAvgAge+=tree.age.age;
                }
            }
        }
    }
    if (mLiving) {
        mAvgHeight /= double(mLiving);
        mAvgAge /= double(mLiving);
    }

    if (mSaplingTrees.count() > mLiving*1.3)
        cleanupStorage();

    mRUS->statistics().add(this);
    //qDebug() << ru->index() << species->id()<< ": (living/avg.height):" <<  mLiving << mAvgHeight;
}

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
