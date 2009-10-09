#ifndef CSVFILE_H
#define CSVFILE_H

#include <QObject>
#include <QtScript>

class CSVFile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool captions WRITE setHasCaptions READ hasCaptions);
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
    bool streamingMode() const { return mStreamingMode; }
    bool hasCaptions() const { return mHasCaptions; }
    int rowCount() const { return mRowCount; } ///< number or rows (excl. captions), or -1.
    int colCount() const { return mColCount; } ///< number of columns, or -1
    // setters
    void setHasCaptions(const bool hasCaps) { mHasCaptions = hasCaps; }
    static void addToScriptEngine(QScriptEngine &engine);
public slots:
    bool loadFile(const QString &fileName); ///< load @p fileName. load the complete file at once.
    QString columnName(const int col) { if (col<mColCount) return mCaptions[col]; return QString(); } ///< get caption of ith column.
    QVariant value(const int row, const int col); ///< get value of cell denoted by @p row and @p cell. Not available in streaming mode.
    QVariant row(const int row); ///< retrieve content of the full row @p row as a Variant

private:
    void clear();
    bool mHasCaptions;
    bool mStreamingMode;
    QStringList mCaptions;
    QStringList mRows;
    QString mSeparator;
    int mRowCount;
    int mColCount;
};

#endif // CSVFILE_H
