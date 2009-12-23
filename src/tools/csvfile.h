#ifndef CSVFILE_H
#define CSVFILE_H

#include <QObject>
#include <QtScript>

class CSVFile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool captions WRITE setHasCaptions READ hasCaptions); ///< if true, the first line are considered to be headers
    Q_PROPERTY(bool flat WRITE setFlat READ flat); ///< if true, there is only one column (a flat file)
    Q_PROPERTY(int colCount READ colCount);
    Q_PROPERTY(int rowCount READ rowCount);
public:
    CSVFile(QObject *parent=0);
    CSVFile(const QString &fileName) { loadFile(fileName);} ///< ctor, load @p fileName.
    // actions
    bool openFile(const QString &fileName); ///< open file in streaming mode.
    QVariant colValue(const int col); ///< get value of column with index @p col. Use in streaming mode.
    bool next(); ///< advance to next record (i.e. line). return false if end of file is reached.
    // properties
    bool streamingMode() const { return mStreamingMode; } ///< return true, if in "streaming mode" (for large files)
    bool hasCaptions() const { return mHasCaptions; } ///< true, if first line contains headers
    bool flat() const { return mFlat; } ///< simple list, not multiple columns
    int rowCount() const { return mRowCount; } ///< number or rows (excl. captions), or -1.
    int colCount() const { return mColCount; } ///< number of columns, or -1
    QStringList captions() const { return mCaptions; } ///< retrieve (a copy) of column headers
    QStringList column(const int col) const; ///< retrieve a string list of a given row
    QVariantList values(const int row) const; ///< get a list of the values in row "row"
    // setters
    void setHasCaptions(const bool hasCaps) { mHasCaptions = hasCaps; }
    void setFlat(const bool isflat) { mFlat = isflat; }
    static void addToScriptEngine(QScriptEngine &engine); // called during setup of ScriptEngine
public slots:
    bool loadFile(const QString &fileName); ///< load @p fileName. load the complete file at once.
    bool loadFromString(const QString &content); ///< load from a string.
    QString columnName(const int col) { if (col<mColCount) return mCaptions[col]; return QString(); } ///< get caption of ith column.
    int columnIndex(const QString &columnName) const { return mCaptions.indexOf(columnName); } ///< index of column or -1 if not available
    QVariant value(const int row, const int col) const; ///< get value of cell denoted by @p row and @p cell. Not available in streaming mode.
    QVariant row(const int row); ///< retrieve content of the full row @p row as a Variant

private:
    void clear();
    bool mHasCaptions;
    bool mFlat;
    bool mStreamingMode;
    QStringList mCaptions;
    QStringList mRows;
    QString mSeparator;
    int mRowCount;
    int mColCount;
};

#endif // CSVFILE_H
