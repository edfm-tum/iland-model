/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include <QCoreApplication>
#include <QtTest/QtTest>
#include <clocale>
#include <cstdio>

#include "globalsettings.h"
#include "test_expression.h"
#include "test_grid.h"

namespace {
struct SuiteEntry {
    const char *name;
    QObject *object;
};
} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    std::setlocale(LC_ALL, "C");

    // Initialize a clean JS engine context for headless unit tests
    GlobalSettings::instance()->resetScriptEngine();

    TestExpression testExpression;
    TestGrid testGrid;

    const QVector<SuiteEntry> suites = {
        {"TestExpression", &testExpression},
        {"TestGrid",       &testGrid},
    };

    const QStringList args = app.arguments();
    if (args.contains("--list")) {
        printf("Available test suites:\n");
        for (const auto &s : suites)
            printf("  %s\n", s.name);
        return 0;
    }

    // Optional first positional argument filters to a single suite (e.g. ./iland_tests TestExpression -v2)
    QString suiteFilter;
    QStringList qtestArgs;
    qtestArgs << args.first();
    for (int i = 1; i < args.size(); ++i) {
        if (i == 1 && !args[i].startsWith('-')) {
            bool matchesSuite = false;
            for (const auto &s : suites) {
                if (QString::fromLatin1(s.name).compare(args[i], Qt::CaseInsensitive) == 0 ||
                    QString::fromLatin1(s.name).mid(4).compare(args[i], Qt::CaseInsensitive) == 0) {
                    suiteFilter = QString::fromLatin1(s.name);
                    matchesSuite = true;
                    break;
                }
            }
            if (matchesSuite)
                continue;
        }
        qtestArgs << args[i];
    }

    // Convert filtered arguments back to argc/argv for QTest::qExec
    QVector<QByteArray> rawArgs;
    QVector<char*> argvPtrs;
    for (const QString &a : qtestArgs)
        rawArgs.push_back(a.toLocal8Bit());
    for (QByteArray &ba : rawArgs)
        argvPtrs.push_back(ba.data());

    int failedSuites = 0;
    int executedSuites = 0;
    for (const auto &s : suites) {
        if (!suiteFilter.isEmpty() && suiteFilter != QLatin1String(s.name))
            continue;
        ++executedSuites;
        const int rc = QTest::qExec(s.object, argvPtrs.size(), argvPtrs.data());
        if (rc != 0)
            ++failedSuites;
    }

    printf("\n==========================================================\n");
    printf("iLand Unit Tests Summary: %d/%d suites passed\n",
           executedSuites - failedSuites, executedSuites);
    printf("==========================================================\n");

    return failedSuites == 0 ? 0 : 1;
}
