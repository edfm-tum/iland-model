#include "global.h"
#include "timeevents.h"
#include "helper.h"
#include "csvfile.h"

TimeEvents::TimeEvents()
{
}
QString lastLoadedFile;
bool TimeEvents::loadFromFile(const QString &fileName)
{
    QString source = Helper::loadTextFile(GlobalSettings::instance()->path(fileName));
    if (source.isEmpty())
        throw IException(QString("TimeEvents: input file does not exist or is empty (%1)").arg(fileName));
    lastLoadedFile=fileName;
    return loadFromString(source);
}

bool TimeEvents::loadFromString(const QString &source)
{
    CSVFile infile;
    infile.loadFromString(source);
    QStringList captions = infile.captions();
    int yearcol = infile.columnIndex("year");
    if (yearcol==-1)
        throw IException(QString("TimeEvents: input file '%1' has no 'year' column.").arg(lastLoadedFile));
    int year;
    QVariantList line;
    QPair<QString, QVariant> entry;
    for (int row=0;row<infile.rowCount();row++) {
        year = infile.value(row, yearcol).toInt();
        line = infile.values(row);
        for (int col=0;col<line.count();col++) {
             if (col!=yearcol) {
                 entry.first = captions[col];
                 entry.second = line[col];
                 mData.insert(year, entry);
             }
        }
    } // for each row
    qDebug() << QString("loaded TimeEvents (file: %1). %2 items stored.").arg(lastLoadedFile).arg(mData.count());
    return true;
}

void TimeEvents::run()
{
    int current_year = GlobalSettings::instance()->currentYear();
    QList<QPair<QString, QVariant> > entries = mData.values(current_year);
    QString key;
    int values_set = 0;
    for (int i=0;i<entries.count();i++) {
        key = entries[i].first; // key
        // special values: if (key=="xxx" ->
        // no special value: a xml node...
        const_cast<XmlHelper&>(GlobalSettings::instance()->settings()).setNodeValue(key, entries[i].second.toString());
        values_set++;
    }
    if (values_set)
        qDebug() << "TimeEvents: year" << current_year << ":" << values_set << "values set.";
}
