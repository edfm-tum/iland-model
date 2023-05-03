
#include "microclimate.h"

#include "resourceunit.h"
#include "species.h"
#include "tree.h"


Microclimate::Microclimate(const ResourceUnit *ru)
{
    mRU = ru;
    mCells = new MicroclimateCell[cHeightPerRU*cHeightPerRU];
}

Microclimate::~Microclimate()
{
    delete[] mCells;
}

void Microclimate::calculateVegetation()
{
    QVector<double> ba_total(cHeightPerRU*cHeightPerRU, 0.);
    QVector<double> ba_conifer(cHeightPerRU*cHeightPerRU, 0.);
    QVector<double> lai_total(cHeightPerRU*cHeightPerRU, 0.);

    // loop over trees and calculate aggregate values
    for ( QVector<Tree>::const_iterator t = mRU->constTrees().constBegin(); t != mRU->constTrees().constEnd(); ++t) {
        int idx = cellIndex(t->position());

        ba_total[idx] += t->basalArea();
        lai_total[idx] += t->leafArea();
        if (t->species()->isConiferous())
            ba_conifer[idx] += t->basalArea();
    }

    // now write back to the microclimate store
    for (int i=0;i<cHeightPerRU*cHeightPerRU; ++i) {
        cell(i).setLAI( lai_total[i] / cHeightPixelArea ); // calculate m2/m2
        cell(i).setConiferShare( ba_total[i] > 0. ? ba_conifer[i] / ba_total[i] : 0.);
    }

}



int Microclimate::cellIndex(const QPointF &coord)
{
    // convert to index
    QPointF local = coord - mRU->boundingBox().topLeft();
    Q_ASSERT(local.x()>=0 && local.x()<100 && local.y()>=0 && local.y() < 100);

    int index = ( int( local.x() / cHeightPerRU) ) * cHeightPerRU + ( int( local.y() / cHeightPerRU ) );
    return index;
}

