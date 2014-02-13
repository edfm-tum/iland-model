#ifndef COLORS_H
#define COLORS_H
#include <QObject>
#include <QColor>
#include "grid.h"
/** Colors: helper class for managing/selecting colors
 *
 * */
class Colors: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList colors READ colors NOTIFY colorsChanged)
    Q_PROPERTY(QStringList labels READ labels NOTIFY colorsChanged)
    Q_PROPERTY(int count READ colorCount NOTIFY colorsChanged)
    Q_PROPERTY(double minValue READ minValue WRITE setMinValue NOTIFY colorsChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY colorsChanged)

public:
    Colors(QWidget*parent=0);
    // properties
    QStringList colors() const {return mColors; }
    QStringList labels() const {return mLabels; }
    int colorCount() const { return mColors.count(); }
    double minValue() const {return mMinValue; }
    double maxValue() const {return mMaxValue; }
    void setMinValue(double val) { mMinValue = val; setPalette(mCurrentType, mMinValue, mMaxValue); }
    void setMaxValue(double val) { mMaxValue = val; setPalette(mCurrentType, mMinValue, mMaxValue); }

    void setPalette(const GridViewType type, const float min_val, const float max_val);

    static QColor colorFromValue(const float value, const float min_value=0.f, const float max_value=1.f, const bool reverse=false, const bool black_white=false);
    static QColor colorFromValue(const float value, const GridViewType view_type, const float min_value=0.f, const float max_value=1.f);
    static QColor colorFromPalette(const int value, const GridViewType view_type);
private:
    static QVector<QColor> mBrewerDiv;
    static QVector<QColor> mBrewerQual;
    static QVector<QColor> mTerrainCol;
    QStringList mColors;
    QStringList mLabels;
    double mMinValue;
    double mMaxValue;
    GridViewType mCurrentType;
signals:
    void colorsChanged();
};

#endif // COLORS_H
