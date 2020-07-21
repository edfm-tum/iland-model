
# BiteOutput


## Description
When an agent contains a `BiteOutput` item the model creates output on the level of indivdual cells for the agent.
The data is stored in a table with the name `tableName` in the global iLand output database. Each table contains a 
number of fixed and variable columns. Variable columns are defined with the `columns` property of the item.

### Fixed columns


Column name | Description
-------|------------
year     | simulation year
idx | cell index (cell Id) (see `index` cell variable), useful for spatial analysis
hostTrees | number of host trees (>4m, passing the 'hostFilter') in the current year
treesKilled | number of host trees killed (>4m) in the current year
volumeKilled | total volume (m3) of trees killed (>4m) by the agent in the current year
totalImpact | total impact (e.g. for defoliatores foliage mass consumed)



Example:
```
 ...
	output: new BiteOutput({
		outputFilter: "active=true",
		tableName: 'BiteTabx',
		columns: ['yearsLiving', 'MAT']
	}),
  ...

```

### spatial outputs
To create spatial outputs (i.e., raster maps of relevant variables), one can use the event mechanism
of Bite. For instance, the `onYearEnd` handler of an agent is called at the end of every year:

```
...
onYearEnd: function(agent) { 
  if (Globals.year==1) 
    agent.saveGrid('index', 'temp/pwnbase.asc'); 
}, ...
```
The code writes once a single raster grid with the cell indices (which can be used for spatial analysis 
in conjunction with the database table column `idx`).

## Setup

* ### `outputFilter` (dynamic expression)
A dynamic expression (cell) to select cells for which cell level output should be created. For example, 
to limit output only to cells that are currently colonized, use `active=true`.

* ### `tableName` (string)
The table name in the output database used for cell level output of the agent.

* ### `columns` (array of expressions)
The `columns` property is an array of expressions. For each expression a column in the output table is created in addition to 
the fixed columns (see above). The expressions are evaluated in the cell context (i.e. all cell variables including user-defined variables are available).

