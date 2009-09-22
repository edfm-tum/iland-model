#ifndef OUTPUT_H
#define OUTPUT_H

enum OutputDatatype { OutInteger, OutDouble, OutString };
enum OutputMode { OutDatabase, OutFile, OutText };

class XmlHelper;
class Output;
class OutputColumn
{
public:
    OutputColumn(const QString name, const QString description, const OutputDatatype datatype):
            mName(name), mDescription(description), mDatatype(datatype) {}
    static OutputColumn year() { return OutputColumn("year", "simulation year", OutInteger); }
    static OutputColumn species() { return OutputColumn("species", "tree species", OutString); }
    static OutputColumn ru() { return OutputColumn("ru", "id of ressource unit", OutInteger); }
private:
    QString mName;
    QString mDescription;
    OutputDatatype mDatatype;
friend class Output;
};

class Output
{
public:
    Output(); ///< ctor. Override in derived class to craete columns, etc.
    virtual ~Output();
    virtual void setup(); ///< setup() is called during project setup and can be ovveridden for specific setup

    void open(); ///< open output connection (create actual db connection, ...)
    const bool isOpen() const { return mOpen; }
    void close(); ///< shut down

    virtual void exec(); ///< execute the output

    // properties
    const QList<OutputColumn> getColumns() const { return mColumns; }

    const QString name() const { return mName;  } ///< descriptive name of the ouptut
    const QString description() const { return mDescription; }
    const QString tableName() const { return mTableName; } ///< internal output name (no spaces allowed)

    // save data
    Output & operator<< ( const double& value ) { add(value); return *this; }
    Output & operator<< ( const int value ) { add(value); return *this; }
    Output & operator<< ( const QString &value ) { add(value); return *this; }
    // transactions
    void startTransaction(); ///< start database transaction (does nothing when in file mode)
    void endTransaction(); ///< ends database transaction (does nothing when in file mode)

protected:
    void setName(const QString &name, const QString tableName) { mName = name; mTableName=tableName; }
    void setDescription(const QString &description) { mDescription=description; }
    QList<OutputColumn> &columns()  { return mColumns; }
    int currentYear() const { return GlobalSettings::instance()->currentYear(); }
    const XmlHelper &settings();
    // add data
    void writeRow(); ///< saves the current row/line of data to database/file. Must be called f

    inline void add(const double &value);
    void add(const double &value1, const double &value2) { add(value1); add(value2); }
    void add(const double &value1, const double &value2, const double &value3) { add(value1, value2); add(value3); }
    void add(const double &value1, const double &value2, const double &value3, const double &value4) { add(value1, value2); add(value3, value4); }
    void add(const double &value1, const double &value2, const double &value3, const double &value4, const double value5) { add(value1, value2); add(value3, value4, value5); }
    inline void add(const int intValue);
    inline void add(const QString &stringValue);

private:
    void newRow(); ///< starts a new row
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
