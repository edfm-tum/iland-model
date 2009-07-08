#ifndef TREESPECIES_H
#define TREESPECIES_H

class StampContainer;
class Stamp;
class TreeSpecies
{
public:
    TreeSpecies();

    const Stamp* stamp(const float dbh, const float height) const;
    // maintenance
    void setStampContainer(const StampContainer *container) { m_stamps = container; }
private:
    Q_DISABLE_COPY(TreeSpecies);
    const StampContainer *m_stamps;
};

#endif // TREESPECIES_H
