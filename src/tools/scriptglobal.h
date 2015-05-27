/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
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

#ifndef SCRIPTGLOBAL_H
#define SCRIPTGLOBAL_H

#include <QObject>
#include <QVariant>
#include <QJSValue>

// Scripting Interface for MapGrid
class MapGrid; // forward
class MapGridWrapper: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int valid READ isValid)
    Q_PROPERTY(QString name READ name)
public:
    MapGridWrapper(QObject *parent=0);
    ~MapGridWrapper();
    static void addToScriptEngine(QJSEngine &engine);
    MapGrid *map() const { return mMap; } ///< acccess for C++ classes
    bool isValid() const; ///< returns true if map is successfully loaded
    QString name() const;
    //

public slots:
    // query
    double area(int id); ///< retrieve the area (m2) of the polygon denoted by 'id'
    // actions
    void load(QString file_name);
    void saveAsImage(QString file);
    void paint(double min_value, double max_value);
    // active modifications of the map
    void clear(); ///< clears the map (set all values to 0)
    void clearProjectArea(); ///< clear the project area (set to 0), but copy mask with pixels from "outside of project area" (i.e., -1, -2)
    /// paint a shape on the stand grid with id stand_id
    /// paint_function is a valid expression (paramters: x, y as *metric* coordinates)
    /// if wrap_around=true, then the shape is wrapped around the edges of the simulated area (torus)
    void createStand(int stand_id, QString paint_function, bool wrap_around);

    /// copy a bit of the source-map 'source' to this map. The
    /// source rectangle is given by coordinates (x1/y1) to (x2/y2).
    /// The rectangle will be blitted to the new coordinates destx/desty (moved from x1/y1).
    /// id_in: the id of the polygon to copy, id: the id of the pixels in the target
    /// return the size (ha) of the valid thing
    double copyPolygonFromRect(MapGridWrapper *source, int id_in, int id, double destx, double desty, double x1, double y1, double x2, double y2);

    void createMapIndex(); ///< call after creating stands with copyPolygonFromRect

private:
    MapGrid *mMap;
    bool mCreated;

};

/** ScriptGlobal contains a set of
  functions and properties that are accessible by JavaScript.
  */
class Model;
class ScriptGlobal : public QObject
{
    Q_OBJECT
    // read only properties
    Q_PROPERTY(int year READ year)
    Q_PROPERTY(int resourceUnitCount READ resourceUnitCount)
    Q_PROPERTY(QString currentDir WRITE setCurrentDir READ currentDir)
    Q_PROPERTY(double worldX READ worldX)
    Q_PROPERTY(double worldY READ worldY)
    Q_PROPERTY(bool qt5 READ qt5)
    Q_PROPERTY(int msec READ msec)
    Q_PROPERTY(QJSValue viewOptions READ viewOptions WRITE setViewOptions)


public:
    ScriptGlobal(QObject *parent=0);
    static void setupGlobalScripting();
    // properties accesible by scripts
    bool qt5() const {return true; } ///< is this the qt5-model? (changes in script object creation)
    int msec() const; ///< the milliseconds since the start of the day
    int year() const; ///< current year in the model
    int resourceUnitCount() const; ///< get number of resource uinit
    QString currentDir() const { return mCurrentDir; } ///< current execution directory (default is the Script execution directory)
    void setCurrentDir(QString newDir) { mCurrentDir = newDir; } ///< set current working dir
    double worldX(); ///< extent of the world (without buffer) in meters (x-direction)
    double worldY(); ///< extent of the world (without buffer) in meters (y-direction)

    // general functions
    static void loadScript(const QString &fileName);
    static QString executeScript(QString cmd);
    static QObject *scriptOutput; ///< public "pipe" for script output (is redirected to GUI if available)
    static QString formattedErrorMessage(const QJSValue &error_value, const QString &sourcecode);

    // view options
    /* View options:
     * * type: {...}
     * * species: bool
     * * shade: bool
     *
    */
    QJSValue viewOptions(); ///< retrieve current viewing options (JS - object)
    void setViewOptions(QJSValue opts); ///< set current view options

public slots:
    // system stuff
    QVariant setting(QString key); ///< get a value from the global xml-settings (returns undefined if not present)
    void set(QString key, QString value); ///< set the value of a setting
    void print(QString message); ///< print the contents of the message to the log
    void alert(QString message); ///< shows a message box to the user (if in GUI mode)
    void include(QString filename); ///< "include" the given script file and evaluate. The path is relative to the "script" path
    // file stuff
    QString defaultDirectory(QString dir); ///< get default directory of category 'dir'
    QString loadTextFile(QString fileName); ///< load content from a text file in a String (@sa CSVFile)
    void saveTextFile(QString fileName, QString content); ///< save string (@p content) to a text file.
    bool fileExists(QString fileName); ///< return true if the given file exists.
    // add trees
    int addSingleTrees(const int resourceIndex, QString content); ///< add single trees
    int addTrees(const int resourceIndex, QString content); ///< add tree distribution
    int addTreesOnMap(const int standID, QString content); ///< add trees (distribution mode) for stand 'standID'
    // add saplings
    int addSaplingsOnMap(MapGridWrapper *map, const int mapID, QString species, int px_per_hectare, double height, int age);
    // enable/disable outputs
    bool startOutput(QString table_name); ///< starts output 'table_name'. return true if successful
    bool stopOutput(QString table_name); ///< stops output 'table_name'. return true if successful
    // miscellaneous stuff
    void setViewport(double x, double y, double scale_px_per_m); ///< set the viewport of the main project area view
    bool screenshot(QString file_name); ///< make a screenshot from the central viewing widget
    void repaint(); ///< force a repainting of the GUI visualization (if available)
    bool gridToFile(QString grid_type, QString file_name); ///< create a "ESRI-grid" text file 'grid_type' is one of a fixed list of names, 'file_name' the ouptut file location
    void wait(int milliseconds); ///< wait for 'milliseconds' or (if ms=-1 until a key is pressed)
    // vegetation snapshots
    bool saveModelSnapshot(QString file_name);
    bool loadModelSnapshot(QString file_name);
private:
    void throwError(const QString &errormessage);
    QString mCurrentDir;
    Model *mModel;
};

/** The ScriptObjectFactory can instantiate objects of other C++ (QObject-based) types.
 *  This factory approach is used because the V8 (QJSEngine) does not work with
 *  the "new" way of creating objects.
*/
class ScriptObjectFactory: public QObject
{
    Q_OBJECT
public:
    ScriptObjectFactory(QObject *parent=0);
public slots:
    QJSValue newCSVFile(QString filename); ///< create a new instance of CSVFile and return it
    QJSValue newClimateConverter(); ///< create new instance of ClimateConverter and return it
    QJSValue newMap(); ///< create new instance of Map and return it
    int stats() {return mObjCreated;} ///< return the number of created objects
private:
    int mObjCreated;

};

#endif // SCRIPTGLOBAL_H
