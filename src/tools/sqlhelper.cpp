#include "sqlhelper.h"
#include <QtSql>

SqlHelper::SqlHelper()
{
}

/** execute 'query' against database 'database'. The first column of the first row are returned.
  A Null-Variant is returned, if the query has no results. */
QVariant SqlHelper::queryValue(const QString &query, const QSqlDatabase &database)
{
    QSqlQuery q(database);
    if (!q.exec(query)) {
        qDebug() << "query"<< query << " raised SQL-Error:" << q.lastError().text();
        return QVariant();
    }
    if (q.next()) {
        return q.value(0);
    }
    return QVariant();
}

bool SqlHelper::execQuery(const QString &query, const QSqlDatabase &database)
{
    QSqlQuery q(database);
    bool success = q.exec(query);
    if (!success)
        qDebug() << "query"<< query << " raised SQL-Error:" << q.lastError().text();
    return success;
}
