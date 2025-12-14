#include "outputwriterthread.h"
#include "globalsettings.h"
#include <QtSql>
#include <QFile>
#include <QTextStream>

OutputWriterThread::OutputWriterThread(QObject *parent)
    : QThread(parent), mSemaphore(MAX_QUEUE_SIZE), mAbort(false)
{
}

OutputWriterThread::~OutputWriterThread()
{
    stop();
    wait();
}

void OutputWriterThread::stop()
{
    QMutexLocker locker(&mMutex);
    mAbort = true;
    mCondition.wakeOne();
}

void OutputWriterThread::addBatch(const OutputBatch &batch)
{
    mSemaphore.acquire();
    QMutexLocker locker(&mMutex);
    mQueue.enqueue(batch);
    mCondition.wakeOne();
}

void OutputWriterThread::run()
{
    // Setup DB connection for this thread
    // We use a unique connection name
    QString connectionName = "output_thread_connection";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(mDatabaseName);
        // Copy connect options if necessary
        if (!db.open()) {
            qWarning() << "OutputWriterThread: Failed to open database connection:" << db.lastError().text();
            return;
        }
        // Optimization pragmas
        QSqlQuery q(db);
        q.exec("PRAGMA synchronous = OFF");
        q.exec("PRAGMA journal_mode = WAL;");
    }
    mDatabase = QSqlDatabase::database(connectionName);

    qDebug() << "OutputWriterThread: Thread ID:" << QThread::currentThreadId();
    qDebug() << "OutputWriterThread: Database file:" << mDatabase.databaseName();
    qDebug() << "OutputWriterThread: Visible tables:" << mDatabase.tables();

    forever {
        OutputBatch batch;
        {
            QMutexLocker locker(&mMutex);
            if (mQueue.isEmpty() && mAbort) {
                break;
            }
            if (mQueue.isEmpty()) {
                mCondition.wait(&mMutex);
                if (mQueue.isEmpty() && mAbort)
                    break;
            }
            if (!mQueue.isEmpty())
                batch = mQueue.dequeue();
        }

        if (!batch.data.isEmpty()) {
            processBatch(batch);
            mSemaphore.release();
        }
    }

    // Cleanup
    qDeleteAll(mQueries);
    mQueries.clear();
    mDatabase.close();
    mDatabase = QSqlDatabase(); // Invalidate to drop reference
    QSqlDatabase::removeDatabase(connectionName);
    qDebug() << "OutputWriterThread: shutting down, all data processed.";
}

void OutputWriterThread::processBatch(const OutputBatch &batch)
{
    if (batch.mode == OutputBatch::Database) {
        writeToDatabase(batch);
    } else {
        writeToFile(batch);
    }
}

void OutputWriterThread::writeToDatabase(const OutputBatch &batch)
{
    if (!mDatabase.isOpen()) return;

    QSqlQuery *query = mQueries.value(batch.tableName, nullptr);
    if (!query) {
        query = new QSqlQuery(mDatabase);
        if (!query->prepare(batch.insertSql)) {
            qWarning() << "OutputWriterThread: Prepare failed for" << batch.tableName << query->lastError().text();
            return;
        }
        mQueries.insert(batch.tableName, query);
    }

    if (batch.startTransaction)
        mDatabase.transaction();

    int rows = batch.data.size() / batch.columnCount;
    for (int i = 0; i < rows; ++i) {
        for (int c = 0; c < batch.columnCount; ++c) {
            query->bindValue(c, batch.data[i * batch.columnCount + c]);
        }
        if (!query->exec()) {
             qWarning() << "OutputWriterThread: Insert failed for" << batch.tableName << query->lastError().text();
        }
    }

    if (batch.startTransaction)
        mDatabase.commit();
}

void OutputWriterThread::writeToFile(const OutputBatch &batch)
{
    QFile file(batch.filePath);
    // Open in Append mode
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "OutputWriterThread: Cannot open file" << batch.filePath;
        return;
    }
    QTextStream out(&file);
    int rows = batch.data.size() / batch.columnCount;
    for (int i = 0; i < rows; ++i) {
        for (int c = 0; c < batch.columnCount; ++c) {
            out << batch.data[i * batch.columnCount + c].toString();
            if (c < batch.columnCount - 1)
                out << ";";
        }
        out << Qt::endl;
    }
}
