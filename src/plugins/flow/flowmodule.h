/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/
#ifndef FLOWMODULE_H
#define FLOWMODULE_H

#include "grid.h"
#include "layeredgrid.h"

#include "flowmodel.h"

class FlowCell
{
public:
    FlowCell() { test = 0; }
    int test;
};


/** Helper class manage and visualize data layers related to the barkbeetle module.
  @ingroup barkbeetle
*/
class FlowLayers: public LayeredGrid<FlowCell> {
  public:
    void setGrid(const Grid<FlowCell> &grid) { mGrid = &grid; }
      void setModel(FlowModel *model) { mFlow = model; }
    double value(const FlowCell& data, const int index) const;
    const QVector<LayeredGridBase::LayerElement> &names();
    bool onClick(const QPointF &world_coord) const;
private:
    QVector<LayeredGridBase::LayerElement> mNames;
    FlowModel *mFlow { nullptr};
};




class ResourceUnit; // forward
class Tree; // forward
class BarkBeetleOut; // forward
class Climate; // forward
/** The BarkBeetleModule class is the main class of the bark beetle module.
 * The module simulates the spruce bark beetle (Ips typographus) spatially explicit on the landscape.
 * The number of possible bark beetle generations is calculated based on climate data (BBGenerations)
  @ingroup barkbeetle
*/
class FlowModule
{
public:
    FlowModule();
    ~FlowModule();
    static double cellsize() { return 10.; }

    void setup(); ///< general setup
    void loadParameters(bool do_reset=true); ///< load params from XML


    /// main function to run the flow module
    void run();

    /// function that is called whenever a tree dies somewhere in iLand
    void treeDeath(const Tree *tree, const int removal_type);

    void yearBegin(); ///< called automatically

private:
    struct SBBParams {
        SBBParams(): minDbh(10.f), cohortsPerGeneration(30), cohortsPerSisterbrood(50),
            spreadKernelMaxDistance(100.), backgroundInfestationProbability(0.0001), initialInfestationProbability(0.),
            stormInfestationProbability(1.), winterMortalityBaseLevel(0.),
            outbreakDurationMin(0.), outbreakDurationMax(0.), deadTreeSelectivity(1.),
            sanitationTreatmentProb(0.) {}
        float minDbh; ///< minimum dbh of spruce trees that are considered as potential hosts
        int cohortsPerGeneration; ///< 'packages' of beetles that spread from an infested pixel
        int cohortsPerSisterbrood; ///< cohorts that spread from a pixel when a full sister brood developed
        QString spreadKernelFormula; ///< formula of the PDF for the BB-spread
        double spreadKernelMaxDistance; ///< upper limit for the spread distance (the kernel is cut at this distance)
        double backgroundInfestationProbability; ///< p that a pixel gets spontaneously infested each year
        double initialInfestationProbability; ///< p that a pixel is infested at startup (as a result of pre-simulation dynamics)
        double stormInfestationProbability; ///< p that a pixel with storm damage gets infested
        double winterMortalityBaseLevel; ///< p that a infested pixel dies out over the winter (due to antagonists, bad luck, ...)
        double outbreakDurationMin; ///< minimum value for the duration of a barkbeetle outbreak
        double outbreakDurationMax; ///< maximum value for the duration of a barkbeetle outbreak#
        double deadTreeSelectivity; ///< how effectively beetles are attracted by dead trees (e.g. windthrown) (5x5 pixel). 1: all beetles go into dead trees, 0: no effect of dead trees
        double sanitationTreatmentProb; ///< probability (0..1) that a sanitation treatment is effective for a cell (if so, no beetles spread from the cell)

    } params;

    Grid<FlowCell> mGrid;
    Grid<float> mFSI;
    FlowLayers mLayers;

    FlowModel mFlow;

    // retrieve forest information from iLand and
    // calculate the forest structure index
    void calculateFSI();

    friend class FlowScript;
    friend class FlowOut;

};



#endif // FLOWMODULE_H
