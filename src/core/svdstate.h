#ifndef SVDSTATE_H
#define SVDSTATE_H
#include <QHash>
class SVDStates;
struct SVDState
{
    SVDState(): composition(0), structure(0), function(0), dominant_species_index(-1), Id(0) { for (int i=0;i<5;++i) admixed_species_index[i]=-1; }
    int composition;
    int structure;
    int function;
    int dominant_species_index;
    int admixed_species_index[5];
    /// the unique Id of the state within the current simulation.
    int Id;
    /// get a string with the main species on the resource unit
    /// dominant species is uppercase, all other lowercase
    QString compositionString() const;
    /// a human readable string describing the state
    QString stateLabel() const;
    /// link to the SVD container class
    static SVDStates *svd;
};

// functions for the hashing of states
inline bool operator==(const SVDState &s1, const SVDState &s2)
{
    // this does not include comparing the 'Id'!
    bool equal = s1.composition==s2.composition && s1.structure==s2.structure && s1.function==s2.function && s1.dominant_species_index==s2.dominant_species_index;
    if (!equal) return false;
    for (int i=0;i<5;++i)
        equal = equal && (s1.admixed_species_index[i]==s2.admixed_species_index[i]);
    return equal;
}

inline uint qHash(const SVDState &key, uint seed)
{
    uint hash_value = qHash(key.composition, seed) ^ qHash(key.structure, seed) ^ qHash(key.function, seed);
    return hash_value;
}

class ResourceUnit; // forward
class SVDStates
{
public:
    SVDStates();
    /// calculate and returns the Id ofthe state that
    /// the resource unit is currently in
    int evaluateState(ResourceUnit *ru);
    const SVDState state(int index) { return mStates[index]; }

    /// get a string with the main species on the resource unit
    /// dominant species is uppercase, all other lowercase
    QString compositionString(int index) { return mCompositionString[index]; }

    /// create a human readable string representation of the string
    QString stateLabel(int index);

private:
    QString createCompositionString(const SVDState &s);
    QVector<SVDState> mStates;
    QVector<QString> mCompositionString;
    QHash<SVDState, int> mStateLookup;
};
#endif // SVDSTATE_H
