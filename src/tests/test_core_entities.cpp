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

#include "test_core_entities.h"

#include <QFile>
#include <QJSEngine>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QtTest>
#include <cmath>

#include "activity.h"
#include "exception.h"
#include "fmstand.h"
#include "fmunit.h"
#include "fomescript.h"
#include "globalsettings.h"
#include "saplings.h"
#include "scheduler.h"
#include "scriptglobal.h"
#include "snag.h"
#include "soil.h"
#include "timeevents.h"
#include "xmlhelper.h"

namespace {
QString writeProjectXml(const QTemporaryDir &dir, const QString &name, const QString &body)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        ts << body;
    }
    return path;
}

class DummyActivity : public ABE::Activity
{
public:
    DummyActivity() : ABE::Activity(nullptr)
    {
        schedule().clear();
        schedule().topt = 50;
    }
};
} // namespace

void TestCoreEntities::saplingCellStateAndSlots()
{
    SaplingTree t;
    t.setSapling(0.8f, 3, 2);
    t.set_sprout(true);
    t.set_browsed(true);
    QVERIFY(t.is_occupied());
    QVERIFY(t.is_sprout());
    QVERIFY(t.is_browsed());
    t.set_browsed(false);
    QVERIFY(!t.is_browsed());
    QVERIFY(t.is_sprout());
    t.clear();
    QVERIFY(!t.is_occupied());

    // Red -> Green Bug #2: SaplingCell::checkState() swaps CellEmpty and CellFree
    SaplingCell cell;
    QCOMPARE(cell.state, SaplingCell::CellInvalid);
    cell.checkState();
    QCOMPARE(cell.state, SaplingCell::CellInvalid);

    // 0 saplings on a valid cell -> CellEmpty
    cell.state = SaplingCell::CellEmpty;
    cell.checkState();
    QCOMPARE(cell.state, SaplingCell::CellEmpty);
    QVERIFY(cell.hasFreeSlots());

    // 1..4 saplings <= 1.3m -> CellFree
    QVERIFY(cell.addSapling(0.6f, 1, 0) != nullptr);
    cell.checkState();
    QCOMPARE(cell.state, SaplingCell::CellFree);
    QVERIFY(cell.hasFreeSlots());
    QCOMPARE(cell.n_occupied(), 1);
    QVERIFY(cell.has_new_saplings());
    QVERIFY(cell.saplingOfSpecies(0) != nullptr);
    QVERIFY(cell.saplingOfSpecies(4) == nullptr);

    // Fill all 5 slots (<= 1.3m) -> 6th rejected, checkState() -> CellFull
    for (int i = 1; i < NSAPCELLS; ++i)
        QVERIFY(cell.addSapling(0.5f + 0.1f * i, 2, i) != nullptr);
    QVERIFY(cell.addSapling(0.4f, 1, 0) == nullptr);
    cell.checkState();
    QCOMPARE(cell.state, SaplingCell::CellFull);
    QVERIFY(!cell.hasFreeSlots());

    // Single tall sapling > 1.3m -> CellFull even with 4 open slots
    for (int i = 0; i < NSAPCELLS; ++i)
        cell.saplings[i].clear();
    cell.state = SaplingCell::CellEmpty;
    QVERIFY(cell.addSapling(1.5f, 5, 1) != nullptr);
    cell.checkState();
    QCOMPARE(cell.state, SaplingCell::CellFull);
    QCOMPARE(cell.max_height(), 1.5f);
    QVERIFY(!cell.has_new_saplings());

    // Clear all saplings -> back to CellEmpty
    cell.saplings[0].clear();
    cell.checkState();
    QCOMPARE(cell.state, SaplingCell::CellEmpty);
}

void TestCoreEntities::cnPairPoolAndSnagThresholds()
{
    CNPair::setCFraction(0.5);
    CNPair pair;
    pair.addBiomass(1000.0, 50.0);
    QCOMPARE(pair.C, 500.0);
    QCOMPARE(pair.N, 10.0);
    QCOMPARE(pair.CN(), 50.0);
    QCOMPARE(pair.biomass(), 1000.0);
    QVERIFY(pair.isValid());
    QVERIFY(!pair.isEmpty());

    CNPair neg(-1.0, 5.0);
    QVERIFY(!neg.isValid());
    QCOMPARE((pair + CNPair(100.0, 2.0)).C, 600.0);
    QCOMPARE((pair - CNPair(100.0, 2.0)).N, 8.0);
    QCOMPARE((pair * 0.5).C, 250.0);

    // Weighted parameter merging on CNPool
    CNPool p1(100.0, 5.0, 0.2);
    CNPool p2(300.0, 15.0, 0.6);
    QVERIFY(std::abs(p1.parameter(p2) - 0.5) < 1e-12);
    p1 += p2;
    QCOMPARE(p1.C, 400.0);
    QVERIFY(std::abs(p1.parameter() - 0.5) < 1e-12);

    // Non-monotonic decay classes must throw IException
    QVERIFY_THROWS_EXCEPTION(IException, {
        Snag::setupThresholds(10.0, 30.0, 50.0, "0.8, 0.9, 0.4, 0.2");
    });

    Snag::setupThresholds(10.0, 30.0, 50.0, "0.2, 0.4, 0.6, 0.8");
    double minDbh = 0.0, maxDbh = 0.0;
    Snag::snagThresholds(minDbh, maxDbh);
    QCOMPARE(minDbh, 10.0);
    QCOMPARE(maxDbh, 30.0);
}

void TestCoreEntities::soilValidationMassBalanceAndDisturbance()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString xmlPath = writeProjectXml(tmpDir, "soil_proj.xml", R"(<project>
  <system><path><home>.</home></path></system>
  <model>
    <site>
      <somDecompRate>0.02</somDecompRate>
      <soilHumificationRate>0.3</soilHumificationRate>
    </site>
  </model>
</project>
)");
    GlobalSettings::instance()->loadProjectFile(xmlPath);

    Soil soil(nullptr);

    // Invalid kyl
    QVERIFY_THROWS_EXCEPTION(IException, {
        soil.setInitialState(CNPool(1000., 20., 0.), CNPool(5000., 25., 0.1), CNPair(40000., 4000.), 0.5, 0.5);
    });

    // Invalid YL
    QVERIFY_THROWS_EXCEPTION(IException, {
        soil.setInitialState(CNPool(-10., 20., 0.5), CNPool(5000., 25., 0.1), CNPair(40000., 4000.), 0.5, 0.5);
    });

    // Red -> Green Bug #3: invalid YR must throw
    QVERIFY_THROWS_EXCEPTION(IException, {
        soil.setInitialState(CNPool(1000., 20., 0.5), CNPool(-50., 5., 0.1), CNPair(40000., 4000.), 0.5, 0.5);
    });

    // Red -> Green Bug #3: invalid SOM must throw
    QVERIFY_THROWS_EXCEPTION(IException, {
        soil.setInitialState(CNPool(1000., 20., 0.5), CNPool(5000., 25., 0.1), CNPair(40000., -10.), 0.5, 0.5);
    });

    // Valid ICBM/2N annual carbon conservation check
    soil.setInitialState(CNPool(2000., 50., 0.5), CNPool(5000., 25., 0.1), CNPair(40000., 4000.), 0.6, 0.8);
    const double cBefore = soil.totalCarbon(); // 47.0 t/ha
    QCOMPARE(cBefore, 47.0);

    soil.newYear();
    soil.setSoilInput(CNPool(1000., 25., 0.5), CNPool(500., 2.5, 0.1), 600., 400.);
    soil.setClimateFactor(1.0);
    soil.calculateYear();

    const double cInput = 1.5; // 1000 + 500 kg/ha = 1.5 t/ha
    const double cAfter = soil.totalCarbon();
    const double cResp = soil.fluxToAtmosphere().C; // flux is tracked in t/ha (total_before + total_in - total_after)
    QVERIFY2(std::abs((cBefore + cInput) - (cAfter + cResp)) < 1e-9,
             qPrintable(QString("C balance mismatch: before+in=%1, after+resp=%2")
                            .arg(cBefore + cInput).arg(cAfter + cResp)));
    QVERIFY(soil.availableNitrogen() > 0.0);

    // Disturbance removals
    const double yrBefore = soil.youngRefractory().C;
    const double ylBefore = soil.youngLabile().C;
    const double somBefore = soil.oldOrganicMatter().C;
    soil.disturbance(0.5, 0.25, 0.1);
    QVERIFY(std::abs(soil.youngRefractory().C - yrBefore * 0.5) < 1e-9);
    QVERIFY(std::abs(soil.youngLabile().C - ylBefore * 0.75) < 1e-9);
    QVERIFY(std::abs(soil.oldOrganicMatter().C - somBefore * 0.9) < 1e-9);
    const double expectedDisturbed = yrBefore * 0.5 + ylBefore * 0.25 + somBefore * 0.1;
    QVERIFY(std::abs(soil.fluxToDisturbance().C - expectedDisturbed) < 1e-9);
}

void TestCoreEntities::timeEventsXmlAndJsExecution()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString xmlPath = writeProjectXml(tmpDir, "te_proj.xml", R"(<project>
  <system><path><home>.</home></path></system>
  <model>
    <climate><co2concentration>380</co2concentration></climate>
  </model>
</project>
)");
    GlobalSettings::instance()->loadProjectFile(xmlPath);
    GlobalSettings::instance()->resetScriptEngine();
    ScriptGlobal::setupGlobalScripting();

    TimeEvents teBad;
    QVERIFY_THROWS_EXCEPTION(IException, {
        teBad.loadFromString(QStringList({"colA;colB", "1;2"}));
    });

    TimeEvents te;
    QVERIFY(te.loadFromString(QStringList({
        "year;model.climate.co2concentration;javascript",
        "5;420;te_marker = 77",
        "10;450;te_marker = 99"
    })));
    QCOMPARE(te.value(5, "model.climate.co2concentration").toInt(), 420);

    GlobalSettings::instance()->setCurrentYear(5);
    te.run();
    QCOMPARE(GlobalSettings::instance()->settings().valueInt("model.climate.co2concentration", -1, false), 420);
    QCOMPARE(GlobalSettings::instance()->scriptEngine()->globalObject().property("te_marker").toInt(), 77);
    GlobalSettings::instance()->setCurrentYear(0);
}

void TestCoreEntities::abeUnitObjAndSchedulerClearStand()
{
    // Red -> Green Bug #5: UnitObj null guards are inverted (`if (!mStand || mStand->unit())`)
    ABE::UnitObj uoNull;
    QCOMPARE(uoNull.harvestMode(), QString("invalid"));
    QCOMPARE(uoNull.speciesComposition(), QString("invalid"));
    QCOMPARE(uoNull.thinningIntensity(), QString("invalid"));
    QCOMPARE(uoNull.U(), 0.0);
    QCOMPARE(uoNull.MAIChange(), 0.0);
    QCOMPARE(uoNull.MAILevel(), 0.0);

    ABE::FMUnit unit(nullptr);
    unit.setHarvestMode("cablecrane");
    unit.setThinningIntensity(3);
    unit.setU(110.0);
    unit.setAnnualIncrement(8.25);
    unit.setAverageMAI(7.5);

    ABE::FMStand stand(&unit, 1);
    stand.setArea(2.0);
    stand.setU(115.0);

    ABE::UnitObj uoValid;
    uoValid.setStand(&stand);
    QCOMPARE(uoValid.harvestMode(), QString("cablecrane"));
    QCOMPARE(uoValid.thinningIntensity(), QString("high"));
    QCOMPARE(uoValid.U(), 115.0);
    QCOMPARE(uoValid.MAIChange(), 8.25);
    QCOMPARE(uoValid.MAILevel(), 7.5);
    QCOMPARE(uoValid.speciesComposition(), QString("invalid")); // agent() is nullptr -> safe guard

    // Red -> Green Bug #6: Scheduler::clearItemsOfStand skips consecutive items & increments end()
    ABE::FMStand standA(&unit, 10);
    standA.setArea(1.0);
    ABE::FMStand standB(&unit, 20);
    standB.setArea(1.0);
    DummyActivity act;
    ABE::ActivityFlags flagsA(&act);
    ABE::ActivityFlags flagsB(&act);

    ABE::Scheduler *sched = unit.scheduler();
    sched->addTicket(&standA, &flagsA, 0.8, 0.9);
    sched->addTicket(&standA, &flagsA, 0.7, 0.9); // consecutive standA ticket
    sched->addTicket(&standB, &flagsB, 0.6, 0.9);
    sched->addTicket(&standA, &flagsA, 0.5, 0.9); // trailing standA ticket at end()

    const int removed = sched->clearItemsOfStand(&standA);
    QCOMPARE(removed, 3);
    QCOMPARE(sched->scoreOf(10), -1.0);
    QVERIFY(sched->scoreOf(20) > 0.0);

    QCOMPARE(sched->clearItemsOfStand(&standB), 1);
}
