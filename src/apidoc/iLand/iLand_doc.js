/**
The iLand Core JavaScript API provides access to the main simulation engine,
spatial grid calculations, individual tree-level attributes, and model control.

### Overview

With the core API, you can:
- **Control the Simulation:** Run/pause the model, change project settings, and track years using the `Globals` object.
- **Interact with Trees:** Retrieve individual tree attributes (DBH, height, species) or perform management/cutting via the `Management` object.
- **Access Spatial Grids:** Manipulate 2D grids (e.g., elevation, species distribution) using `Grid` and polygonal `Map` layers.
- **Handle Data Files:** Read and write CSV/text tables easily using `CSVFile`.

### Quick JavaScript Examples

#### 1. Controlling the Simulation and Settings
```javascript
// Access the current simulation year
var currentYear = Globals.year;

// Retrieve a setting from the XML project file
var baseFolder = Globals.setting("system.path.home");

// Set a configuration variable dynamically
Globals.set("model.settings.seedDispersal", false);
```

#### 2. Querying and Filtering Trees
```javascript
// Run a query on adult trees
management.load('dbh>30');
var trees = management.count;
console.log("Found " + trees.length + " spruce trees with DBH > 30cm");
```

#### 3. Reading and Writing Data
```javascript
var file = new CSVFile();
if (file.load("input_data.csv")) {
    console.log("Loaded " + file.rowCount + " rows of data.");
    // Access individual cell values
    var val = file.value(0, "column_name");
}
```

@main iLand
@module iLand
*/
