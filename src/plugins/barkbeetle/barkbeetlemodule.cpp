#include "barkbeetlemodule.h"

#include "globalsettings.h"
#include "model.h"
#include "modelcontroller.h"
#include "resourceunit.h"
#include "species.h"
#include "debugtimer.h"

BarkBeetleModule::BarkBeetleModule()
{
    mLayers.setGrid(mGrid);
    mRULayers.setGrid(mRUGrid);

}

void BarkBeetleModule::setup()
{
    // setup the wind grid
    mGrid.setup(GlobalSettings::instance()->model()->heightGrid()->metricRect(), cellsize());
    BarkBeetleCell cell;
    mGrid.initialize(cell);

    mRUGrid.setup(GlobalSettings::instance()->model()->RUgrid().metricRect(), GlobalSettings::instance()->model()->RUgrid().cellsize());
    BarkBeetleRUCell ru_cell;
    mRUGrid.initialize(ru_cell);

    XmlHelper xml(GlobalSettings::instance()->settings().node("modules.barkbeetle"));
    qDebug() << "BBSETUP TEST:" << xml.valueDouble(".test", 0.);

    GlobalSettings::instance()->controller()->addLayers(&mLayers, "bb");
    GlobalSettings::instance()->controller()->addLayers(&mRULayers, "bbru");



}

void BarkBeetleModule::setup(const ResourceUnit *ru)
{
    if (!ru)
        return;
    //qDebug() << "BB setup for RU" << ru->id();

}

void BarkBeetleModule::loadParameters()
{
    const XmlHelper xml = GlobalSettings::instance()->settings().node("modules.barkbeetle");
    params.cohortsPerGeneration = xml.valueDouble(".cohortsPerGeneration", params.cohortsPerGeneration);
    params.cohortsPerSisterbrood = xml.valueDouble(".cohortsPerSisterbrood", params.cohortsPerSisterbrood);
    params.spreadKernelFormula = xml.value(".spreadKernelFormula", "100*(1-x)^4");
    mKernelPDF.setup(params.spreadKernelFormula);
    params.backgroundInfestationProbability = xml.valueDouble(".backgroundInfestationProbability", params.backgroundInfestationProbability);


}

void BarkBeetleModule::run()
{
    // calculate generations
    DebugTimer d("BB:generations");
    ResourceUnit **ruptr = GlobalSettings::instance()->model()->RUgrid().begin();
    BarkBeetleRUCell *bbptr = mRUGrid.begin();
    while (bbptr<mRUGrid.end()) {
        if (*ruptr) {
            bbptr->scanned = false;
            bbptr->generations = mGenerations.calculateGenerations(*ruptr);
        }
        ++bbptr; ++ruptr;

    }

    // clear cells
    HeightGridValue *hgv = GlobalSettings::instance()->model()->heightGrid()->begin();
    for (BarkBeetleCell *b=mGrid.begin();b!=mGrid.end();++b, ++hgv) {
        b->clear();
        //scanResourceUnitTrees(mGrid.indexOf(b)); // scan all - not efficient
        if (hgv->isValid())
            b->dbh = drandom()<0.8 ? nrandom(20.,40.) : 0.; // random landscape
    }

    hgv = GlobalSettings::instance()->model()->heightGrid()->begin();
    for (BarkBeetleCell *b=mGrid.begin();b!=mGrid.end();++b, ++hgv) {
        if (b->)
    }
    qDebug() << "bark beetle run called";
}

void BarkBeetleModule::scanResourceUnitTrees(const QPoint &position)
{
    QPointF p_m = mGrid.cellCenterPoint(position);
    // if this resource unit was already scanned in this bark beetle event, then do nothing
    // the flags are reset
    if (!mRUGrid.coordValid(p_m))
        return;

    if (mRUGrid.valueAt(p_m).scanned)
        return;

    ResourceUnit *ru = GlobalSettings::instance()->model()->ru(p_m);
    if (!ru)
        return;

    QVector<Tree>::const_iterator tend = ru->trees().constEnd();
    for (QVector<Tree>::const_iterator t = ru->trees().constBegin(); t!=tend; ++t) {
        if (!t->isDead() && t->species()->id()==QStringLiteral("piab") && t->dbh()>params.minDbh) {

            const QPoint &tp = t->positionIndex();
            QPoint pcell(tp.x()/cPxPerHeight, tp.y()/cPxPerHeight);

            BarkBeetleCell &bb=mGrid.valueAtIndex(pcell);
            bb.dbh = qMax(bb.dbh, t->dbh());

        }
    }
    // set the "processed" flag
    mRUGrid.valueAt(p_m).scanned = true;
}


void BarkBeetleModule::yearBegin()
{

}


//*********************************************************************************
//************************************ BarkBeetleLayers ***************************
//*********************************************************************************


double BarkBeetleLayers::value(const BarkBeetleCell &data, const int param_index) const
{
    switch(param_index){
    case 0: return data.n; // height
    case 1: return data.dbh; // diameter of host
    default: throw IException(QString("invalid variable index for a BarkBeetleCell: %1").arg(param_index));
    }
}


const QVector<LayeredGridBase::LayerElement> BarkBeetleLayers::names() const
{
    return QVector<LayeredGridBase::LayerElement>()
            << LayeredGridBase::LayerElement(QLatin1Literal("value"), QLatin1Literal("grid value of the pixel"), GridViewRainbow)
            << LayeredGridBase::LayerElement(QLatin1Literal("dbh"), QLatin1Literal("diameter of thickest spruce tree on the 10m pixel"), GridViewRainbow);

}

bool BarkBeetleLayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click
}


double BarkBeetleRULayers::value(const BarkBeetleRUCell &data, const int index) const
{
    switch(index){
    case 0: return data.generations; // number of generation
    default: throw IException(QString("invalid variable index for a BarkBeetleRUCell: %1").arg(index));
    }

}

const QVector<LayeredGridBase::LayerElement> BarkBeetleRULayers::names() const
{
    return QVector<LayeredGridBase::LayerElement>()
            << LayeredGridBase::LayerElement(QLatin1Literal("generations"), QLatin1Literal("total number of bark beetle generations"), GridViewHeat);
}

bool BarkBeetleRULayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click

}
