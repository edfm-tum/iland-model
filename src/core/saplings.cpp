#include "global.h"
#include "saplings.h"

#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"
#include "establishment.h"
#include "species.h"
#include "seeddispersal.h"


Saplings::Saplings()
{

}

void Saplings::setup()
{
    mGrid.setup(GlobalSettings::instance()->model()->grid()->metricRect(), GlobalSettings::instance()->model()->grid()->cellsize());

    // mask out out-of-project areas
    HeightGrid *hg = GlobalSettings::instance()->model()->heightGrid();
    for (int i=0;i<mGrid.count();++i) {
        if (!hg->valueAtIndex(mGrid.index5(i)).isValid())
            mGrid[i].state = SaplingCell::CellInvalid;
        else
            mGrid[i].state = SaplingCell::CellFree;
    }


}

void Saplings::establishment(const ResourceUnit *ru)
{
    Grid<float> &seedmap =  const_cast<Grid<float>& > (ru->ruSpecies().first()->species()->seedDispersal()->seedMap() );
    HeightGrid *height_grid = GlobalSettings::instance()->model()->heightGrid();
    FloatGrid *lif_grid = GlobalSettings::instance()->model()->grid();

    QPoint iseedmap =  seedmap.indexAt(ru->boundingBox().topLeft()) ;
    QPoint imap =  mGrid.indexAt(ru->boundingBox().topLeft());

    int species_idx = irandom(0, ru->ruSpecies().size()-1);
    for (int s_idx = 0; s_idx<ru->ruSpecies().size(); ++s_idx) {

        // start from a random species (and cycle through the available species)
        species_idx = ++species_idx % ru->ruSpecies().size();

        ResourceUnitSpecies *rus = ru->ruSpecies()[species_idx];
        // check if there are seeds of the given species on the resource unit
        float seeds = 0.f;
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
        SaplingCell *s;
        int isc = 0; // index on 2m cell
        for (int iy=0; iy<cPxPerRU; ++iy) {
            s = mGrid.ptr(imap.x(), imap.y()+iy); // ptr to the row
            isc = mGrid.index(imap.x(), imap.y()+iy);

            for (int ix=0;ix<cPxPerRU; ++ix, ++s, ++isc, ++mTested) {
                if (s->state == SaplingCell::CellFree) {
                    bool viable = true;
                    // is a sapling of the current species already on the pixel?
                    // * test for sapling height already in cell state
                    // * test for grass-cover already in cell state
                    int i_occupied = -1;
                    for (int i=0;i<NSAPCELLS;++i) {
                        if (!s->saplings[i].is_occupied() && i_occupied<0)
                            i_occupied=i;
                        if (s->saplings[i].species_index == species_idx) {
                            viable = false;
                        }
                    }

                    if (viable && i_occupied>=0) {
                        // grass cover?
                        DBG_IF(i_occupied<0, "establishment", "invalid value i_occupied<0");
                        float seed_map_value = seedmap[seedmap.index10(isc)];
                        if (seed_map_value==0.f)
                            continue;
                        const HeightGridValue &hgv = (*height_grid)[height_grid->index5(isc)];
                        float lif_value = (*lif_grid)[isc];
                        double lif_corrected = rus->species()->speciesSet()->LRIcorrection(lif_value, 4. / hgv.height);
                        // check for the combination of seed availability and light on the forest floor
                         if (drandom() < seed_map_value*lif_corrected*abiotic_env ) {
                             // ok, lets add a sapling at the given position
                             s->saplings[i_occupied].setSapling(0.05f, 1, species_idx);
                             s->checkState();
                             mAdded++;

                         }

                    }

                }
            }
        }

    }

}
