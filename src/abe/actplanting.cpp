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
#include "abe_global.h"
#include "actplanting.h"

#include "fmstp.h"
#include "fmstand.h"
#include "fomescript.h"
#include "fmtreelist.h"
#include "forestmanagementengine.h"

#include "model.h"
#include "mapgrid.h"
#include "resourceunit.h"
#include "resourceunitspecies.h"

#include "debugtimer.h"

namespace ABE {

/** @class ActPlanting
    @ingroup abe
    The ActPlanting class implements artificial regeneration (i.e., planting of trees).

  */


// Planting patterns
QVector<QPair<QString, int> > planting_patterns =QVector<QPair<QString, int> >()
<< QPair<QString, int>(
        "11"\
        "11", 2)
<< QPair<QString, int>("11111"\
                       "11111"\
                       "11111"\
                       "11111"\
                       "11111", 5)
<< QPair<QString, int>("1111111111"\
                       "1111111111"\
                       "1111111111"\
                       "1111111111"\
                       "1111111111"\
                       "1111111111"\
                       "1111111111"\
                       "1111111111"\
                       "1111111111"\
                       "1111111111", 10)
<< QPair<QString, int>("00110"\
                       "11110"\
                       "11111"\
                       "01111"\
                       "00110", 5)
<< QPair<QString, int>("0000110000"\
                       "0011111100"\
                       "0111111110"\
                       "0111111110"\
                       "1111111111"\
                       "1111111111"\
                       "0111111110"\
                       "0011111110"\
                       "0011111100"\
                       "0000110000", 10);
QStringList planting_pattern_names = QStringList() << "rect2" << "rect10" << "rect20" << "circle5" << "circle10";

QStringList ActPlanting::mAllowedProperties;


ActPlanting::ActPlanting(FMSTP *parent): Activity(parent)
{
    //mBaseActivity.setIsScheduled(true); // use the scheduler
    //mBaseActivity.setDoSimulate(true); // simulate per default
    if (mAllowedProperties.isEmpty())
        mAllowedProperties = QStringList() << Activity::mAllowedProperties
                                           << "species" << "fraction" << "height" << "age" << "clear"
                                           << "pattern" << "spacing" << "offset" << "random" << "n";

}

void ActPlanting::setup(QJSValue value)
{
    // check if regeneration is enabled
    if (GlobalSettings::instance()->model()->settings().regenerationEnabled == false)
        throw IException("Cannot set up planting acitivities when iLand regeneration module is disabled.");
    Activity::setup(value); // setup base events

    QJSValue items = FMSTP::valueFromJs(value, "items");
    mItems.clear();
    // iterate over array or over single object
    if ((items.isArray() || items.isObject()) && !items.isCallable()) {
        QJSValueIterator it(items);
        while (it.hasNext()) {
            it.next();
            if (it.name()==QStringLiteral("length"))
                continue;

            mItems.append(SPlantingItem());
            SPlantingItem &item = mItems.last();
            qDebug() << it.name() << ": " << FomeScript::JStoString(it.value());
            FMSTP::checkObjectProperties(it.value(), mAllowedProperties, "setup of planting activity:" + name() + "; " + it.name());

            item.setup(it.value());
        }
    } else {
        mItems.append(SPlantingItem());
        SPlantingItem &item = mItems.last();
        FMSTP::checkObjectProperties(items, mAllowedProperties, "setup of planting activity:" + name());

        item.setup(items);
    }
    mRequireLoading = false;
    for (QVector<SPlantingItem>::const_iterator it=mItems.constBegin(); it!=mItems.constEnd(); ++it) {
        if (it->clear == true)
            mRequireLoading = true;
    }
}

bool ActPlanting::execute(FMStand *stand)
{
    qCDebug(abe) << stand->context() << "execute of planting activity....";
    DebugTimer time("ABE:ActPlanting:execute");

    for (int s=0;s<mItems.count();++s) {
        mItems[s].run(stand);
    }


    return true;
}

QStringList ActPlanting::info()
{
    QStringList lines = Activity::info();

    foreach(const SPlantingItem &item, mItems) {
        lines << "-";
        lines << QString("species: %1").arg(item.species->id());
        lines << QString("fraction: %1").arg(item.fraction);
        lines << QString("clear: %1").arg(item.clear);
        lines << QString("pattern: %1").arg(item.group_type>-1?planting_pattern_names[item.group_type]:"");
        lines << QString("spacing: %1").arg(item.spacing);
        lines << QString("offset: %1").arg(item.offset);
        lines << QString("random: %1").arg(item.group_random_count>0);
        lines << "/-";
    }
    return lines;
}

void ActPlanting::runSinglePlantingItem(FMStand *stand, QJSValue value)
{
    if (!stand) return;
    if (FMSTP::verbose())
        qCDebug(abe()) << "run Single Planting Item for Stand" << stand->id();
    DebugTimer time("ABE:runSinglePlantingItem");
    SPlantingItem item;
    item.setup(value);

    item.run(stand);

}




bool ActPlanting::SPlantingItem::setup(QJSValue value)
{

    QString species_id = FMSTP::valueFromJs(value, "species",QString(), "setup of planting item for planting activity.").toString();
    species = GlobalSettings::instance()->model()->speciesSet()->species(species_id);
    if (!species)
        throw IException(QString("'%1' is not a valid species id for setting up a planting item.").arg(species_id));
    fraction = FMSTP::valueFromJs(value, "fraction", "0").toNumber();
    height = FMSTP::valueFromJs(value, "height", "0.05").toNumber();
    age = FMSTP::valueFromJs(value, "age", "1").toInt();
    clear = FMSTP::valueFromJs(value, "clear", "false").toBool();

    // pattern
    QString group = FMSTP::valueFromJs(value, "pattern", "").toString();
    group_type = planting_pattern_names.indexOf(group);
    if (!group.isEmpty() && group!="undefined" && group_type==-1)
        throw IException(QString("Planting-activity: the pattern '%1' is not valid!").arg(group));
    spacing = FMSTP::valueFromJs(value, "spacing", "0").toInt();
    offset = FMSTP::valueFromJs(value, "offset", "0").toInt();

    bool random = FMSTP::boolValueFromJs(value, "random", false);
    if (random)
        group_random_count = FMSTP::valueFromJs(value, "n", "0").toInt();
    else
        group_random_count = 0;

    grouped = group_type >= 0;
    return true;
}

void ActPlanting::SPlantingItem::run(FMStand *stand)
{
    QRectF box = ForestManagementEngine::instance()->standGrid()->boundingBox(stand->id());
    const MapGrid *sgrid = ForestManagementEngine::instance()->standGrid();
    Model *model = GlobalSettings::instance()->model();
    GridRunner<float> runner(model->grid(), box);
    if (!grouped) {
        // distribute saplings randomly.
        // this adds saplings to SaplingCell (only if enough slots are available)
        while (runner.next()) {
            if (sgrid->standIDFromLIFCoord(runner.currentIndex()) != stand->id())
                continue;
            //
            if (drandom() < fraction) {
                ResourceUnit *ru = model->ru(runner.currentCoord());
                ru->saplingCell(runner.currentIndex())->addSapling(height, age, species->index());
            }
        }
    } else {
        // grouped saplings
        const QString &pp = planting_patterns[group_type].first;
        int n = planting_patterns[group_type].second;

        if (spacing==0 && group_random_count == 0) {
            // pattern based planting (filled)
            runner.reset();
            while (runner.next()) {
                QPoint qp = runner.currentIndex();
                if (sgrid->standIDFromLIFCoord(qp) != stand->id())
                    continue;
                // check location in the pre-defined planting patterns
                int idx = (qp.x()+offset)%n + n*((qp.y()+offset)%n);
                if (pp[idx]=='1') {
                    ResourceUnit *ru = model->ru(runner.currentCoord());
                    SaplingCell *sc = ru->saplingCell(qp);

                    if (clear) {
                        // clear all sapling trees on the cell
                        model->saplings()->clearSaplings(sc,ru,true);
                    }
                    sc->addSapling(height,age,species->index());
                }
            }
        } else {
            // pattern based (with spacing / offset, random...)
            int ispacing = spacing / cPxSize;
            QPoint p = model->grid()->indexAt(box.topLeft())-QPoint(offset, offset);
            QPoint pstart = p;
            QPoint p_end = model->grid()->indexAt(box.bottomRight());
            QPoint po;
            p.setX(qMax(p.x(),0)); p.setY(qMax(p.y(),0));

            int n_ha = group_random_count * box.width()*box.height()/10000.;
            bool do_random = group_random_count>0;

            while( p.x() < p_end.x() && p.y() < p_end.y()) {
                if (do_random) {
                    // random position!
                    if (n_ha--<=0)
                        break;
                    // select a random position (2m grid index)
                    p = model->grid()->indexAt(QPointF( nrandom(box.left(), box.right()), nrandom(box.top(), box.bottom()) ));
                }

                // apply the pattern....
                for (int y=0;y<n;++y) {
                    for (int x=0;x<n;++x) {
                        po=p + QPoint(x,y);
                        if (sgrid->standIDFromLIFCoord(po) != stand->id())
                            continue;
                        ResourceUnit *ru = model->ru(model->grid()->cellCenterPoint(po));
                        SaplingCell *sc = ru->saplingCell(po);

                        if (clear) {
                            // clear all sapling trees
                            model->saplings()->clearSaplings(sc,ru,true);
                        }
                        sc->addSapling(height,age,species->index());
                    }
                }
                if (!do_random) {
                    // apply offset
                    p.rx() += ispacing;
                    if (p.x()>= p_end.x()) {
                        p.rx() = pstart.x();
                        p.ry() += ispacing;
                    }
                }
            }
        }
    }


}



} // namesapce
