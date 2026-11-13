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

#include "test_xmlhelper.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QtTest>
#include <cmath>

#include "exception.h"
#include "xmlhelper.h"

namespace {
QString writeTempXml(const QTemporaryDir &dir, const QString &name, const QString &content)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << content;
    }
    return path;
}

const char *kSampleXml = R"(<project>
  <system>
    <path>
      <home>.</home>
    </path>
  </system>
  <model>
    <world>
      <cellSize>2</cellSize>
      <buffer>200</buffer>
    </world>
    <site>
      <latitude>47.5</latitude>
      <enabled>true</enabled>
      <disabledStr>false</disabledStr>
      <oneBool>1</oneBool>
      <zeroBool>0</zeroBool>
      <emptyTag/>
    </site>
    <parameter>
      <torus>false</torus>
      <customReal>2.75</customReal>
      <customStr>alpha</customStr>
    </parameter>
  </model>
  <species>
    <item>piab</item>
    <item>fagus</item>
    <item>pinus</item>
  </species>
</project>
)";
} // namespace

void TestXmlHelper::hierarchicalPathsAndTypedGetters()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString xmlPath = writeTempXml(tmpDir, "sample.xml", kSampleXml);

    XmlHelper xml(xmlPath);
    QVERIFY(xml.hasNode("model.world.cellSize"));
    QVERIFY(!xml.hasNode("model.world.doesNotExist"));

    QCOMPARE(xml.value("model.world.cellSize", "", false), QString("2"));
    QCOMPARE(xml.valueInt("model.world.cellSize", -1, false), 2);
    QVERIFY(std::abs(xml.valueDouble("model.site.latitude", 0.0, false) - 47.5) < 1e-12);

    QVERIFY(xml.valueBool("model.site.enabled", false, false));
    QVERIFY(!xml.valueBool("model.site.disabledStr", true, false));
    QVERIFY(xml.valueBool("model.site.oneBool", false, false));
    QVERIFY(!xml.valueBool("model.site.zeroBool", true, false));

    // Self-closing tag & missing key fallbacks
    QCOMPARE(xml.value("model.site.emptyTag", "fallback", false), QString("fallback"));
    QCOMPARE(xml.valueDouble("model.site.emptyTag", 9.25, false), 9.25);
    QCOMPARE(xml.valueInt("model.site.missing", 77, false), 77);
    QVERIFY(xml.valueBool("model.site.missing", true, false));

    // Array indexing [0]..[2]
    QCOMPARE(xml.value("species.item[0]", "", false), QString("piab"));
    QCOMPARE(xml.value("species.item[1]", "", false), QString("fagus"));
    QCOMPARE(xml.value("species.item[2]", "", false), QString("pinus"));
    QCOMPARE(xml.value("species.item[5]", "none", false), QString("none"));
}

void TestXmlHelper::relativeNodesAndSubtreeCtor()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString xmlPath = writeTempXml(tmpDir, "rel.xml", kSampleXml);

    XmlHelper xml(xmlPath);
    xml.setCurrentNode("model.world");
    QVERIFY(xml.isValid());
    QCOMPARE(xml.valueInt(".cellSize", -1, false), 2);
    QCOMPARE(xml.valueInt(".buffer", -1, false), 200);

    XmlHelper siteSub(xml.node("model.site"));
    QVERIFY(siteSub.isValid());
    QCOMPARE(siteSub.valueDouble("latitude", 0.0, false), 47.5);
    QVERIFY(siteSub.valueBool("enabled", false, false));

    xml.setCurrentNode("model.nonExistent");
    QVERIFY(!xml.isValid());
}

void TestXmlHelper::createAndModifyNodes()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString xmlPath = writeTempXml(tmpDir, "edit.xml", kSampleXml);
    const QString savedPath = tmpDir.filePath("saved.xml");

    XmlHelper xml(xmlPath);
    QVERIFY(xml.createNode("model.newSection.alpha"));
    QVERIFY(!xml.createNode("model.newSection.alpha")); // duplicate returns false
    QVERIFY(xml.hasNode("model.newSection.alpha"));

    QVERIFY(xml.setNodeValue("model.newSection.alpha", "42"));
    QCOMPARE(xml.valueInt("model.newSection.alpha", -1, false), 42);

    // Self-closing <emptyTag/> must accept new value
    QVERIFY(xml.setNodeValue("model.site.emptyTag", "filled"));
    QCOMPARE(xml.value("model.site.emptyTag", "", false), QString("filled"));

    // Element with element children cannot be set as scalar text
    QVERIFY(!xml.setNodeValue("model.world", "invalidScalar"));

    QVERIFY(xml.nodeHasChildren("model.world"));
    QVERIFY(!xml.nodeHasChildren("model.world.cellSize"));
    QVERIFY(!xml.dump("model.world").isEmpty());

    xml.saveToFile(savedPath);
    XmlHelper reloaded(savedPath);
    QCOMPARE(reloaded.valueInt("model.newSection.alpha", -1, false), 42);
    QCOMPARE(reloaded.value("model.site.emptyTag", "", false), QString("filled"));
}

void TestXmlHelper::parameterCacheUpdates()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString xmlPath = writeTempXml(tmpDir, "params.xml", kSampleXml);

    XmlHelper xml(xmlPath);
    QCOMPARE(xml.paramValueBool("torus", true), false);
    QVERIFY(std::abs(xml.paramValue("customReal", 0.0) - 2.75) < 1e-12);
    QCOMPARE(xml.paramValueString("customStr", ""), QString("alpha"));

    QVERIFY(xml.setNodeValue("model.parameter.torus", "true"));
    QCOMPARE(xml.paramValueBool("torus", false), true);
}

void TestXmlHelper::invalidXmlThrows()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());

    QVERIFY_THROWS_EXCEPTION(IException, {
        XmlHelper missing(tmpDir.filePath("missing_file.xml"));
    });

    const QString badXmlPath = writeTempXml(tmpDir, "malformed.xml", "<root><unclosed></root>");
    QVERIFY_THROWS_EXCEPTION(IException, {
        XmlHelper malformed(badXmlPath);
    });
}
