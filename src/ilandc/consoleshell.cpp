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

#include "consoleshell.h"

#include <QtCore>
#include <QtDebug>

#include "global.h"
#include "model.h"
#include "modelcontroller.h"
#include "scriptglobal.h"
#include "version.h"

QTextStream *ConsoleShell::mLogStream = nullptr;
bool ConsoleShell::mFlushLog = false;

ConsoleShell::ConsoleShell()
{
}

ConsoleShell::~ConsoleShell()
{
    closeLogging();
}

void ConsoleShell::run()
{
    const int exit_code = runSimulation();
    closeLogging();
    QCoreApplication::exit(exit_code);
}

int ConsoleShell::runSimulation()
{
    const QStringList args = QCoreApplication::arguments();
    const QString xml_name = args.at(1);

    // Parse number of years to run
    bool ok = false;
    const int years = args.at(2).toInt(&ok);
    if (years < 0 || !ok) {
        fprintf(stderr, "Error: '%s' is an invalid number of years to run!\n", args.at(2).toLocal8Bit().constData());
        return 1;
    }

    if (!QFile::exists(xml_name)) {
        fprintf(stderr, "Error: invalid XML project file: %s\n", xml_name.toLocal8Bit().constData());
        return 1;
    }

    try {
        ModelController iland_model;
        GlobalSettings::instance()->setModelController(&iland_model);
        QObject::connect(&iland_model, SIGNAL(year(int)), SLOT(runYear(int)));
        iland_model.setFileName(xml_name);
        if (iland_model.hasError()) {
            qWarning() << "!!!! ERROR !!!!";
            qWarning() << iland_model.lastError();
            qWarning() << "!!!! ERROR !!!!";
            return 1;
        }

        mTriggers.clear();
        if (args.count() > 3) {
            qWarning() << "set command line values:";
            auto &settings = const_cast<XmlHelper&>(GlobalSettings::instance()->settings());
            bool paths_changed = false;

            for (int i = 3; i < args.count(); ++i) {
                QString line = args.at(i);
                if (line.size() >= 2 && line.startsWith('"') && line.endsWith('"'))
                    line = line.mid(1, line.size() - 2);

                const int eq_pos = line.indexOf('=');
                if (eq_pos <= 0) {
                    qWarning() << "!!!! ERROR !!!!";
                    qWarning() << "Invalid command line option (expected key=value):" << line;
                    qWarning() << "!!!! ERROR !!!!";
                    return 1;
                }

                const QString key = line.left(eq_pos).trimmed();
                QString value = line.mid(eq_pos + 1);
                if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"'))
                    value = value.mid(1, value.size() - 2);

                // Special CLI JavaScript hooks (not XML settings)
                if (key == QLatin1String("onCreate") || key == QLatin1String("onFinish")) {
                    mTriggers.append(qMakePair(key, value));
                    qWarning() << QString("registered CLI trigger '%1': %2").arg(key, value);
                    continue;
                }

                if (!settings.hasNode(key)) {
                    if (!settings.createNode(key)) {
                        qWarning() << "!!!! ERROR !!!!";
                        qWarning() << "Tried to create missing project file key" << key << ", but failed!";
                        qWarning() << "!!!! ERROR !!!!";
                        return 1;
                    }
                    qWarning() << "Note: created missing XML key" << key;
                }
                if (!settings.setNodeValue(key, value)) {
                    qWarning() << "!!!! ERROR !!!!";
                    qWarning() << "Failed to set project file key" << key << "to" << value;
                    qWarning() << "!!!! ERROR !!!!";
                    return 1;
                }
                if (key.startsWith(QLatin1String("system.path.")))
                    paths_changed = true;

                qWarning() << QString("set '%1' to value '%2'. result: '%3'").arg(key, value, settings.value(key));
            }

            if (paths_changed) {
                GlobalSettings::instance()->setupDirectories(settings.node("system.path"),
                                                             QFileInfo(xml_name).absolutePath());
            }
        }

        if (!setupLogging()) {
            fprintf(stderr, "Error: failed to set up logging!\n");
            return 1;
        }

        qDebug() << "**************************************************";
        qDebug() << "***********     iLand console session     ********";
        qDebug() << "**************************************************";

        qWarning() << "*** creating model...";
        qWarning() << "**************************************************";

        iland_model.create();
        if (iland_model.hasError()) {
            qWarning() << "!!!! ERROR !!!!";
            qWarning() << iland_model.lastError();
            qWarning() << "!!!! ERROR !!!!";
            return 1;
        }
        if (!runJavascript("onCreate"))
            return 1;

        qWarning() << "**************************************************";
        qWarning() << "*** running model for" << years << "years";
        qWarning() << "**************************************************";

        iland_model.run(years + 1);
        if (iland_model.hasError()) {
            qWarning() << "!!!! ERROR !!!!";
            qWarning() << iland_model.lastError();
            qWarning() << "!!!! ERROR !!!!";
            return 1;
        }

        // Invoke global JS onFinish() if defined in the loaded script (matching GUI behavior)
        GlobalSettings::instance()->executeJSFunction("onFinish");
        if (!ScriptGlobal::lastErrorMessage().isEmpty()) {
            qWarning() << "!!!! ERROR in onFinish() !!!!";
            qWarning() << ScriptGlobal::lastErrorMessage();
            return 1;
        }

        if (!runJavascript("onFinish"))
            return 1;

        qWarning() << "**************************************************";
        qWarning() << "*** model run finished.";
        qWarning() << "**************************************************";
        return 0;

    } catch (const IException &e) {
        qWarning() << "*** An exception occured ***";
        qWarning() << e.message();
        return 1;
    } catch (const std::exception &e) {
        qWarning() << "*** An (std)exception occured ***";
        qWarning() << e.what();
        return 1;
    }
}

void ConsoleShell::runYear(int year)
{
    printf("\r%s: simulating year %d %s          ",
           QDateTime::currentDateTime().toString("hh:mm:ss").toLocal8Bit().data(),
           year - 1,
           GlobalSettings::instance()->controller()->timeString().toLocal8Bit().data());
}

static QMutex qdebug_mutex;
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    QMutexLocker m(&qdebug_mutex);
    QTextStream *stream = ConsoleShell::logStream();

    switch (type) {
    case QtDebugMsg:
        if (stream) {
            *stream << QTime::currentTime().toString("hh:mm:ss:zzz") << ": " << msg << Qt::endl;
            if (ConsoleShell::flush())
                stream->flush();
        }
        break;
    case QtWarningMsg:
        if (stream) {
            *stream << QTime::currentTime().toString("hh:mm:ss:zzz") << ": " << msg << Qt::endl;
            if (ConsoleShell::flush())
                stream->flush();
        }
        printf("%s: %s\n", QDateTime::currentDateTime().toString("hh:mm:ss").toLocal8Bit().data(), msg.toLocal8Bit().data());
        break;
    case QtInfoMsg:
        if (stream) {
            *stream << QTime::currentTime().toString("hh:mm:ss:zzz") << ": " << msg << Qt::endl;
            if (ConsoleShell::flush())
                stream->flush();
        }
        printf("%s: %s\n", QDateTime::currentDateTime().toString("hh:mm:ss").toLocal8Bit().data(), msg.toLocal8Bit().data());
        break;
    case QtCriticalMsg:
        if (stream) {
            *stream << msg << Qt::endl;
            if (ConsoleShell::flush())
                stream->flush();
        }
        printf("Critical: %s\n", msg.toLocal8Bit().data());
        break;
    case QtFatalMsg:
        if (stream) {
            *stream << msg << Qt::endl;
            if (ConsoleShell::flush())
                stream->flush();
        }
        printf("Fatal: %s\n", msg.toLocal8Bit().data());
        break;
    }
}

bool ConsoleShell::setupLogging()
{
    closeLogging();

    QString fname = GlobalSettings::instance()->settings().value("system.logging.logFile", "logfile.txt");
    mFlushLog = GlobalSettings::instance()->settings().valueBool("system.logging.flush");
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    fname.replace("$date$", timestamp);
    fname = GlobalSettings::instance()->path(fname, "log");

    QDir().mkpath(QFileInfo(fname).absolutePath());
    QFile *file = new QFile(fname);

    if (!file->open(QIODevice::WriteOnly)) {
        qDebug() << "cannot open logfile" << fname;
        delete file;
        return false;
    }
    qDebug() << "Log output is redirected to logfile" << fname;
    mLogStream = new QTextStream(file);
    qInstallMessageHandler(myMessageOutput);
    return true;
}

void ConsoleShell::closeLogging()
{
    qInstallMessageHandler(nullptr);
    QMutexLocker m(&qdebug_mutex);
    if (mLogStream) {
        mLogStream->flush();
        QIODevice *dev = mLogStream->device();
        delete mLogStream;
        mLogStream = nullptr;
        if (dev) {
            dev->close();
            delete dev;
        }
    }
}

bool ConsoleShell::runJavascript(const QString &key)
{
    for (const auto &trigger : mTriggers) {
        if (trigger.first == key) {
            qWarning() << "executing trigger" << key;
            qWarning() << GlobalSettings::instance()->executeJavascript(trigger.second);
            if (!ScriptGlobal::lastErrorMessage().isEmpty()) {
                qWarning() << "!!!! ERROR in CLI trigger" << key << "!!!!";
                qWarning() << ScriptGlobal::lastErrorMessage();
                return false;
            }
        }
    }
    return true;
}
