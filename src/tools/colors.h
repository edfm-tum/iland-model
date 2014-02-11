#ifndef COLORS_H
#define COLORS_H
#include <QColor>
#include "grid.h"
/** Colors: helper class for managing/selecting colors
 *
 * */
class Colors
{
public:
    Colors();
    static QColor colorFromValue(const float value, const float min_value=0.f, const float max_value=1.f, const bool reverse=false, const bool black_white=false);
    static QColor colorFromValue(const float value, const GridViewType view_type, const float min_value=0.f, const float max_value=1.f);
    static QColor colorFromPalette(const int value, const GridViewType view_type);
private:
    static QVector<QColor> mBrewerDiv;
    static QVector<QColor> mBrewerQual;
    static QVector<QColor> mTerrainCol;
};

#endif // COLORS_H
