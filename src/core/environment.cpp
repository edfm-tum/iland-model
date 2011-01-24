#include "global.h"
#include "environment.h"
#include "helper.h"
#include "csvfile.h"
#include "gisgrid.h"

#include "climate.h"
#include "speciesset.h"

/** Represents the physical simulation site with regard to climate, soil properties and such.
    Data is read from various sources and presented to the core model with a standardized interface.
*/
Environment::Environment()
{
    mInfile = 0;
    mGrid = 0;
    mGridMode = false;
    mCurrentSpeciesSet = 0;
    mCurrentClimate = 0;
}
Environment::~Environment()
{
    if (mInfile) {
        delete mInfile;
    }
    if (mGrid)
        delete mGrid;
}

bool Environment::loadFromFile(const QString &fileName)
{
    QString source = Helper::loadTextFile(GlobalSettings::instance()->path(fileName));
    if (source.isEmpty())
        throw IException(QString("Environment: input file does not exist or is empty (%1)").arg(fileName));
    return loadFromString(source);
}

// ******** specific keys *******
const QString speciesKey = "model.species.source";
const QString climateKey = "model.climate.tableName";

bool Environment::loadFromString(const QString &source)
{
    try {
        if (mInfile)
            delete mInfile;
        mInfile = new CSVFile();

        mInfile->loadFromString(source);
        mKeys = mInfile->captions();

        XmlHelper xml(GlobalSettings::instance()->settings());
        mSpeciesSets.clear(); // note: the objects are not destroyed - potential memory leak.
        mClimate.clear();
        mRowCoordinates.clear();
        mCreatedObjects.clear();

        int index;
        if (mGridMode) {
            int id = mInfile->columnIndex("id");
            if (id<0)
                throw IException("Environment:: (grid mode) input file has no 'id' column!");
            for (int row=0;row<mInfile->rowCount();row++) {
                mRowCoordinates[mInfile->value(row, id).toString()] = row;
            }

        } else {
            // ***  Matrix mode ******
            // each row must contain 'x' and 'y' coordinates
            // setup coordinates (x,y)
            int ix,iy;
            ix = mInfile->columnIndex("x");
            iy = mInfile->columnIndex("y");
            if (ix<0 || iy<0)
                throw IException("Environment:: (matrix mode) input file has no x/y coordinates!");
            for (int row=0;row<mInfile->rowCount();row++) {
                QString key=QString("%1_%2")
                        .arg(mInfile->value(row, ix).toString())
                        .arg(mInfile->value(row, iy).toString());
                mRowCoordinates[key] = row;
            }
        }



        // ******** setup of Species Sets *******
        if ((index = mKeys.indexOf(speciesKey))>-1) {
            DebugTimer t("environment:load species");
            QStringList speciesNames = mInfile->column(index);
            speciesNames.removeDuplicates();
            qDebug() << "creating species sets:" << speciesNames;
            foreach (const QString &name, speciesNames) {
                xml.setNodeValue(speciesKey,name); // set xml value
                // create species sets
                SpeciesSet *set = new SpeciesSet();
                mSpeciesSets.push_back(set);
                mCreatedObjects[name] = (void*)set;
                set->setup();
            }
            qDebug() << mSpeciesSets.count() << "species sets created.";
        } else {
            // no species sets specified
            SpeciesSet *speciesSet = new SpeciesSet();
            mSpeciesSets.push_back(speciesSet);
            speciesSet->setup();
            mCurrentSpeciesSet = speciesSet;
        }

        // ******** setup of Climate *******
        if ((index = mKeys.indexOf(climateKey))>-1) {
            DebugTimer t("environment:load climate");
            QStringList climateNames = mInfile->column(index);
            climateNames.removeDuplicates();
            qDebug() << "creating climatae: " << climateNames;
            foreach (QString name, climateNames) {
                xml.setNodeValue(climateKey,name); // set xml value
                // create climate sets
                Climate *climate = new Climate();
                mClimate.push_back(climate);
                mCreatedObjects[name]=(void*)climate;
                climate->setup();
            }
            qDebug() << mClimate.count() << "climates created";
        } else {
            // no climate defined - setup default climate
            Climate *c = new Climate();
            mClimate.push_back(c);
            c->setup();
            mCurrentClimate = c;
        }
        if (!mCurrentClimate && mClimate.count()>0)
            mCurrentClimate = mClimate[0];
        if (!mCurrentSpeciesSet && mSpeciesSets.count()>0)
            mCurrentSpeciesSet = mSpeciesSets[0];
        return true;

    } catch(const IException &e) {
        QString addMsg;
        if (!mClimate.isEmpty())
            addMsg = QString("last Climate: %1 ").arg(mClimate.last()->name());
        if (!mSpeciesSets.isEmpty())
            addMsg += QString("last Speciesset table: %1").arg(mSpeciesSets.last()->name());
        QString error_msg = QString("An error occured during the setup of the environment: \n%1\n%2").arg(e.toString()).arg(addMsg);
        qDebug() << error_msg;
        Helper::msg(error_msg);
        return false;
    }
}

/** sets the "pointer" to a "position" (metric coordinates).
    All specified values are set (also the climate/species-set pointers).
*/
void Environment::setPosition(const QPointF position)
{
    // no changes occur, when the "environment" is not loaded
    if (!isSetup())
        return;
    QString key;
    int ix, iy, id;
    if (mGridMode) {
        // grid mode
        id = mGrid->value(position);
        key = QString::number(id);
        if (id==-1)
            return; // no data for the resource unit
    } else {
        // access data in the matrix by resource unit indices
        ix = int(position.x() / 100.); // suppose size of 1 ha for each coordinate
        iy = int(position.y() / 100.);
        key=QString("%1_%2").arg(ix).arg(iy);
    }

    if (mRowCoordinates.contains(key)) {
        XmlHelper xml(GlobalSettings::instance()->settings());
        int row = mRowCoordinates[key];
        QString value;
        if (logLevelInfo()) qDebug() << "settting up point" << position << "with row" << row;
        for (int col=0;col<mInfile->colCount(); col++) {
            if (mKeys[col]=="x" || mKeys[col]=="y" || mKeys[col]=="id") // ignore "x" and "y" keys
                continue;
            value = mInfile->value(row,col).toString();
            if (logLevelInfo()) qDebug() << "set" << mKeys[col] << "to" << value;
            xml.setNodeValue(mKeys[col], value);
            // special handling for constructed objects:
            if (mKeys[col]==speciesKey)
                mCurrentSpeciesSet = (SpeciesSet*)mCreatedObjects[value];
            if (mKeys[col]==climateKey)
                mCurrentClimate = (Climate*)mCreatedObjects[value];

        }

    } else {
        if (mGridMode)
            throw IException(QString("Environment:setposition: invalid grid id (or not present in input file): %1m/%2m (mapped to id %3).")
                             .arg(position.x()).arg(position.y()).arg(id));
        else
            throw IException(QString("Environment:setposition: invalid coordinates (or not present in input file): %1m/%2m (mapped to indices %3/%4).")
                             .arg(position.x()).arg(position.y()).arg(ix).arg(iy));
    }
}

bool Environment::setGridMode(const QString &grid_file_name)
{
    mGrid = new GisGrid();
    mGrid->loadFromFile(grid_file_name);
    mGridMode = true;
    return true;
}
