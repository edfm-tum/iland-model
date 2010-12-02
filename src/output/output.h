#ifndef OUTPUT_H
#define OUTPUT_H
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtSql/QSqlQuery>

enum OutputDatatype { OutInteger, OutDouble, OutString };
enum OutputMode { OutDatabase, OutFile, OutText };

class XmlHelper;
class Output;
class GlobalSettings;
class OutputColumn
{
public:
    OutputColumn(const QString name, const QString description, const OutputDatatype datatype):
            mName(name), mDescription(description), mDatatype(datatype) {}
    static OutputColumn year() { return OutputColumn("year", "simulation year", OutInteger); }
    static OutputColumn species() { return OutputColumn("species", "tree species", OutString); }
    static OutputColumn ru() { return OutputColumn("ru", "id of ressource unit", OutInteger); }
    const QString &name() const { return mName; }
    const QString &description() const { return mDescription; }
    QString datatype() const { switch(mDatatype) { case OutInteger: return QString("integer"); case OutDouble: return QString("double"); default: return QString("string"); } }
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
    bool isOpen() const { return mOpen; } ///< returns true if output is open, i.e. has a open database connection
    void close(); ///< shut down the connection.
    bool isEnabled() const { return mEnabled; } ///< returns true if output is enabled, i.e. is "turned on"
    void setEnabled(const bool enabled) { mEnabled=enabled; }
    bool isRowEmpty() const { return mIndex==0; } ///< returns true if the buffer of the current row is empty

    virtual void exec(); ///< main function that executes the output

    // properties
    const QList<OutputColumn> getColumns() const { return mColumns; }

    const QString name() const { return mName;  } ///< descriptive name of the ouptut
    const QString description() const { return mDescription; } ///< description of output
    const QString tableName() const { return mTableName; } ///< internal output name (no spaces allowed)
    QString wikiFormat() const; ///< return output description in a (tiki)-wiki format

    // save data
    Output & operator<< ( const double& value ) { add(value); return *this; }
    Output & operator<< ( const int value ) { add(value); return *this; }
    Output & operator<< ( const QString &value ) { add(value); return *this; }

protected:
    void setName(const QString &name, const QString tableName) { mName = name; mTableName=tableName; }
    void setDescription(const QString &description) { mDescription=description; }
    QList<OutputColumn> &columns()  { return mColumns; }
    int currentYear() const { return gl->currentYear(); }
    const XmlHelper &settings() const { return gl->settings(); } ///< access XML settings (see class description)
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
    static const GlobalSettings *gl; ///< pointer to globalsettings object
    void newRow(); ///< starts a new row (resets the internal counter)
    void openDatabase(); ///< database open, create output table and prepare insert statement
    inline void saveDatabase(); ///< database save (exeute the "insert" statement)
    OutputMode mMode;
    bool mOpen;
    bool mEnabled;
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
