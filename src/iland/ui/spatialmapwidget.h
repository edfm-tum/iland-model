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
#ifndef SPATIALMAPWIDGET_H
#define SPATIALMAPWIDGET_H

#include <QImage>
#include <QPoint>
#include <QString>
#include <QWidget>

#include "colors.h"
#include "viewport.h"
#include "vizmetadata.h"

class SpatialMapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpatialMapWidget(QWidget *parent = nullptr);

    void setLayout(const SpatialGridLayout *layout, const QString &spatialKey);
    void setFrame(const SpatialFrameData &frame, GridViewType palette, double scaleMin, double scaleMax, const QString &speciesLabel = QString());
    void setEmptyMessage(const QString &message);
    void resetZoom();
    QImage grabImage();

signals:
    void cellHovered(const QString &infoText);
    void ruClicked(int ruIndex, int rid);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QRect mapAreaRect() const;
    void resetViewportGeometry(bool resetZoomLevel);
    QString formatNumber(double v) const;

    const SpatialGridLayout *mLayout = nullptr;
    QString mSpatialKey = QStringLiteral("rid");
    SpatialFrameData mFrame;
    GridViewType mPalette = GridViewTurbo;
    double mScaleMin = 0.0;
    double mScaleMax = 1.0;
    QString mSpeciesLabel;
    QString mEmptyMessage;

    Viewport mVp;
    bool mViewportInitialized = false;
    QPoint mPressPos;
    QPoint mDragLastPos;
    bool mDragging = false;
    QPoint mHoveredCell = QPoint(-1, -1);

    Colors mColorsInit; // ensures Colors static palettes are populated
};

/**
 * @brief Standalone legend widget for spatial Resource Unit maps (docked in Output legend).
 */
class SpatialLegendWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpatialLegendWidget(QWidget *parent = nullptr);

    void setLegendData(const SpatialFrameData &frame, GridViewType palette, double scaleMin, double scaleMax,
                       const QString &speciesLabel, const QString &layoutSource, const QString &scaleModeLabel = QString());
    void clearLegend(const QString &message = QString());

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SpatialFrameData mFrame;
    GridViewType mPalette = GridViewTurbo;
    double mScaleMin = 0.0;
    double mScaleMax = 1.0;
    QString mSpeciesLabel;
    QString mLayoutSource;
    QString mScaleModeLabel;
    QString mEmptyMessage;
    bool mHasFrame = false;

    Colors mColorsInit; // ensures Colors static palettes are populated
};

#endif // SPATIALMAPWIDGET_H
