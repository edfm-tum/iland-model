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

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QtTest>
#include <cmath>

#include "dem.h"
#include "gisgrid.h"
#include "grid.h"
#include "layeredgrid.h"

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

namespace {
struct TestLayerCell {
    float a;
    float b;
};

class TestLayeredGrid : public LayeredGrid<TestLayerCell>
{
public:
    TestLayeredGrid(const Grid<TestLayerCell> &grid) : LayeredGrid<TestLayerCell>(grid)
    {
        mNames << LayerElement("layerA", "First layer", GridViewRainbow)
               << LayerElement("layerB", "Second layer", GridViewRainbow);
    }

    const QVector<LayerElement> &names() override { return mNames; }

    using LayeredGrid<TestLayerCell>::value;
    double value(const TestLayerCell &data, const int index) const override
    {
        switch (index) {
        case 0: return data.a;
        case 1: return data.b;
        default: return 0.0;
        }
    }

private:
    QVector<LayerElement> mNames;
};
} // namespace

void TestGrid::gisCoordinateTransformsAndGisGrid()
{
    setupGISTransformation(5000.0, 20000.0, 0.0, 37.5);
    const QPointF originModel = GisGrid::worldToModel(QPointF(5000.0, 20000.0));
    QVERIFY(std::abs(originModel.x()) < 1e-9);
    QVERIFY(std::abs(originModel.y()) < 1e-9);

    const QList<QPointF> pts = {
        QPointF(0.0, 0.0),
        QPointF(123.45, 678.9),
        QPointF(-50.0, 25.25)
    };
    for (const QPointF &p : pts) {
        const QPointF back = GisGrid::worldToModel(GisGrid::modelToWorld(p));
        QVERIFY(std::abs(back.x() - p.x()) < 1e-9);
        QVERIFY(std::abs(back.y() - p.y()) < 1e-9);
    }

    setupGISTransformation(100.0, 200.0, 0.0, 90.0);
    const QPointF rotBack = GisGrid::worldToModel(GisGrid::modelToWorld(QPointF(10.0, 20.0)));
    QVERIFY(std::abs(rotBack.x() - 10.0) < 1e-9);
    QVERIFY(std::abs(rotBack.y() - 20.0) < 1e-9);

    setupGISTransformation(0.0, 0.0, 0.0, 0.0);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString ascPath = tmpDir.filePath("gis_test.asc");
    {
        QFile f(ascPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream ts(&f);
        ts << "ncols 4\n"
           << "nrows 3\n"
           << "xllcorner 0\n"
           << "yllcorner 0\n"
           << "cellsize 10\n"
           << "NODATA_value -9999\n"
           << "1 2 2 -9999\n"
           << "3 4 1 2\n"
           << "5 1 2 3\n";
    }

    GisGrid gg;
    QVERIFY(gg.loadFromFile(ascPath));
    QCOMPARE(gg.rows(), 3);
    QCOMPARE(gg.cols(), 4);
    QCOMPARE(gg.cellSize(), 10.0);
    QCOMPARE(gg.minValue(), 1.0);
    QCOMPARE(gg.maxValue(), 5.0);
    QCOMPARE(gg.value(-5.0, -5.0), -1.0);
    QCOMPARE(gg.value(3, 2), -1.0);

    QList<double> distinct = gg.distinctValues();
    std::sort(distinct.begin(), distinct.end());
    QCOMPARE(distinct, QList<double>({-1.0, 1.0, 2.0, 3.0, 4.0, 5.0}));

    gg.clip(QRectF(0.0, 0.0, 20.0, 20.0));
    QCOMPARE(gg.value(QPointF(25.0, 5.0)), -1.0);
    QCOMPARE(gg.value(QPointF(5.0, 5.0)), 5.0);
}

void TestGrid::layeredGridAndExport()
{
    setupGISTransformation(0.0, 0.0, 0.0, 0.0);

    Grid<TestLayerCell> raw(10.0f, 3, 2);
    for (int i = 0; i < raw.count(); ++i) {
        raw[i].a = static_cast<float>(i + 1);
        raw[i].b = static_cast<float>((i + 1) * 10);
    }

    TestLayeredGrid lg(raw);
    QCOMPARE(lg.indexOf("layerA"), 0);
    QCOMPARE(lg.indexOf("layerB"), 1);
    QCOMPARE(lg.indexOf("missing"), -1);
    QCOMPARE(lg.layerNames(), QStringList({"layerA", "layerB"}));
    QCOMPARE(lg.value(1, 0, 0), 2.0);
    QCOMPARE(lg.value(1, 0, 1), 20.0);

    double minV = 0.0, maxV = 0.0;
    lg.range(minV, maxV, 1);
    QCOMPARE(minV, 10.0);
    QCOMPARE(maxV, 60.0);

    Grid<double> *copied = lg.copyGrid(1);
    QVERIFY(copied != nullptr);
    QCOMPARE(copied->constValueAtIndex(2, 1), 60.0);
    delete copied;

    const QString esri = gridToESRIRaster(lg, "layerA");
    QVERIFY(!esri.isEmpty());
    QVERIFY(esri.contains("ncols 3"));
    QVERIFY(esri.contains("nrows 2"));
}

void TestGrid::demSlopeAspectAndBorderSafety()
{
    DEM demFlat;
    demFlat.setup(QRectF(0.0, 0.0, 50.0, 50.0), 10.0f);
    demFlat.initialize(200.0f);
    float slp = -1.0f, asp = -1.0f;
    QCOMPARE(demFlat.orientation(QPointF(25.0, 25.0), slp, asp), 200.0f);
    QCOMPARE(slp, 0.0f);

    // North-facing 45 deg slope (elevation decreases northward as y increases)
    DEM demNorth;
    demNorth.setup(QRectF(0.0, 0.0, 50.0, 50.0), 10.0f);
    for (int y = 0; y < demNorth.sizeY(); ++y)
        for (int x = 0; x < demNorth.sizeX(); ++x)
            demNorth.valueAtIndex(x, y) = 500.0f - y * 10.0f;
    QCOMPARE(demNorth.orientation(QPointF(25.0, 25.0), slp, asp), 480.0f);
    QVERIFY(std::abs(slp - 1.0f) < 1e-5f);
    QVERIFY(std::abs(asp - 0.0f) < 1e-4f || std::abs(asp - 360.0f) < 1e-4f);

    // East-facing 45 deg slope (elevation decreases eastward as x increases)
    DEM demEast;
    demEast.setup(QRectF(0.0, 0.0, 50.0, 50.0), 10.0f);
    for (int y = 0; y < demEast.sizeY(); ++y)
        for (int x = 0; x < demEast.sizeX(); ++x)
            demEast.valueAtIndex(x, y) = 500.0f - x * 10.0f;
    demEast.orientation(QPointF(25.0, 25.0), slp, asp);
    QVERIFY(std::abs(slp - 1.0f) < 1e-5f);
    QVERIFY(std::abs(asp - 90.0f) < 1e-4f);

    // South-facing 45 deg slope (elevation increases northward with y)
    DEM demSouth;
    demSouth.setup(QRectF(0.0, 0.0, 50.0, 50.0), 10.0f);
    for (int y = 0; y < demSouth.sizeY(); ++y)
        for (int x = 0; x < demSouth.sizeX(); ++x)
            demSouth.valueAtIndex(x, y) = 100.0f + y * 10.0f;
    demSouth.orientation(QPointF(25.0, 25.0), slp, asp);
    QVERIFY(std::abs(slp - 1.0f) < 1e-5f);
    QVERIFY(std::abs(asp - 180.0f) < 1e-4f);

    // West-facing 45 deg slope (elevation increases eastward with x)
    DEM demWest;
    demWest.setup(QRectF(0.0, 0.0, 50.0, 50.0), 10.0f);
    for (int y = 0; y < demWest.sizeY(); ++y)
        for (int x = 0; x < demWest.sizeX(); ++x)
            demWest.valueAtIndex(x, y) = 100.0f + x * 10.0f;
    demWest.orientation(QPointF(25.0, 25.0), slp, asp);
    QVERIFY(std::abs(slp - 1.0f) < 1e-5f);
    QVERIFY(std::abs(asp - 270.0f) < 1e-4f);

    // TPI and derived slope/aspect/view grids
    DEM demPeak;
    demPeak.setup(QRectF(0.0, 0.0, 50.0, 50.0), 10.0f);
    demPeak.initialize(100.0f);
    demPeak.valueAtIndex(2, 2) = 150.0f;
    QVERIFY(demPeak.topographicPositionIndex(QPointF(25.0, 25.0), 15.0f) > 30.0f);
    QCOMPARE(demPeak.slopeGrid()->sizeX(), 5);
    QCOMPARE(demPeak.aspectGrid()->sizeX(), 5);
    QVERIFY(std::isfinite(demPeak.viewGrid()->constValueAtIndex(2, 2)));

    // Red -> Green Bug #1: east border (x == sizeX() - 1) must not read row (0, y+1) out-of-bounds across rows
    DEM demBorder;
    demBorder.setup(QRectF(0.0, 0.0, 50.0, 50.0), 10.0f);
    demBorder.initialize(100.0f);
    demBorder.valueAtIndex(0, 3) = 900.0f;
    slp = -1.0f;
    asp = -1.0f;
    const float h = demBorder.orientation(demBorder.cellCenterPoint(QPoint(4, 2)), slp, asp);
    QCOMPARE(h, 0.0f);
    QCOMPARE(slp, 0.0f);
}
