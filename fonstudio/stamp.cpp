#include "stamp.h"
#include <math.h>

bool Stamp::load(const QString& filename)
{
    if (!mImage.load(filename))
        return false;
    rScale=1.f; hScale=1.f;
    return true;
}

/************************************
** get Value at position (x,y)
** the return value is scaled to 0..hScale
** range for x/y is -1..+1
**************************************/
float Stamp::getXY(const float x, const float y)
{
    int ix, iy;
    ix = int( (x+1.f) * ( mImage.width() / 2.) );  // scale from -1..1
    iy = int( (y+1.f) * (mImage.height() / 2.) );
    if (!mImage.valid(ix, iy))
        return 1.f;
    QRgb p = mImage.pixel(ix, iy);
    return (qGray(p)/255.)*hScale;

}

/************************************
** get Value at radius r (0..rScale) and
** angle phi (in radians).
** note: 0: East, pi/2: North, pi: West, 3pi/2: South
**************************************/
float Stamp::get(const float r, const float phi)
{
    if (fabs(r)>rScale)
        return 1.f;

    float x, y;
    float r_unit = r/rScale; // scale to size of radius
    x = r_unit * sin(phi);
    y = r_unit * cos(phi);
    return getXY(x,y);
}

