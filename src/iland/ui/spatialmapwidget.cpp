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

#include "spatialmapwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace {

QString formatLegendNumber(double v)
{
    const double av = std::abs(v);
    if (av == 0.0)
        return QStringLiteral("0");
    if (av >= 10000.0)
        return QString::number(v, 'f', 0);
    if (av >= 100.0)
        return QString::number(v, 'f', 1);
    if (av >= 1.0)
        return QString::number(v, 'f', 2);
    return QString::number(v, 'g', 3);
}

} // namespace

SpatialMapWidget::SpatialMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

QRect SpatialMapWidget::mapAreaRect() const
{
    return QRect(0, 0, qMax(1, width()), qMax(1, height()));
}

void SpatialMapWidget::resetViewportGeometry(bool resetZoomLevel)
{
    if (!mLayout || !mLayout->isValid()) {
        mViewportInitialized = false;
        return;
    }
    const QRectF world = mLayout->metricRect();
    const QRect screen = mapAreaRect();
    if (!world.isValid() || world.width() <= 0.0 || world.height() <= 0.0 || screen.width() <= 10 || screen.height() <= 10)
        return;

    const double padX = qMax(world.width() * 0.04, 25.0);
    const double padY = qMax(world.height() * 0.04, 25.0);
    const QRectF padded = world.adjusted(-padX, -padY, padX, padY);

    if (!mViewportInitialized || resetZoomLevel) {
        mVp = Viewport(padded, screen);
        mViewportInitialized = true;
    } else {
        mVp.setWorldRect(padded);
        mVp.setScreenRect(screen);
    }
}

void SpatialMapWidget::setLayout(const SpatialGridLayout *layout, const QString &spatialKey)
{
    const bool rectChanged = (!mLayout || !layout || mLayout->metricRect() != layout->metricRect());
    mLayout = layout;
    mSpatialKey = spatialKey;
    mHoveredCell = QPoint(-1, -1);
    resetViewportGeometry(rectChanged || !mViewportInitialized);
    update();
}

void SpatialMapWidget::setFrame(const SpatialFrameData &frame, GridViewType palette, double scaleMin, double scaleMax, const QString &speciesLabel)
{
    mFrame = frame;
    mPalette = palette;
    mScaleMin = scaleMin;
    mScaleMax = (scaleMax > scaleMin) ? scaleMax : (scaleMin + 1.0);
    mSpeciesLabel = speciesLabel;
    mEmptyMessage.clear();
    if (!mViewportInitialized)
        resetViewportGeometry(true);
    update();
}

void SpatialMapWidget::setEmptyMessage(const QString &message)
{
    mEmptyMessage = message;
    update();
}

void SpatialMapWidget::resetZoom()
{
    resetViewportGeometry(true);
    update();
}

QImage SpatialMapWidget::grabImage()
{
    return grab().toImage();
}

QString SpatialMapWidget::formatNumber(double v) const
{
    return formatLegendNumber(v);
}

void SpatialMapWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    resetViewportGeometry(false);
}

void SpatialMapWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QRect mapRect = mapAreaRect();
    painter.fillRect(mapRect, QColor(246, 247, 249));

    if (!mEmptyMessage.isEmpty()) {
        painter.setPen(QColor(95, 100, 110));
        painter.drawText(rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap, mEmptyMessage);
        return;
    }

    if (!mLayout || !mLayout->isValid()) {
        painter.setPen(QColor(95, 100, 110));
        painter.drawText(rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap,
                         QStringLiteral("No spatial RU grid layout available.\nLoad a project XML or, when inspecting an external database, use 'Spatial grid...' in Output options."));
        return;
    }

    if (!mViewportInitialized)
        resetViewportGeometry(true);

    const QString effectiveKey = mFrame.useRid ? QStringLiteral("rid") : QStringLiteral("ru");
    const Grid<int> &grid = mLayout->gridForKey(effectiveKey.isEmpty() ? mSpatialKey : effectiveKey);
    if (grid.isEmpty()) {
        painter.setPen(QColor(95, 100, 110));
        painter.drawText(mapRect, Qt::AlignCenter, QStringLiteral("Empty spatial grid."));
        return;
    }

    painter.save();
    painter.setClipRect(mapRect);

    const QRectF worldBounds = grid.metricRect();
    const QRect totalScreen = mVp.toScreen(worldBounds);
    painter.fillRect(totalScreen, QColor(252, 252, 252));

    const bool drawBorders = (mVp.meterToPixel(grid.cellsize()) >= 6);
    const QPen cellBorderPen(QColor(255, 255, 255, 90), 1);
    const QColor emptyValidColor(225, 228, 234);

    for (int iy = 0; iy < grid.sizeY(); ++iy) {
        for (int ix = 0; ix < grid.sizeX(); ++ix) {
            const int cellKey = grid.constValueAtIndex(ix, iy);
            if (cellKey < 0)
                continue;

            const QRect r = mVp.toScreen(grid.cellRect(QPoint(ix, iy)));
            if (!r.intersects(mapRect))
                continue;

            if (mFrame.isDominantSpeciesMode) {
                auto it = mFrame.dominantSpeciesByKey.constFind(cellKey);
                if (it != mFrame.dominantSpeciesByKey.constEnd()) {
                    const QColor fill = mFrame.speciesInfo.value(it.value()).second;
                    const double share = mFrame.dominantShareByKey.value(cellKey, 1.0);
                    if (share < 0.5) {
                        painter.fillRect(r, Qt::white);
                        painter.fillRect(r, QBrush(fill, Qt::Dense3Pattern));
                    } else {
                        painter.fillRect(r, fill);
                    }
                } else {
                    painter.fillRect(r, emptyValidColor);
                }
            } else {
                QColor fill = emptyValidColor;
                auto it = mFrame.valuesByKey.constFind(cellKey);
                if (it != mFrame.valuesByKey.constEnd()) {
                    fill = Colors::colorFromValue(static_cast<float>(it.value()), mPalette,
                                                  static_cast<float>(mScaleMin), static_cast<float>(mScaleMax));
                } else if (mFrame.zeroFillValidCells) {
                    fill = Colors::colorFromValue(0.0f, mPalette,
                                                  static_cast<float>(mScaleMin), static_cast<float>(mScaleMax));
                }
                painter.fillRect(r, fill);
            }
            if (drawBorders) {
                painter.setPen(cellBorderPen);
                painter.drawRect(r.adjusted(0, 0, -1, -1));
            }
        }
    }

    // Highlight hovered cell
    if (grid.isIndexValid(mHoveredCell) && grid.constValueAtIndex(mHoveredCell) >= 0) {
        const QRect hr = mVp.toScreen(grid.cellRect(mHoveredCell));
        painter.setPen(QPen(Qt::black, 2));
        painter.drawRect(hr.adjusted(0, 0, -1, -1));
    }

    painter.setPen(QColor(180, 185, 195));
    painter.drawRect(totalScreen);
    painter.restore();
}

void SpatialMapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && mapAreaRect().contains(event->pos())) {
        mPressPos = event->pos();
        mDragLastPos = event->pos();
        mDragging = true;
    }
}

void SpatialMapWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (mDragging && (event->buttons() & Qt::LeftButton)) {
        setCursor(Qt::ClosedHandCursor);
        mVp.moveTo(mDragLastPos, event->pos());
        mDragLastPos = event->pos();
        update();
        return;
    }

    if (!mLayout || !mLayout->isValid() || !mapAreaRect().contains(event->pos())) {
        if (mHoveredCell != QPoint(-1, -1)) {
            mHoveredCell = QPoint(-1, -1);
            update();
        }
        return;
    }

    const QString effectiveKey = mFrame.useRid ? QStringLiteral("rid") : QStringLiteral("ru");
    const Grid<int> &grid = mLayout->gridForKey(effectiveKey.isEmpty() ? mSpatialKey : effectiveKey);
    if (grid.isEmpty())
        return;

    const QPointF worldPos = mVp.toWorld(event->pos());
    if (!grid.coordValid(worldPos)) {
        if (mHoveredCell != QPoint(-1, -1)) {
            mHoveredCell = QPoint(-1, -1);
            update();
        }
        emit cellHovered(QString());
        return;
    }

    const QPoint cellIdx = grid.indexAt(worldPos);
    if (cellIdx != mHoveredCell) {
        mHoveredCell = cellIdx;
        update();
    }

    if (!grid.isIndexValid(cellIdx)) {
        emit cellHovered(QString());
        return;
    }

    const int cellKey = grid.constValueAtIndex(cellIdx);
    if (cellKey < 0) {
        emit cellHovered(QString("x=%1 m, y=%2 m (outside landscape)").arg(qRound(worldPos.x())).arg(qRound(worldPos.y())));
        return;
    }

    const int ruIdx = mLayout->hasRuIndexGrid() && mLayout->ruIndexGrid().isIndexValid(cellIdx)
                          ? mLayout->ruIndexGrid().constValueAtIndex(cellIdx)
                          : -1;
    const int ridVal = mLayout->hasRidGrid() && mLayout->ridGrid().isIndexValid(cellIdx)
                           ? mLayout->ridGrid().constValueAtIndex(cellIdx)
                           : -1;

    QString ruText = (ruIdx >= 0 && ridVal >= 0)
                         ? QString("RU %1 (rid %2)").arg(ruIdx).arg(ridVal)
                         : QString("%1 %2").arg(mFrame.useRid ? QStringLiteral("rid") : QStringLiteral("RU")).arg(cellKey);

    QString valText;
    if (mFrame.isDominantSpeciesMode) {
        if (mFrame.dominantSpeciesByKey.contains(cellKey)) {
            const QString sp = mFrame.dominantSpeciesByKey.value(cellKey);
            const QString spName = mFrame.speciesInfo.value(sp).first;
            const double val = mFrame.rawValueForDominant.value(cellKey, 0.0);
            const double sharePct = mFrame.dominantShareByKey.value(cellKey, 1.0) * 100.0;
            valText = QString("%1 (%2): %3 (%4%)").arg(spName, sp, formatNumber(val)).arg(sharePct, 0, 'f', 0);
        } else {
            valText = QStringLiteral("no data");
        }
    } else if (mFrame.valuesByKey.contains(cellKey)) {
        valText = QString("%1: %2").arg(mFrame.metricLabel, formatNumber(mFrame.valuesByKey.value(cellKey)));
    } else if (mFrame.zeroFillValidCells) {
        valText = QString("%1: 0").arg(mFrame.metricLabel);
    } else {
        valText = QStringLiteral("no data");
    }

    emit cellHovered(QString("%1 | %2").arg(ruText, valText));
}

void SpatialMapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (mDragging && event->button() == Qt::LeftButton) {
        setCursor(Qt::ArrowCursor);
        mDragging = false;

        if ((event->pos() - mPressPos).manhattanLength() <= 3 && mLayout && mLayout->isValid() && mapAreaRect().contains(event->pos())) {
            const QString effectiveKey = mFrame.useRid ? QStringLiteral("rid") : QStringLiteral("ru");
            const Grid<int> &grid = mLayout->gridForKey(effectiveKey.isEmpty() ? mSpatialKey : effectiveKey);
            const QPointF wp = mVp.toWorld(event->pos());
            if (grid.coordValid(wp)) {
                const QPoint idx = grid.indexAt(wp);
                if (grid.isIndexValid(idx)) {
                    const int ruIdx = mLayout->hasRuIndexGrid() && mLayout->ruIndexGrid().isIndexValid(idx)
                                          ? mLayout->ruIndexGrid().constValueAtIndex(idx)
                                          : -1;
                    const int ridVal = mLayout->hasRidGrid() && mLayout->ridGrid().isIndexValid(idx)
                                           ? mLayout->ridGrid().constValueAtIndex(idx)
                                           : -1;
                    if (ruIdx >= 0 || ridVal >= 0)
                        emit ruClicked(ruIdx, ridVal);
                }
            }
        }
    }
}

void SpatialMapWidget::wheelEvent(QWheelEvent *event)
{
    if (!mLayout || !mLayout->isValid() || !mapAreaRect().contains(event->position().toPoint()))
        return;

    const double steps = event->angleDelta().y() / 120.0;
    const double factor = qMax(1.0 - (0.2 * steps), 0.2);
    mVp.zoomTo(event->position().toPoint(), factor);
    update();
    event->accept();
}

// ---------------------------------------------------------------------------
// SpatialLegendWidget
// ---------------------------------------------------------------------------

SpatialLegendWidget::SpatialLegendWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumHeight(140);
}

void SpatialLegendWidget::setLegendData(const SpatialFrameData &frame, GridViewType palette, double scaleMin, double scaleMax,
                                        const QString &speciesLabel, const QString &layoutSource, const QString &scaleModeLabel)
{
    mFrame = frame;
    mPalette = palette;
    mScaleMin = scaleMin;
    mScaleMax = (scaleMax > scaleMin) ? scaleMax : (scaleMin + 1.0);
    mSpeciesLabel = speciesLabel;
    mLayoutSource = layoutSource;
    mScaleModeLabel = scaleModeLabel;
    mEmptyMessage.clear();
    mHasFrame = true;
    update();
}

void SpatialLegendWidget::clearLegend(const QString &message)
{
    mFrame = SpatialFrameData();
    mScaleModeLabel.clear();
    mEmptyMessage = message;
    mHasFrame = false;
    update();
}

void SpatialLegendWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    painter.setPen(palette().mid().color());
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    const int x0 = 10;
    const int availW = qMax(20, width() - 20);
    int y = 10;

    if (!mHasFrame) {
        if (!mEmptyMessage.isEmpty()) {
            painter.setPen(QColor(110, 115, 125));
            painter.drawText(rect().adjusted(10, 10, -10, -10), Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, mEmptyMessage);
        }
        return;
    }

    painter.setFont(font());
    painter.setPen(palette().text().color());
    painter.drawText(QRect(x0, y, availW, 18), Qt::AlignVCenter | Qt::AlignLeft, QString("Year: %1").arg(mFrame.year));
    y += 20;

    if (!mSpeciesLabel.isEmpty()) {
        QRect spBounds = painter.boundingRect(QRect(x0, y, availW, 40), Qt::TextWordWrap, mSpeciesLabel);
        painter.setPen(QColor(80, 86, 96));
        painter.drawText(QRect(x0, y, availW, spBounds.height() + 2), Qt::TextWordWrap, mSpeciesLabel);
        y += spBounds.height() + 6;
    }
    y += 4;

    if (mFrame.isDominantSpeciesMode) {
        QHash<QString, int> counts;
        for (auto it = mFrame.dominantSpeciesByKey.constBegin(); it != mFrame.dominantSpeciesByKey.constEnd(); ++it)
            counts[it.value()]++;
        QStringList spList = counts.keys();
        std::sort(spList.begin(), spList.end(), [&](const QString &a, const QString &b) {
            return counts.value(a) > counts.value(b);
        });

        for (const QString &sp : spList) {
            if (y + 18 > height() - 26)
                break;
            const auto info = mFrame.speciesInfo.value(sp);
            painter.fillRect(QRect(x0, y + 2, 14, 14), info.second);
            painter.setPen(QColor(90, 90, 90));
            painter.drawRect(QRect(x0, y + 2, 14, 14));
            painter.setPen(palette().text().color());
            const QString spName = info.first;
            const QString lbl = (!spName.isEmpty() && spName != sp)
                                    ? QString("%1 (%2): %3").arg(spName, sp).arg(counts.value(sp))
                                    : QString("%1: %2").arg(sp).arg(counts.value(sp));
            painter.drawText(QRect(x0 + 20, y, availW - 20, 18), Qt::AlignVCenter | Qt::AlignLeft,
                             fontMetrics().elidedText(lbl, Qt::ElideRight, availW - 20));
            y += 20;
        }
        if (!spList.isEmpty() && y + 22 <= height() - 26) {
            y += 4;
            const QRect swatch(x0, y + 2, 14, 14);
            painter.fillRect(swatch, Qt::white);
            painter.fillRect(swatch, QBrush(QColor(85, 90, 100), Qt::Dense3Pattern));
            painter.setPen(QColor(90, 90, 90));
            painter.drawRect(swatch);
            painter.setPen(QColor(95, 100, 110));
            painter.drawText(QRect(x0 + 20, y, availW - 20, 18), Qt::AlignVCenter | Qt::AlignLeft,
                             fontMetrics().elidedText(QStringLiteral("Dashed fill: share < 50%"), Qt::ElideRight, availW - 20));
            y += 20;
        }
    } else {
        if (!mScaleModeLabel.isEmpty() && y + 20 < height() - 30) {
            painter.setPen(QColor(95, 100, 110));
            painter.drawText(QRect(x0, y, availW, 16), Qt::AlignVCenter | Qt::AlignLeft,
                             fontMetrics().elidedText(mScaleModeLabel, Qt::ElideRight, availW));
            y += 20;
        }
        const int barW = 20;
        const int barH = qBound(90, height() - y - 40, 240);
        if (barH >= 50) {
            for (int py = 0; py < barH; ++py) {
                const float frac = 1.0f - static_cast<float>(py) / static_cast<float>(qMax(1, barH - 1));
                const float val = static_cast<float>(mScaleMin + frac * (mScaleMax - mScaleMin));
                const QColor c = Colors::colorFromValue(val, mPalette,
                                                        static_cast<float>(mScaleMin), static_cast<float>(mScaleMax));
                painter.setPen(c);
                painter.drawLine(x0, y + py, x0 + barW, y + py);
            }
            painter.setPen(QColor(130, 135, 145));
            painter.drawRect(x0, y, barW, barH);

            painter.setPen(palette().text().color());
            const int nTicks = 5;
            const bool clippedTop = (!mFrame.valuesByKey.isEmpty() && mScaleMax < mFrame.maxVal - 1e-6);
            for (int t = 0; t < nTicks; ++t) {
                const double frac = 1.0 - static_cast<double>(t) / static_cast<double>(nTicks - 1);
                const double val = mScaleMin + frac * (mScaleMax - mScaleMin);
                const int ty = y + qRound((1.0 - frac) * barH);
                painter.drawLine(x0 + barW, ty, x0 + barW + 4, ty);
                QString tickLabel = formatLegendNumber(val);
                if (t == 0 && clippedTop)
                    tickLabel.prepend(QStringLiteral("≥ "));
                painter.drawText(QRect(x0 + barW + 8, ty - 8, availW - barW - 8, 16),
                                 Qt::AlignVCenter | Qt::AlignLeft, tickLabel);
            }
            y += barH + 18;
        }
    }

    if (!mLayoutSource.isEmpty() && y + 18 < height()) {
        QFont small = font();
        small.setPointSizeF(qMax(7.0, small.pointSizeF() - 1.5));
        painter.setFont(small);
        painter.setPen(QColor(120, 125, 135));
        painter.drawText(QRect(x0, height() - 28, availW, 22), Qt::TextWordWrap, mLayoutSource);
    }
}
