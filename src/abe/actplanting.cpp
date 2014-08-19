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
#include "sapling.h"

namespace ABE {

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




ActPlanting::ActPlanting(FMSTP *parent): Activity(parent)
{
    //mBaseActivity.setIsScheduled(true); // use the scheduler
    //mBaseActivity.setDoSimulate(true); // simulate per default

}

void ActPlanting::setup(QJSValue value)
{
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
            qDebug() << it.name() << ": " << it.value().toString();
            item.setup(it.value());
        }
    } else {
        mItems.append(SPlantingItem());
        SPlantingItem &item = mItems.last();
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
    QRectF box = ForestManagementEngine::instance()->standGrid()->boundingBox(stand->id());
    const MapGrid *sgrid = ForestManagementEngine::instance()->standGrid();
    Model *model = GlobalSettings::instance()->model();
    GridRunner<float> gr(model->grid(), box);

    // sapling_list:
    // a hash with key=LIF-coordinates of the 2x2m Pixel
    // value: pair: ptr to RUS, int: index of the sapling tree.
    // this circumvents problems when sapling trees are added (and consequently pointers into the QVector are invalidated)
    QMultiHash<QPoint, QPair<ResourceUnitSpecies *, int> > sapling_list;
    if (mRequireLoading)
        sapling_list = sgrid->saplingTreeHash(stand->id());

    // loop over all cells on the LIF grid that are within the bounding box of the stand.
    for (int s=0;s<mItems.count();++s) {
        gr.reset();
        if (!mItems[s].grouped) {
            while (float *p=gr.next()) {
                if (sgrid->LIFgridValue(gr.currentIndex()) != stand->id())
                    continue;
                //
                if (drandom() < mItems[s].fraction) {
                    ResourceUnit *ru = model->ru(gr.currentCoord());
                    ResourceUnitSpecies &rus =  ru->resourceUnitSpecies(mItems[s].species);
                    int t=rus.addSapling(gr.currentIndex(), mItems[s].height);
                    if (mRequireLoading)
                        sapling_list.insert(gr.currentIndex(), QPair<ResourceUnitSpecies*, int>(&rus, t));
                }
            }
        } else {
            int offset = mItems[s].offset;
            const QString &pp = planting_patterns[mItems[s].group_type].first;
            int n = planting_patterns[mItems[s].group_type].second;

            if (mItems[s].spacing==0 && mItems[s].group_random_count == 0) {
                // pattern based planting (filled)
                gr.reset();
                while (gr.next()) {
                    QPoint qp = gr.currentIndex();
                    if (sgrid->LIFgridValue(qp) != stand->id())
                        continue;
                    int idx = (qp.x()+offset)%n + n*((qp.y()+offset)%n);
                    if (pp[idx]=='1') {
                        ResourceUnit *ru = model->ru(gr.currentCoord());
                        ResourceUnitSpecies &rus =  ru->resourceUnitSpecies(mItems[s].species);

                        if (mItems[s].clear) {
                            // clear all sapling trees
                            QMultiHash<QPoint, QPair<ResourceUnitSpecies *, int> >::const_iterator i = sapling_list.find(qp);
                            while (i != sapling_list.end() && i.key() == qp) {
                                i.value().first->changeSapling().clearSapling(i.value().second, false);
                                ++i;
                            }
                            sapling_list.remove(qp);
                        }
                        // add sapling
                        int t = rus.addSapling(gr.currentIndex(), mItems[s].height);
                        if (mRequireLoading)
                            sapling_list.insert(gr.currentIndex(), QPair<ResourceUnitSpecies*, int>(&rus, t));
                    }
                }
            } else {
                // pattern based (with spacing / offset, random...)
                int spacing = mItems[s].spacing / cPxSize;
                QPoint p = model->grid()->indexAt(box.topLeft())-QPoint(offset, offset);
                QPoint pstart = p;
                QPoint p_end = model->grid()->indexAt(box.bottomRight());
                QPoint po;
                p.setX(qMax(p.x(),0)); p.setY(qMax(p.y(),0));

                int n_ha = mItems[s].group_random_count * box.width()*box.height()/10000.;
                bool do_random = mItems[s].group_random_count>0;

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
                            if (sgrid->LIFgridValue(po) != stand->id())
                                continue;
                            ResourceUnit *ru = model->ru(model->grid()->cellCenterPoint(po));
                            ResourceUnitSpecies &rus =  ru->resourceUnitSpecies(mItems[s].species);

                            if (mItems[s].clear) {
                                // clear all sapling trees
                                QMultiHash<QPoint, QPair<ResourceUnitSpecies *, int> >::const_iterator i = sapling_list.find(po);
                                while (i != sapling_list.end() && i.key() == po) {
                                    i.value().first->changeSapling().clearSapling(i.value().second, false);
                                    ++i;
                                }
                                sapling_list.remove(po);
                            }
                            // add sapling
                            int t = rus.addSapling(po, mItems[s].height);
                            if (mRequireLoading)
                                sapling_list.insert(po, QPair<ResourceUnitSpecies*, int>(&rus, t));
                        }
                    }
                    if (!do_random) {
                        // apply offset
                        p.rx() += spacing;
                        if (p.x()>= p_end.x()) {
                            p.rx() = pstart.x();
                            p.ry() += spacing;
                        }
                    }
                }
            }
        }
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




bool ActPlanting::SPlantingItem::setup(QJSValue value)
{
    QString species_id = FMSTP::valueFromJs(value, "species",QString(), "setup of planting item for planting activity.").toString();
    species = GlobalSettings::instance()->model()->speciesSet()->species(species_id);
    if (!species)
        throw IException(QString("'%1' is not a valid species id for setting up a planting item.").arg(species_id));
    fraction = FMSTP::valueFromJs(value, "fraction", "0").toNumber();
    height = FMSTP::valueFromJs(value, "height", "0.05").toNumber();
    clear = FMSTP::valueFromJs(value, "clear", "false").toBool();

    // pattern
    QString group = FMSTP::valueFromJs(value, "pattern", "").toString();
    group_type = planting_pattern_names.indexOf(group);
    if (!group.isEmpty() && group!="undefined" && group_type==-1)
        throw IException(QString("Planting-activity: the pattern '%1' is not valid!").arg(group));
    spacing = FMSTP::valueFromJs(value, "spacing", "0").toNumber();
    offset = FMSTP::valueFromJs(value, "offset", "0").toNumber();

    bool random = FMSTP::boolValueFromJs(value, "random", false);
    if (random)
        group_random_count = FMSTP::valueFromJs(value, "n", "0").toNumber();
    else
        group_random_count = 0;

    grouped = group_type >= 0;
    return true;
}



} // namesapce
