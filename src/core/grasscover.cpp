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
#include "grasscover.h"

#include "globalsettings.h"
#include "xmlhelper.h"
#include "model.h"
#include "modelcontroller.h"

GrassCover::GrassCover()
{
    mLayers = new GrassCoverLayers();
    mLayers->setGrid(mGrid, this);
    mEnabled = false;
    mType = Invalid;
}

GrassCover::~GrassCover()
{
    delete mLayers;
}

void GrassCover::setup()
{
    XmlHelper xml=GlobalSettings::instance()->settings();
    if (!xml.valueBool("model.settings.grass.enabled")) {
        // clear grid
        mGrid.clear();
        GlobalSettings::instance()->controller()->removeLayers(mLayers);
        mEnabled=false;
        qDebug() << "grass module not enabled";
        return;
    }
    // create the grid
    mGrid.setup(GlobalSettings::instance()->model()->grid()->metricRect(), GlobalSettings::instance()->model()->grid()->cellsize());
    mGrid.wipe();

    mType = Invalid;
    QString type = xml.value("model.settings.grass.type");
    if (type == QStringLiteral("pixel"))
        mType = Pixel;

    if (type==QStringLiteral("continuous"))
        mType = Continuous;

    if (mType == Invalid)
        throw IException("GrassCover::setup: invalid 'grass.type'. Allowed: 'continous' and 'pixel'.");

    if (mType == Pixel) {
        // setup of pixel based / discrete approach
        QString formula = xml.value("model.settings.grass.grassDuration");
        if (formula.isEmpty())
            throw IException("GrassCover::setup(): missing equation for 'grassDuration'.");
        mPDF.setup(formula, 0., 100.);
        mGrassLIFThreshold = xml.valueDouble("model.settings.grass.LIFThreshold", 0.2);

        // clear array
        for (int i=0;i<GRASSCOVERSTEPS;++i) {
            mEffect[i] = 0.;
        }

    } else {

        // setup of continuous grass concept

        QString formula = xml.value("model.settings.grass.grassPotential");
        if (formula.isEmpty())
            throw IException("setup of 'grass': required expression 'grassPotential' is missing.");
        mGrassPotential.setExpression(formula);
        mGrassPotential.linearize(0.,1., qMin(GRASSCOVERSTEPS, 1000));

        formula = xml.value("model.settings.grass.grassEffect");
        if (formula.isEmpty())
            throw IException("setup of 'grass': required expression 'grassEffect' is missing.");
        mGrassEffect.setExpression(formula);
        mMaxTimeLag = xml.valueDouble("model.settings.grass.maxTimeLag");
        if (mMaxTimeLag==0)
            throw IException("setup of 'grass': value of 'maxTimeLag' is invalid or missing.");
        mGrowthRate = GRASSCOVERSTEPS / mMaxTimeLag;

        // set up the effect on regeneration in NSTEPS steps
        for (int i=0;i<GRASSCOVERSTEPS;++i) {
            double effect = mGrassEffect.calculate(i/double(GRASSCOVERSTEPS-1));
            mEffect[i] = limit(effect, 0., 1.);
        }

        mMaxState = limit(mGrassPotential.calculate(1.f), 0., 1.)*(GRASSCOVERSTEPS-1); // the max value of the potential function
    }

    GlobalSettings::instance()->controller()->addLayers(mLayers, QStringLiteral("grass cover"));
    mEnabled = true;
    qDebug() << "setup of grass cover complete.";

}

void GrassCover::setInitialValues(const QVector<float *> &LIFpixels, const int percent)
{
    if (!enabled())
        return;
    if (mType == Continuous) {
        grass_grid_type cval = limit(percent / 100., 0., 1.)*(GRASSCOVERSTEPS-1);
        if (cval > mMaxState)
            cval = mMaxState;

        Grid<float> *lif_grid = GlobalSettings::instance()->model()->grid();
        for (QVector<float *>::const_iterator it = LIFpixels.constBegin(); it!=LIFpixels.constEnd(); ++it)
            mGrid.valueAtIndex(lif_grid->indexOf(*it)) = cval;
    } else {
        // mType == Pixel
        Grid<float> *lif_grid = GlobalSettings::instance()->model()->grid();
        for (QVector<float *>::const_iterator it = LIFpixels.constBegin(); it!=LIFpixels.constEnd(); ++it) {
            if (percent > irandom(0,100))
                mGrid.valueAtIndex(lif_grid->indexOf(*it)) = mPDF.get();
            else
                mGrid.valueAtIndex(lif_grid->indexOf(*it)) = 0;
        }

    }
}

void GrassCover::execute()
{
    if (!enabled())
        return;

    // Main function of the grass submodule
    float *lif = GlobalSettings::instance()->model()->grid()->begin();
    float *end_lif = GlobalSettings::instance()->model()->grid()->end();
    grass_grid_type *gr = mGrid.begin();

    if (mType == Continuous) {
        // loop over every LIF pixel
        int skipped=0;
        for (; lif!=end_lif;++lif, ++gr) {
            // calculate potential grass vegetation cover
            if (*lif == 1.f && *gr==mMaxState) {
                ++skipped;
                continue;
            }

            int potential = limit(mGrassPotential.calculate(*lif), 0., 1.)*(GRASSCOVERSTEPS-1);
            *gr = qMin( int(*gr) + mGrowthRate, potential);

        }
        //qDebug() << "skipped" << skipped;
    } else {
        // type = Pixel
        for (; lif!=end_lif;++lif, ++gr) {
            if (*gr>1)
                (*gr)--; // count down the years (until gr=1)

            if (*gr==0 && *lif>mGrassLIFThreshold) {
                // enable grass cover
                double v = qMax(mPDF.get(), 0.);
                *gr = v + 1;
            }
            if (*gr==1 && *lif<mGrassLIFThreshold) {
                // now LIF is below the threshold - this enables the pixel get grassy again
                *gr = 0;
            }
        }
    }

}



double GrassCoverLayers::value(const grass_grid_type &data, const int index) const
{
    if (!mGrassCover->enabled()) return 0.;
    switch(index){
    case 0: return mGrassCover->effect(data); //effect
    case 1: return mGrassCover->cover(data); // cover
    default: throw IException(QString("invalid variable index for a GrassCoverLayers: %1").arg(index));
    }
}

const QVector<LayeredGridBase::LayerElement> &GrassCoverLayers::names()
{
    if (mNames.isEmpty())
        mNames = QVector<LayeredGridBase::LayerElement>()
                << LayeredGridBase::LayerElement(QLatin1Literal("effect"), QLatin1Literal("prohibiting effect on regeneration [0..1]"), GridViewGreens)
                << LayeredGridBase::LayerElement(QLatin1Literal("cover"), QLatin1Literal("current grass cover on pixels [0..1 for continuous, or #(years+2) for pixel mode]"), GridViewGreens);
    return mNames;

}
