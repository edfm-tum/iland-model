#ifndef SQLHELPER_H
#define SQLHELPER_H
#include <QtSql>

class SqlHelper
{
public:
    SqlHelper();
    static QVariant queryValue(const QString &query, const QSqlDatabase &database); ///< query a single value from database
    static bool executeSql(const QString &query, const QSqlDatabase &database); ///< execute DML (insert, update, ...)
};

#endif // SQLHELPER_H
