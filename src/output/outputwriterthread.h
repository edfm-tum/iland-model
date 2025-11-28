#ifndef OUTPUTWRITERTHREAD_H
#define OUTPUTWRITERTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QVector>
#include <QVariant>
#include <QSqlDatabase>
#include <QHash>

class QSqlQuery;

struct OutputBatch {
    QString tableName; // For DB output
    QString filePath; // For File output
    QString insertSql; // The prepared statement for DB
    QVector<QVariant> data; // Flattened data
    int columnCount;
    enum Mode { Database, File } mode;
    bool startTransaction;
};

class OutputWriterThread : public QThread
{
    Q_OBJECT
public:
    OutputWriterThread(QObject *parent = nullptr);
    ~OutputWriterThread();

    void addBatch(const OutputBatch &batch);
    void stop();
    void setDatabaseName(const QString &name) { mDatabaseName = name; }
    void resetAbort() { QMutexLocker locker(&mMutex); mAbort = false; }

protected:
    void run() override;

private:
    void processBatch(const OutputBatch &batch);
    void writeToDatabase(const OutputBatch &batch);
    void writeToFile(const OutputBatch &batch);

    QQueue<OutputBatch> mQueue;
    QMutex mMutex;
    QWaitCondition mCondition;
    bool mAbort;

    QString mDatabaseName;
    QSqlDatabase mDatabase;
    QHash<QString, QSqlQuery*> mQueries; // Cache prepared queries
};

#endif // OUTPUTWRITERTHREAD_H
