#include "treeout.h"
#include "helper.h"
#include "tree.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"
#include "expressionwrapper.h"

TreeOut::TreeOut()
{
    setName("Tree Output", "tree");
    setDescription("Output of indivdual trees. Use the ''filter'' property to reduce amount of data (filter by resource-unit, year, species, ...).\n" \
                   "The output is triggered after the growth of the current season (trees died in the current year show therefore '1' as ''isDead''). " \
                   "Initial values (without any growth) are output as 'startyear-1'.");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::species()
            << OutputColumn("id", "id of the tree", OutInteger)
            << OutputColumn("dbh", "dbh (cm) of the tree", OutDouble)
            << OutputColumn("height", "height (m) of the tree", OutDouble)
            << OutputColumn("isDead", "1: tree is dead, 0: tree lives", OutDouble)
            << OutputColumn("basalArea", "basal area of tree in m2", OutDouble)
            << OutputColumn("volume_m3", "volume of tree (m3)", OutDouble)
            << OutputColumn("leafArea_m2", "current leaf area of the tree (m2)", OutDouble)
            << OutputColumn("foliageMass", "current mass of foliage (kg)", OutDouble)
            << OutputColumn("woodyMass", "kg Biomass in woody department", OutDouble)
            << OutputColumn("fineRootMass", "kg Biomass in fine-root department", OutDouble)
            << OutputColumn("coarseRootMass", "kg Biomass in coarse-root department", OutDouble)
            << OutputColumn("lri", "LightResourceIndex of the tree (raw light index from iLand, without applying resource-unit modifications)", OutDouble)
            << OutputColumn("lightResponse", "light response value (including species specific response to the light level)", OutDouble)
            << OutputColumn("stressIndex", "scalar (0..1) indicating the stress level (see [Mortality]).", OutDouble)
            << OutputColumn("reserve_kg", "NPP currently available in the reserve pool (kg Biomass)", OutDouble);


 }

void TreeOut::setup()
{
    qDebug() << "treeout::setup() called";
    if (!settings().isValid())
        throw IException("TreeOut::setup(): no parameter section in init file!");
    QString filter = settings().value(".filter","");
    mFilter.setExpression(filter);
}

void TreeOut::exec()
{
    AllTreeIterator at(GlobalSettings::instance()->model());
    DebugTimer t("TreeOut::exec()");
    TreeWrapper tw;
    mFilter.setModelObject(&tw);
    while (Tree *t=at.next()) {
        if (!mFilter.isEmpty()) { // skip fields
            tw.setTree(t);
            if (!mFilter.execute())
                continue;
        }
        *this << currentYear() << t->ru()->index() << t->species()->id();
        *this << t->id() << t->dbh() << t->height() << (t->isDead()?1:0) << t->basalArea() << t->volume();
        *this << t->leafArea() << t->mFoliageMass << t->mWoodyMass <<  t->mFineRootMass << t->mCoarseRootMass;
        *this << t->lightResourceIndex() << t->mLightResponse << t->mStressIndex << t->mNPPReserve;
        writeRow();
    }

}

