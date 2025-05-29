#ifndef DEADTREE_H
#define DEADTREE_H
class Species; // forward
class Tree; // forward
/**
 * @brief The DeadTree class encapsulates a single dead tree
 * (either standing or lying) that is tracked as invidual element.
 */
class DeadTree
{
public:
    DeadTree(const Tree *tree);
    /// main update function for both snags and DWD
    /// return false if tracking stops
    bool calculate();

    // properties
    /// true if standing, false if downed dead wood
    bool isStanding() const { return mIsStanding; }
    /// tree volume of the stem at the time of death
    double volume() const { return mVolume; }
    /// current biomass (kg)
    double biomass() const { return mBiomass; }
    /// proportion of remaining biomass (0..1)
    double proportionBiomass() const { return mBiomass / mInititalBiomass; }
    /// decayClass: 1..5
    int decayClass() const { return mDecayClass; }
    /// species ptr
    const Species *species() const { return mSpecies; }
private:
    bool calculateSnag(); // process standing snag
    bool calculateDWD(); // process lying deadwood
    /// set decay class (I to V) based on the
    /// proportion of remaining biomass
    void updateDecayClass();
    float mX {0};
    float mY {0};
    const Species *mSpecies {nullptr };
    bool mIsStanding {true};
    short int mYearsStandingDead {0};
    short int mYearsDowned {0};
    short int mDecayClass {0};
    float mVolume {0};
    float mInititalBiomass {0}; // kg biomass at time of death
    float mBiomass {0}; // kg biomass currently
};

#endif // DEADTREE_H
