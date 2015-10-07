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
    QString formula = xml.value("model.settings.grass.grassPotential");
    if (formula.isEmpty())
        throw IException("setup of 'grass': required expression 'grassPotential' is missing.");
    mGrassPotential.setExpression(formula);
    mGrassPotential.linearize(0.,1., 512);

    formula = xml.value("model.settings.grass.grassEffect");
    if (formula.isEmpty())
        throw IException("setup of 'grass': required expression 'grassEffect' is missing.");
    mGrassEffect.setExpression(formula);
    mMaxTimeLag = xml.valueDouble("model.settings.grass.maxTimeLag");
    if (mMaxTimeLag==0)
        throw IException("setup of 'grass': value of 'maxTimeLag' is invalid or missing.");
    mGrowthRate = 256 / mMaxTimeLag;

    // set up the effect on regeneration in 256 steps
    for (int i=0;i<256;++i) {
        double effect = mGrassEffect.calculate(i/255.);
        mEffect[i] = limit(effect, 0., 1.);
    }

    mMaxState = limit(mGrassPotential.calculate(1.f), 0., 1.)*255; // the max value of the potential function

    GlobalSettings::instance()->controller()->addLayers(mLayers, QStringLiteral("grass cover"));
    mEnabled = true;
    qDebug() << "setup of grass cover complete.";

}

void GrassCover::setInitialValues(const QVector<float *> &LIFpixels, const int percent)
{
    if (!enabled())
        return;
    unsigned char cval = limit(percent / 100., 0., 1.)*255;
    Grid<float> *lif_grid = GlobalSettings::instance()->model()->grid();
    for (QVector<float *>::const_iterator it = LIFpixels.constBegin(); it!=LIFpixels.constEnd(); ++it)
        mGrid.valueAtIndex(lif_grid->indexOf(*it)) = cval;
}

void GrassCover::execute()
{
    if (!enabled())
        return;

    // Main function of the grass submodule
    float *lif = GlobalSettings::instance()->model()->grid()->begin();
    float *end_lif = GlobalSettings::instance()->model()->grid()->end();
    unsigned char *gr = mGrid.begin();

    // loop over every LIF pixel
    for (; lif!=end_lif;++lif, ++gr) {
        // calculate potential grass vegetation cover
        if (*lif == 1.f && *gr==mMaxState)
            continue;

        int potential = limit(mGrassPotential.calculate(1.f- *lif), 0., 1.)*255;
        *gr = qMin( int(*gr) + mGrowthRate, potential);

    }

}



double GrassCoverLayers::value(const unsigned char &data, const int index) const
{
    if (!mGrassCover->enabled()) return 0.;
    switch(index){
    case 0: return mGrassCover->effect(data); //effect
    case 1: return data/255.; // state
    default: throw IException(QString("invalid variable index for a GrassCoverLayers: %1").arg(index));
    }
}

const QVector<LayeredGridBase::LayerElement> &GrassCoverLayers::names()
{
    if (mNames.isEmpty())
        mNames = QVector<LayeredGridBase::LayerElement>()
                << LayeredGridBase::LayerElement(QLatin1Literal("effect"), QLatin1Literal("prohibiting effect on regeneration [0..1]"), GridViewGreens)
                << LayeredGridBase::LayerElement(QLatin1Literal("cover"), QLatin1Literal("current grass cover on pixels [0..1]"), GridViewGreens);
    return mNames;

}
