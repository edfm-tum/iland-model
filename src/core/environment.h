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

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
#include <QtCore>

class Climate;
class SpeciesSet;
class CSVFile;

/** Environment specifes the geographical properties of the landscape.
    This is achieved by specifying (user defined) values (e.g. soil depth) for each resource unit.
    Resource units are specified by x/y indices.
    see http://iland.boku.ac.at/initialize+the+landscape */
class Environment
{
public:
    Environment();
    ~Environment();
    bool isSetup() const { return mInfile!=0; }
    // setup
    void setDefaultValues(Climate *climate, SpeciesSet *speciesSet) {mCurrentClimate=climate; mCurrentSpeciesSet=speciesSet; }
    bool loadFromString(const QString &source);
    bool loadFromFile(const QString &fileName);
    QList<Climate*> climateList() const { return mClimate; } ///< created climates.
    QList<SpeciesSet*> speciesSetList() const { return mSpeciesSets; } ///< created species sets
    // access
    void setPosition(const QPointF position); ///< set position (metric coordinates). Subsequent calls to retriever functions are for the current location.
    Climate *climate() const { return mCurrentClimate;} ///< get climate at current pos
    SpeciesSet *speciesSet() const {return mCurrentSpeciesSet;} ///< get species set on current pos

private:
    Climate *mCurrentClimate; ///< climate at current location
    SpeciesSet *mCurrentSpeciesSet; ///< species set at current location
    QList<Climate*> mClimate; ///< created climates.
    QList<SpeciesSet*> mSpeciesSets; ///< created species sets
    QStringList mKeys;
    QHash<QString, int> mRowCoordinates;
    QHash<QString, void*> mCreatedObjects;
    CSVFile *mInfile;

};

#endif // ENVIRONMENT_H
