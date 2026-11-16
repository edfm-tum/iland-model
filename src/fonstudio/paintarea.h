#ifndef PAINTAREA_H
#define PAINTAREA_H

#include <QWidget>
#include <QPoint>
#include <QPointF>

class Stamp;

class PaintArea : public QWidget
{
    Q_OBJECT

public:
    explicit PaintArea(QWidget *parent = nullptr);

    void setStamp(const Stamp *stamp, float dbh, float hd, float crownRadius);
    void setReferenceContext(int maxLogicalSize, float globalMaxValue);
    void clearStamp();

    void set3DMode(bool enable3D);
    bool is3DMode() const { return m_is3D; }

    void setFixedScale(bool fixed);
    bool isFixedScale() const { return m_fixedScale; }

    void setShowValues(bool show);
    bool showValues() const { return m_showValues; }

    void setShowCrownCircle(bool show);
    bool showCrownCircle() const { return m_showCrownCircle; }

    void resetView();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void paint2DHeatmap(QPainter &painter, const QRect &plotRect);
    void paint3DBars(QPainter &painter, const QRect &plotRect);
    void paintColorLegend(QPainter &painter, const QRect &legendRect, float maxVal);
    QColor colorForValue(float val, float maxVal) const;

    const Stamp *m_stamp;
    float m_dbh;
    float m_hd;
    float m_crownRadius;

    int m_referenceSize;
    float m_globalMax;

    bool m_is3D;
    bool m_fixedScale;
    bool m_showValues;
    bool m_showCrownCircle;

    // View interaction state
    double m_zoom;
    QPointF m_panOffset;
    double m_yawDeg;
    double m_pitchDeg;

    bool m_dragging;
    Qt::MouseButton m_dragButton;
    QPoint m_lastMousePos;
    QPoint m_hoverPos;
    bool m_hasHover;
};

#endif // PAINTAREA_H
