#include "treespecies.h"

#include "core/stampcontainer.h"

TreeSpecies::TreeSpecies()
{
}

const Stamp* TreeSpecies::stamp(const float dbh, const float height) const
{
    Q_ASSERT_X(m_stamps!=0, "TreeSpecies::stamp", "stamp NULL");
    return m_stamps->stamp(dbh, height);
}

const Stamp* TreeSpecies::readerStamp(const double crown_radius)
{
     Q_ASSERT_X(m_readerstamps!=0, "TreeSpecies::stamp", "stamp NULL");
    return m_readerstamps->readerStamp(crown_radius);
}

