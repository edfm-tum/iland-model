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

#include "test_script_engine.h"

#include <QFile>
#include <QJSEngine>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QtTest>
#include <cmath>

#include "globalsettings.h"
#include "helper.h"
#include "scriptglobal.h"

namespace {
QString writeScriptProjectXml(const QTemporaryDir &dir)
{
    const QString path = dir.filePath("js_project.xml");
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << "<project>\n"
           << "  <system><path><home>" << dir.path() << "</home></path></system>\n"
           << "  <model>\n"
           << "    <world><location><x>0</x><y>0</y></location></world>\n"
           << "    <site><availableNitrogen>45</availableNitrogen></site>\n"
           << "  </model>\n"
           << "</project>\n";
    }
    return path;
}
} // namespace

void TestScriptEngine::globalsFileIoSettingsAndInclude()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    GlobalSettings::instance()->loadProjectFile(writeScriptProjectXml(tmpDir));
    GlobalSettings::instance()->setCurrentYear(7);
    GlobalSettings::instance()->resetScriptEngine();
    ScriptGlobal::setupGlobalScripting();

    {
        QFile incFile(tmpDir.filePath("helper.js"));
        QVERIFY(incFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&incFile);
        ts << "function tripleVal(x) { return x * 3; }\n";
    }

    const QString script = QString(R"(
        Globals.currentDir = "%1";
        Globals.saveTextFile("note.txt", "hello-iland");
        Globals.set("model.site.availableNitrogen", "82");
        include("helper.js");
        ({
            year: Globals.year,
            rnd: Globals.random(5, 10),
            exists: Globals.fileExists("note.txt"),
            text: Globals.loadTextFile("note.txt"),
            nVal: Globals.setting("model.site.availableNitrogen"),
            tripled: tripleVal(14)
        });
    )").arg(tmpDir.path());

    const QJSValue res = GlobalSettings::instance()->scriptEngine()->evaluate(script);
    QVERIFY2(!res.isError(), qPrintable(res.toString()));
    QCOMPARE(res.property("year").toInt(), 7);
    const double rnd = res.property("rnd").toNumber();
    QVERIFY(rnd >= 5.0 && rnd <= 10.0);
    QVERIFY(res.property("exists").toBool());
    QCOMPARE(res.property("text").toString(), QString("hello-iland"));
    QCOMPARE(res.property("nVal").toString(), QString("82"));
    QCOMPARE(res.property("tripled").toInt(), 42);

    // Embedded ABE JS library resource must be accessible via Qt resource system
    QVERIFY(!Helper::loadTextFile(":/abe-lib/ABE-library.js").isEmpty());

    GlobalSettings::instance()->setCurrentYear(0);
}

void TestScriptEngine::scriptGridOperationsAndGarbageCollection()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    GlobalSettings::instance()->loadProjectFile(writeScriptProjectXml(tmpDir));
    GlobalSettings::instance()->resetScriptEngine();
    ScriptGlobal::setupGlobalScripting();

    const QString script = R"(
        var g = new Grid();
        var okCreate = g.create(4, 4, 10);
        g.setOrigin(100, 200);
        g.setValue(0, 0, 5);
        g.setValueAt(115, 215, 10); // cell (1, 1) relative to (100, 200)
        g.apply("x * 2 + 1");      // cell (0,0)=11, cell (1,1)=21, others=1
        var c = g.copy();
        c.combine("a + b", {a: g, b: c}); // doubles every cell: (0,0)=22, (1,1)=42, others=2
        var preSum = c.sum("x");
        c.aggregate(2);            // 2x2 cells with cellsize 20 (averages 4x4 block values)
        c.save("grid_out.asc");
        var g2 = new Grid("grid_out.asc");
        var out = {
            okCreate: okCreate,
            v00AfterApply: g.value(0, 0),
            v11AfterApply: g.value(1, 1),
            preSum: preSum,
            aggWidth: c.width,
            aggHeight: c.height,
            aggCellsize: c.cellsize,
            loadedWidth: g2.width,
            loadedHeight: g2.height,
            loadedCellsize: g2.cellsize,
            loadedV00: g2.value(0, 0)
        };
        g = null;
        c = null;
        g2 = null;
        out;
    )";

    const QJSValue res = GlobalSettings::instance()->scriptEngine()->evaluate(script);
    QVERIFY2(!res.isError(), qPrintable(res.toString()));
    QVERIFY(res.property("okCreate").toBool());
    QCOMPARE(res.property("v00AfterApply").toNumber(), 11.0);
    QCOMPARE(res.property("v11AfterApply").toNumber(), 21.0);
    // 14 cells * 2 + 22 + 42 = 92
    QCOMPARE(res.property("preSum").toNumber(), 92.0);
    QCOMPARE(res.property("aggWidth").toInt(), 2);
    QCOMPARE(res.property("aggHeight").toInt(), 2);
    QCOMPARE(res.property("aggCellsize").toInt(), 20);
    QCOMPARE(res.property("loadedWidth").toInt(), 2);
    QCOMPARE(res.property("loadedHeight").toInt(), 2);
    QCOMPARE(res.property("loadedCellsize").toInt(), 20);
    // 2x2 block mean of {(0,0)=22, (1,0)=2, (0,1)=2, (1,1)=42} = 68 / 4 = 17.0
    QCOMPARE(res.property("loadedV00").toNumber(), 17.0);

    // Explicit GC must destroy ScriptGrid instances safely without ModelController
    GlobalSettings::instance()->scriptEngine()->collectGarbage();
}

void TestScriptEngine::jsCsvFileAndFactoryObjects()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    GlobalSettings::instance()->loadProjectFile(writeScriptProjectXml(tmpDir));
    GlobalSettings::instance()->resetScriptEngine();
    ScriptGlobal::setupGlobalScripting();

    {
        QFile f(tmpDir.filePath("sample.csv"));
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "name;val\n"
           << "piab;10\n"
           << "fasy;25.5\n";
    }

    const QString script = R"(
        var f1 = Factory.newCSVFile(Globals.path("sample.csv"));
        var f2 = new CSVFile();
        f2.loadFile(Globals.path("sample.csv"));
        ({
            r1: f1.rowCount,
            c1: f1.colCount,
            r2: f2.rowCount,
            s0: f1.jsValue(0, "name"),
            isNumVal: typeof f2.jsValue(1, "val"),
            v1: f2.jsValue(1, "val")
        });
    )";

    const QJSValue res = GlobalSettings::instance()->scriptEngine()->evaluate(script);
    QVERIFY2(!res.isError(), qPrintable(res.toString()));
    QCOMPARE(res.property("r1").toInt(), 2);
    QCOMPARE(res.property("c1").toInt(), 2);
    QCOMPARE(res.property("r2").toInt(), 2);
    QCOMPARE(res.property("s0").toString(), QString("piab"));
    QCOMPARE(res.property("isNumVal").toString(), QString("number"));
    QVERIFY(std::abs(res.property("v1").toNumber() - 25.5) < 1e-12);
}

void TestScriptEngine::loadScriptErrorsAndFunctionStack()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    GlobalSettings::instance()->loadProjectFile(writeScriptProjectXml(tmpDir));
    GlobalSettings::instance()->resetScriptEngine();
    ScriptGlobal::setupGlobalScripting();

    // Missing file must populate lastErrorMessage()
    ScriptGlobal::loadScript(tmpDir.filePath("does_not_exist.js"));
    QVERIFY(!ScriptGlobal::lastErrorMessage().isEmpty());
    QVERIFY(ScriptGlobal::lastErrorMessage().contains("does_not_exist.js"));

    // Syntax error must populate lastErrorMessage() with file and line details
    const QString brokenPath = tmpDir.filePath("broken_syntax.js");
    {
        QFile f(brokenPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "var ok = 1;\nfunction broken( {\n";
    }
    ScriptGlobal::loadScript(brokenPath);
    QVERIFY(!ScriptGlobal::lastErrorMessage().isEmpty());
    QVERIFY(ScriptGlobal::lastErrorMessage().contains("broken_syntax.js"));

    // Simulate Model::loadProject() order: setupGlobalScripting() runs after loadScript()
    // and must not overwrite a pre-existing startup/syntax error message.
    GlobalSettings::instance()->resetScriptEngine();
    ScriptGlobal::loadScript(brokenPath);
    QVERIFY(!ScriptGlobal::lastErrorMessage().isEmpty());
    ScriptGlobal::setupGlobalScripting();
    QVERIFY(!ScriptGlobal::lastErrorMessage().isEmpty());
    QVERIFY(ScriptGlobal::lastErrorMessage().contains("broken_syntax.js"));

    // Valid file clears lastErrorMessage(), and executeJSFunction records errors + stack
    const QString validPath = tmpDir.filePath("throwing_fn.js");
    {
        QFile f(validPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "function failHere() { throw new Error('boom-from-fn'); }\n";
    }
    ScriptGlobal::loadScript(validPath);
    QVERIFY(ScriptGlobal::lastErrorMessage().isEmpty());

    const QString fnRes = ScriptGlobal::executeJSFunction("failHere");
    QVERIFY(!fnRes.isEmpty());
    QVERIFY(ScriptGlobal::lastErrorMessage().contains("boom-from-fn"));
}
