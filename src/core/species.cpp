#include "species.h"

#include "stampcontainer.h"

Species::Species()
{
}

const Stamp* Species::stamp(const float dbh, const float height) const
{
    Q_ASSERT_X(m_stamps!=0, "Species::stamp", "stamp NULL");
    return m_stamps->stamp(dbh, height);
}



