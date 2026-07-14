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
    void loadParameters(); ///< load params from XML


    /// main function to run the flow module
    void run();
    /// function to run a hazard
    void runFlow(QString type="avalanche", int experimentID=0);

    /// function that is called whenever a tree dies somewhere in iLand
    void treeDeath(const Tree *tree, const int removal_type);

    void yearBegin(); ///< called automatically
    const FlowModel &flowModel() const { return mFlow; }

private:
    FlowParameters mParamsAvalanche;
    FlowParameters mParamsRockfall;
    FlowParameters mParamsMudflow;

    Grid<FlowCell> mGrid;
    Grid<float> mFSI;
    FlowLayers mLayers;

    FlowModel mFlow;

    QString mRunFunction;
    bool mCustomFSISet;
    bool mForestEffectEnabled;
    bool mCustomLandscapeSet;

    // retrieve forest information from iLand and
    // calculate the forest structure index
    void calculateFSI(const QString &type);

    friend class FlowScript;
    friend class FlowOut;

};



#endif // FLOWMODULE_H
