#include <QtCore>
#include <QtSql>
#include "global.h"
#include "xmlhelper.h"
#include "speciesset.h"
#include "species.h"

SpeciesSet::SpeciesSet()
{
    mSetupQuery = 0;
}

SpeciesSet::~SpeciesSet()
{
   clear();
}

void SpeciesSet::clear()
{
    qDeleteAll(mSpecies.values());
    mSpecies.clear();
    mActiveSpecies.clear();
}

const Species *SpeciesSet::species(const int &index)
{
    foreach(Species *s, mSpecies)
        if (s->index() == index)
            return s;
    return NULL;
}

/** loads active species from a database table and creates/setups the species.
    The function uses the global database-connection.
  */
int SpeciesSet::setup()
{
    const XmlHelper &xml = GlobalSettings::instance()->settings();
    QString tableName = xml.value("model.species.source", "species");
    QString readerFile = xml.value("model.species.reader", "reader.bin");
    readerFile = GlobalSettings::instance()->path(readerFile, "lip");
    mReaderStamp.load(readerFile);

    QSqlQuery query(GlobalSettings::instance()->dbin());
    mSetupQuery = &query;
    QString sql = QString("select * from %1").arg(tableName);
    query.exec(sql);
    clear();
    qDebug() << "attempting to load a species set from" << tableName;
    while (query.next()) {
        if (var("active").toInt()==0)
            continue;

        Species *s = new Species(this); // create
        // call setup routine (which calls SpeciesSet::var() to retrieve values
        s->setup();

        mSpecies.insert(s->id(), s); // store
        if (s->active())
            mActiveSpecies.append(s);
    } // while query.next()
    qDebug() << "loaded" << mSpecies.count() << "active species:";
    qDebug() << mSpecies.keys();

    mSetupQuery = 0;

    // setup nitrogen response
    XmlHelper resp(xml.node("model.species.nitrogenResponseClasses"));
    if (!resp.isValid())
        throw IException("model.species.nitrogenResponseClasses not present!");
    mNitrogen_1a = resp.valueDouble("class_1_a");
    mNitrogen_1b = resp.valueDouble("class_1_b");
    mNitrogen_2a = resp.valueDouble("class_2_a");
    mNitrogen_2b = resp.valueDouble("class_2_b");
    mNitrogen_3a = resp.valueDouble("class_3_a");
    mNitrogen_3b = resp.valueDouble("class_3_b");
    if (mNitrogen_1a*mNitrogen_1b*mNitrogen_2a*mNitrogen_2b*mNitrogen_3a*mNitrogen_3b == 0)
        throw IException("at least one parameter of model.species.nitrogenResponseClasses is not valid (value=0)!");

    // setup CO2 response
    XmlHelper co2(xml.node("model.species.CO2Response"));
    mCO2base = co2.valueDouble("baseConcentration");
    mCO2comp = co2.valueDouble("compensationPoint");
    mCO2beta0 = co2.valueDouble("beta0");
    mCO2p0 = co2.valueDouble("p0");
    if (mCO2base*mCO2comp*(mCO2base-mCO2comp)*mCO2beta0*mCO2p0==0)
        throw IException("at least one parameter of model.species.CO2Response is not valid!");

    return mSpecies.count();

}


/** retrieves variables from the datasource available during the setup of species.
  */
QVariant SpeciesSet::var(const QString& varName)
{
    Q_ASSERT(mSetupQuery!=0);

    int idx = mSetupQuery->record().indexOf(varName);
    if (idx>=0)
        return mSetupQuery->value(idx);
    throw IException(QString("SpeciesSet: variable not set: %1").arg(varName));
    //throw IException(QString("load species parameter: field %1 not found!").arg(varName));
    // lookup in defaults
    //qDebug() << "variable" << varName << "not found - using default.";
    //return GlobalSettings::instance()->settingDefaultValue(varName);
}

inline double SpeciesSet::nitrogenResponse(const double &availableNitrogen, const double &NA, const double &NB) const
{
    if (availableNitrogen<=NB)
        return 0;
    double x = 1. - exp(NA * (availableNitrogen-NB));
    return x;
}

/// calculate nitrogen response for a given amount of available nitrogen and a respone class
/// for fractional values, the response value is interpolated between the fixedly defined classes (1,2,3)
double SpeciesSet::nitrogenResponse(const double availableNitrogen, const double &responseClass) const
{
    double value1, value2, value3;
    if (responseClass>2.) {
        if (responseClass==3.)
            return nitrogenResponse(availableNitrogen, mNitrogen_3a, mNitrogen_3b);
        else {
            // interpolate between 2 and 3
            value2 = nitrogenResponse(availableNitrogen, mNitrogen_2a, mNitrogen_2b);
            value3 = nitrogenResponse(availableNitrogen, mNitrogen_3a, mNitrogen_3b);
            return value2 + (responseClass-2)*(value3-value2);
        }
    }
    if (responseClass==2)
        return nitrogenResponse(availableNitrogen, mNitrogen_2a, mNitrogen_2b);
    if (responseClass==1)
        return nitrogenResponse(availableNitrogen, mNitrogen_1a, mNitrogen_1b);
    // last ressort: interpolate between 1 and 2
    value1 = nitrogenResponse(availableNitrogen, mNitrogen_1a, mNitrogen_1b);
    value2 = nitrogenResponse(availableNitrogen, mNitrogen_2a, mNitrogen_2b);
    return value1 + (responseClass-1)*(value2-value1);
}

/** calculation for the CO2 response for the ambientCO2 for the water- and nitrogen responses given.
    The calculation follows Friedlingsstein 1995 (see also links to equations in code)
*/
double SpeciesSet::co2Response(const double ambientCO2, const double nitrogenResponse, const double soilWaterResponse) const
{
    if (nitrogenResponse==0)
        return 0.;

    double co2_water = 2. - soilWaterResponse;
    double beta = mCO2beta0 * co2_water * nitrogenResponse;

    double r =1. +  M_LN2 * beta; // NPP increase for a doubling of atmospheric CO2 (Eq. 17)

    // fertilization function (cf. Farquhar, 1980) based on Michaelis-Menten expressions
    double deltaC = mCO2base - mCO2comp;
    double K2 = ((2*mCO2base - mCO2comp) - r*deltaC ) / ((r-1.)*deltaC*(2*mCO2base - mCO2comp)); // Eq. 16
    double K1 = (1. + K2*deltaC) / deltaC;

    double response = mCO2p0 * K1*(ambientCO2 - mCO2comp) / (1 + K2*(ambientCO2-mCO2comp)); // Eq. 16
    return response;

}


