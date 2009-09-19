#ifndef OUTPUT_H
#define OUTPUT_H

enum OutputDatatype { OutInteger, OutDouble, OutString };
enum OutputMode { OutDatabase, OutFile, OutText };

class XmlHelper;
class Output;
class OutputColumn
{
public:
    OutputColumn(const QString name, const QString description, const OutputDatatype datatype): mName(name), mDescription(description), mDatatype(datatype) {}
private:
    QString mName;
    QString mDescription;
    OutputDatatype mDatatype;
friend class Output;
};

class Output
{
public:
    Output();
    virtual ~Output();
    const QList<OutputColumn> getColumns() const { return mColumns; }
    void setColumns(QList<OutputColumn> columns);
    void close();
    const bool isOpen() const { return mOpen; }

    virtual void exec();
    virtual void setup();
    void save(); ///< saves the data
    // properties
    const QString name() const { return mName;  }
    const QString description() const { return mDescription; }
    const QString tableName() const { return mTableName; }

    // add data
    inline void add(const double &value);
    void add(const double &value1, const double &value2) { add(value1); add(value2); }
    void add(const double &value1, const double &value2, const double &value3) { add(value1, value2); add(value3); }
    void add(const double &value1, const double &value2, const double &value3, const double &value4) { add(value1, value2); add(value3, value4); }
    void add(const double &value1, const double &value2, const double &value3, const double &value4, const double value5) { add(value1, value2); add(value3, value4, value5); }
    inline void add(const int intValue);
    inline void add(const QString &stringValue);
    Output & operator<< ( const double& value ) { add(value); return *this; }
    Output & operator<< ( const int value ) { add(value); return *this; }
    Output & operator<< ( const QString &value ) { add(value); return *this; }
protected:
    void setName(const QString &name, const QString tableName) { mName = name; mTableName=tableName; }
    void setDescription(const QString &description) { mDescription=description; }
    QList<OutputColumn> columns() const { return mColumns; }
    const XmlHelper &settings();
private:
    void newRow(); ///< starts a new row
    void open(); ///< open output connection
    void openDatabase(); ///< database open
    inline void saveDatabase(); ///< database save (does the "insert")
    OutputMode mMode;
    bool mOpen;
    QString mName; ///< name of the output
    QString mTableName; ///< name of the table/output file
    QString mDescription; ///< textual description of the content
    QList<OutputColumn> mColumns; ///< list of columns of output
    QVector<QVariant> mRow; ///< current row
    QSqlQuery mInserter;
    int mCount;
    int mIndex;

};


void Output::add(const double &value)
{
    DBG_IF(mIndex>=mCount || mIndex<0,"Output::add(double)","output index out of range!");
    mRow[mIndex++].setValue(value);
}
void Output::add(const QString &value)
{
    DBG_IF(mIndex>=mCount || mIndex<0,"Output::add(string)","output index out of range!");
    mRow[mIndex++].setValue(value);
}
void Output::add(const int value)
{
    DBG_IF(mIndex>=mCount || mIndex<0,"Output::add(int)","output index out of range!");
    mRow[mIndex++].setValue(value);
}
#endif // OUTPUT_H
