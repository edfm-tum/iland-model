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

#ifndef STANDLOADER_H
#define STANDLOADER_H
#include <QtCore/QString>

class Model;
class ResourceUnit;
class RandomCustomPDF;
class Species;
class StandLoader
{
public:
    StandLoader(Model *model): mModel(model), mRandom(0) {}
    ~StandLoader();
    void processInit();
    /// load a single tree file (picus or iland style). return number of trees loaded.
    int loadPicusFile(const QString &fileName, ResourceUnit *ru=NULL);
    /// load a tree distribution based on dbh classes. return number of trees loaded.
    int loadiLandFile(const QString &fileName, ResourceUnit *ru=NULL);
    int loadSingleTreeList(const QString &content, ResourceUnit*ru = NULL, const QString &fileName="");
    int loadDistributionList(const QString &content, ResourceUnit *ru = NULL, const QString &fileName="");
private:
    struct InitFileItem
    {
        Species *species;
        int count;
        double dbh_from, dbh_to;
        double hd;
        int age;
        double density;
    };
    /// load tree initialization from a file. return number of trees loaded.
    int loadInitFile(const QString &fileName, const QString &type, ResourceUnit *ru=NULL);
    void executeiLandInit(ResourceUnit *ru); ///< shuffle tree positions
    void copyTrees(); ///< helper function to quickly fill up the landscape by copying trees
    void evaluateDebugTrees(); ///< set debug-flag for trees by evaluating the param-value expression "debug_tree"
    Model *mModel;
    RandomCustomPDF *mRandom;
     QVector<InitFileItem> mInitItems;
};

#endif // STANDLOADER_H
