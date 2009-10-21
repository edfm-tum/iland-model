#ifndef SQLHELPER_H
#define SQLHELPER_H
#include <QtSql>

class SqlHelper
{
public:
    SqlHelper();
    static QVariant queryValue(const QString &query, const QSqlDatabase &database);
    static bool execQuery(const QString &query, const QSqlDatabase &database);
};

#endif // SQLHELPER_H
