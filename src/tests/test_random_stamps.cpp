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

#include "test_random_stamps.h"

#include <QBuffer>
#include <QDataStream>
#include <QSet>
#include <QtTest/QtTest>
#include <cmath>

#include "exception.h"
#include "grid.h"
#include "random.h"
#include "randomgenerator.h"
#include "stamp.h"
#include "stampcontainer.h"

void TestRandomStamps::rngDeterminismAndBounds()
{
    const QList<RandomGenerator::ERandomGenerators> engines = {
        RandomGenerator::ergMersenneTwister,
        RandomGenerator::ergWellRNG512,
        RandomGenerator::ergXORShift96,
        RandomGenerator::ergFastRandom
    };

    for (auto eng : engines) {
        RandomGenerator::setup(eng, 12345u);
        QVector<unsigned long> seq1;
        for (int i = 0; i < 50; ++i)
            seq1 << RandomGenerator::randInt();

        RandomGenerator::setup(eng, 12345u);
        QVector<unsigned long> seq2;
        for (int i = 0; i < 50; ++i)
            seq2 << RandomGenerator::randInt();
        QCOMPARE(seq1, seq2);

        RandomGenerator::setup(eng, 54321u);
        QVector<unsigned long> seq3;
        for (int i = 0; i < 50; ++i)
            seq3 << RandomGenerator::randInt();
        if (eng != RandomGenerator::ergXORShift96)
            QVERIFY(seq1 != seq3);
    }

    RandomGenerator::setup(RandomGenerator::ergMersenneTwister, 98765u);
    int counts36[3] = {0, 0, 0};
    for (int i = 0; i < 1000; ++i) {
        const double d = drandom();
        QVERIFY(d >= 0.0 && d <= 1.0);

        const double nr = nrandom(-5.0, 5.0);
        QVERIFY(nr >= -5.0 && nr <= 5.0);

        // irandom(from, to) must be strictly [from, to)
        const int ir = irandom(3, 6);
        QVERIFY(ir >= 3 && ir < 6);
        counts36[ir - 3]++;
    }
    QVERIFY(counts36[0] > 0 && counts36[1] > 0 && counts36[2] > 0);

    // Gaussian sanity over 10,000 draws: mean=10, sd=2
    double sum = 0.0;
    double sumSq = 0.0;
    const int nNorm = 10000;
    for (int i = 0; i < nNorm; ++i) {
        const double x = RandomGenerator::randNorm(10.0, 2.0);
        sum += x;
        sumSq += x * x;
    }
    const double mean = sum / nNorm;
    const double sd = std::sqrt(sumSq / nNorm - mean * mean);
    QVERIFY(mean > 9.85 && mean < 10.15);
    QVERIFY(sd > 1.85 && sd < 2.15);
}

void TestRandomStamps::randomIndexWeightedAndCustomPdf()
{
    RandomGenerator::setup(RandomGenerator::ergMersenneTwister, 4242u);

    // RandomIndex: must visit 0..24 as a bijection
    RandomIndex ri(25);
    QSet<int> visited;
    while (ri.next()) {
        QVERIFY(ri.index() >= 0 && ri.index() < 25);
        QVERIFY(!visited.contains(ri.index()));
        visited.insert(ri.index());
    }
    QCOMPARE(visited.size(), 25);

    // RandomWeighted
    RandomWeighted rw;
    rw.setup(3);
    rw.setWeight(0, 10);
    rw.setWeight(1, 30);
    rw.setWeight(2, 60);
    QVERIFY(std::abs(rw.getRelWeight(0) - 0.1) < 1e-12);
    QVERIFY(std::abs(rw.getRelWeight(1) - 0.3) < 1e-12);
    QVERIFY(std::abs(rw.getRelWeight(0, 2) - 0.9) < 1e-12);

    int rwCounts[3] = {0, 0, 0};
    for (int i = 0; i < 5000; ++i) {
        const int idx = rw.get();
        QVERIFY(idx >= 0 && idx < 3);
        rwCounts[idx]++;
    }
    QVERIFY(rwCounts[0] < rwCounts[1] && rwCounts[1] < rwCounts[2]);
    QVERIFY(std::abs(rwCounts[2] / 5000.0 - 0.6) < 0.05);

    // RandomCustomPDF uninitialized get() throws
    RandomCustomPDF uninitPdf;
    QVERIFY_THROWS_EXCEPTION(IException, {
        uninitPdf.get();
    });

    // Linear density f(x) = 2*x on [0, 1] -> E[X] = 2/3, P(0..0.5) = 0.25
    RandomCustomPDF pdf;
    pdf.setup("2 * x", 0.0, 1.0, false, 200);
    QVERIFY(std::abs(pdf.getProbOfRange(0.0, 0.5) - 0.25) < 0.02);

    double pdfSum = 0.0;
    for (int i = 0; i < 5000; ++i)
        pdfSum += pdf.get();
    QVERIFY(std::abs(pdfSum / 5000.0 - 2.0 / 3.0) < 0.04);

    // Cumulative mode isSumFunc=true with F(x) = x^2 -> P(0..0.5) = 0.25
    RandomCustomPDF cdf;
    cdf.setup("x^2", 0.0, 1.0, true, 200);
    QVERIFY(std::abs(cdf.getProbOfRange(0.0, 0.5) - 0.25) < 0.02);
}

void TestRandomStamps::stampCreationAndContainerSerialization()
{
    FloatGrid evenGrid(2.0f, 10, 10);
    evenGrid.initialize(1.0f);
    QVERIFY(stampFromGrid(evenGrid, 5) == nullptr);

    FloatGrid oddGrid(2.0f, 13, 13);
    oddGrid.initialize(0.25f);
    oddGrid.valueAtIndex(6, 6) = 0.75f;
    Stamp *fromOdd = stampFromGrid(oddGrid, 5);
    QVERIFY(fromOdd != nullptr);
    QCOMPARE(fromOdd->offset(), 2);
    QCOMPARE(fromOdd->size(), 5);
    QCOMPARE(fromOdd->dataSize(), Stamp::est8x8);
    QCOMPARE((*fromOdd)(2, 2), 0.75f);
    fromOdd->setCrownRadius(3.0f);
    QVERIFY(std::abs(fromOdd->crownArea() - 9.0f * static_cast<float>(M_PI)) < 1e-4f);
    delete fromOdd;

    // Build reader container & main container -> serialize -> reload -> attachReaderStamps -> invert
    StampContainer readersRaw;
    readersRaw.setDescription("test-readers");
    Stamp *r1 = new Stamp(Stamp::est4x4);
    r1->setOffset(1);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            r1->setData(x, y, 0.5f);
    readersRaw.addReaderStamp(r1, 2.0f);

    QByteArray readersBytes;
    {
        QBuffer buf(&readersBytes);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDataStream out(&buf);
        readersRaw.save(out);
    }

    StampContainer readersLoaded;
    {
        QBuffer buf(&readersBytes);
        QVERIFY(buf.open(QIODevice::ReadOnly));
        QDataStream in(&buf);
        readersLoaded.load(in);
    }
    QCOMPARE(readersLoaded.count(), 1);
    QVERIFY(readersLoaded.readerStamp(2.0f) != nullptr);

    StampContainer scRaw;
    scRaw.setDescription("test-stamps");
    Stamp *s1 = new Stamp(Stamp::est8x8);
    s1->setOffset(2);
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 5; ++x)
            s1->setData(x, y, 0.25f);
    scRaw.addStamp(s1, 20.0f, 80.0f, 2.0f);

    QByteArray scBytes;
    {
        QBuffer buf(&scBytes);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QDataStream out(&buf);
        scRaw.save(out);
    }

    StampContainer scLoaded;
    {
        QBuffer buf(&scBytes);
        QVERIFY(buf.open(QIODevice::ReadOnly));
        QDataStream in(&buf);
        scLoaded.load(in);
    }
    QCOMPARE(scLoaded.count(), 1);
    QCOMPARE(scLoaded.description(), QString("test-stamps"));

    scLoaded.attachReaderStamps(readersLoaded);
    const Stamp *lookedUp = scLoaded.stamp(20.0f, 16.0f);
    QVERIFY(lookedUp != nullptr);
    QVERIFY(lookedUp->reader() != nullptr);
    QCOMPARE((*lookedUp)(0, 0), 0.25f);

    // Out-of-bounds clamping must still return a valid Stamp*
    QVERIFY(scLoaded.stamp(500.0f, 60.0f) != nullptr);

    scLoaded.invert();
    QCOMPARE((*scLoaded.stamp(20.0f, 16.0f))(0, 0), 0.75f);
}
