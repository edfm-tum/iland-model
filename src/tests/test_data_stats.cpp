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

#include "test_data_stats.h"

#include <QFile>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QtTest>
#include <cmath>

#include "climateconverter.h"
#include "csvfile.h"
#include "floatingaverage.h"
#include "settingmetadata.h"
#include "sqlhelper.h"
#include "statdata.h"
#include "xmlhelper.h"

void TestDataStats::csvFileDelimitersCommentsAndEditing()
{
    // Semicolon delimited with # header comments, quoted cell, and CRLF endings
    CSVFile csvSemi;
    QVERIFY(csvSemi.loadFromString("# comment line\r\nname;val;flag\r\n\"alpha_1\";12.5;7\r\nbeta;3.0;2\r\n"));
    QCOMPARE(csvSemi.rowCount(), 2);
    QCOMPARE(csvSemi.colCount(), 3);
    QCOMPARE(csvSemi.columnName(0), QString("name"));
    QCOMPARE(csvSemi.columnIndex("val"), 1);
    QCOMPARE(csvSemi.value(0, "name").toString(), QString("alpha_1"));
    QCOMPARE(csvSemi.value(0, 1).toDouble(), 12.5);
    QCOMPARE(csvSemi.column(2), QStringList({"7", "2"}));

    // Comma delimited
    CSVFile csvComma;
    QVERIFY(csvComma.loadFromString("a,b,c\n10,20,30\n40,50,60\n"));
    QCOMPARE(csvComma.rowCount(), 2);
    QCOMPARE(csvComma.colCount(), 3);
    QCOMPARE(csvComma.value(1, "b").toInt(), 50);

    // Tab delimited
    CSVFile csvTab;
    QVERIFY(csvTab.loadFromString("x\ty\n1.5\t2.5\n"));
    QCOMPARE(csvTab.rowCount(), 1);
    QCOMPARE(csvTab.value(0, "y").toDouble(), 2.5);

    // Headerless CSV -> synthetic c0, c1 captions
    CSVFile csvNoCaps;
    csvNoCaps.setHasCaptions(false);
    QVERIFY(csvNoCaps.loadFromString("10;20\n30;40\n"));
    QCOMPARE(csvNoCaps.rowCount(), 2);
    QCOMPARE(csvNoCaps.columnName(0), QString("c0"));
    QCOMPARE(csvNoCaps.columnName(1), QString("c1"));
    QCOMPARE(csvNoCaps.value(1, "c1").toInt(), 40);

    // Edit cell -> saveFile -> loadFile round-trip
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString outPath = tmpDir.filePath("edited.csv");
    csvSemi.setValue(1, 1, 99.5);
    csvSemi.saveFile(outPath);

    CSVFile reloaded;
    QVERIFY(reloaded.loadFile(outPath));
    QCOMPARE(reloaded.rowCount(), 2);
    QCOMPARE(reloaded.colCount(), 3);
    QCOMPARE(reloaded.value(0, "name").toString(), QString("alpha_1"));
    QCOMPARE(reloaded.value(1, "val").toDouble(), 99.5);
}

void TestDataStats::statDataMomentsPercentilesAndRanks()
{
    StatData empty;
    QCOMPARE(empty.sum(), 0.0);
    QCOMPARE(empty.mean(), 0.0);
    QCOMPARE(empty.standardDev(), 0.0);

    QVector<double> v = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    StatData sd(v);
    QCOMPARE(sd.sum(), 40.0);
    QCOMPARE(sd.mean(), 5.0);
    QCOMPARE(sd.min(), 2.0);
    QCOMPARE(sd.max(), 9.0);
    QVERIFY(std::abs(sd.standardDev() - 2.0) < 1e-12);
    // For even N=8, StatData::median() uses index N/2 - 1 = 3 -> 4.0
    QCOMPARE(sd.median(), 4.0);
    QVERIFY(sd.percentile25() <= sd.median());
    QVERIFY(sd.median() <= sd.percentile75());

    QVector<double> odd = {10.0, 2.0, 6.0};
    StatData sdOdd(odd);
    QCOMPARE(sdOdd.median(), 6.0);

    const QVector<double> rankInput = {30.0, 10.0, 20.0};
    // descending=false -> highest value gets rank 1
    QCOMPARE(StatData::calculateRanks(rankInput, false), QVector<int>({1, 3, 2}));
    // descending=true -> lowest value gets rank 1
    QCOMPARE(StatData::calculateRanks(rankInput, true), QVector<int>({3, 1, 2}));

    QVector<double> normVec = {1.0, 3.0, 6.0};
    StatData::normalize(normVec, 100.0);
    QCOMPARE(normVec[0] + normVec[1] + normVec[2], 100.0);
    QCOMPARE(normVec[0], 10.0);
    QCOMPARE(normVec[2], 60.0);
}

void TestDataStats::floatingAverageWindow()
{
    FloatingAverage fa(3);
    QCOMPARE(fa.add(3.0), 3.0);
    QCOMPARE(fa.sum(), 3.0);
    QCOMPARE(fa.add(6.0), 4.5);
    QCOMPARE(fa.sum(), 9.0);
    QCOMPARE(fa.add(9.0), 6.0);
    QCOMPARE(fa.sum(), 18.0);
    // Rollover drops 3.0 -> window {6, 9, 12}
    QCOMPARE(fa.add(12.0), 9.0);
    QCOMPARE(fa.sum(), 27.0);
    QCOMPARE(fa.average(), 9.0);
}

void TestDataStats::climateConverterAndSqlHelper()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString csvPath = tmpDir.filePath("raw_clim.csv");
    const QString dbPath = tmpDir.filePath("clim.sqlite");

    {
        QFile f(csvPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "yr;mo;dy;tk;tmin_k;prec_cm;rad;vpd\n"
           << "2000;1;1;283.15;278.15;0.5;12.0;0.4\n"
           << "2000;1;2;285.15;280.15;1.2;14.5;0.6\n"
           << "2000;1;3;281.15;275.15;0.0;9.5;0.3\n";
    }

    ClimateConverter cc;
    cc.setFileName(csvPath);
    cc.setDatabase(dbPath);
    cc.setTableName("clim01");
    cc.setCaptions(true);
    cc.setYear("c0");
    cc.setMonth("c1");
    cc.setDay("c2");
    cc.setTemp("c3 - 273.15");
    cc.setMinTemp("c4 - 273.15");
    cc.setPrec("c5 * 10");
    cc.setRad("c6");
    cc.setVpd("c7");
    cc.run();

    const QString connName = "test_verify_cc";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(dbPath);
        QVERIFY(db.open());

        QCOMPARE(SqlHelper::queryValue("SELECT COUNT(*) FROM clim01", db).toInt(), 3);
        QVERIFY(std::abs(SqlHelper::queryValue("SELECT temp FROM clim01 WHERE day = 1", db).toDouble() - 10.0) < 1e-6);
        QVERIFY(std::abs(SqlHelper::queryValue("SELECT min_temp FROM clim01 WHERE day = 1", db).toDouble() - 5.0) < 1e-6);
        QVERIFY(std::abs(SqlHelper::queryValue("SELECT prec FROM clim01 WHERE day = 1", db).toDouble() - 5.0) < 1e-6);

        QVERIFY(SqlHelper::executeSql("UPDATE clim01 SET prec = 99 WHERE day = 1", db));
        QCOMPARE(SqlHelper::queryValue("SELECT prec FROM clim01 WHERE day = 1", db).toInt(), 99);
        db.close();
    }
    QSqlDatabase::removeDatabase(connName);
}

void TestDataStats::settingMetaDataXmlSync()
{
    SettingMetaData smd;
    QStringList keys;
    QStringList values;
    smd.loadFromFile(":/project_file_metadata.txt", keys, values);
    QVERIFY(!keys.isEmpty());
    QCOMPARE(keys.size(), values.size());

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString xmlPath = tmpDir.filePath("minimal_project.xml");
    {
        QFile f(xmlPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "<project><system/></project>\n";
    }

    const QStringList checkKeys = {"system.path.home", "model.world.cellSize"};
    const QStringList missing = smd.checkXMLKeys(xmlPath, checkKeys);
    QCOMPARE(missing, checkKeys);

    smd.updateXMLFile(xmlPath, missing);
    const QStringList afterUpdate = smd.checkXMLKeys(xmlPath, checkKeys);
    QVERIFY(afterUpdate.isEmpty());

    XmlHelper verifyXml(xmlPath);
    QVERIFY(verifyXml.hasNode("system.path.home"));
    QVERIFY(verifyXml.hasNode("model.world.cellSize"));
}
