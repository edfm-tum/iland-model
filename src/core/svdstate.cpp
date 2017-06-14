#include "svdstate.h"

#include "resourceunit.h"
#include "species.h"
#include "model.h"

SVDStates *SVDState::svd = 0;

SVDStates::SVDStates()
{
    // add an empty state ("unforested") to the machine
    SVDState s = SVDState();
    mStates.push_back(s);
    mCompositionString.push_back( createCompositionString(s) );
    mStateLookup[mStates[0]]=0;
    s.svd = this;

    qDebug() << "setup of SVDStates completed.";
}

int SVDStates::evaluateState(ResourceUnit *ru)
{
    SVDState s;

    double h = ru->topHeight();
    int hcls = limit( int(h/4), 0, 20);
    s.structure = hcls;
    double lai = ru->statistics().leafAreaIndex();
    if (lai>2.) s.function=1;
    if (lai>4.) s.function=2;

    // species
    int other_i = 0;

    double total_ba = ru->statistics().basalArea();
    if (total_ba>0.) {
        QList<ResourceUnitSpecies*>::const_iterator it=ru->ruSpecies().constBegin();
        for (; it!=ru->ruSpecies().constEnd(); ++it) {
            double rel_ba = (*it)->statistics().basalArea() / total_ba;
            if (rel_ba>0.66)
                s.dominant_species_index = (*it)->species()->index();
            else if (rel_ba>0.2)
                s.admixed_species_index[other_i++] = (*it)->species()->index();
        }
        if (other_i>=5)
            throw IException("SVDStates: too many species!");
        // generate +- unique number: this is mostly used for hashing, uniqueness is not strictly required
        s.composition=s.dominant_species_index;
        for (int i=0;i<5;++i)
            if (s.admixed_species_index[i]>-1)
                s.composition = (s.composition << 6) + s.admixed_species_index[i];
    }

    // lookup state in the hash table and return
    if (!mStateLookup.contains(s)) {
        s.Id = mStates.size();
        mStates.push_back(s);
        mCompositionString.push_back(createCompositionString(s));
        mStateLookup[s] = s.Id; // add to hash
    }

    return mStateLookup.value(s); // which is a unique index
}

QString SVDStates::stateLabel(int index)
{
    if (index<0 || index>=mStates.size())
        return QStringLiteral("invalid");
    const SVDState &s=state(index);
    QString label;
    if (s.dominant_species_index>=0)
        label=GlobalSettings::instance()->model()->speciesSet()->species(s.dominant_species_index)->id().toUpper() + " ";
    for (int i=0;i<5;++i)
        if (s.admixed_species_index[i]>=0)
            label += GlobalSettings::instance()->model()->speciesSet()->species(s.admixed_species_index[i])->id().toLower() + " ";
    label += QString("%1m (%2)").arg( s.structure*4 + 2).arg(s.function);
    return label;
}

QString SVDStates::createCompositionString(const SVDState &s)
{
    QString label;
    if (s.dominant_species_index>=0)
        label=GlobalSettings::instance()->model()->speciesSet()->species(s.dominant_species_index)->id().toUpper() + " ";
    for (int i=0;i<5;++i)
        if (s.admixed_species_index[i]>=0)
            label += GlobalSettings::instance()->model()->speciesSet()->species(s.admixed_species_index[i])->id().toLower() + " ";
    if (label.isEmpty()) {
        if (s.structure>0)
            return QStringLiteral("mix");
        else
            return QStringLiteral("unforested");
    }
    return label;
}

QString SVDState::compositionString() const
{
    if (svd)
        return svd->compositionString(Id);
    else
        return QStringLiteral("invalid");
}

QString SVDState::stateLabel() const
{
    if (svd)
        return svd->stateLabel(Id);
    else
        return QStringLiteral("invalid");
}
