#ifndef SAPLINGS_H
#define SAPLINGS_H

#include "grid.h"

struct SaplingTree {
    SaplingTree() { age=0; species_index=0; stress_years=0; flags=0; height=0.f;  }
    short unsigned int age;  // number of consectuive years the sapling suffers from dire conditions
    short unsigned int species_index; // index of the species within the resource-unit-species container
    unsigned char stress_years; // (upper 16bits) + age of sapling (lower 16 bits)
    unsigned char flags;
    float height; // height of the sapling in meter
    bool is_occupied() const { return height>0.f; }
    void setSapling(const float h_m, const int age_yrs, const int species_idx) { height=h_m; age=static_cast<short unsigned int>(age_yrs); stress_years=0; species_index=static_cast<short unsigned int>(species_idx); }
};
#define NSAPCELLS 5
struct SaplingCell {
    enum ECellState { CellInvalid=0, CellFree=1, CellFull=2};
    SaplingCell() {
        state=CellInvalid;
    }
    ECellState state;
    SaplingTree saplings[NSAPCELLS];
    void checkState() { if (state==CellInvalid) return;
                        bool free = false;
                        for (int i=0;i<NSAPCELLS;++i) {
                            // locked for all species, if a sapling of one species >1.3m
                            if (saplings[i].height>1.3f) {state = CellFull; return; }
                            // locked, if all slots are occupied.
                            if (!saplings[i].is_occupied())
                                free=true;
                        }
                        state = free? CellFree : CellFull;
                      }
};
class ResourceUnit;


class Saplings
{
public:
    Saplings();
    void setup();
    // main functions
    void establishment(const ResourceUnit *ru);
    void clearStats() { mAdded=0; mTested=0;}
    int saplingsAdded() const { return mAdded; }
    int pixelTested() const { return mTested; }

private:
    Grid<SaplingCell> mGrid;
    int mAdded;
    int mTested;
};

#endif // SAPLINGS_H
