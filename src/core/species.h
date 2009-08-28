#ifndef SPECIES_H
#define SPECIES_H

class StampContainer;
class Stamp;
class Species
{
public:
    Species();

    const Stamp* stamp(const float dbh, const float height) const;
    // maintenance
    void setStampContainer(const StampContainer *writer) { m_stamps = writer; }
private:
    Q_DISABLE_COPY(Species);
    const StampContainer *m_stamps;
};

// die gehört dann woanders hin!!!!


#endif // SPECIES_H
