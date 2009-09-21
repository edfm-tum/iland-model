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
    newRow();
}


/** create the database table and opens up the output.
  */
void Output::openDatabase()
{
    QSqlDatabase db = GlobalSettings::instance()->dbout();
    // create the "create table" statement
    QString sql = "create table " +mTableName + "(";
    QString insert="insert into " + mTableName + " (";
    QString values;

    foreach(const OutputColumn &col, columns()) {
        switch(col.mDatatype) {
            case OutInteger: sql+=col.mName + " integer"; break;
            case OutDouble: sql+=col.mName + " real"; break;
            case OutString: sql+=col.mName + " text"; break;
        }
        insert+=col.mName+",";
        values+=QString(":")+col.mName+",";

        sql+=",";
    }
    sql[sql.length()-1]=')'; // replace last "," with )
    qDebug()<< sql;
    QSqlQuery creator(db);
    QString drop=QString("drop table if exists %1").arg(tableName());
    creator.exec(drop); // drop table
    creator.exec(sql); // (re-)create table

    if (creator.lastError().isValid()){
        throw IException(QString("Error creating output: %1").arg( creator.lastError().text()) );
    }
    insert[insert.length()-1]=')';
    values[values.length()-1]=')';
    insert += QString(" values (") + values;
    qDebug() << insert;
    mInserter = QSqlQuery(db);
    mInserter.prepare(insert);
    if (mInserter.lastError().isValid()){
        throw IException(QString("Error creating output: %1").arg( mInserter.lastError().text()) );
    }
    for (int i=0;i<columns().count();i++)
        mInserter.bindValue(i,mRow[i]);

    mOpen = true;
}

void Output::newRow()
{
    mIndex = 0;
}



void Output::writeRow()
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
    // setup columns
    mCount = columns().count();
    mRow.resize(mCount);
    newRow();
    // setup output
    switch(mMode) {
        case OutDatabase:
            openDatabase(); break;
        default: throw IException("Invalid output mode");
    }
}

void Output::close()
{

}

void Output::startTransaction()
{
    if (mMode==OutDatabase)
        GlobalSettings::instance()->dbout().transaction();
}
void Output::endTransaction()
{
     if (mMode==OutDatabase)
         GlobalSettings::instance()->dbout().commit();
}


void Output::saveDatabase()
{
   for (int i=0;i<mCount;i++)
        mInserter.bindValue(i,mRow[i]);
    mInserter.exec();
    if (mInserter.lastError().isValid()){
        throw IException(QString("Error during writing output: %1").arg( mInserter.lastError().text()) );
    }

    newRow();
}

