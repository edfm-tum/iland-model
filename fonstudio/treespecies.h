#ifndef TREESPECIES_H
#define TREESPECIES_H

class StampContainer;
class Stamp;
class TreeSpecies
{
public:
    TreeSpecies();
    inline Stamp* stamp(const float dbh, const float height);
private:
    Q_DISABLE_COPY(TreeSpecies);
    StampContainer *m_stamps;
};

#endif // TREESPECIES_H
