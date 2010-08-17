#ifndef SAPLING_H
#define SAPLING_H
#include <QtCore/QVector>

/// SaplingTree holds information of a sapling (which represents N trees). Emphasis is on storage efficiency.
class SaplingTree {
public:
    SaplingTree() { pixel_index=-1; stress_years=0; height=0.f; }
    bool isValid() const {return pixel_index>-1; }
    int pixel_index; // index of the pixel (within the Home-resource-unit!) the sapling lives on.
    int stress_years; // number of consectuive years the sapling suffers from dire conditions.
    float height; // height of the sapling in meter
private:
};

class ResourceUnitSpecies; // forward
class Species;
class Sapling
{
public:
    Sapling();
    void setup(ResourceUnitSpecies *masterRUS) { mRUS = masterRUS; }
    void calculateGrowth(); ///< perform growth + mortality + recruitment of all saplings of this RU and species
private:
    bool growSapling(SaplingTree &tree, const double f_env_yr, const Species* species);
    ResourceUnitSpecies *mRUS;
    QVector<SaplingTree> mSaplingTrees;
};

#endif // SAPLING_H
