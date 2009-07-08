#include "treespecies.h"

#include "core/stampcontainer.h"

TreeSpecies::TreeSpecies()
{
}

inline Stamp* TreeSpecies::stamp(const float dbh, const float height)
{
    Q_ASSERT_X(m_stamps!=0, "TreeSpecies::stamp", "stamp NULL");
    return m_stamps->stamp(dbh, height);
}
