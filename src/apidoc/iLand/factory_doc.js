/**
## NOTE: The `Factory` is no longer necessary, and its us is deprecated!

To create an object in Javascript, you can use the standard Javascript operator `new`:

## Examples
    // Create a instance of a Map object
    var map = new Map;
    // now you are good to use the objects properties and methods
    console.log(map.name); // print the name of the stand grid file


The `Factory` is a helper object for the creation of other iLand script objects such as
a {{#crossLink "Map"}}{{/crossLink}}.

Technically, the `Factory`can instantiate objects of other C++ (QObject-based) types.
This factory approach is used because the V8 (QJSEngine) had limitations with regard to the `new` operator of Javascript.
Now (with version Qt6, and likely also some versions before) the proper use of `new` is possible again.



 @module iLand
 @class Factory
 */

Factory = {


    /**
    Create an instance of a {{#crossLink "CSVFile"}}{{/crossLink}}, a convenience class for reading and processing
    tabular data.

    @method newCSVFile
    @param {string} file_name The file to load with
    @return {object} A Javascript {{#crossLink "CSVFile"}}{{/crossLink}} object
    */

    /**
    Create an instance of a {{#crossLink "Map"}}{{/crossLink}}, a wrapper for a GIS raster file.

    @method newMap
    @return {object} A Javascript {{#crossLink "Map"}}{{/crossLink}} object
    */

    /**
    Create an instance of a Grid. Initially, the grid is empty.

    @method newGrid
    @return {object} A Javascript {{#crossLink "Grid"}}{{/crossLink}} object
    */
    /**
    Create an instance of a ClimateConverter (see https://iland-model.org/Object+ClimateConverter).

    @method newClimateConverter
    @return {object} A Javascript ClimateConverter object
    */

}

