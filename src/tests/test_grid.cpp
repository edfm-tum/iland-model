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

#include "test_grid.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <cmath>

#include "gisgrid.h"
#include "grid.h"

void TestGrid::setupAndCoordinateMapping()
{
    // 100 x 200m extent with 10m cells -> 10 x 20 cells
    FloatGrid grid(QRectF(0.0, 0.0, 100.0, 200.0), 10.0f);
    QVERIFY(!grid.isEmpty());
    QCOMPARE(grid.sizeX(), 10);
    QCOMPARE(grid.sizeY(), 20);
    QCOMPARE(grid.count(), 200);

    // Cell (3, 7) center is (35.0, 75.0)
    const QPoint idx(3, 7);
    const QPointF center = grid.cellCenterPoint(idx);
    QCOMPARE(center.x(), 35.0);
    QCOMPARE(center.y(), 75.0);
    QCOMPARE(grid.indexAt(center), idx);

    // Boundary validity checks
    QVERIFY(grid.coordValid(0.0f, 0.0f));
    QVERIFY(grid.coordValid(99.9f, 199.9f));
    QVERIFY(!grid.coordValid(100.0f, 50.0f));
    QVERIFY(!grid.coordValid(-0.1f, 50.0f));
}

void TestGrid::downsamplingIndexHelpers()
{
    // Verify fast integer downsampling from a 2m LIF grid (100x100m = 50x50 cells)
    // to 4m (index2), 10m HeightGrid (index5), and 20m SeedMap (index10)
    const QRectF extent(0.0, 0.0, 100.0, 100.0);
    FloatGrid g2m(extent, 2.0f);
    FloatGrid g4m(extent, 4.0f);
    FloatGrid g10m(extent, 10.0f);
    FloatGrid g20m(extent, 20.0f);

    for (int i = 0; i < g2m.count(); ++i) {
        const QPointF pt = g2m.cellCenterPoint(i);
        QCOMPARE(g2m.index2(i),  g4m.index(g4m.indexAt(pt)));
        QCOMPARE(g2m.index5(i),  g10m.index(g10m.indexAt(pt)));
        QCOMPARE(g2m.index10(i), g20m.index(g20m.indexAt(pt)));
    }
}

void TestGrid::wipeAndInitialize()
{
    FloatGrid fg(10.0f, 5, 4);
    fg.initialize(3.5f);
    QCOMPARE(fg.constValueAtIndex(0, 0), 3.5f);
    QCOMPARE(fg.constValueAtIndex(4, 3), 3.5f);

    fg.wipe();
    QCOMPARE(fg.sum(), 0.0f);

    // wipe(value) with non-zero float and int values
    fg.wipe(1.5f);
    for (int i = 0; i < fg.count(); ++i) {
        QCOMPARE(fg[i], 1.5f);
    }

    Grid<int> ig(10.0f, 4, 4);
    ig.wipe(42);
    for (int i = 0; i < ig.count(); ++i) {
        QCOMPARE(ig[i], 42);
    }
}

void TestGrid::statisticsAndModifiers()
{
    Grid<double> g(10.0f, 2, 2);
    g.valueAtIndex(0, 0) = 1.0;
    g.valueAtIndex(1, 0) = 3.0;
    g.valueAtIndex(0, 1) = 5.0;
    g.valueAtIndex(1, 1) = 7.0;

    QCOMPARE(g.min(), 1.0);
    QCOMPARE(g.max(), 7.0);
    QCOMPARE(g.sum(), 16.0);
    QCOMPARE(g.avg(), 4.0);

    g.add(2.0);
    QCOMPARE(g.avg(), 6.0);

    g.limit(4.0, 8.0);
    QCOMPARE(g.min(), 4.0);
    QCOMPARE(g.max(), 8.0);

    // 2x2 averaging down to 1x1
    Grid<double> coarse = g.averaged(2);
    QCOMPARE(coarse.sizeX(), 1);
    QCOMPARE(coarse.sizeY(), 1);
    QCOMPARE(coarse.constValueAtIndex(0, 0), g.avg());
}

void TestGrid::gridRunnerAndFloodFill()
{
    Grid<int> g(1.0f, 5, 5);
    g.wipe();

    // Iterate over a 3x2 inner sub-rectangle [1..3] x [2..3]
    GridRunner<int> runner(g, QRect(QPoint(1, 2), QPoint(4, 4)));
    int visited = 0;
    while (int *cell = runner.next()) {
        *cell = 1;
        ++visited;
    }
    QCOMPARE(visited, 6);
    QCOMPARE(g.sum(), 6);

    // Neighbor check at SW corner (0, 0): south and west must be null
    GridRunner<int> fullRunner(&g);
    QVERIFY(fullRunner.next() != nullptr);
    int *nb4[4] = {nullptr, nullptr, nullptr, nullptr};
    fullRunner.neighbors4(nb4);
    QVERIFY(nb4[0] != nullptr); // north (0, 1)
    QVERIFY(nb4[1] != nullptr); // east  (1, 0)
    QVERIFY(nb4[2] == nullptr); // west  out of bounds
    QVERIFY(nb4[3] == nullptr); // south out of bounds

    // Flood-fill the 6 connected cells of value 1 with color 9
    const int filled = g.floodFill(QPoint(1, 2), 1, 9);
    QCOMPARE(filled, 6);
    QCOMPARE(g.constValueAtIndex(3, 3), 9);
    QCOMPARE(g.constValueAtIndex(0, 0), 0);
}

void TestGrid::asciiRasterRoundTrip()
{
    setupGISTransformation(0.0, 0.0, 0.0, 0.0);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString ascPath = tmpDir.filePath("test_grid.asc");

    FloatGrid src(QRectF(100.0, 200.0, 30.0, 20.0), 10.0f); // 3 x 2 cells
    src.initialize(12.5f);
    src.valueAtIndex(0, 0) = 1.25f;
    src.valueAtIndex(2, 1) = FloatGrid::nullValue(); // NODATA cell

    QVERIFY(gridToFile(src, ascPath));

    FloatGrid loaded;
    QVERIFY(loaded.loadGridFromFile(ascPath));
    QCOMPARE(loaded.sizeX(), 3);
    QCOMPARE(loaded.sizeY(), 2);
    QCOMPARE(loaded.cellsize(), 10.0f);
    QCOMPARE(loaded.metricRect(), src.metricRect());
    QVERIFY(std::abs(loaded.constValueAtIndex(0, 0) - 1.25f) < 1e-5f);
    QVERIFY(std::abs(loaded.constValueAtIndex(1, 0) - 12.5f) < 1e-5f);
    QVERIFY2(loaded.isNull(loaded.constValueAtIndex(2, 1)),
             "NODATA cell must survive ESRI ASCII export/import as Grid::nullValue()");
}
