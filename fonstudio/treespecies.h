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
    void setStampContainer(const StampContainer *writer) { m_stamps = writer; }
private:
    Q_DISABLE_COPY(TreeSpecies);
    const StampContainer *m_stamps;
};

// die gehört dann woanders hin!!!!


#endif // TREESPECIES_H
