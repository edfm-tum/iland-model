#include "snags.h"
#include "tree.h"
#include "species.h"

/** @class Snags manages the standing dead trees and the litter transfer to the soil within a resource unit.

  */
Snags::Snags()
{
    clearInputPools();
}
void Snags::clearInputPools()
{
    mNewFoliageC=mNewFoliageN=mNewBranchesC=mNewBranchesN=mNewStemC=mNewStemN=mNewCoarseRootC=mNewCoarseRootN=mNewFineRootC=mNewFineRootN=0.;
    for (int i=0;i<NBRANCHYEARS;i++) {
        mBranchC[i]=0.;
        mBranchN[i]=0.;
    }
}

/// decompose a tree
/// see http://iland.boku.ac.at/snag+dynamics for the general concept
void Snags::decompose(const Tree* tree)
{
    // split the trees biomass to the several available litter components
    // (1) foliage: goes directly (in the first year) to the soil litter


    // (2) branches: are decomposed in the first five years
    double branch = tree->biomassBranch() * biomassCFraction;
    double cn_branch = 40.; // -> tree->species()->CNbranch();
    mNewBranchesC += branch;
    mNewBranchesN += branch / cn_branch;

    // (3) stems: are subject to the snag dynamics depending on dbh of the tree

    // (4) fine roots: similar to foliage: direct transfer to soil

    // (5) coarse root: are directly routed to the coarse woody debris pool of the soil
}


void Snags::calculate()
{
    // (2) branches
    // (2.1) split this years input into the buckets
    double f = 1. / double(NBRANCHYEARS);
    double branch_c = mNewBranchesC;
    double branch_n = mNewBranchesN;
    for (int i=0;i<NBRANCHYEARS; i++) {
        mBranchC[i] += branch_c * f;
        mBranchN[i] += branch_n * f;
    }
    // (2.2) calculate the flow to the soil and update the buckets
    // flow to soil: mBranchC[0] and mBranchC[1]: TODO!

    for (int i=0;i<NBRANCHYEARS-1; i++) {
        mBranchC[i] = mBranchC[i+1];
        mBranchN[i] = mBranchN[i+1];
    }
    mBranchC[NBRANCHYEARS-1] = 0.;
    mBranchN[NBRANCHYEARS-1] = 0.;

    // snag dynamics... TODO

}
