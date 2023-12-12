#ifndef ABE_PATCH_H
#define ABE_PATCH_H

#include <QObject>

namespace ABE {

class Patches; // forward

class Patch : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int area READ area)
    Q_PROPERTY(QRectF rectangle READ rectangle WRITE setRectangle)
public:
    explicit Patch(Patches* patches, int id, QObject *parent = nullptr);
    int id() const {return mPatchId; }
    int area() const { return mArea; }
    QRectF rectangle() const {return mRect; }
    void setRectangle(QRectF t) { mRect = t; mArea =  t.width()*t.height();}
    QVector<int> &indices()  { return mCells; }
signals:

private:
    Patches *mPatches; ///< the patch containter
    int mArea; ///< number of cells occupied by patch
    QRectF mRect; ///< rectangle in stand grid coordinates

    QVector<int> mCells; ///< indices of cells assoicated to the patch in the grid of Patches
    int mPatchId; ///< numeric (unique) patch id

};

} // namespace ABE

#endif // ABE_PATCH_H
