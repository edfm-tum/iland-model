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

#ifndef CONSOLESHELL_H
#define CONSOLESHELL_H
#include <QObject>
#include <QPair>
#include <QList>
#include <QString>

/** ConsoleShell encapsulates the iLand model
    when used in the console application ("ilandc")
*/

class QTextStream;
class ConsoleShell: public QObject
{
    Q_OBJECT
public:
    ConsoleShell();
    ~ConsoleShell() override;
    static QTextStream* logStream() { return mLogStream; }
    static bool flush() { return mFlushLog; }
public slots:
    void run(); // execute the iLand model
    void runYear(int year); // slot called every year
private:
    int runSimulation();
    bool setupLogging();
    void closeLogging();
    bool runJavascript(const QString &key);

    QList<QPair<QString, QString>> mTriggers;
    static bool mFlushLog; // immediately flush output to the logfile
    static QTextStream *mLogStream;
};

#endif // CONSOLESHELL_H
