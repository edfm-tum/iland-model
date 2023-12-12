#ifndef ABE_PATCHES_H
#define ABE_PATCHES_H

#include "patch.h"
#include "grid.h"
#include "tree.h"

#include <QObject>



namespace ABE {

class FMStand; // forward

class Patches : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<Patch*> list READ list)
    Q_PROPERTY(QRectF rectangle READ rectangle)
public:
    explicit Patches(QObject *parent = nullptr);
    ~Patches() override { clear(); }
    void setup(FMStand *stand);

    // main functions
    /// return the patch the given tree is on
    int patch(const Tree *tree) const { return patch(tree->positionIndex()); }
    int patch(QPoint pos) const {
        QPoint p = QPoint(pos.x() / cPxPerHeight - mStandOffset.x(),
                          pos.y() / cPxPerHeight - mStandOffset.y() );
        if (!mStandGrid.isIndexValid(p))
            throw IException(QString("Invalid access to Patches: ix: %1,iy: %2.").arg(pos.x()).arg(pos.y()));
        return mStandGrid.constValueAtIndex(p);
    }

    // properties
    FMStand* stand() const { return mStand; }
    Grid<short int> &grid()  { return mStandGrid; }
    QList<Patch*> list() const { return mPatches; }
    QRectF rectangle() const { return mStandRect; }

    void updateGrid();
    /// get patch from a tree (static)
    static int getPatch(QPoint position_lif);
public slots:
    void createRandom(int n);
    void clear();
signals:
private:
    FMStand *mStand;
    QList<Patch*> mPatches;
    QRectF mStandRect; ///< metric rect of the stand
    QPoint mStandOffset; ///< offset of the stand on the 10m grid
    Grid<short int> mStandGrid;

};


} // namespace ABE

#endif // ABE_PATCHES_H
