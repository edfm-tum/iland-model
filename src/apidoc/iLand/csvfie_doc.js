/**
The `CSVFile` is a helper class to load and process tabular data which is stored in a text file.

The type of delimiter is auto-detected from the first lines of the file. Possible delimeters are `;`, `,`, a tabulator (\t) or space(s).

Lines beginning with `#` are and empty lines are ignored. Quotes around (`"`) values are removed.



Example
=======

    @Example
        var csv = Factory.newCSVFile( Globals.path('temp/text.csv') );
        var index1 = csv.columnIndex('col_1'); // returns -1 if the column is not valid

        for (var i=0;i<csv.rowCount; ++i) {
            console.log( csv.value(i, index1) );
            // equivalent, but a bit slower
            console.log( csv.value(i, 'col_1') );
        }

        // loading from a string
        csv.loadFromString('a;b;c
                            1;2;3
                            1;4;12.1');
        console.log( csv.value(2,2) ); // -> 12.1


 @module iLand
 @class CSVFile
 */

CSVFile = {

    // properties


    /**
        if `true`, then the first line of the input file are considered to be headers.

        See also: {{#crossLink "CSVFile/columnIndex:method"}}{{/crossLink}}

        @property captions
        @type bool
    */
    /**
        if `true`, it is assumed that the file contains only one column (default: false).

        @property flat
        @type bool
    */
    /**
        The number of columns in the table (-1 if not loaded).

        @property colCount
        @type integer
    */
    /**
        The number of rows in the table (-1 if not loaded).

        @property rowCount
        @type integer
    */
    // methods

    bool loadFile(const QString &fileName); ///< load @p fileName. load the complete file at once.
    bool loadFromString(const QString &content); ///< load content from a given string.
    QString columnName(const int col) { if (col<mColCount) return mCaptions[col]; return QString(); } ///< get caption of ith column.
    int columnIndex(const QString &columnName) const { return mCaptions.indexOf(columnName); } ///< index of column or -1 if not available
    // value function with a column name
    QVariant value(const int row, const QString column_name) const { return value(row, columnIndex(column_name)); }

    QVariant value(const int row, const int col) const; ///< get value of cell denoted by @p row and @p cell. Not available in streaming mode.
    QVariant row(const int row); ///< retrieve content of the full row @p row as a Variant

    void setValue(const int row, const int col, QVariant value); ///< set the value of the column
    void saveFile(const QString &fileName); ///< save the current content to a file

    /**
    Load a table from the file given in `fileName`.

    See also: {{#crossLink "Factory/newCSVFile:method"}}{{/crossLink}}

    @method loadFile
    @fileName {string} file to load (Use {{#crossLink "Globals/path:method"}}{{/crossLink}} for using relative paths).
    @return { bool } `true` on sucess
    @Example
        var g = Globals.grid('height');
        var mean_h = g.sum('height') / g.count;
        g.apply('height/' + mean_h); // scale the height grid
      */

    /**
    Apply the expression `expression` on all pixels of the grid and return the sum of the values

    See also: {{#crossLink "Grid/apply:method"}}{{/crossLink}}

    @method sum
    @param {string} expresion expression to evaluate
    @return { double } sum of `expression` over all cells
    @Example
        var g = Globals.grid('height');
        var mean_h = g.sum('height') / g.count;
        g.apply('height/' + mean_h); // scale the height grid
      */


}
