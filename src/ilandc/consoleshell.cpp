/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
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

#include "consoleshell.h"

#include <QtCore>
#include <QKeyEvent>

#include "global.h"
#include "model.h"
#include "modelcontroller.h"

QTextStream *ConsoleShell::mLogStream = 0;

// a try to really get keyboard strokes in console mode...
// did not work.
class KeyboardTaker : public QThread
 {
 public:
     void run();
     bool stop;
 };

 void KeyboardTaker::run()
 {
     stop = false;
     QTextStream qin(stdin, QFile::ReadOnly);
     while (!stop) {
         QString input = qin.read(1);
         if (!input.isNull()) {
             // .. process input
             qWarning() << "input:" << input;
         }
     }
 }

ConsoleShell::ConsoleShell()
{
}

/*
*/

void ConsoleShell::run()
{

    QString xml_name = QCoreApplication::arguments().at(1);
    if (!QFile::exists(xml_name)) {
        qDebug() << "invalid XML project file: " << xml_name;
        return;
    }
    // get the number of years to run...
    bool ok;
    int years = QCoreApplication::arguments().at(2).toInt(&ok);
    if (years<0 || !ok) {
        qDebug() << QCoreApplication::arguments().at(2) << "is an invalid number of years to run!";
        return;
    }

    try {

        ModelController iland_model;
        QObject::connect(&iland_model, SIGNAL(year(int)),SLOT(runYear(int)));
        iland_model.setFileName(xml_name);

        setupLogging();

        qDebug() << "**************************************************";
        qDebug() << "***********     iLand console session     ********";
        qDebug() << "**************************************************";
        qDebug() << "started at: " << QDateTime::currentDateTime();
        qDebug() << "**************************************************";

        qWarning() << "**************************************************";
        qWarning() << "*** creating model...";
        qWarning() << "**************************************************";

        iland_model.create();
        qWarning() << "**************************************************";
        qWarning() << "*** running model for" << years << "years";
        qWarning() << "**************************************************";

        iland_model.run(years + 1);

        qWarning() << "**************************************************";
        qWarning() << "*** model run finished.";
        qWarning() << "*** " << QDateTime::currentDateTime();
        qWarning() << "**************************************************";

    } catch (const IException &e) {
        qWarning() << "*** An exception occured ***";
        qWarning() << e.message();
    }
    catch (const std::exception &e) {
        qWarning() << "*** An exception occured ***";
        qWarning() << e.what();
    }
    QCoreApplication::quit();


}

void ConsoleShell::runYear(int year)
{
    printf("simulating year %d ...\n", year-1);
}

void myMessageOutput(QtMsgType type, const char *msg)
 {

    //QMutexLocker m(&qdebug_mutex);
    switch (type) {
     case QtDebugMsg:
        *ConsoleShell::logStream() << msg << endl;
         break;
     case QtWarningMsg:
        *ConsoleShell::logStream() << msg << endl;
        printf("Warning: %s\n", msg);

         break;
     case QtCriticalMsg:
        *ConsoleShell::logStream() << msg << endl;
        printf("Critical: %s\n", msg);
         break;
     case QtFatalMsg:
        *ConsoleShell::logStream() << msg << endl;
        printf("Fatal: %s\n", msg);
     }
 }


void ConsoleShell::setupLogging()
{
    if (mLogStream) {
        if (mLogStream->device())
            delete mLogStream->device();
        delete mLogStream;
        mLogStream = NULL;
    }

    QString fname = GlobalSettings::instance()->settings().value("system.logging.logFile", "logfile.txt");
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    fname.replace("$date$", timestamp);
    fname = GlobalSettings::instance()->path(fname, "log");
    QFile *file = new QFile(fname);

    if (!file->open(QIODevice::WriteOnly)) {
        qDebug() << "cannot open logfile" << fname;
    } else {
        qDebug() << "Log output is redirected to logfile" << fname;
        mLogStream = new QTextStream(file);
    }
    qInstallMsgHandler(myMessageOutput);


}
