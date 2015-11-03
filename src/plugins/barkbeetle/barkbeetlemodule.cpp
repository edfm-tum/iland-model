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

#include "barkbeetlemodule.h"

#include "globalsettings.h"
#include "model.h"
#include "modelcontroller.h"
#include "resourceunit.h"
#include "species.h"
#include "debugtimer.h"
#include "outputmanager.h"
#include "climate.h"
#include "abe/forestmanagementengine.h"


int BarkBeetleCell::total_infested = 0;
int BarkBeetleCell::max_iteration = 0;



BarkBeetleModule::BarkBeetleModule()
{
    mLayers.setGrid(mGrid);
    mRULayers.setGrid(mRUGrid);
    mSimulate = false;
    mEnabled = false;
    mYear = 0;

}

BarkBeetleModule::~BarkBeetleModule()
{
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

    double ru_value = GlobalSettings::instance()->settings().valueDouble("modules.barkbeetle.backgroundInfestationProbability");
    // probabilistic OR-calculation: p_1ha = 1 - (1 - p_pixel)^n   -> p_pixel = 1 - (1-p_ha)^(1/n)
    // we need the probability of damage starting on a single 10m - pixel.
    double pixel_value = 1. - pow(1. - ru_value, 1./( cellsize()*cellsize()));

    // set all pixel within the resource unit
    GridRunner<BarkBeetleCell> runner(mGrid, ru->boundingBox());
    while (BarkBeetleCell *cell = runner.next())
        cell->backgroundInfestationProbability = pixel_value;

}

void BarkBeetleModule::loadParameters()
{
    const XmlHelper xml = GlobalSettings::instance()->settings().node("modules.barkbeetle");
    params.cohortsPerGeneration = xml.valueDouble(".cohortsPerGeneration", params.cohortsPerGeneration);
    params.cohortsPerSisterbrood = xml.valueDouble(".cohortsPerSisterbrood", params.cohortsPerSisterbrood);
    params.spreadKernelMaxDistance = xml.valueDouble(".spreadKernelMaxDistance", params.spreadKernelMaxDistance);
    params.spreadKernelFormula = xml.value(".spreadKernelFormula", "100*(1-x)^4");
    params.minDbh = xml.valueDouble(".minimumDbh", params.minDbh);
    mKernelPDF.setup(params.spreadKernelFormula,0.,params.spreadKernelMaxDistance);
    params.backgroundInfestationProbability = xml.valueDouble(".backgroundInfestationProbability", params.backgroundInfestationProbability);
    params.stormInfestationProbability = xml.valueDouble(".stormInfestationProbability", params.stormInfestationProbability);
    params.deadTreeSelectivity = xml.valueDouble(".deadTreeSelectivity", params.deadTreeSelectivity);


    QString formula = xml.value(".colonizeProbabilityFormula", "0.1");
    mColonizeProbability.setExpression(formula);

    formula = xml.value(".winterMortalityFormula", "polygon(days, 0,0, 30, 0.6)");
    mWinterMortalityFormula.setExpression(formula);

    formula = xml.value(".outbreakClimateSensitivityFormula", "1");
    mOutbreakClimateSensitivityFormula.setExpression(formula);
    double **p = &mClimateVariables[0];
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Pspring");
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Psummer");
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Pautumn");
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Pwinter");
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Tspring");
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Tsummer");
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Tautumn");
    *p++ = mOutbreakClimateSensitivityFormula.addVar("Twinter");
    mOutbreakClimateSensitivityFormula.parse();

    params.outbreakDurationMin = xml.valueDouble(".outbreakDurationMin", 0.);
    params.outbreakDurationMax = xml.valueDouble(".outbreakDurationMax", 0.);
    formula = xml.value(".outbreakDurationMortalityFormula", "0");
    mOutbreakDurationFormula.setExpression(formula);

    QString ref_table_name = xml.value(".referenceClimate.tableName");
    QString ref_climate_values = xml.value(".referenceClimate.seasonalPrecipSum");
    QStringList tmp = ref_climate_values.split(',');
    mRefClimateAverages.clear();
    foreach(QString v, tmp) mRefClimateAverages.push_back(v.toDouble());
    ref_climate_values = xml.value(".referenceClimate.seasonalTemperatureAverage");
    tmp = ref_climate_values.split(',');
    foreach(QString v, tmp) mRefClimateAverages.push_back(v.toDouble());
    if (mRefClimateAverages.count()!=8)
        throw IException("Barkbeetle Setup: Error: invalid values for seasonalPrecipSum or seasonalTemperatureAverage (4 ','-separated values expected).");

    mRefClimate = 0;
    foreach(const Climate *clim, GlobalSettings::instance()->model()->climates())
        if (clim->name()==ref_table_name) {
            mRefClimate = clim; break;
        }
    qDebug() << "refclimate" << mRefClimateAverages;
    if (!mRefClimate)
        throw IException(QString("Barkbeetle Setup: Error: a climate table '%1' (given in modules.barkbeetle.referenceClimate.tableName) for the barkbeetle reference climate does not exist.").arg(ref_table_name));

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
        b->reset();

    BarkBeetleRUCell ru_cell;
    mRUGrid.initialize(ru_cell);

    BarkBeetleCell::resetCounters();
    stats.clear();


}

void BarkBeetleModule::loadAllVegetation()
{
    // refetch vegetation information (if necessary)
    foreach (const ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
        scanResourceUnitTrees(ru->boundingBox().center());
    }

    // save the damage information of the last year
    for (BarkBeetleRUCell *bbru=mRUGrid.begin(); bbru!=mRUGrid.end(); ++bbru) {
        bbru->killed_pixels = 0; // reset
    }



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

    // outbreak probability
    calculateOutbreakFactor();

    // load the vegetation (skipped if this is not the initial iteration)
    loadAllVegetation();

    // background probability of infestation, calculation of antagonist levels
    startSpread();

    // the spread of beetles (and attacking of trees)
    barkbeetleSpread();

    // write back the effects of the bark beetle module to the forest in iLand
    barkbeetleKill();

    // create some outputs....
    qDebug() << "iter/background-inf/winter-mort/storm-inf/N spread/N landed/N infested: " << mIteration << stats.infestedBackground << stats.NWinterMortality << stats.infestedStorm << stats.NCohortsSpread << stats.NCohortsLanded << stats.NInfested;
    GlobalSettings::instance()->outputManager()->execute("barkbeetle");
    //GlobalSettings::instance()->outputManager()->save();

    // execute the after bark-beetle infestation event
    if (!mAfterExecEvent.isEmpty()) {
        // evaluate the javascript function...
        GlobalSettings::instance()->executeJavascript(mAfterExecEvent);
    }

}

void BarkBeetleModule::treeDeath(const Tree *tree)
{
    // do nothing if the tree was killed by bark beetles
    if (tree->isDeadBarkBeetle())
        return;
    // we only process trees here that are either
    // killed by storm or deliberately killed and dropped
    // by management and are not are already salvaged
    if ( !(tree->isDeadWind() || tree->isCutdown()))
        return;


    // ignore the death of trees that are too small or are not Norway spruce
    if (tree->dbh()<params.minDbh || tree->species()->id()!=QStringLiteral("piab"))
        return;

    BarkBeetleCell &cell = mGrid.valueAt(tree->position());
    if (tree->isDeadWind())
        cell.deadtrees = BarkBeetleCell::StormDamage;
    if (tree->isCutdown())
        cell.deadtrees = BarkBeetleCell::BeetleTrapTree;



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

    BarkBeetleRUCell &ru_cell = mRUGrid.valueAt(position);
    ru_cell.host_pixels=0;

    // reset the DBH on all pixels within the resource unit
    GridRunner<BarkBeetleCell> runner(mGrid, ru->boundingBox());
    while (runner.next())
        runner.current()->dbh=0.f;

    QVector<Tree>::const_iterator tend = ru->trees().constEnd();
    for (QVector<Tree>::const_iterator t = ru->trees().constBegin(); t!=tend; ++t) {
        if (!t->isDead() && t->species()->id()==QStringLiteral("piab") && t->dbh()>params.minDbh) {

            const QPoint &tp = t->positionIndex();
            QPoint pcell(tp.x()/cPxPerHeight, tp.y()/cPxPerHeight);

            BarkBeetleCell &bb=mGrid.valueAtIndex(pcell);
            // count the host pixels (only once)
            if (bb.dbh==0.f)
                ru_cell.host_pixels++;

            if (t->dbh() > bb.dbh) {
                bb.dbh = t->dbh();
                bb.tree_stress = t->stressIndex();
            }


        }
    }
    // set the "processed" flag
    ru_cell.scanned = true;
}



void BarkBeetleModule::yearBegin()
{
    // reset the scanned flag of resource units (force reload of stand structure)
    for (BarkBeetleRUCell *bbru=mRUGrid.begin();bbru!=mRUGrid.end();++bbru) {
        bbru->scanned = false;
        bbru->infested=0;
    }

    // reset the effect of wind-damaged trees and "fangbaueme"
    for (BarkBeetleCell *c=mGrid.begin(); c!=mGrid.end(); ++c)
        c->deadtrees = BarkBeetleCell::NoDeadTrees;

    mYear = GlobalSettings::instance()->instance()->currentYear();
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

void BarkBeetleModule::calculateOutbreakFactor()
{
    if (!mRefClimate)
        return;
    const double *t = mRefClimate->temperatureMonth();
    const double *p = mRefClimate->precipitationMonth();
    // Pspring, Psummer, Pautumn, Pwinter, Tspring, Tsummer, Tautumn, Twinter
    // seasonal sum precip -> relative values
    *mClimateVariables[0] = (p[2]+p[3]+p[4]) / mRefClimateAverages[0];
    *mClimateVariables[1] = (p[5]+p[6]+p[7]) / mRefClimateAverages[1];
    *mClimateVariables[2] = (p[8]+p[9]+p[10]) / mRefClimateAverages[2];
    *mClimateVariables[3] = (p[11]+p[0]+p[1])  / mRefClimateAverages[3]; // not really clean.... using all month of the current year
    // temperatures (mean monthly temp) -> delta
    *mClimateVariables[4] = (t[2]+t[3]+t[4])/3. - mRefClimateAverages[4];
    *mClimateVariables[5] = (t[5]+t[6]+t[7])/3. - mRefClimateAverages[5];
    *mClimateVariables[6] = (t[8]+t[9]+t[10])/3. - mRefClimateAverages[6];
    *mClimateVariables[7] = (t[11]+t[0]+t[1])/3. - mRefClimateAverages[7];

    mRc = qMax(mOutbreakClimateSensitivityFormula.execute(), 0.);
    qDebug() << "Barkbeelte: rc:" << mRc;

}

void BarkBeetleModule::startSpread()
{
    // calculate winter mortality
    //  probability of infestation
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

        } else if (b->isPotentialHost()) {
            // calculate probability
            double odds_base = b->backgroundInfestationProbability / (1. - b->backgroundInfestationProbability);
            double p_mod = (odds_base*mRc) / (1. + odds_base*mRc);
            if (drandom() < p_mod) {
                // background activation
                b->setInfested(true);
                b->outbreakYear = mYear; // this outbreak starts in the current year
                stats.infestedBackground++;
            }
        }

        b->n = 0;
        b->killed=false;
        b->packageOutbreakYear = 0.f;

    }

    prepareInteractions();

    // tell the forest management (at least if someone is interested)
    // if bark beetle attacks are likely
    ABE::ForestManagementEngine *abe = GlobalSettings::instance()->model()->ABEngine();
    if (abe) {
        // reset the scanned flag of resource units (force reload of stand structure)
        bool forest_changed = false;
        for (BarkBeetleRUCell *bbru=mRUGrid.begin();bbru!=mRUGrid.end();++bbru) {
            if (bbru->generations>=1. && bbru->infested>0) {
                // notify about potential bb-attack
                ResourceUnit *ru = GlobalSettings::instance()->model()->RUgrid()[mRUGrid.indexOf(bbru)];
                forest_changed |= abe->notifyBarkbeetleAttack(ru, bbru->generations, bbru->infested);

            }

        }
        if (forest_changed)
            prepareInteractions(true);
    }

}

void BarkBeetleModule::prepareInteractions(bool update_interaction)
{
    // loop over all cells of the grid and decide
    // for each pixel if it is in the proximinity of (attractive) deadwood
    // we assume an influence within the 5x5 pixel neighborhood

    if (!update_interaction && params.stormInfestationProbability<1.) {
        // reduce the effect of wind-damaged trees for bark beetle spread (disable pixels with p=1-stormInfestationProbability), but do
        // it only during the first pass
        for (BarkBeetleCell *c=mGrid.begin(); c!=mGrid.end(); ++c)
            if (c->deadtrees == BarkBeetleCell::StormDamage && drandom()>params.stormInfestationProbability)
                c->deadtrees = BarkBeetleCell::NoDeadTrees;
    }


    for (int y=0;y<mGrid.sizeY();++y)
        for (int x=0;x<mGrid.sizeX();++x) {
            BarkBeetleCell &cell = mGrid.valueAtIndex(x,y);
            if (cell.deadtrees==BarkBeetleCell::NoDeadTrees) {
                int has_neighbors=0;
                for (int dy=-2;dy<=2;++dy)
                    for (int dx=-2;dx<=2;++dx)
                        has_neighbors += mGrid.isIndexValid(x+dx,y+dy) ? (mGrid(x+dx,y+dy).deadtrees==BarkBeetleCell::StormDamage || mGrid(x+dx,y+dy).deadtrees==BarkBeetleCell::BeetleTrapTree ? 1: 0) : 0;

                if (has_neighbors>0)
                    cell.deadtrees = BarkBeetleCell::StormDamageVicinity;


            }
            if (cell.deadtrees==BarkBeetleCell::StormDamage) {
                // the pixel acts as a source
                cell.setInfested(true);
                cell.outbreakYear = mYear; // this outbreak starts in the current year
                stats.infestedStorm++;
            }
            if (cell.infested) {
                // record infestation for the resource unit
                mRUGrid[mGrid.cellCenterPoint(QPoint(x,y))].infested++;
            }
        }
}


void BarkBeetleModule::barkbeetleSpread()
{
    DebugTimer t("BBSpread");

    double ant_years = qMax(nrandom(params.outbreakDurationMin, params.outbreakDurationMax), 1.);
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

            // antagonists:
            double t_ob = mYear - b->outbreakYear;
            double p_antagonist_mort = mOutbreakDurationFormula.calculate(limit(t_ob / ant_years, 0., 1.));
            n_packets = qRound(n_packets * (1. - p_antagonist_mort));

            stats.NCohortsSpread += n_packets;

            // mark this cell as "dead" (as the beetles have killed the host trees and now move on)
            b->finishedSpread(mIteration>0 ? mIteration+1 : generation);
            // mark the resource unit, that some killing is required
            bbru.killed_trees = true;
            bbru.killed_pixels++;
            bbru.host_pixels--;

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


                // effect of windthrown trees or "fangbaume"
                if (targeter.current()->isNeutralized())
                    if (drandom() < params.deadTreeSelectivity)
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
                    target->packageOutbreakYear += b->outbreakYear;
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
                    b->outbreakYear = b->n>0 ? b->packageOutbreakYear / float(b->n) : mYear; // b->n always >0, but just to silence compiler warning ;)
                    stats.NInfested++;
                } else {
                    b->n = 0; // reset the counter
                    b->packageOutbreakYear = 0.f;
                }

            }
        }

    } // for(generations)

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
                        tree->setDeathReasonBarkBeetle();
                        n_killed++;
                        basal_area+=tree->basalArea();
                        if (!mSimulate) // remove tree only if not in simulation mode
                            tree->removeDisturbance(0., 1., // 0% of the stem to soil, 100% to snag (keeps standing)
                                                    0., 1.,   // 100% of branches to snags
                                                    0.);      // 100% of foliage to snags (will be dropped from there anyways)

                    }
                }
            }

        }
    stats.NTreesKilled = n_killed;
    stats.BasalAreaKilled = basal_area;

    // reset the effect of wind-damaged trees and "fangbaueme" -> year begin
//    for (BarkBeetleCell *c=mGrid.begin(); c!=mGrid.end(); ++c)
//        c->deadtrees = BarkBeetleCell::NoDeadTrees;

}


//*********************************************************************************
//************************************ BarkBeetleLayers ***************************
//*********************************************************************************


double BarkBeetleLayers::value(const BarkBeetleCell &data, const int param_index) const
{
    switch(param_index){
    case 0: return data.n; // grid value on pixel
    case 1: return data.dbh; // diameter of host
    case 2: return data.infested?1.:0.; // infested yes/no
    case 3: if (data.isHost()) { // dead
                if (data.infested)
                    return data.max_iteration+1; // infested right now (will be dead soon next year)
                else
                    return data.killedYear; // iteration when killed
            }
            return -1; // no host
    case 4: return data.p_colonize; // probability of kill
    case 5: return double(data.deadtrees); // availabilitiy of deadwood (spruce)
    case 6: return data.backgroundInfestationProbability;
    case 7: return data.outbreakYear;
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
                << LayeredGridBase::LayerElement(QLatin1Literal("dead"), QLatin1Literal("iteration at which the treees on the pixel were killed (0: alive, -1: no host trees). Newly infested pixels are included (max iteration + 1)."), GridViewRainbow)
                << LayeredGridBase::LayerElement(QLatin1Literal("p_killed"), QLatin1Literal("highest probability (within one year) that a pixel is colonized/killed (integrates the number of arriving beetles and the defense state) 0..1"), GridViewHeat)
                << LayeredGridBase::LayerElement(QLatin1Literal("deadwood"), QLatin1Literal("10: trees killed by storm, 8: trap trees, 5: active vicinity of 5/8, 0: no dead trees"), GridViewRainbow)
                << LayeredGridBase::LayerElement(QLatin1Literal("outbreakProbability"), QLatin1Literal("background infestation probability (p that outbreak starts at each 10m pixel per year) (does not include the interannual climate sensitivity)"), GridViewGray)
                << LayeredGridBase::LayerElement(QLatin1Literal("outbreakAge"), QLatin1Literal("year of the outbreak (simulation year) that led to the infestation of the pixel."), GridViewGray);
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



