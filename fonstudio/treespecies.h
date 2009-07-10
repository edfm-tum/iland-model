#ifndef TREESPECIES_H
#define TREESPECIES_H

class StampContainer;
class Stamp;
class TreeSpecies
{
public:
    TreeSpecies();

    const Stamp* stamp(const float dbh, const float height) const;
    const Stamp* readerStamp(const double crown_radius);
    // maintenance
    void setStampContainer(const StampContainer *writer, const StampContainer *reader) { m_stamps = writer; m_readerstamps=reader;}
private:
    Q_DISABLE_COPY(TreeSpecies);
    const StampContainer *m_stamps;
    const StampContainer *m_readerstamps;
};

// die gehört dann woanders hin!!!!


#endif // TREESPECIES_H
