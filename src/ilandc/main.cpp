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

#include <QtCore/QCoreApplication>
#include <stdio.h>
#include <QDebug>
#include <QDateTime>
#include <QStringList>
#include "../iland/version.h"
#include "exception.h"
#include <stdexcept>
#include <QTimer>
#include <QString>

#include "consoleshell.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("iLand console (%s - #%s)\n", currentVersion(), svnRevision());
    printf("This is the console version of iLand, \n the individual based landscape and disturbance model.\n");
    printf("More at: http://iland.boku.ac.at \n");
    printf("(c) Werner Rammer, Rupert Seidl, 2009- \n");
    printf("****************************************\n\n");
    if (a.arguments().count()<3) {
        printf("Usage: \n");
        printf("ilandc.exe <xml-project-file> <years> <...other options>\n");
        return 0;
    }
    ConsoleShell iland_shell;

    QTimer::singleShot(0, &iland_shell, SIGNAL(run()));
    a.installEventFilter(&iland_shell);
    return a.exec();
}