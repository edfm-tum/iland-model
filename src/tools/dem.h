#ifndef DEM_H
#define DEM_H
#include "grid.h"
/** DEM is a digital elevation model class.
   It uses a float grid internally.
   slope is calculated in "%", i.e. a value of 1 is 45° (90° -> inf)

   The aspect angles are defined as follows (like ArcGIS):
          0°
          N
   270° W x E 90°
          S
         180°

   Values for height of -1 indicate "out of scope", "invalid" values

  */

class DEM: public FloatGrid
{
public:
    DEM(const QString &fileName) { loadFromFile(fileName); }
    bool loadFromFile(const QString &fileName);
    // create and fill grids for aspect/slope
    void createSlopeGrid();
    const FloatGrid *aspectGrid() { createSlopeGrid(); return &aspect_grid; }
    const FloatGrid *slopeGrid() { createSlopeGrid(); return &slope_grid; }
    const FloatGrid *viewGrid() { createSlopeGrid(); return &view_grid; }
    // special functions for DEM
    /// get the elevation (m) at point (x/y)
    float elevation(const float x, const float y) const { return constValueAt(x,y); }
    float elevation(const QPointF p) const { return constValueAt(p.x(),p.y()); }
    /// get the direction of the slope at point (x/y)
    /// if the slope at the point is 0, "north" (0) is returned.
    float direction(const float x, const float y);
    float slope(const float x, const float y);
    /// get orientation at specific point (x,y) and height
    float orientation(const QPointF &point, float &rslope_angle, float &rslope_aspect);
    float orientation(const float x, const float y, float &rslope_angle, float &rslope_aspect)
                        { return orientation(QPointF(x,y), rslope_angle, rslope_aspect); }


private:
    FloatGrid aspect_grid;
    FloatGrid slope_grid;
    FloatGrid view_grid;
};

#endif // DEM_H
