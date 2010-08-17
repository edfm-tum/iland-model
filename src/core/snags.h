#ifndef SNAGS_H
#define SNAGS_H

#define NBRANCHYEARS 5
class Tree; // forward

class Snags
{
public:
    Snags();
    void decompose(const Tree* tree); ///< decompose a tree to litter compartments. This function is called directly after the death of a tree.
    /// calculate
    void calculate(); ///< main routine, calculates snag dyamics, transfer to soil
private:
    // flux pools
    void clearInputPools();
    double mNewFoliageC, mNewFoliageN; ///< foliage matter (kg) from current year
    double mNewBranchesC, mNewBranchesN; ///< twigs matter (kg) from current year
    double mNewStemC, mNewStemN; ///< stem matter (kg) from current year
    double mNewCoarseRootC, mNewCoarseRootN; ///< coarse root matter (kg) from current year
    double mNewFineRootC, mNewFineRootN; ///< fine root matter (kg) from current year
    // pools
    double mBranchC[NBRANCHYEARS], mBranchN[NBRANCHYEARS];

};

#endif // SNAGS_H
