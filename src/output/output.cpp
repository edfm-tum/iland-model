/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
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
     which is set to the top-node of the output (defined by tableName() which is set in the constructor). Access settings
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
const GlobalSettings *Output::gl = GlobalSettings::instance();


void Output::exec()
{
    qDebug() << "Output::exec() called! (should be overrided!)";
}

void Output::setup()
{
}

Output::~Output()
{
    //mInserter.clear();
    if (mInserter)
        delete mInserter;
}

Output::Output()
{
    mCount=0;
    mMode = OutDatabase;
    mOpen = false;
    mEnabled = false;
    mInserter = nullptr;
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
    //qDebug()<< sql;
    mInserter = new QSqlQuery(db);
    if (mInserter->isValid())
        mInserter->clear();
    QSqlQuery creator(db);
    QString drop=QString("drop table if exists %1").arg(tableName());
    creator.exec(drop); // drop table (if exists)
    creator.exec(sql); // (re-)create table
    //creator.exec("delete from " + tableName()); // clear table??? necessary?

    if (creator.lastError().isValid()){
        throw IException(QString("Error creating output: %1 \n Statement: %2").arg( creator.lastError().text()).arg(sql) );
    }
    insert[insert.length()-1]=')';
    values[values.length()-1]=')';
    insert += QString(" values (") + values;
    //qDebug() << insert;

    mInserter->prepare(insert);
    if (mInserter->lastError().isValid()){
        throw IException(QString("Error creating output: %1 \n Statement: %2").arg( mInserter->lastError().text()).arg(insert) );
    }
    for (int i=0;i<columns().count();i++)
        mInserter->bindValue(i,mRow[i]);

    mOpen = true;
}

void Output::openFile()
{
    QString path = GlobalSettings::instance()->path(mTableName + ".csv", "output");
    mOutputFile.setFileName(path);
    if (!mOutputFile.open(QIODevice::WriteOnly | QIODevice::Text))
          throw IException(QString("The file '%1' for output '%2' cannot be opened!").arg(path, name()) );

    // create header
    mFileStream.setDevice(&mOutputFile);
    QString line; bool first=true;
    foreach(const OutputColumn &col, columns()) {
        if (first) {
            line = col.name();
            first = false;
        }  else {
            line += ";" + col.name();
        }
    }
    mFileStream << line << Qt::endl;

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
    case OutFile:
            saveFile(); break;
        default: throw IException("Invalid output mode");
    }

}

static QMutex __protectWriteRow;
void Output::singleThreadedWriteRow()
{
    QMutexLocker l(&__protectWriteRow);
    writeRow();
}

/// delete columns that were added after the column 'find_name'
bool Output::clearColumnsAfter(QString find_name)
{
    if (mColumns.isEmpty() || mColumns.last().name() == find_name)
        return false; // nothing to do

    QList<OutputColumn>::iterator i=mColumns.begin();
    while (i!=mColumns.end()) {
        if (i->name()==find_name)
            break;
        ++i;
    }
    mColumns.erase(++i, mColumns.end());
    mCount = mColumns.size();
    return true;
}

void Output::truncateTable()
{
    QSqlDatabase db = GlobalSettings::instance()->dbout();
    QSqlQuery query(db);
    QString stmt=QString("delete from %1").arg(tableName());
    query.exec(stmt); //
    qDebug() << "truncated table" << tableName() << "(=delete all records from output database)";

}

void Output::open()
{
    if (isOpen())
        return;
    // setup columns
    mCount = columns().count();
    mRow.resize(mCount);
    mOpen = true;
    newRow();
    // setup output
    switch(mMode) {
        case OutFile:
            openFile(); break;
        case OutDatabase:
            openDatabase(); break;
        default: throw IException("Invalid output mode");
    }
}

void Output::close()
{
    if (!isOpen())
        return;
    mOpen = false;
    switch (mMode) {
        case OutDatabase:
            // calling finish() ensures, that the query and all locks are freed.
            // having (old) locks on database connections, degrades insert performance.
            if (mInserter->isValid())
                mInserter->finish();
            delete mInserter;
            mInserter = nullptr;
         break;
    case OutFile:
        mOutputFile.close();
        break;
        default:
         qWarning() << "Output::close with invalid mode";
    }

}


void Output::saveDatabase()
{
   for (int i=0;i<mCount;i++)
        mInserter->bindValue(i,mRow[i]);
    mInserter->exec();
    if (mInserter->lastError().isValid()){
        throw IException(QString("Error during saving of output tables: '%1'' (native code: '%2', driver: '%3')")
                         .arg( mInserter->lastError().text())
                         .arg(mInserter->lastError().nativeErrorCode())
                         .arg(mInserter->lastError().driverText()) );
    }

    newRow();
}

void Output::saveFile()
{
    for (int i=0;i<mCount;++i) {
        mFileStream << mRow[i].toString();
        if (i!=mCount-1)
            mFileStream << ";";
    }
    mFileStream << Qt::endl;
    newRow();
}

static QString wikiToMarkdown(const QString &text)
{
    QStringList lines = text.split("\n");
    QStringList resultLines;
    bool inTable = false;
    bool inXml = false;
    QString rootTag;

    for (int i = 0; i < lines.size(); ++i) {
        QString originalLine = lines[i].remove('\r');
        QString line = originalLine.trimmed();

        if (inXml) {
            resultLines.append(originalLine);
            if (line.contains("</" + rootTag + ">")) {
                resultLines.append("```");
                inXml = false;
                rootTag = "";
            }
            continue;
        }

        // Check for start of XML block
        if (line.startsWith("<") && !line.startsWith("</") && !line.startsWith("<!--") && !line.startsWith("<?")) {
            int end = line.indexOf(">");
            if (end != -1) {
                int space = line.indexOf(" ");
                int tagEnd = (space != -1 && space < end) ? space : end;
                QString tagName = line.mid(1, tagEnd - 1);
                if (!tagName.isEmpty()) {
                    inXml = true;
                    rootTag = tagName;
                    if (inTable) {
                        inTable = false;
                    }
                    // Ensure a blank line before the XML block starts
                    if (!resultLines.isEmpty() && !resultLines.last().isEmpty()) {
                        resultLines.append("");
                    }
                    resultLines.append("```xml");
                    resultLines.append(originalLine);

                    // In case the tag is closed on the same line
                    if (line.contains("</" + rootTag + ">")) {
                        resultLines.append("```");
                        inXml = false;
                        rootTag = "";
                    }
                    continue;
                }
            }
        }

        // Handle headings
        if (line.startsWith("!!!")) {
            line = "### " + line.mid(3).trimmed();
        } else if (line.startsWith("!!")) {
            line = "## " + line.mid(2).trimmed();
        } else if (line.startsWith("!")) {
            line = "# " + line.mid(1).trimmed();
        }

        // Handle TikiWiki tables
        if (line.startsWith("||")) {
            inTable = true;
            QString headerLine = line.mid(2);
            bool endsWithDoublePipe = false;
            if (headerLine.endsWith("||")) {
                headerLine = headerLine.left(headerLine.length() - 2);
                endsWithDoublePipe = true;
            }
            QStringList headers = headerLine.split("|");

            // Ensure a blank line before the table starts
            if (!resultLines.isEmpty() && !resultLines.last().isEmpty()) {
                resultLines.append("");
            }

            QString mdHeader = "|";
            QString mdSeparator = "|";
            for (const QString &h : headers) {
                QString cleanH = h.trimmed();
                if (cleanH.startsWith("__") && cleanH.endsWith("__") && cleanH.length() > 4) {
                    cleanH = "**" + cleanH.mid(2, cleanH.length() - 4) + "**";
                } else {
                    cleanH.replace("__", "**");
                }
                cleanH.replace("''", "*");
                cleanH.replace("$", "\\$");
                mdHeader += " " + cleanH + " |";
                mdSeparator += " --- |";
            }
            resultLines.append(mdHeader);
            resultLines.append(mdSeparator);

            if (endsWithDoublePipe) {
                inTable = false;
            }
            continue;
        }

        if (inTable) {
            bool endsWithDoublePipe = false;
            QString rowLine = line;
            if (rowLine.endsWith("||")) {
                rowLine = rowLine.left(rowLine.length() - 2);
                endsWithDoublePipe = true;
            }

            QStringList cells = rowLine.split("|");
            QString mdRow = "|";
            for (const QString &c : cells) {
                QString cleanC = c.trimmed();
                cleanC.replace("''", "*");
                cleanC.replace("__", "**");
                cleanC.replace("$", "\\$");
                mdRow += " " + cleanC + " |";
            }
            resultLines.append(mdRow);

            if (endsWithDoublePipe) {
                inTable = false;
            }
            continue;
        }

        // Handle inline replacements for non-XML/non-table lines
        line.replace("''", "*");
        line.replace("__", "**");
        line.replace("$", "\\$");

        // Append line with proper spacing
        if (line.isEmpty()) {
            if (resultLines.isEmpty() || !resultLines.last().isEmpty()) {
                resultLines.append("");
            }
        } else {
            // Ensure heading or paragraph line has a blank line before it if appropriate
            if (!resultLines.isEmpty()) {
                QString prev = resultLines.last();
                if (!prev.isEmpty()) {
                    resultLines.append("");
                }
            }
            resultLines.append(line);
        }
    }

    if (inXml) {
        resultLines.append("```");
    }

    return resultLines.join("\n");
}

QString Output::wikiFormat() const
{
    QString cleanDesc = wikiToMarkdown(description());
    QString result = QString("## %1\n\n**Table Name:** **%2**\n\n%3\n\n").arg(name(), tableName(), cleanDesc);
    
    // Add columns table
    result += "| **caption** | **datatype** | **description** |\n";
    result += "| --- | --- | --- |\n";
    foreach(const OutputColumn &col, mColumns) {
        QString colDesc = col.description();
        colDesc.replace("''", "*");
        colDesc.replace("__", "**");
        colDesc.replace("$", "\\$"); // Escape dollar sign in column description too
        result += QString("| %1 | %2 | %3 |\n").arg(col.name(), col.datatype(), colDesc);
    }
    return result;
}

