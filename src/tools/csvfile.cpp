#include "csvfile.h"
#include <QtCore>
#include "helper.h"

/** @class CSVFile
  Provides access to table data stored in text files (CSV style).
  Tables have optionally headers in first line (hasCaptions()) and can use various
  delimiters ("tab",";",","," "). If separated by spaces, consecuteive spaces are merged.
  Table dimensions can be accessed with colCount() and rowCount(), cell values as QVariant are retrieved
  by value(). full rows are retrieved using row().
  Files are loaded by loadFile() or by passing a filename to the constructor:
  @code
  CSVFile file(fileName);
  for (int row=0; row<file.rowCount(); row++)
     for (int col=0; col<file.colCount(); col++)
       value = file.value(row, col);
  @endcode
  Planned is also a "streaming" mode for large files (loadFile(), while(file.next()) file.value(x) ), but not finsihed yet.

*/
#include <QtScript>
Q_SCRIPT_DECLARE_QMETAOBJECT(CSVFile, QObject*)
void CSVFile::addToScriptEngine(QScriptEngine &engine)
{
    // about this kind of scripting magic see: http://qt.nokia.com/developer/faqs/faq.2007-06-25.9557303148
    QScriptValue cc_class = engine.scriptValueFromQMetaObject<CSVFile>();
    // the script name for the object is "ClimateConverter".
    engine.globalObject().setProperty("CSVFile", cc_class);
}

CSVFile::CSVFile(QObject *parent)
{
    mHasCaptions = true;
    clear();
}

void CSVFile::clear()
{
    mColCount = mRowCount = -1;
    mCaptions.clear();
    mRows.clear();
}

bool CSVFile::loadFile(const QString &fileName)
{
    clear();
    QString content = Helper::loadTextFile(fileName);
    if (content.isEmpty()) {
        qDebug() << "CSVFile::loadFile" << fileName << "does not exist or is empty.";
        return false;
    }
    // split into rows
    mRows = content.split("\n", QString::SkipEmptyParts);
    if (mRows.count()==0)
        return false;

    for (int i=0;i<mRows.count();i++)
        mRows[i] = mRows[i].trimmed();

    // detect separator
    QString first = mRows.first();
    int c_tab = first.count('\t');
    int c_semi = first.count(';');
    int c_comma = first.count(',');
    int c_space = first.count(' ');
    if (c_tab+c_semi+c_comma+c_space == 0) {
        qDebug() << "CSVFile::loadFile: cannot recognize separator. first line:" << first;
        return false;
    }
    mSeparator=" ";
    if (c_tab > c_semi && c_tab>c_comma) mSeparator="\t";
    if (c_semi > c_tab && c_semi>c_comma) mSeparator=";";
    if (c_comma > c_tab && c_comma>c_semi) mSeparator=",";
    if (mSeparator==" ") {
      for (int i=0;i<mRows.count();i++)
        mRows[i] = mRows[i].simplified();
      first = mRows.first();
    }

    // captions
    if (mHasCaptions) {
        mCaptions = first.split(mSeparator);
        mRows.pop_front(); // drop the first line
    } else {
        // create pseudo captions
        mCaptions = first.split(mSeparator);
        for (int i=0;i<mCaptions.count();i++)
            mCaptions[i] = QString("c%1").arg(i);
    }
    mColCount = mCaptions.count();
    mRowCount = mRows.count();
    mStreamingMode = false;
    return true;
}

QVariant CSVFile::value(const int row, const int col)
{
    if (mStreamingMode)
        return QVariant();

    if (row<0 || row>=mRowCount || col<0 || col>mColCount) {
        qDebug() << "CSVFile::value: invalid index: row col:" << row << col << ". Size is:" << mRowCount << mColCount;
        return QVariant();
    }
    QStringList line = mRows[row].split(mSeparator);
    QVariant result;
    if (col<line.count()) {
        result = line[col];
    }
    return result;
}
QVariant CSVFile::row(const int row)
{
    if (mStreamingMode)
        return QVariant();

    if (row<0 || row>=mRowCount) {
        qDebug() << "CSVFile::row: invalid index: row " << row << ". Size is:" << mRowCount ;
        return QVariant();
    }

    QVariant result = mRows[row];
    return result;
}

bool CSVFile::openFile(const QString &fileName)
{
    mStreamingMode = true;
    return false;
}
