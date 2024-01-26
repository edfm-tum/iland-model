#ifndef ABE_PATCH_H
#define ABE_PATCH_H

#include <QObject>
#include <QRectF>

namespace ABE {

class Patches; // forward

class Patch : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double area READ area)
    Q_PROPERTY(QRectF rectangle READ rectangle WRITE setRectangle)
public:
    explicit Patch(Patches* patches, int id, QObject *parent = nullptr);
    void update(); ///< update area and rectangle after updating indices
    int id() const {return mPatchId; }
    /// area of the patch in ha
    double area() const { return mArea; }
    QRectF rectangle() const {return mRect; }
    void setRectangle(QRectF t) { mRect = t; mArea =  t.width()*t.height();}
    QVector<int> &indices()  { return mCells; }
signals:

private:
    Patches *mPatches; ///< the patch containter
    double mArea; ///< number of cells occupied by patch
    QRectF mRect; ///< rectangle in stand grid coordinates

    QVector<int> mCells; ///< indices of cells assoicated to the patch in the grid of Patches
    int mPatchId; ///< numeric (unique) patch id

};

} // namespace ABE

#endif // ABE_PATCH_H
