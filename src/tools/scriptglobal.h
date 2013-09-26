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
    void paint(double min_value=0., double max_value=1.);

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


public:
    ScriptGlobal(QObject *parent=0);
    static void setupGlobalScripting();
    // properties accesible by scripts
    int year() const; ///< current year in the model
    int resourceUnitCount() const; ///< get number of resource uinit
    QString currentDir() const { return mCurrentDir; } ///< current execution directory (default is the Script execution directory)
    void setCurrentDir(QString newDir) { mCurrentDir = newDir; } ///< set current working dir

    // general functions
    static void loadScript(const QString &fileName);
    static QString executeScript(QString cmd);
    static QObject *scriptOutput; ///< public "pipe" for script output (is redirected to GUI if available)

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
    int addSaplingsOnMap(const MapGridWrapper *map, const int mapID, QString species, int px_per_hectare, double height=-1);
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
    int stats() {return mObjCreated;} ///< return the number of created objects
private:
    int mObjCreated;

};

#endif // SCRIPTGLOBAL_H
