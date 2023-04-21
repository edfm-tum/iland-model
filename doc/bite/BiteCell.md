# BiteCell

A cell is the smalled spatial execution unit of an agent. Each agent has a grid of cells that covers the full simulated landscape with the cell size defined by the agent.

## Properties

-   

    ### `active` (bool)

    A cell is `active` when currently colonized by the agent. Setting `active` to `true` initiates the agent for the given cell.

-   

    ### `spreading` (bool)

    is `true` when a cell is activelty dispersing the agent. Can be set via Javascript.

-   

    ### `yearsLiving` (int, read-only)

    The number of years a cell is continuously occupied by the agent (the value is reset if a cell dies).

-   

    ### `outbreakYears` (int, read-only)

    The number of years the current agent is in an outbreak phase. The value is 0 if outbreaks are not simulated (see [LifeCycle](BiteLifeCycle.md) for details).

-   

    ### `agent` ([agent](BiteAgent.md))

    A reference to the agent object.

-   

    ### `trees` (TreeList)

    A list of trees (see <https://iland-model.org/apidoc/classes/TreeList.html>) on the cell (trees \>4m). The `trees` can be queried, filtered or modified. The tree list is populated automatically during execution of the agent (but see `reloadTrees()`).

<!-- -->

    // use in dynamic expressions: an aggregate over all trees in the list
    agentBiomassCell: function(cell) { cell.trees.sum('stemmass*0.23'); }, ...

-   

    ### `saplings` (SaplingList)

    A list of saplings (tree cohorts \<4m) (see <https://iland-model.org/apidoc/classes/SaplingList.html>) on the cell. The `saplings` can be queried, filtered or modified. The sapling list is populated automatically during execution of the agent (but see `reloadSaplings()`).

**Note**: this is under construction - you may have to reload saplings explicitly.

## Methods

-   

    ### `hasValue(string var_name)`: bool

    returns `true` if a variable with the name `var_name` is available. Can be used for all types of cell variables, e.g. input grids, climate data, etc.

-   

    ### `value(string var_name)`: numeric

    returns the value of the variable `var_name`, or throws an error if the variable is not available.

-   

    ### `setValue(string var_name, numeric value)`

    updates the value of `var_name` with `value`. Note that not all variables can be updated (e.g. climate variables are read only).

-   

    ### `dailyClimateTimeseries(string type)`: dict of daily climate variable

    Use to access an array of climate variable for each day of the year for the current cell. The type of climate variable is given by parameter `type`. This can be used, if the required climatic indices are not built into the model. Note that accessing via the Javascript can come with a performance penalty (see code example below).

    `type` can be one of the following:

        | Variable | Description                                    |
        |----------|------------------------------------------------|
        | tmin     | daily minimum temperature (°C)                 |
        | tmax     | daily maximum temperature (°C)                 |
        | tmean    | daily mean temperature (mean of tmin and tmax) |
        | prec     | daily precipitation (mm)                       |
        | vpd      | vapour pressure deficit (kPa)                  |
        | rad      | daily radiation sum (MJ/m2)                    |


        ``` 

            // test GDD calculation

            function calcGDD(cell) {
                var tc = cell.dailyClimateTimeseries('tmean'); // retrieve from iLand a copy of temperatures
                var gdd=0;
                for (const t in tc)
                    gdd += Math.max(tc[t] - 5, 0);
                return gdd;
            }

            function calcGDD_reduce(cell) {
                var tc = cell.dailyClimateTimeseries('tmean'); // retrieve from iLand a copy of temperatures
                /* use map/reduce-approach for the sum.
                   Note the initialValue argument: if not provided, the *first* value of the data is used as is (even if < 5 degrees) */
                let s = tc.reduce(function(p,c) {
                 return p + (c>5 ? c-5 : 0);
              }, initialValue = 0);
              return s;
            }

            function getGDD(cell) {
                return cell.value('GDD');
            }

            function testPerf(cell,n, m) {
                var x;
                switch (m) {
                    case 0:
                        // naive javascript loop
                        for (let i=0;i<n;++i) {
                            x = calcGDD(cell);
                        }
                        return x;
                    case 1: 
                        // BITE/iLand implementation (C++)
                        for (let i=0;i<n;++i) {
                            x = getGDD(cell);
                        }
                        return x;
                    case 2: 
                        // javascript with reduce()
                        for (let i=0;i<n;++i) {
                            x = calcGDD_reduce(cell);
                        }
                        return x;
                }
            }

            ```


            Running `testPerf` for 100,000 times yields the following results (on my laptop):\

            | Implementation                  | Time                      |
            |---------------------------------|---------------------------|
            | native BITE implementation      | 0.11 s                    |
            | naive JS loop (calcGDD())       | 39.5 s ( \~ 350 x slower) |
            | optimized Javascript (reduce()) | 1.9 s ( \~ 20 x slower)   |

-   

    ### `reloadTrees()`

    (re-)loads all trees on the cell to the internal list (see `trees`). Reloading ignores all potential filters that have been applied previously to the list of trees.

-   

    ### `reloadSaplings()`

    (re-)loads all saplings on the cell to the internal list (see `saplings`). Reloading ignores all potential filters that have been applied previously to the list.

-   

    ### `die()`

    lets the agent on a cell die (i.e. set biomass to 0, `active` and `spreading` to `false`).
