#include "barkbeetlemodule.h"

#include "globalsettings.h"
#include "model.h"
#include "modelcontroller.h"
#include "resourceunit.h"
#include "species.h"
#include "debugtimer.h"
#include "outputmanager.h"



int BarkBeetleCell::total_infested = 0;

BarkBeetleModule::BarkBeetleModule()
{
    mLayers.setGrid(mGrid);
    mRULayers.setGrid(mRUGrid);
    mSimulate = false;

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

    GlobalSettings::instance()->controller()->addLayers(&mLayers, "bb");
    GlobalSettings::instance()->controller()->addLayers(&mRULayers, "bbru");

    // load settings from the XML file
    loadParameters();

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
    params.spreadKernelMaxDistance = xml.valueDouble(".spreadKernelMaxDistance", params.spreadKernelMaxDistance);
    params.spreadKernelFormula = xml.value(".spreadKernelFormula", "100*(1-x)^4");
    mKernelPDF.setup(params.spreadKernelFormula,0.,params.spreadKernelMaxDistance);
    params.backgroundInfestationProbability = xml.valueDouble(".backgroundInfestationProbability", params.backgroundInfestationProbability);
    QString formula = xml.value(".colonizeProbabilityFormula", "0.1");
    mColonizeProbability.setExpression(formula);

    formula = xml.value(".winterMortalityFormula", "polygon(days, 0,0, 30, 0.6)");
    mWinterMortalityFormula.setExpression(formula);
    params.winterMortalityBaseLevel = xml.valueDouble(".baseWinterMortality", 0.5);

    mAfterExecEvent = xml.value(".onAfterBarkbeetle");

    HeightGridValue *hgv = GlobalSettings::instance()->model()->heightGrid()->begin();
    for (BarkBeetleCell *b=mGrid.begin();b!=mGrid.end();++b, ++hgv) {
        b->reset();
        //scanResourceUnitTrees(mGrid.indexOf(b)); // scan all - not efficient
//        if (hgv->isValid()) {
//            b->dbh = drandom()<0.6 ? nrandom(20.,40.) : 0.; // random landscape
//            b->tree_stress = nrandom(0., 0.4);
//        }
    }

    yearBegin(); // also reset the "scanned" flags

}

void BarkBeetleModule::clearGrids()
{

    for (BarkBeetleCell *b=mGrid.begin();b!=mGrid.end();++b)
        b->clear();

    BarkBeetleRUCell ru_cell;
    mRUGrid.initialize(ru_cell);

    BarkBeetleCell::resetCounters();
    stats.clear();

}

void BarkBeetleModule::loadAllVegetation()
{
    foreach (const ResourceUnit *ru, GlobalSettings::instance()->model()->ruList())
        scanResourceUnitTrees(ru->boundingBox().center());
}

void BarkBeetleModule::run(int iteration)
{
    // reset statistics
    BarkBeetleCell::resetCounters();
    int old_max_gen = stats.maxGenerations;
    stats.clear();
    mIteration = iteration;

    // calculate the potential bark beetle generations for each resource unit
    if (iteration==0)
        calculateGenerations();
    else
        stats.maxGenerations = old_max_gen; // save that value....

    // load the vegetation (skipped if not the initial iteration)
    loadAllVegetation();

    // background probability of infestation
    startSpread();

    // the spread of beetles (and attacking of trees)
    barkbeetleSpread();

    // write back to the real iLand structure: TODO
    if (!mSimulate) {
        barkbeetleKill();
    }

    // create some outputs....
    qDebug() << "iter/background-inf/winter-mort/N spread/N landed/N infested: " << mIteration << stats.infestedBackground << stats.NWinterMortality << stats.NCohortsSpread << stats.NCohortsLanded << stats.NInfested;
    GlobalSettings::instance()->outputManager()->execute("barkbeetle");
    //GlobalSettings::instance()->outputManager()->save();

    // execute the after fire event
    if (!mAfterExecEvent.isEmpty()) {
        // evaluate the javascript function...
        GlobalSettings::instance()->executeJavascript(mAfterExecEvent);
    }




}

void BarkBeetleModule::scanResourceUnitTrees(const QPointF &position)
{
    // if this resource unit was already scanned in this bark beetle event, then do nothing
    // the flags are reset
    if (!mRUGrid.coordValid(position))
        return;

    if (mRUGrid.valueAt(position).scanned)
        return;

    ResourceUnit *ru = GlobalSettings::instance()->model()->ru(position);
    if (!ru)
        return;

    QVector<Tree>::const_iterator tend = ru->trees().constEnd();
    for (QVector<Tree>::const_iterator t = ru->trees().constBegin(); t!=tend; ++t) {
        if (!t->isDead() && t->species()->id()==QStringLiteral("piab") && t->dbh()>params.minDbh) {

            const QPoint &tp = t->positionIndex();
            QPoint pcell(tp.x()/cPxPerHeight, tp.y()/cPxPerHeight);

            BarkBeetleCell &bb=mGrid.valueAtIndex(pcell);
            if (t->dbh() > bb.dbh) {
                bb.dbh = t->dbh();
                bb.tree_stress = t->stressIndex();
            }

        }
    }
    // set the "processed" flag
    mRUGrid.valueAt(position).scanned = true;
}


void BarkBeetleModule::yearBegin()
{
    // reset the scanned flag of resource units (force reload of stand structure)
    for (BarkBeetleRUCell *bbru=mRUGrid.begin();bbru!=mRUGrid.end();++bbru)
        bbru->scanned = false;
}

void BarkBeetleModule::calculateGenerations()
{
    // calculate generations
    DebugTimer d("BB:generations");
    ResourceUnit **ruptr = GlobalSettings::instance()->model()->RUgrid().begin();
    BarkBeetleRUCell *bbptr = mRUGrid.begin();
    while (bbptr<mRUGrid.end()) {
        if (*ruptr) {
            bbptr->scanned = false;
            bbptr->killed_trees = false;
            bbptr->generations = mGenerations.calculateGenerations(*ruptr);
            bbptr->add_sister = mGenerations.hasSisterBrood();
            bbptr->cold_days = bbptr->cold_days_late + mGenerations.frostDaysEarly();
            bbptr->cold_days_late = mGenerations.frostDaysLate(); // save for next year
            stats.maxGenerations = std::max(int(bbptr->generations), stats.maxGenerations);
        }
        ++bbptr; ++ruptr;

    }
}

void BarkBeetleModule::startSpread()
{
    // calculate winter mortality
    // background probability of infestation
    for (BarkBeetleCell *b=mGrid.begin();b!=mGrid.end();++b) {
        if (b->infested) {
            stats.infestedStart++;
            // base mortality
            if (drandom()<params.winterMortalityBaseLevel) {
                // the beetles on the pixel died
                b->setInfested(false);
                stats.NWinterMortality++;
            } else {
                // winter mortality - maybe the beetles die due to low winter temperatures
                int cold_days = mRUGrid.constValueAt(mGrid.cellCenterPoint(mGrid.indexOf(b))).cold_days;
                // int cold_days = mRUGrid.valueAtIndex(mGrid.indexOf(b)/cHeightPerRU).cold_days;
                double p_winter = mWinterMortalityFormula.calculate(cold_days);
                if (drandom()<p_winter) {
                    b->setInfested(false);
                    stats.NWinterMortality++;
                }
            }

        } else if (b->isPotentialHost() && drandom()<params.backgroundInfestationProbability) {
            // background activation
            b->setInfested(true);
            stats.infestedBackground++;
        }

        b->n = 0;
        b->killed=false;
    }

}

void BarkBeetleModule::barkbeetleSpread()
{
    DebugTimer t("BBSpread");
    for (int generation=1;generation<=stats.maxGenerations;++generation) {

        GridRunner<BarkBeetleCell> runner(mGrid, mGrid.rectangle());
        GridRunner<BarkBeetleCell> targeter(mGrid, mGrid.rectangle());
        while (BarkBeetleCell *b=runner.next()) {
            if (!b->infested)
                continue;
            QPoint start_index = runner.currentIndex();
            BarkBeetleRUCell &bbru=mRUGrid.valueAt(runner.currentCoord());
            if (generation>bbru.generations)
                continue;

            // the number of cohorts that spread is higher, if sister broods
            // could develop (e.g. 1 generation and 1 sister generation -> higher number of beetles that
            // start spreading from the current pixel
            int n_packets = bbru.add_sister ? params.cohortsPerSisterbrood : params.cohortsPerGeneration;
            stats.NCohortsSpread += n_packets;

            // mark this cell as "dead" (as the beetles have killed the host trees and now move on)
            b->finishedSpread(mIteration>0 ? mIteration+1 : generation);
            // mark the resource unit, that some killing is required
            bbru.killed_trees = true;

            BarkBeetleCell *nb8[8];
            for (int i=0;i<n_packets;++i) {
                // estimate distance and direction of spread
                double rho = mKernelPDF.get(); // distance (m)
                double phi = nrandom(0, 2*M_PI); // direction (degree)
                // calculate the pixel

                QPoint pos = start_index + QPoint(qRound( rho * sin(phi) / cHeightSize ),  qRound( rho * cos(phi) / cHeightSize ) );
                // don't spread to the initial start pixel
                if (start_index == pos)
                    continue;
                targeter.setPosition(pos);
                if (!targeter.isValid())
                    continue;

                BarkBeetleCell *target=0;
                if (targeter.current()->isPotentialHost()) {
                    // found a potential host at the immediate target pixel
                    target = targeter.current();
                } else {
                    // get elements of the moore-neighborhood
                    // and look for a potential host
                    targeter.neighbors8(nb8);
                    int idx = irandom(0,7);
                    for (int j=0;j<8;j++) {
                        BarkBeetleCell *nb = nb8[ (idx+j) % 8 ];
                        if (nb && nb->isPotentialHost()) {
                            target = nb;
                            break;
                        }
                    }
                }

                // attack the target pixel if a target could be identified
                if (target) {
                    target->n++;
                }
            }
        }  // while

        // now evaluate whether the landed beetles are able to infest the target trees
        runner.reset();
        while (BarkBeetleCell *b=runner.next()) {
            if (b->n > 0) {
                stats.NCohortsLanded+=b->n;
                stats.NPixelsLanded++;
                // the cell is attacked by n packages. Calculate the probability that the beetles win.
                // the probability is derived from an expression with the parameter "tree_stress"
                double p_col = limit(mColonizeProbability.calculate(b->tree_stress), 0., 1.);
                // the attack happens 'n' times, therefore the probability is higher
                double p_ncol = 1. - pow(1.-p_col, b->n);
                b->p_colonize = std::max(b->p_colonize, float(p_ncol));
                if (drandom() < p_ncol) {
                    // attack successful - the pixel gets infested
                    b->setInfested(true);
                    stats.NInfested++;
                }
                b->n = 0; // reset the counter

            }
        }
    }
}

void BarkBeetleModule::barkbeetleKill()
{
    int n_killed=0;
    double basal_area=0;
    for (BarkBeetleRUCell *rucell=mRUGrid.begin(); rucell!=mRUGrid.end(); ++rucell)
        if (rucell->killed_trees) {
            // there are killed pixels within the resource unit....
            QVector<Tree> &tv = GlobalSettings::instance()->model()->RUgrid().constValueAtIndex(mRUGrid.indexOf(rucell))->trees();
            for (QVector<Tree>::const_iterator t=tv.constBegin(); t!=tv.constEnd(); ++t) {
                if (!t->isDead() && t->dbh()>params.minDbh && t->species()->id()==QStringLiteral("piab")) {
                    // check if on killed pixel?
                    if (mGrid.constValueAt(t->position()).killed) {
                        // yes: kill the tree:
                        Tree *tree = const_cast<Tree*>(&(*t));
                        n_killed++;
                        basal_area+=tree->basalArea();
                        tree->removeDisturbance(0., 1., // 0% of the stem to soil, 100% to snag (keeps standing)
                                                1., 0.,   // 100% of branches to soil
                                                1.);      // 100% of foliage to soil

                    }
                }
            }

        }
    stats.NTreesKilled = n_killed;
    stats.BasalAreaKilled = basal_area;

}


//*********************************************************************************
//************************************ BarkBeetleLayers ***************************
//*********************************************************************************


double BarkBeetleLayers::value(const BarkBeetleCell &data, const int param_index) const
{
    switch(param_index){
    case 0: return data.n; // height
    case 1: return data.dbh; // diameter of host
    case 2: return data.infested?1.:0.; // infested yes/no
    case 3: return data.isHost()? data.killedYear : -1.; // -1: no host, 0: not killed, >0: iteration/generation
    case 4: return data.p_colonize; // probability of kill
    default: throw IException(QString("invalid variable index for a BarkBeetleCell: %1").arg(param_index));
    }
}


const QVector<LayeredGridBase::LayerElement> &BarkBeetleLayers::names()
{
    if (mNames.isEmpty())
        mNames = QVector<LayeredGridBase::LayerElement>()
                << LayeredGridBase::LayerElement(QLatin1Literal("value"), QLatin1Literal("grid value of the pixel"), GridViewRainbow)
                << LayeredGridBase::LayerElement(QLatin1Literal("dbh"), QLatin1Literal("diameter of thickest spruce tree on the 10m pixel"), GridViewRainbow)
                << LayeredGridBase::LayerElement(QLatin1Literal("infested"), QLatin1Literal("infested pixels (1) are colonized by beetles."), GridViewHeat)
                << LayeredGridBase::LayerElement(QLatin1Literal("dead"), QLatin1Literal("iteration at which the treees on the pixel were killed (0: alive, -1: no host trees)"), GridViewRainbow)
                << LayeredGridBase::LayerElement(QLatin1Literal("p_killed"), QLatin1Literal("highest probability (within one year) that a pixel is colonized/killed (integrates the number of arriving beetles and the defense state) 0..1"), GridViewHeat);
    return mNames;

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

const QVector<LayeredGridBase::LayerElement> &BarkBeetleRULayers::names()
{
    if (mNames.isEmpty())
        mNames = QVector<LayeredGridBase::LayerElement>()
                << LayeredGridBase::LayerElement(QLatin1Literal("generations"), QLatin1Literal("total number of bark beetle generations"), GridViewHeat);
    return mNames;
}

bool BarkBeetleRULayers::onClick(const QPointF &world_coord) const
{
    qDebug() << "received click" << world_coord;
    return true; // handled the click

}
