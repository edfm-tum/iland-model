#include "deadtree.h"

#include "tree.h"

DeadTree::DeadTree(const Tree *tree)
{
    mSpecies = tree->species();
    QPointF tree_pos = tree->position();
    mX = tree_pos.x();
    mY = tree_pos.y();
    mVolume = tree->volume();
    mInititalBiomass = tree->biomassStem();
    mBiomass = tree->biomassStem();
    if (mInititalBiomass <= 0.)
        throw IException("DeadTree: invalid stem biomass of <=0!");
}

bool DeadTree::calculate()
{
    if (isStanding()) {
        mYearsStandingDead++;
        calculateSnag();
    } else {
        // lying deadwood
        mYearsDowned++;
        return calculateDWD();
    }
    return true;
}

bool DeadTree::calculateSnag()
{
    // update biomass...
    mBiomass *= 0.9;
    updateDecayClass();

    // calculate probability of falling down
    double p_fall = 0.1;

    // transfer to DWD?
    if (drandom() < p_fall) {
        mIsStanding = false;
        // implicit transfer of biomass to DWD
        return false; // changed to DWD
    }
    return true;
}

bool DeadTree::calculateDWD()
{
    // update biomass...
    mBiomass *= 0.9;
    updateDecayClass();

    // drop out?
    if (proportionBiomass() < 0.01) {
        // stop tracking this stem.
        // TODO: move remaining biomass to snag pool!
        // mark to be cleared
        mSpecies = nullptr;
        return false;
    }
    return true;
}

void DeadTree::updateDecayClass()
{
    double remaining = proportionBiomass();
    mDecayClass = 5;
    if (remaining > 0.15) mDecayClass = 4;
    if (remaining > 0.5) mDecayClass = 3;
    if (remaining > 0.75) mDecayClass = 2;
    if (remaining > 0.9) mDecayClass = 1;
}
