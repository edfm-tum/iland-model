#ifndef CSVFILE_H
#define CSVFILE_H

#include <QObject>

class CSVFile : public QObject
{
    Q_OBJECT
public:
    CSVFile();
    CSVFile(const QString &fileName) { loadFile(fileName);} ///< ctor, load @p fileName.
    // actions
    bool loadFile(const QString &fileName); ///< load @p fileName. load the complete file at once.
    bool openFile(const QString &fileName); ///< open file in streaming mode.
    QVariant cell(const int row, const int col); ///< get value of cell denoted by @p row and @p cell. Not available in streaming mode.
    QVariant value(const int col); ///< get value of column with index @p col. Use in streaming mode.
    bool next(); ///< advance to next record (i.e. line). return false if end of file is reached.

    // properties
    bool streamingMode() const { return mStreamingMode; }
    bool hasCaptions() const { return mHasCaptions; }
    int rowCount() const { return mRowCount; } ///< number or rows (excl. captions), or -1.
    int colCount() const { return mColCount; } ///< number of columns, or -1
    QString columnName(const int col) { if (col<mColCount) return mCaptions[col]; return QString(); }

    // setters
    void setHasCaptions(const bool hasCaps) { mHasCaptions = hasCaps; }

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
