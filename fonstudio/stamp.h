#ifndef STAMP_H
#define STAMP_H

#include <QImage>

class Stamp
{
public:
    Stamp() {}
    Stamp(const QString& filename) { load(filename); }
    bool load(const QString& filename);
    float getXY(const float x, const float y);
    float get(const float r, const float phi);
    void setScale(const float radius, const float intensity) { rScale=radius; hScale=intensity; }
private:
    QImage mImage;
    float rScale;
    float hScale;

};

#endif // STAMP_H
