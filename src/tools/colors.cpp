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
#include "colors.h"
#include "dem.h"

QVector<QColor> Colors::mBrewerDiv = QVector<QColor>() << QColor("#543005") << QColor("#8c510a") << QColor("#bf812d") << QColor("#dfc27d")
                                                       << QColor("#f6e8c3") << QColor("#f5f5f5") << QColor("#fdbf6f") << QColor("##c7eae5")
                                                       << QColor("#80cdc1") << QColor("#35978f") << QColor("#01665e") <<  QColor("#003c30");

QVector<QColor> Colors::mBrewerQual = QVector<QColor>() << QColor("#a6cee3") << QColor("#1f78b4") << QColor("#b2df8a") << QColor("#33a02c")
                                                       << QColor("#fb9a99") << QColor("#e31a1c") << QColor("#fdbf6f") << QColor("#ff7f00")
                                                       << QColor("#cab2d6") << QColor("#6a3d9a") << QColor("#ffff99") <<  QColor("#b15928");


QVector<QColor> Colors::mTerrainCol = QVector<QColor>() << QColor("#00A600") << QColor("#24B300") << QColor("#4CBF00") << QColor("#7ACC00")
                                                       << QColor("#ADD900") << QColor("#E6E600") << QColor("#E8C727") << QColor("#EAB64E")
                                                       << QColor("#ECB176") << QColor("#EEB99F") << QColor("#F0CFC8") <<  QColor("#F2F2F2");

QVector<QColor> Colors::mCustomColors = QVector<QColor>();

void Colors::setPalette(const GridViewType type, const float min_val, const float max_val)
{
    if (mNeedsPaletteUpdate==false && type==mCurrentType &&
            (mAutoScale==false || (minValue()==min_val && maxValue()==max_val))  )
        return;

    mHasFactors = false;
    int n = 50;
    if (type >= GridViewBrewerDiv) {
        // categorical values...
        mHasFactors = true;
        n=mFactorLabels.size();
        if (mFactorLabels.isEmpty()) {
            n=max_val;
            mFactorLabels.clear();
            for (int i=0;i<n;++i)
                mFactorLabels.append(QString("Label %1").arg(i));
        }
    }
    if (type != GridViewCustom) {
        mColors.clear();
        for (int i=0;i<n;++i)
            if (mHasFactors)
                mColors.append(colorFromValue(i, type, 0., 1.).name());
            else
                mColors.append(colorFromValue(1. - i/double(n), type, 0., 1.).name());

    }
    mLabels = QStringList() << QString::number(min_val)
                            << QString::number((3.*min_val + max_val)/4.)
                            << QString::number((min_val+max_val)/2.)
                            << QString::number((min_val + 3.*max_val)/4.)
                            << QString::number(max_val);

    if (mAutoScale) {
        mMinValue = min_val;
        mMaxValue = max_val;
    }
    mCurrentType = type;
    mNeedsPaletteUpdate = false;
    emit colorsChanged();
}

void Colors::setFactorLabels(QStringList labels)
{
    mFactorLabels = labels;
    mNeedsPaletteUpdate = true;
}

void Colors::setFactorColors(QStringList colors)
{
    mColors = colors;
    // fill also the custom colors
    mCustomColors.clear();
    for (int i=0;i<mColors.size();++i)
        mCustomColors.append(QColor(mColors[i]));
}

Colors::Colors(QWidget *parent): QObject(parent)
{
    mNeedsPaletteUpdate =true;
    mAutoScale = true;
    mHasFactors = false;
    mMeterPerPixel = 1.;
    //default start palette
    //setPalette(GridViewRainbow, 0, 1);
    // factors test
    setCaption("");
    setPalette(GridViewTerrain, 0, 4);
}

QColor Colors::colorFromPalette(const int value, const GridViewType view_type)
{
    if (value<0)
        return Qt::white;
    int n = qMax(value,0) % 12;
    QColor col;
    switch(view_type) {
    case GridViewBrewerDiv: col = mBrewerDiv[n]; break;
    case GridViewBrewerQual: col = mBrewerQual[n]; break;
    case GridViewTerrain: col = mTerrainCol[n]; break;
    case GridViewCustom: col = n<mCustomColors.size() ?  mCustomColors[n] : QColor(); break;
    default: return QColor();
    }
    if (value < 12)
        return col;
    n = qMax(value,0) % 60;
    if (n<12) return col;
    if (n<24) return col.darker(200);
    if (n<36) return col.lighter(150);
    if (n<48) return col.darker(300);
    return col.lighter(200);

}

QColor Colors::shadeColor(const QColor col, const QPointF &coordinates, const DEM *dem)
{
    if (dem) {
        float val = dem->viewGrid()->constValueAt(coordinates); // scales from 0..1
        double h, s, v;
        col.getHsvF(&h, &s, &v);
        // we adjust the 'v', the lightness: if val=0.5 -> nothing changes
        v=limit( v - (1.-val)*0.4, 0.1, 1.);
        QColor c;
        c.setHsvF(h,s,v);
        return c;

    } else
        return col;
}

// colors
QColor Colors::colorFromValue(const float value,
                              const float min_value, const float max_value,
                              const bool reverse, const bool black_white)
{
    float rval = value;
    rval = std::max(min_value, rval);
    rval = std::min(max_value, rval);
    if (reverse)
        rval = max_value - rval;
    float rel_value;
    QColor col;
    if (min_value < max_value) {
        // default: high values -> red (h=0), low values to blue (h=high)
        rel_value = 1 - (rval - min_value) / (max_value - min_value);
        if (black_white) {
            int c = (1.-rel_value)*255;
            col = QColor(c,c,c);
        } else
            col=  QColor::fromHsvF(0.66666666666*rel_value, 0.95, 0.95).rgb();
    } else {
        col = Qt::white;
    }
    return col;
}

QColor Colors::colorFromValue(const float value, const GridViewType view_type, const float min_value, const float max_value)
{
    if (view_type==GridViewGray || view_type==GridViewGrayReverse)
        return colorFromValue(value, min_value, max_value, view_type==GridViewGrayReverse, true);

    if (view_type==GridViewRainbow || view_type==GridViewRainbowReverse)
        return colorFromValue(value, min_value, max_value, view_type==GridViewRainbowReverse, false);

    if (view_type == GridViewGreens || view_type==GridViewBlues || view_type==GridViewReds) {
        float rval = value;
        rval = std::max(min_value, rval);
        rval = std::min(max_value, rval);
        float rel_value = (max_value!=min_value)?(rval - min_value) / (max_value - min_value): 0;
        int r,g,b;
        switch (view_type) {
        case GridViewGreens:  // 11,111,19
            r=220 - rel_value*(220-11); g=220-rel_value*(220-111); b=220-rel_value*(220-19); break;
        case GridViewBlues: //15,67,138
            r=220 - rel_value*(220-15); g=220-rel_value*(220-67); b=220-rel_value*(220-138); break;
        case GridViewReds: //219,31,72
            r=240 - rel_value*(220-219); g=240-rel_value*(220-31); b=240-rel_value*(220-72); break;
        default: r=g=b=0;
        }
        return QColor(r,g,b);

    }
    if (view_type == GridViewHeat) {
        float rval = value;
        rval = std::max(min_value, rval);
        rval = std::min(max_value, rval);
        float rel_value = 1 - (rval - min_value) / (max_value - min_value);
        int g=255, b=0;
        if (rel_value < 0.5)
            g = rel_value*2.f * 255;
        if (rel_value>0.5)
            b = (rel_value-0.5)*2.f * 255;
        return QColor(255,g,b);

    }
    return colorFromPalette(value, view_type);


}
