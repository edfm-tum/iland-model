#include "global.h"
#include "environment.h"
#include "helper.h"
#include "csvfile.h"


#include "climate.h"
#include "speciesset.h"

/** Represents the physical simulation site with regard to climate, soil properties and such.
    Data is read from various sources and presented to the core model with a standardized interface.
*/
Environment::Environment()
{
    mInfile=0;
}
Environment::~Environment()
{
    if (mInfile)
        delete mInfile;
}

bool Environment::loadFromFile(const QString &fileName)
{
    QString source = Helper::loadTextFile(fileName);
    if (source.isEmpty())
        throw IException(QString("Environment: input file does not exist or is empty (%1)").arg(fileName));
    return loadFromString(source);
}

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
        int index;
        // setup coordinates (x,y)
        int ix,iy;
        ix = mInfile->columnIndex("x");
        iy = mInfile->columnIndex("y");
        if (ix<0 || iy<0)
            throw IException("Environment:: input file has no x/y coordinates!");
        for (int row=0;row<mInfile->rowCount();row++) {
            QString key=QString("%1_%2")
                        .arg(mInfile->value(row, ix).toString())
                        .arg(mInfile->value(row, iy).toString());
            mRowCoordinates[key] = row;
        }

        // ******** specific keys *******
        const QString speciesKey = "model.species.source";
        const QString climateKey = "model.climate.tableName";

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
                set->setup();
                mSpeciesSets.push_back(set);
            }
            qDebug() << mSpeciesSets.count() << "species sets created.";
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
                climate->setup();
                mClimate.push_back(climate);
            }
            qDebug() << mClimate.count() << "climates created";
        }
        return true;

    } catch(const IException &e) {
        QString error_msg = QString("An error occured during the setup of the environment: \n%1").arg(e.toString());
        qDebug() << error_msg;
        Helper::msg(error_msg);
        return false;
    }
}

Climate *Environment::climate()
{

}

SpeciesSet *Environment::speciesSet()
{

}

void Environment::setPosition(const QPointF position)
{
    if (!mInfile)
        throw IException("Environment:setposition: input file not loaded.");

    int ix, iy;
    ix = int(position.x() / 100.); // suppose size of 1 ha for each coordinate
    iy = int(position.y() / 100.);
    QString key=QString("%1_%2").arg(ix).arg(iy);
    if (mRowCoordinates.contains(key)) {
        XmlHelper xml(GlobalSettings::instance()->settings());
        int row = mRowCoordinates[key];
        QString value;
        qDebug() << "settting up point" << position << "with row" << row;
        for (int col=0;col<mInfile->colCount(); col++) {
            if (mKeys[col]=="x" || mKeys[col]=="y")
                continue;
            value = mInfile->value(row,col).toString();
            qDebug() << "set" << mKeys[col] << "to" << value;
            xml.setNodeValue(mKeys[col], value);
        }

    } else
        throw IException(QString("Environment:setposition: invalid coordinates (or not present in input file): %1/%2").arg(position.x()).arg(position.y()));
}
