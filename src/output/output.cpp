#include "global.h"
#include "output.h"
#include <QtCore>
#include <QtSql>


/** @class Output
   The Output class abstracts output data (database, textbased, ...).
   @par Setup of Outputs
   @code

   @endcode

*/
const XmlHelper &Output::settings()
{
    return GlobalSettings::instance()->settings();
}
void Output::exec()
{
    qDebug() << "Output::exec() called! (should be overrided!)";
}

void Output::setup()
{
}

Output::~Output()
{
}

Output::Output()
{
    mCount=0;
    mMode = OutDatabase;
    mOpen = false;
}

void Output::setColumns(QList<OutputColumn> columns)
{
    mColumns = columns;
    // setup space
    mCount = columns.count();
    mRow.resize(mCount);

}

/** create the database table and opens up the output.
  */
void Output::openDatabase()
{
    QSqlDatabase db = GlobalSettings::instance()->dbout();
    // create the "create table" statement
    QString sql = "create table tablename(";
    QString insert="insert into tablename (";
    QString values;

    foreach(const OutputColumn &col, columns()) {
        switch(col.mDatatype) {
            case OutInteger: sql+=col.mDatatype + " integer"; break;
            case OutDouble: sql+=col.mDatatype + " real"; break;
            case OutString: sql+=col.mDatatype + " text"; break;
        }
        insert+=col.mDatatype+",";
        values+=QString(":")+col.mDatatype+",";

        sql+=",";
    }
    sql[sql.length()-1]=')'; // replace last "," with )
    qDebug()<< sql;
    QSqlQuery creator(sql, db);
    if (!creator.exec()) {
        throw IException(QString("Error creating output: %1").arg( db.lastError().text()) );
    }
    insert[insert.length()-1]=')';
    values[values.length()-1]=')';
    insert += QString(" values (") + values;
    qDebug() << insert;
    mInserter.prepare(insert);
    for (int i=0;i<columns().count();i++)
        mInserter.bindValue(i,mRow[i]);

    mOpen = true;
}

void Output::newRow()
{
    mIndex = 0;
}



void Output::save()
{
    DBG_IF(mIndex!=mCount, "Output::save()", "received invalid number of values!");
    if (!isOpen())
        open();
    switch(mMode) {
        case OutDatabase:
            saveDatabase(); break;
        default: throw IException("Invalid output mode");
    }

}
void Output::open()
{
    if (isOpen())
        return;
    switch(mMode) {
        case OutDatabase:
            openDatabase(); break;
        default: throw IException("Invalid output mode");
    }
    newRow();
}

void Output::saveDatabase()
{
    mInserter.exec();
    newRow();
}

Output testOutput;
void setupOutput()
{
    QList<OutputColumn> cols;
    cols << OutputColumn("id", "id of the tree", OutInteger)
         << OutputColumn("name", "tree species name", OutString)
         << OutputColumn("v1", "a double value", OutDouble);

    testOutput.setColumns(cols);
}

void writeOutput()
{

    testOutput << 23 << "species1" << 23.3;
    testOutput.save();

}

void testTempOutput()
{
    Output myTest;
    QList<OutputColumn> cols;
    cols << OutputColumn("id", "id of the tree", OutInteger)
         << OutputColumn("name", "tree species name", OutString)
         << OutputColumn("v1", "a double value", OutDouble);

    myTest.setColumns(cols);
    myTest.exec();

}
