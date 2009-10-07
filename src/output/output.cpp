#include "global.h"
#include "output.h"
#include <QtCore>
#include <QtSql>


/** @class Output
   The Output class abstracts output data (database, textbased, ...).
   To create a new output, create a class derived from Output and perform the following steps:
   - Overwrite constructor:
     Create columns and set fixed properties (e.g. table name)
   - overwrite setup()
     this function is called after the project file is read. You can access a XmlHelper calling settings()
     which is set to the top-node of the output (defined by tableName() set in contstructor). Access settings
     using relative xml-pathes (see example).
   - overwrite exec()
     add data using the stream operators or add() function of Output. Call writeRow() after each row. Each invokation
     of exec() is a database transaction.
   - Add the output to the constructor of @c OutputManager

   @par Example
   @code
   // (1) Overwrite constructor and set name, description and columns
   TreeOut::TreeOut()
    {
        setName("Tree Output", "tree");
        setDescription("Output of indivdual trees.");
        columns() << OutputColumn("id", "id of the tree", OutInteger)
                 << OutputColumn("name", "tree species name", OutString)
                 << OutputColumn("v1", "a double value", OutDouble);
     }
    // (2) optionally: some special settings (here: filter)
    void TreeOut::setup()
    {
        QString filter = settings().value(".filter","");
        if (filter!="")
            mFilter = QSharedPointer<Expression>(new Expression(filter));
    }

    // (3) the execution
    void TreeOut::exec()
    {
        AllTreeIterator at(GlobalSettings::instance()->model());
        while (Tree *t=at.next()) {
            if (mFilter && !mFilter->execute()) // skip if filter present
                continue;
            *this << t->id() << t->species()->id() << t->dbh(); // stream operators
            writeRow(); // executes DB insert
        }
    }
    // in outputmanager.cpp:
    OutputManager::OutputManager()
    {
        ...
        mOutputs.append(new TreeOut); // add the output
        ...
    }
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
    mEnabled = false;
    mTransactionOpen = false;
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
    creator.exec(drop); // drop table (if exists)
    creator.exec(sql); // (re-)create table
    creator.exec("delete from " + tableName()); // clear table??? necessary?

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
    endTransaction();
}

void Output::startTransaction()
{
    if (mMode==OutDatabase && mTransactionOpen==false) {
        GlobalSettings::instance()->dbout().transaction();
        mTransactionOpen = true;
    }
}
void Output::endTransaction()
{
    if (mMode==OutDatabase && mTransactionOpen) {
         GlobalSettings::instance()->dbout().commit();
         mTransactionOpen = false;
     }
}


void Output::saveDatabase()
{
   for (int i=0;i<mCount;i++)
        mInserter.bindValue(i,mRow[i]);
    mInserter.exec();
    if (mInserter.lastError().isValid()){
        throw IException(QString("Error during saving of output tables: %1").arg( mInserter.lastError().text()) );
    }

    newRow();
}

