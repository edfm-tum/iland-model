/**
The `Grid` class encapsulates a floating point grid (double precision)

Memory management
-----------------

Examples
--------
 @module iLand
 @class Grid
 */
Grid = {

    /**
        gets or sets the name of the grid (that are used for calculations)

        @property name
        @type string
    */


    /**
        the height (y-dimension) of the grid (number of pixels)

        See also: {{#crossLink "Grid/cellsize:property"}}{{/crossLink}}, {{#crossLink "Grid/width:property"}}{{/crossLink}}

        @property height
        @type integer
        @readOnly
    */

    /**
        the width (x-dimension) of the grid (number of pixels)

        See also: {{#crossLink "Grid/cellsize:property"}}{{/crossLink}}, {{#crossLink "Grid/height:property"}}{{/crossLink}}

        @property width
        @type integer
        @readOnly
    */
    /**
        the number of pixels of the grid.

        See also: {{#crossLink "Grid/cellsize:property"}}{{/crossLink}}, {{#crossLink "Grid/height:property"}}{{/crossLink}}, {{#crossLink "Grid/width:property"}}{{/crossLink}}

        @property count
        @type integer
        @readOnly
    */
    /**
        the cell size of the grid in meters.

        See also: {{#crossLink "Grid/count:property"}}{{/crossLink}}, {{#crossLink "Grid/height:property"}}{{/crossLink}}, {{#crossLink "Grid/width:property"}}{{/crossLink}}

        @property cellsize
        @type integer
        @readOnly
    */
    /**
        is `true` if the grid contains data.

        @property isValid
        @type boolean
        @readOnly
    */

    // methods

    /// create a copy of the current grid and return a new script object
    //QJSValue copy();
    /// fill the grid with 0-values
    //void clear();

    /**
    create a copy of the current grid and return a new grid object. The `name` of the copied grid is _x_.

    @method copy
    @return {grid} a copy of the grid
    @Example
        var a = Globals.grid('height');
        var ac = a.copy();
      */

    /**
    fill the grid with 0-values

    @method clear
      */
    /**
    retrieve some key parameters of the grid as a string.

    @method info
    @return {string} the information string
    @Example
        var g = Globals.grid('height');
        console.log(g.info());
        //prints:  grid-dimensions: 1820/1020 (cellsize: 10, N cells: 1856400), grid-name='height'
      */

    /**
    save to a file `file_name` as ESRI ASCII raster file.


    See also: {{#crossLink "Grid/load:method"}}{{/crossLink}}

    @method save
    @param {string} file_name destination file name (relative to project folder)
      */

    /**
    load from a file `file_name` (ESRI ASCII raster grid). The `name` property is set to the base file name (without path and extension).

    See also: {{#crossLink "Grid/save:method"}}{{/crossLink}}

    @method load
    @param {string} file_name source file name (relative to project folder)
    @return { boolean } true on success.
      */

    /**
    apply a function on the values of the grid, thus modifiying the grid (see the copy() function).
    The function is given as a string representing an [Expression](http://iland.boku.ac.at/Expression) and is run for each cell of the grid.
    In the expression, the current value of the grid cell can be accessed using the {{#crossLink "Grid/name:property"}}{{/crossLink}} property.

    See also: {{#crossLink "Grid/copy:method"}}{{/crossLink}}, {{#crossLink "Grid/combine:method"}}{{/crossLink}}

    @method apply
    @param {string} expression expression that is applied to each cell of the grid
    @Example
        var g = Globals.grid('height'); // name is 'height'
        g.apply('x*x'); // error, invalid variable
        g.apply('min(height, 30'); // ok
        g.apply('if(height<=4, 0, height)'); // ok
        var h = g.copy().apply('x^2'); // use copy() if the grid should not change (note: copies are named 'x')
      */

    /**

    combine multiple grids, and set the value of the internal grid to the result of `expression` for each cell. The function expects
    an object that includes named source grids. The `expression` is an [iLand Expression](http://iland.boku.ac.at/Expression),
    and you can refer to combined grids in `grid_objects` with the respective name of the grid. Note that the function
    alters the data of the grid.


    All grids must have the same dimensions, and the grid iteself can be accessed by adding the grid to `grid_objects`.


    See also: {{#crossLink "Grid/copy:method"}}{{/crossLink}}, {{#crossLink "Grid/apply:method"}}{{/crossLink}}

    @method combine
    @param {string} expression expression that is applied to each cell of the grid
    @param {object} grid_objects object including the source grids
    @Example
        var g = Globals.grid('height'); // name of g is 'height'
        var j = Globals.grid('count');
        var k = g.copy().apply('if(height>30,1,0)'); // note: calls may be chained,

        // update 'k' by combining g,j, and k
        k.combine('height*count * filter', { height: g, filter: k, count: j } );
      */

    /**
    apply the expression `expression` on all pixels of the grid and return the sum of the values

    See also: {{#crossLink "Grid/apply:method"}}{{/crossLink}}

    @method sum
    @param {string} expresion expression to evaluate
    @return { double } sum of `expression` over all cells
    @Example
        var g = Globals.grid('height');
        var mean_h = g.sum('height') / g.count;
        g.apply('height/' + mean_h); // scale the height grid
      */

    /**
    access individual cell values of the grid at the given position. If the grid is empty, or the the
    given position is invalid, -1 is returned.

    See also: {{#crossLink "Grid/setValue:method"}}{{/crossLink}}

    @method value
    @param {integer} x index in x direction (between 0 and grid.width-1)
    @param {integer} y index in y direction (between 0 and grid.height-1)
    @return { double } value of the grid at position (`x`, `y`)

      */
    /**
    set the value at position (`x`, `y`) to `value`.

    See also: {{#crossLink "Grid/value:method"}}{{/crossLink}}

    @method setValue
    @param {integer} x index in x direction (between 0 and grid.width-1)
    @param {integer} y index in y direction (between 0 and grid.height-1)
    @param {double} value value to set
    @Example
        // todo
      */
}
