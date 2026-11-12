#include "paintarea.h"
#include "stamp.h"
#include "globalsettings.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <vector>

PaintArea::PaintArea(QWidget *parent)
    : QWidget(parent),
      m_stamp(nullptr),
      m_dbh(0.f),
      m_hd(0.f),
      m_crownRadius(0.f),
      m_referenceSize(15),
      m_globalMax(1.f),
      m_is3D(false),
      m_fixedScale(true),
      m_showValues(true),
      m_showCrownCircle(true),
      m_zoom(1.0),
      m_panOffset(0.0, 0.0),
      m_yawDeg(-35.0),
      m_pitchDeg(38.0),
      m_dragging(false),
      m_dragButton(Qt::NoButton),
      m_hasHover(false)
{
    setMouseTracking(true);
    setMinimumSize(320, 320);
}

void PaintArea::setStamp(const Stamp *stamp, float dbh, float hd, float crownRadius)
{
    m_stamp = stamp;
    m_dbh = dbh;
    m_hd = hd;
    m_crownRadius = crownRadius;
    update();
}

void PaintArea::setReferenceContext(int maxLogicalSize, float globalMaxValue)
{
    m_referenceSize = qMax(3, maxLogicalSize);
    m_globalMax = (globalMaxValue > 0.f) ? globalMaxValue : 1.f;
    update();
}

void PaintArea::clearStamp()
{
    m_stamp = nullptr;
    update();
}

void PaintArea::set3DMode(bool enable3D)
{
    if (m_is3D != enable3D) {
        m_is3D = enable3D;
        m_panOffset = QPointF(0.0, 0.0);
        m_zoom = 1.0;
        update();
    }
}

void PaintArea::setFixedScale(bool fixed)
{
    if (m_fixedScale != fixed) {
        m_fixedScale = fixed;
        update();
    }
}

void PaintArea::setShowValues(bool show)
{
    if (m_showValues != show) {
        m_showValues = show;
        update();
    }
}

void PaintArea::setShowCrownCircle(bool show)
{
    if (m_showCrownCircle != show) {
        m_showCrownCircle = show;
        update();
    }
}

void PaintArea::resetView()
{
    m_zoom = 1.0;
    m_panOffset = QPointF(0.0, 0.0);
    m_yawDeg = -35.0;
    m_pitchDeg = 38.0;
    update();
}

QColor PaintArea::colorForValue(float val, float maxVal) const
{
    if (val <= 0.0001f)
        return QColor(245, 247, 250);

    float t = qBound(0.f, val / qMax(0.0001f, maxVal), 1.f);
    // Smooth multi-stop heatmap: pale yellow -> amber -> orange -> deep crimson -> dark maroon
    struct Stop { float pos; int r, g, b; };
    static const Stop stops[] = {
        {0.00f, 255, 247, 188},
        {0.25f, 254, 196,  79},
        {0.50f, 254, 127,  45},
        {0.75f, 217,  30,  42},
        {1.00f, 110,   5,  35}
    };
    for (int i = 0; i < 4; ++i) {
        if (t >= stops[i].pos && t <= stops[i+1].pos) {
            float local = (t - stops[i].pos) / (stops[i+1].pos - stops[i].pos);
            int r = qRound(stops[i].r + local * (stops[i+1].r - stops[i].r));
            int g = qRound(stops[i].g + local * (stops[i+1].g - stops[i].g));
            int b = qRound(stops[i].b + local * (stops[i+1].b - stops[i].b));
            return QColor(r, g, b);
        }
    }
    return QColor(110, 5, 35);
}

void PaintArea::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    painter.fillRect(rect(), QColor(250, 250, 252));

    if (!m_stamp) {
        painter.setPen(QColor(130, 135, 145));
        QFont f = painter.font();
        f.setPointSize(11);
        painter.setFont(f);
        painter.drawText(rect(), Qt::AlignCenter,
                         "No LIP stamp selected.\nOpen a LIP (.bin) file and select a cell in the overview grid.");
        return;
    }

    int legendWidth = 72;
    int topHeaderHeight = 32;
    int bottomBarHeight = 28;

    // Header summary
    QRect headerRect(12, 4, width() - 24, topHeaderHeight);
    painter.setPen(QColor(40, 45, 55));
    QFont headerFont = painter.font();
    headerFont.setBold(true);
    headerFont.setPointSize(10);
    painter.setFont(headerFont);

    int s = m_stamp->size();
    float footprintMeters = s * cPxSize;
    QString titleText;
    if (m_dbh > 0.f) {
        float h = m_dbh * m_hd / 100.f;
        titleText = QString("DBH: %1 cm | Height: %2 m (H/D: %3) | Crown r: %4 m | Matrix: %5x%5 (%6x%6 m)")
                        .arg(m_dbh, 0, 'f', 1)
                        .arg(h, 0, 'f', 1)
                        .arg(m_hd, 0, 'f', 0)
                        .arg(m_crownRadius, 0, 'f', 2)
                        .arg(s)
                        .arg(footprintMeters, 0, 'f', 0);
    } else {
        titleText = QString("Reader Stamp | Crown radius: %1 m | Matrix: %2x%2 (%3x%3 m)")
                        .arg(m_crownRadius, 0, 'f', 2)
                        .arg(s)
                        .arg(footprintMeters, 0, 'f', 0);
    }
    painter.drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, titleText);

    QRect plotRect(10, topHeaderHeight + 4,
                   width() - legendWidth - 20,
                   height() - topHeaderHeight - bottomBarHeight - 8);
    QRect legendRect(width() - legendWidth - 4, topHeaderHeight + 20,
                     legendWidth - 8, plotRect.height() - 40);

    // Determine effective max value
    float localMax = 0.f;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x)
            localMax = qMax(localMax, (*m_stamp)(x, y));
    float scaleMax = m_fixedScale ? qMax(m_globalMax, localMax) : qMax(0.01f, localMax);

    if (m_is3D) {
        paint3DBars(painter, plotRect);
    } else {
        paint2DHeatmap(painter, plotRect);
    }

    paintColorLegend(painter, legendRect, scaleMax);
}

void PaintArea::paint2DHeatmap(QPainter &painter, const QRect &plotRect)
{
    int s = m_stamp->size();
    int offset = m_stamp->offset();

    float localMax = 0.f;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x)
            localMax = qMax(localMax, (*m_stamp)(x, y));
    float scaleMax = m_fixedScale ? qMax(m_globalMax, localMax) : qMax(0.01f, localMax);

    int refCells = m_fixedScale ? qMax(m_referenceSize, s) : s;
    int refOffset = refCells / 2;

    int margin = 36; // space for axis labels & compass
    int availW = qMax(50, plotRect.width() - 2 * margin);
    int availH = qMax(50, plotRect.height() - 2 * margin);
    double baseSide = qMin(availW, availH) * m_zoom;
    double cellPx = baseSide / double(refCells);

    double gridOriginX = plotRect.center().x() - (refCells * cellPx) / 2.0 + m_panOffset.x();
    double gridOriginY = plotRect.center().y() - (refCells * cellPx) / 2.0 + m_panOffset.y();

    painter.save();
    painter.setClipRect(plotRect);

    // Draw reference background grid if fixedScale is active and refCells > s
    QRectF fullGridRect(gridOriginX, gridOriginY, refCells * cellPx, refCells * cellPx);
    painter.fillRect(fullGridRect, QColor(238, 241, 246));
    painter.setPen(QPen(QColor(215, 220, 228), 1.0));
    if (cellPx >= 4.0) {
        for (int i = 0; i <= refCells; ++i) {
            double pos = i * cellPx;
            painter.drawLine(QPointF(gridOriginX + pos, gridOriginY),
                             QPointF(gridOriginX + pos, gridOriginY + refCells * cellPx));
            painter.drawLine(QPointF(gridOriginX, gridOriginY + pos),
                             QPointF(gridOriginX + refCells * cellPx, gridOriginY + pos));
        }
    } else {
        painter.drawRect(fullGridRect);
    }

    // Offset of current stamp within reference grid
    int startCell = refOffset - offset;

    int hoveredX = -1;
    int hoveredY = -1;

    QFont valFont = painter.font();
    valFont.setPointSizeF(qBound(6.5, cellPx * 0.24, 10.0));
    painter.setFont(valFont);

    // Draw stamp cells
    // Note: in LightRoom, higher y index is North. Draw North at top (screen row = s - 1 - y).
    for (int y = 0; y < s; ++y) {
        int screenRow = startCell + (s - 1 - y);
        for (int x = 0; x < s; ++x) {
            int screenCol = startCell + x;
            float val = (*m_stamp)(x, y);
            QRectF cellRect(gridOriginX + screenCol * cellPx,
                            gridOriginY + screenRow * cellPx,
                            cellPx, cellPx);

            QColor fillCol = colorForValue(val, scaleMax);
            painter.fillRect(cellRect, fillCol);

            if (cellPx >= 4.0) {
                painter.setPen(QPen(QColor(180, 185, 195, 160), 0.75));
                painter.drawRect(cellRect);
            }

            if (m_hasHover && cellRect.contains(m_hoverPos)) {
                hoveredX = x;
                hoveredY = y;
            }

            if (m_showValues && cellPx >= 28.0 && val > 0.0005f) {
                float lum = 0.299f * fillCol.redF() + 0.587f * fillCol.greenF() + 0.114f * fillCol.blueF();
                painter.setPen(lum < 0.52f ? Qt::white : QColor(30, 30, 35));
                QString txt = (val >= 0.1f) ? QString::number(val, 'f', 2) : QString::number(val, 'f', 3);
                painter.drawText(cellRect, Qt::AlignCenter, txt);
            }
        }
    }

    // Draw active stamp boundary box
    QRectF stampRect(gridOriginX + startCell * cellPx,
                     gridOriginY + startCell * cellPx,
                     s * cellPx, s * cellPx);
    painter.setPen(QPen(QColor(50, 65, 90), 1.8));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(stampRect);

    // Draw center crosshairs (tree position at 0,0)
    QPointF treeCenter(gridOriginX + (refOffset + 0.5) * cellPx,
                       gridOriginY + (refOffset + 0.5) * cellPx);
    painter.setPen(QPen(QColor(30, 40, 55, 140), 1.0, Qt::DashLine));
    painter.drawLine(QPointF(treeCenter.x(), fullGridRect.top()),
                     QPointF(treeCenter.x(), fullGridRect.bottom()));
    painter.drawLine(QPointF(fullGridRect.left(), treeCenter.y()),
                     QPointF(fullGridRect.right(), treeCenter.y()));

    // Draw crown radius circle if requested
    if (m_showCrownCircle && m_crownRadius > 0.f) {
        double radiusPx = (m_crownRadius / double(cPxSize)) * cellPx;
        painter.setPen(QPen(QColor(20, 120, 60), 2.0, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(treeCenter, radiusPx, radiusPx);
    }

    // Highlight hovered cell
    if (hoveredX >= 0 && hoveredY >= 0) {
        int screenRow = startCell + (s - 1 - hoveredY);
        int screenCol = startCell + hoveredX;
        QRectF hRect(gridOriginX + screenCol * cellPx,
                     gridOriginY + screenRow * cellPx,
                     cellPx, cellPx);
        painter.setPen(QPen(QColor(20, 20, 20), 2.2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(hRect.adjusted(1, 1, -1, -1));
    }

    // Compass & meter scale labels around reference grid
    QFont axisFont = painter.font();
    axisFont.setPointSize(9);
    axisFont.setBold(true);
    painter.setFont(axisFont);
    painter.setPen(QColor(60, 70, 85));

    painter.drawText(QRectF(fullGridRect.center().x() - 30, fullGridRect.top() - 22, 60, 18),
                     Qt::AlignCenter, "N");
    painter.drawText(QRectF(fullGridRect.center().x() - 30, fullGridRect.bottom() + 4, 60, 18),
                     Qt::AlignCenter, QString("S  (%1 m)").arg(refCells * cPxSize));
    painter.drawText(QRectF(fullGridRect.left() - 26, fullGridRect.center().y() - 10, 22, 20),
                     Qt::AlignCenter, "W");
    painter.drawText(QRectF(fullGridRect.right() + 4, fullGridRect.center().y() - 10, 22, 20),
                     Qt::AlignCenter, "E");

    painter.restore();

    // Status / Hover bar at the bottom
    QRect statusRect(12, height() - 26, width() - 24, 22);
    QFont statusFont = painter.font();
    statusFont.setPointSize(9);
    statusFont.setBold(false);
    painter.setFont(statusFont);
    painter.setPen(QColor(70, 75, 85));

    if (hoveredX >= 0 && hoveredY >= 0) {
        int dxCells = hoveredX - offset;
        int dyCells = hoveredY - offset;
        double dxMeters = dxCells * cPxSize;
        double dyMeters = dyCells * cPxSize;
        double distMeters = std::sqrt(dxMeters * dxMeters + dyMeters * dyMeters);
        float val = (*m_stamp)(hoveredX, hoveredY);
        QString hoverInfo = QString("Cell (%1, %2) | Offset: (%3 m E, %4 m N) | Distance: %5 m | Value: %6")
                                .arg(hoveredX)
                                .arg(hoveredY)
                                .arg(dxMeters, 0, 'f', 0)
                                .arg(dyMeters, 0, 'f', 0)
                                .arg(distMeters, 0, 'f', 1)
                                .arg(val, 0, 'f', 4);
        painter.drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter, hoverInfo);
    } else {
        painter.drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter,
                         "Hover cell to inspect values | Scroll to zoom | Drag to pan | Double-click to reset");
    }
}

void PaintArea::paint3DBars(QPainter &painter, const QRect &plotRect)
{
    int s = m_stamp->size();
    int offset = m_stamp->offset();

    float localMax = 0.f;
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x)
            localMax = qMax(localMax, (*m_stamp)(x, y));
    float scaleMax = m_fixedScale ? qMax(m_globalMax, localMax) : qMax(0.01f, localMax);

    int refCells = m_fixedScale ? qMax(m_referenceSize, s) : s;
    double halfRef = refCells / 2.0;

    double yaw = qDegreesToRadians(m_yawDeg);
    double pitch = qDegreesToRadians(m_pitchDeg);
    double cosY = std::cos(yaw), sinY = std::sin(yaw);
    double cosP = std::cos(pitch), sinP = std::sin(pitch);

    double scale = (qMin(plotRect.width(), plotRect.height()) * 0.56 / double(refCells)) * m_zoom;
    double zHeightCells = refCells * 0.45; // max bar height in cell units
    QPointF center = QPointF(plotRect.center().x(), plotRect.center().y() + plotRect.height() * 0.10) + m_panOffset;

    auto project = [&](double gx, double gy, double gz) -> QPointF {
        // gx: West(-) to East(+), gy: South(-) to North(+), gz: vertical height(0..zHeightCells)
        double rx = gx * cosY - gy * sinY;
        double ry = gx * sinY + gy * cosY;
        double sx = center.x() + rx * scale;
        double sy = center.y() - (ry * sinP + gz * cosP) * scale;
        return QPointF(sx, sy);
    };

    painter.save();
    painter.setClipRect(plotRect);

    // 1. Draw base platform (reference extent)
    QPolygonF basePoly;
    basePoly << project(-halfRef, -halfRef, 0)
             << project( halfRef, -halfRef, 0)
             << project( halfRef,  halfRef, 0)
             << project(-halfRef,  halfRef, 0);
    painter.setBrush(QColor(235, 239, 245));
    painter.setPen(QPen(QColor(170, 178, 190), 1.2));
    painter.drawPolygon(basePoly);

    // Reference grid lines on base platform
    int step = (refCells > 31) ? 4 : ((refCells > 15) ? 2 : 1);
    painter.setPen(QPen(QColor(210, 215, 225), 0.7));
    for (int i = -int(halfRef); i <= int(halfRef); i += step) {
        painter.drawLine(project(i, -halfRef, 0), project(i, halfRef, 0));
        painter.drawLine(project(-halfRef, i, 0), project(halfRef, i, 0));
    }

    // Active stamp footprint outline on the base platform
    double halfS = s / 2.0;
    QPolygonF activeBase;
    activeBase << project(-halfS, -halfS, 0)
               << project( halfS, -halfS, 0)
               << project( halfS,  halfS, 0)
               << project(-halfS,  halfS, 0);
    painter.setBrush(QColor(225, 230, 238));
    painter.setPen(QPen(QColor(110, 120, 140), 1.2));
    painter.drawPolygon(activeBase);

    // Compass labels around base platform
    QFont compassFont = painter.font();
    compassFont.setBold(true);
    compassFont.setPointSize(9);
    painter.setFont(compassFont);
    painter.setPen(QColor(50, 60, 75));
    auto drawLabelAt = [&](double gx, double gy, const QString &txt) {
        QPointF pt = project(gx, gy, 0);
        painter.drawText(QRectF(pt.x() - 25, pt.y() - 10, 50, 20), Qt::AlignCenter, txt);
    };
    drawLabelAt(0, halfRef + 1.4, "N");
    drawLabelAt(0, -halfRef - 1.4, "S");
    drawLabelAt(halfRef + 1.4, 0, "E");
    drawLabelAt(-halfRef - 1.4, 0, "W");

    // 2. Sort cells back-to-front by depth ry = gx * sinY + gy * cosY (descending)
    struct BarItem {
        int ix, iy;
        double gx, gy;
        double depth;
        float val;
    };
    std::vector<BarItem> bars;
    bars.reserve(s * s);
    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s; ++x) {
            float val = (*m_stamp)(x, y);
            double gx = (x - offset);
            double gy = (y - offset);
            double depth = gx * sinY + gy * cosY;
            bars.push_back({x, y, gx, gy, depth, val});
        }
    }
    std::sort(bars.begin(), bars.end(), [](const BarItem &a, const BarItem &b) {
        return a.depth > b.depth;
    });

    // Determine which two vertical side faces of each prism are facing the viewer
    // Viewer looks along -ry; in (gx, gy), viewer direction is (-sinY, -cosY)
    bool showEastFace = (-sinY > 0);  // x1 face vs x0 face
    bool showNorthFace = (-cosY > 0); // y1 face vs y0 face

    double pad = 0.06; // slight gap between bars for crisp visual separation
    for (const BarItem &b : bars) {
        double x0 = b.gx - 0.5 + pad;
        double x1 = b.gx + 0.5 - pad;
        double y0 = b.gy - 0.5 + pad;
        double y1 = b.gy + 0.5 - pad;

        if (b.val <= 0.0001f) {
            // Flat tile on base
            QPolygonF tile;
            tile << project(x0, y0, 0) << project(x1, y0, 0)
                 << project(x1, y1, 0) << project(x0, y1, 0);
            painter.setBrush(QColor(242, 244, 248));
            painter.setPen(QPen(QColor(200, 205, 215), 0.5));
            painter.drawPolygon(tile);
            continue;
        }

        double h = qBound(0.02, double(b.val / scaleMax) * zHeightCells, zHeightCells * 1.2);
        QColor baseCol = colorForValue(b.val, scaleMax);
        QPen edgePen(baseCol.darker(155), 0.65);
        painter.setPen(edgePen);

        // X-normal side face (East or West)
        double xs = showEastFace ? x1 : x0;
        QPolygonF xFace;
        xFace << project(xs, y0, 0) << project(xs, y1, 0)
              << project(xs, y1, h) << project(xs, y0, h);
        painter.setBrush(baseCol.darker(130));
        painter.drawPolygon(xFace);

        // Y-normal side face (North or South)
        double ys = showNorthFace ? y1 : y0;
        QPolygonF yFace;
        yFace << project(x0, ys, 0) << project(x1, ys, 0)
              << project(x1, ys, h) << project(x0, ys, h);
        painter.setBrush(baseCol.darker(115));
        painter.drawPolygon(yFace);

        // Top face
        QPolygonF topFace;
        topFace << project(x0, y0, h) << project(x1, y0, h)
                << project(x1, y1, h) << project(x0, y1, h);
        painter.setBrush(baseCol);
        painter.drawPolygon(topFace);
    }

    painter.restore();

    // Status bar hint
    QRect statusRect(12, height() - 26, width() - 24, 22);
    QFont statusFont = painter.font();
    statusFont.setPointSize(9);
    statusFont.setBold(false);
    painter.setFont(statusFont);
    painter.setPen(QColor(70, 75, 85));
    painter.drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QString("3D View (Azimuth: %1 deg, Tilt: %2 deg) | Left-drag to rotate | Right-drag to pan | Scroll to zoom | Double-click to reset")
                         .arg(qRound(m_yawDeg))
                         .arg(qRound(m_pitchDeg)));
}

void PaintArea::paintColorLegend(QPainter &painter, const QRect &legendRect, float maxVal)
{
    if (legendRect.height() < 60)
        return;

    int barWidth = 16;
    QRect barRect(legendRect.left() + 4, legendRect.top() + 14,
                  barWidth, legendRect.height() - 28);

    for (int y = 0; y < barRect.height(); ++y) {
        float frac = 1.0f - float(y) / float(qMax(1, barRect.height() - 1));
        painter.setPen(colorForValue(frac * maxVal, maxVal));
        painter.drawLine(barRect.left(), barRect.top() + y,
                         barRect.right(), barRect.top() + y);
    }
    painter.setPen(QColor(120, 125, 135));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(barRect);

    QFont f = painter.font();
    f.setPointSize(8);
    f.setBold(false);
    painter.setFont(f);
    painter.setPen(QColor(50, 55, 65));

    int ticks = 5;
    for (int i = 0; i <= ticks; ++i) {
        float frac = float(i) / float(ticks);
        int yPos = barRect.bottom() - qRound(frac * barRect.height());
        painter.drawLine(barRect.right(), yPos, barRect.right() + 4, yPos);
        float v = frac * maxVal;
        QString label = (maxVal >= 10.f) ? QString::number(v, 'f', 1) : QString::number(v, 'f', 2);
        painter.drawText(QRect(barRect.right() + 6, yPos - 8, legendRect.width() - barWidth - 8, 16),
                         Qt::AlignLeft | Qt::AlignVCenter, label);
    }
}

void PaintArea::mousePressEvent(QMouseEvent *event)
{
    m_dragging = true;
    m_dragButton = event->button();
    m_lastMousePos = event->pos();
    setCursor(Qt::ClosedHandCursor);
}

void PaintArea::mouseMoveEvent(QMouseEvent *event)
{
    m_hoverPos = event->pos();
    m_hasHover = true;

    if (m_dragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        if (m_is3D && (m_dragButton == Qt::LeftButton) && !(event->modifiers() & Qt::ShiftModifier)) {
            m_yawDeg += delta.x() * 0.6;
            while (m_yawDeg > 180.0) m_yawDeg -= 360.0;
            while (m_yawDeg < -180.0) m_yawDeg += 360.0;
            m_pitchDeg = qBound(8.0, m_pitchDeg + delta.y() * 0.5, 85.0);
        } else {
            m_panOffset += QPointF(delta.x(), delta.y());
        }
    }
    update();
}

void PaintArea::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
    m_dragButton = Qt::NoButton;
    setCursor(Qt::OpenHandCursor);
}

void PaintArea::mouseDoubleClickEvent(QMouseEvent *)
{
    resetView();
}

void PaintArea::wheelEvent(QWheelEvent *event)
{
    double steps = event->angleDelta().y() / 120.0;
    if (steps != 0.0) {
        double factor = std::pow(1.15, steps);
        m_zoom = qBound(0.3, m_zoom * factor, 8.0);
        update();
    }
}

void PaintArea::leaveEvent(QEvent *)
{
    m_hasHover = false;
    update();
}
