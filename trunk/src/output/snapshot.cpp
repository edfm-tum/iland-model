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
#include "snapshot.h"
#include "global.h"
#include "globalsettings.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"
#include "tree.h"
#include "soil.h"
#include "snag.h"

#include <QString>
#include <QtSql>


Snapshot::Snapshot()
{
}

bool Snapshot::openDatabase(const QString &file_name, const bool read)
{
    if (!QSqlDatabase::database("snapshot").isValid()) {
        if (!GlobalSettings::instance()->setupDatabaseConnection("snapshot", file_name, read)) {
            throw IException("Snapshot:createDatabase: database could not be created / opened");
        }
    }
    QSqlDatabase db=QSqlDatabase::database("snapshot");
    if (!read) {
        // create tables
        QSqlQuery q(db);
        // trees
        q.exec("drop table trees");
        q.exec("create table trees (ID integer, RUindex integer, posX integer, posY integer, species text,  age integer, height real, dbh real, leafArea real, opacity real, foliageMass real, woodyMass real, fineRootMass real, coarseRootMass real, NPPReserve real, stressIndex real)");
        // soil
        q.exec("drop table soil");
        q.exec("create table soil (RUindex integer, kyl real, kyr real, inLabC real, inLabN real, inRefC real, inRefN real, YLC real, YLN real, YRC real, YRN real, SOMC real, SOMN real)");
        qDebug() << "Snapshot - tables created. Database" << file_name;
        q.exec("drop table snag");
        q.exec("create table snag(RUIndex integer, climateFactor real, SWD1C real, SWD1N real, SWD2C real, SWD2N real, SWD3C real, SWD3N real, " \
               "totalSWDC real, totalSWDN real, NSnags1 real, NSnags2 real, NSnags3 real, dbh1 real, dbh2 real, dbh3 real, height1 real, height2 real, height3 real, " \
               "volume1 real, volume2 real, volume3 real, tsd1 real, tsd2 real, tsd3 real, ksw1 real, ksw2 real, ksw3 real, halflife1 real, halflife2 real, halflife3 real, " \
               "branch1C real, branch1N real, branch2C real, branch2N real, branch3C real, branch3N real, branch4C real, branch4N real, branch5C real, branch5N real, branchIndex integer)");
    }
    return true;
}

bool Snapshot::createSnapshot(const QString &file_name)
{
    openDatabase(file_name, false);
    // save the trees
    saveTrees();
    // save soil pools
    saveSoil();
    // save snags / deadwood pools
    saveSnags();
    QSqlDatabase::database("snapshot").close();
    return true;
}

bool Snapshot::loadSnapshot(const QString &file_name)
{
    openDatabase(file_name, true);
    loadTrees();
    loadSoil();
    loadSnags();
    // after changing the trees, do a complete apply/read pattern cycle over the landscape...
    GlobalSettings::instance()->model()->onlyApplyLightPattern();
    qDebug() << "applied light pattern...";
    QSqlDatabase::database("snapshot").close();
    return true;
}

void Snapshot::saveTrees()
{
    QSqlDatabase db=QSqlDatabase::database("snapshot");
    AllTreeIterator at(GlobalSettings::instance()->model());
    QSqlQuery q(db);
    if (!q.prepare(QString("insert into trees (ID, RUindex, posX, posY, species,  age, height, dbh, leafArea, opacity, foliageMass, woodyMass, fineRootMass, coarseRootMass, NPPReserve, stressIndex) " \
                      "values (:id, :index, :x, :y, :spec, :age, :h, :d, :la, :opa, :mfol, :mwood, :mfr, :mcr, :npp, :si)")))
        throw IException(QString("Snapshot::saveTrees: prepare:") + q.lastError().text());

    int n = 0;
    db.transaction();
    while (Tree *t = at.next()) {
        q.addBindValue(t->id());
        q.addBindValue(t->ru()->index());
        q.addBindValue(t->mPositionIndex.x());
        q.addBindValue(t->mPositionIndex.y());
        q.addBindValue(t->species()->id());
        q.addBindValue(t->age());
        q.addBindValue(t->height());
        q.addBindValue(t->dbh());
        q.addBindValue(t->leafArea());
        q.addBindValue(t->mOpacity);
        q.addBindValue(t->biomassFoliage());
        q.addBindValue(t->biomassStem());
        q.addBindValue(t->biomassFineRoot());
        q.addBindValue(t->biomassCoarseRoot());
        q.addBindValue(t->mNPPReserve);
        q.addBindValue(t->mStressIndex);
        if (!q.exec()) {
            throw IException(QString("Snapshot::saveTrees: execute:") + q.lastError().text());
        }
        if (++n % 10000 == 0) {
            qDebug() << n << "trees saved...";
            QCoreApplication::processEvents();
        }
    }
    db.commit();
    qDebug() << "Snapshot: finished trees. N=" << n;
}

void Snapshot::loadTrees()
{
    QSqlDatabase db=QSqlDatabase::database("snapshot");
    QSqlQuery q(db);
    q.exec("select * from trees");
    int ru_index = -1;
    int new_ru;
    ResourceUnit *ru = 0;
    int n=0;
    while (q.next()) {
        new_ru = q.value(1).toInt();
        if (new_ru != ru_index) {
            ru_index = new_ru;
            // remove all trees...
            ru = GlobalSettings::instance()->model()->ru(ru_index);
            ru->trees().clear();
        }
        if (!ru)
            throw IException("Snapshot::loadTrees: Invalid resource unit");
        Tree t;
        t.setRU(ru);
        t.mId = q.value(0).toInt();
        t.mPositionIndex.setX(q.value(2).toInt());
        t.mPositionIndex.setY(q.value(3).toInt());
        Species *s = GlobalSettings::instance()->model()->speciesSet()->species(q.value(4).toString());
        if (!s)
            throw IException("Snapshot::loadTrees: Invalid species");
        t.setSpecies(s);
        t.mAge = q.value(5).toInt();
        t.mHeight = q.value(6).toFloat();
        t.mDbh = q.value(7).toFloat();
        t.mLeafArea = q.value(8).toFloat();
        t.mOpacity = q.value(9).toFloat();
        t.mFoliageMass = q.value(10).toFloat();
        t.mWoodyMass = q.value(11).toFloat();
        t.mFineRootMass = q.value(12).toFloat();
        t.mCoarseRootMass = q.value(13).toFloat();
        t.mNPPReserve = q.value(14).toFloat();
        t.mStressIndex = q.value(15).toFloat();
        t.mStamp = s->stamp(t.mDbh, t.mHeight);
        ru->trees().push_back(t);
        if (++n % 10000 == 0) {
            qDebug() << n << "trees loaded...";
            QCoreApplication::processEvents();
        }
    }
    qDebug() << "Snapshot: finished trees. N=" << n;
}


void Snapshot::saveSoil()
{
    QSqlDatabase db=QSqlDatabase::database("snapshot");
    QSqlQuery q(db);
    if (!q.prepare(QString("insert into soil (RUindex, kyl, kyr, inLabC, inLabN, inRefC, inRefN, YLC, YLN, YRC, YRN, SOMC, SOMN) " \
                      "values (:idx, :kyl, :kyr, :inLabC, :iLN, :iRC, :iRN, :ylc, :yln, :yrc, :yrn, :somc, :somn)")))
        throw IException(QString("Snapshot::saveSoil: prepare:") + q.lastError().text());

    int n = 0;
    db.transaction();
    foreach (ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        Soil *s = ru->soil();
        q.addBindValue(s->mRU->index());
        q.addBindValue(s->mKyl);
        q.addBindValue(s->mKyr);
        q.addBindValue(s->mInputLab.C);
        q.addBindValue(s->mInputLab.N);
        q.addBindValue(s->mInputRef.C);
        q.addBindValue(s->mInputRef.N);
        q.addBindValue(s->mYL.C);
        q.addBindValue(s->mYL.N);
        q.addBindValue(s->mYR.C);
        q.addBindValue(s->mYR.N);
        q.addBindValue(s->mSOM.C);
        q.addBindValue(s->mSOM.N);
        if (!q.exec()) {
            throw IException(QString("Snapshot::saveSoil: execute:") + q.lastError().text());
        }
        if (++n % 1000 == 0) {
            qDebug() << n << "soil resource units saved...";
            QCoreApplication::processEvents();
        }
    }

    db.commit();
    qDebug() << "Snapshot: finished Soil. N=" << n;
}

void Snapshot::loadSoil()
{
    QSqlDatabase db=QSqlDatabase::database("snapshot");
    QSqlQuery q(db);
    q.exec("select * from soil");
    int ru_index = -1;
    ResourceUnit *ru = 0;
    int n=0;
    while (q.next()) {
        ru_index = q.value(0).toInt();
        ru = GlobalSettings::instance()->model()->ru(ru_index);
        if (!ru)
            throw IException("Snapshot::loadSoil: Invalid resource unit");
        Soil *s = ru->soil();
        s->mKyl = q.value(1).toDouble();
        s->mKyr = q.value(2).toDouble();
        s->mInputLab.C = q.value(3).toDouble();
        s->mInputLab.N = q.value(4).toDouble();
        s->mInputRef.C = q.value(5).toDouble();
        s->mInputRef.N = q.value(6).toDouble();
        s->mYL.C = q.value(7).toDouble();
        s->mYL.N = q.value(8).toDouble();
        s->mYR.C = q.value(9).toDouble();
        s->mYR.N = q.value(10).toDouble();
        s->mSOM.C = q.value(11).toDouble();
        s->mSOM.N = q.value(12).toDouble();

        if (++n % 1000 == 0) {
            qDebug() << n << "soil units loaded...";
            QCoreApplication::processEvents();
        }
    }
    qDebug() << "Snapshot: finished soil. N=" << n;

}


void Snapshot::saveSnags()
{
    QSqlDatabase db=QSqlDatabase::database("snapshot");
    QSqlQuery q(db);
    if (!q.prepare(QString("insert into snag(RUIndex, climateFactor, SWD1C, SWD1N, SWD2C, SWD2N, SWD3C, SWD3N, " \
                           "totalSWDC, totalSWDN, NSnags1, NSnags2, NSnags3, dbh1, dbh2, dbh3, height1, height2, height3, " \
                           "volume1, volume2, volume3, tsd1, tsd2, tsd3, ksw1, ksw2, ksw3, halflife1, halflife2, halflife3, " \
                           "branch1C, branch1N, branch2C, branch2N, branch3C, branch3N, branch4C, branch4N, branch5C, branch5N, branchIndex) " \
                          "values (?,?,?,?,?,?,?,?, " \
                           "?,?,?,?,?,?,?,?,?,?,?," \
                           "?,?,?,?,?,?,?,?,?,?,?,?," \
                           "?,?,?,?,?,?,?,?,?,?,?)")))
        throw IException(QString("Snapshot::saveSnag: prepare:") + q.lastError().text());

    int n = 0;
    db.transaction();
    foreach (ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        Snag *s = ru->snag();
        q.addBindValue(s->mRU->index());
        q.addBindValue(s->mClimateFactor);
        q.addBindValue(s->mSWD[0].C);
        q.addBindValue(s->mSWD[0].N);
        q.addBindValue(s->mSWD[1].C);
        q.addBindValue(s->mSWD[1].N);
        q.addBindValue(s->mSWD[2].C);
        q.addBindValue(s->mSWD[2].N);
        q.addBindValue(s->mTotalSWD.C);
        q.addBindValue(s->mTotalSWD.N);
        q.addBindValue(s->mNumberOfSnags[0]);
        q.addBindValue(s->mNumberOfSnags[1]);
        q.addBindValue(s->mNumberOfSnags[2]);
        q.addBindValue(s->mAvgDbh[0]);
        q.addBindValue(s->mAvgDbh[1]);
        q.addBindValue(s->mAvgDbh[2]);
        q.addBindValue(s->mAvgHeight[0]);
        q.addBindValue(s->mAvgHeight[1]);
        q.addBindValue(s->mAvgHeight[2]);
        q.addBindValue(s->mAvgVolume[0]);
        q.addBindValue(s->mAvgVolume[1]);
        q.addBindValue(s->mAvgVolume[2]);
        q.addBindValue(s->mTimeSinceDeath[0]);
        q.addBindValue(s->mTimeSinceDeath[1]);
        q.addBindValue(s->mTimeSinceDeath[2]);
        q.addBindValue(s->mKSW[0]);
        q.addBindValue(s->mKSW[1]);
        q.addBindValue(s->mKSW[2]);
        q.addBindValue(s->mHalfLife[0]);
        q.addBindValue(s->mHalfLife[1]);
        q.addBindValue(s->mHalfLife[2]);
        q.addBindValue(s->mOtherWood[0].C); q.addBindValue(s->mOtherWood[0].N);
        q.addBindValue(s->mOtherWood[1].C); q.addBindValue(s->mOtherWood[1].N);
        q.addBindValue(s->mOtherWood[2].C); q.addBindValue(s->mOtherWood[2].N);
        q.addBindValue(s->mOtherWood[3].C); q.addBindValue(s->mOtherWood[3].N);
        q.addBindValue(s->mOtherWood[4].C); q.addBindValue(s->mOtherWood[4].N);
        q.addBindValue(s->mBranchCounter);

        if (!q.exec()) {
            throw IException(QString("Snapshot::saveSnag: execute:") + q.lastError().text());
        }
        if (++n % 1000 == 0) {
            qDebug() << n << "snags saved...";
            QCoreApplication::processEvents();
        }
    }

    db.commit();
    qDebug() << "Snapshot: finished Snags. N=" << n;
}

void Snapshot::loadSnags()
{
    QSqlDatabase db=QSqlDatabase::database("snapshot");
    QSqlQuery q(db);
    q.exec("select * from snag");
    int ru_index = -1;
    ResourceUnit *ru = 0;
    int n=0;
    int ci=0;
    while (q.next()) {
        ru_index = q.value(ci++).toInt();
        ru = GlobalSettings::instance()->model()->ru(ru_index);
        if (!ru)
            throw IException("Snapshot::loadSoil: Invalid resource unit");
        Snag *s = ru->snag();
        s->mClimateFactor = q.value(ci++).toDouble();
        s->mSWD[0].C = q.value(ci++).toDouble();
        s->mSWD[0].N = q.value(ci++).toDouble();
        s->mSWD[1].C = q.value(ci++).toDouble();
        s->mSWD[1].N = q.value(ci++).toDouble();
        s->mSWD[2].C = q.value(ci++).toDouble();
        s->mSWD[2].N = q.value(ci++).toDouble();
        s->mTotalSWD.C = q.value(ci++).toDouble();
        s->mTotalSWD.N = q.value(ci++).toDouble();
        s->mNumberOfSnags[0] = q.value(ci++).toDouble();
        s->mNumberOfSnags[1] = q.value(ci++).toDouble();
        s->mNumberOfSnags[2] = q.value(ci++).toDouble();
        s->mAvgDbh[0] = q.value(ci++).toDouble();
        s->mAvgDbh[1] = q.value(ci++).toDouble();
        s->mAvgDbh[2] = q.value(ci++).toDouble();
        s->mAvgHeight[0] = q.value(ci++).toDouble();
        s->mAvgHeight[1] = q.value(ci++).toDouble();
        s->mAvgHeight[2] = q.value(ci++).toDouble();
        s->mAvgVolume[0] = q.value(ci++).toDouble();
        s->mAvgVolume[1] = q.value(ci++).toDouble();
        s->mAvgVolume[2] = q.value(ci++).toDouble();
        s->mTimeSinceDeath[0] = q.value(ci++).toDouble();
        s->mTimeSinceDeath[1] = q.value(ci++).toDouble();
        s->mTimeSinceDeath[2] = q.value(ci++).toDouble();
        s->mKSW[0] = q.value(ci++).toDouble();
        s->mKSW[1] = q.value(ci++).toDouble();
        s->mKSW[2] = q.value(ci++).toDouble();
        s->mHalfLife[0] = q.value(ci++).toDouble();
        s->mHalfLife[1] = q.value(ci++).toDouble();
        s->mHalfLife[2] = q.value(ci++).toDouble();
        s->mOtherWood[0].C = q.value(ci++).toDouble(); s->mOtherWood[0].N = q.value(ci++).toDouble();
        s->mOtherWood[1].C = q.value(ci++).toDouble(); s->mOtherWood[1].N = q.value(ci++).toDouble();
        s->mOtherWood[2].C = q.value(ci++).toDouble(); s->mOtherWood[2].N = q.value(ci++).toDouble();
        s->mOtherWood[3].C = q.value(ci++).toDouble(); s->mOtherWood[3].N = q.value(ci++).toDouble();
        s->mOtherWood[4].C = q.value(ci++).toDouble(); s->mOtherWood[4].N = q.value(ci++).toDouble();
        s->mBranchCounter = q.value(ci++).toInt();

        if (++n % 1000 == 0) {
            qDebug() << n << "snags loaded...";
            QCoreApplication::processEvents();
        }
    }
    qDebug() << "Snapshot: finished snags. N=" << n;

}



