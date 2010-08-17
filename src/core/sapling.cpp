#include "sapling.h"
#include "model.h"
#include "species.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"

/** @class Sapling
    Sapling stores saplings and computes sapling growth (before recruitment).
    http://iland.boku.ac.at/sapling+growth+and+competition
    Saplings are established in a separate step (@sa Regeneration). If sapling reach a height of 4m, they are recruited and become "real" iLand-trees.
    Within the regeneration layer, a cohort-approach is applied.

  */


Sapling::Sapling()
{
    mRUS = 0;
}
/// growth function for an indivudal sapling.
/// returns true, if sapling survives, false if sapling dies or is recruited to iLand.
bool Sapling::growSapling(SaplingTree &tree, const double f_env_yr, const Species* species)
{
    // (1) calculate height growth potential for the tree (uses linerization of expressions...)
    double h_pot = species->saplingGrowthParameters().heightGrowthPotential.calculate(tree.height);
    double delta_h_pot = h_pot - tree.height;

    // (2) reduce height growth potential with species growth response f_env_yr and with light state (i.e. LIF-value) of home-pixel.
    double delta_h = delta_h_pot * f_env_yr;

    Grid<float> &lif_map = *GlobalSettings::instance()->model()->grid();
    double lif_value = lif_map.valueAtIndex(tree.pixel_index);

    delta_h = delta_h * lif_value; // without correction


}

void Sapling::calculateGrowth()
{
    Q_ASSERT(mRUS);
    if (mSaplingTrees.isEmpty())
        return;

    ResourceUnit *ru = const_cast<ResourceUnit*> (mRUS->ru() );
    const Species *species = mRUS->species();

    // calculate necessary growth modifier (this is done only once per year)
    mRUS->calculate(true); // calculate the 3pg module (this is done only if that did not happen up to now); true: call comes from regeneration
    double f_env_yr = mRUS->prod3PG().fEnvYear();

    QVector<SaplingTree>::iterator it;
    QVector<SaplingTree>::iterator tend=mSaplingTrees.end();
    for (it = mSaplingTrees.begin(); it!=tend; ++it) {
        SaplingTree &tree = *it;
        if (tree.isValid()) {
            // growing
            if (growSapling(tree, f_env_yr, species)) {
                // book keeping (only for survivors)
                int &height_map_pos = ru->saplingHeightAt(tree.pixel_index);
                height_map_pos = std::max(height_map_pos, int(tree.height*100));
            }
        }
    }
}
