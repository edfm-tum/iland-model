#ifndef MODEL_H
#define MODEL_H

// forwards
class RessourceUnit;
class SpeciesSet;

class Model
{
public:
    Model();
    ~Model();
    void clear(); ///< free ressources
    void loadProject(const QDomElement &node); ///< setup and load a project
private:
    void initialize(); ///< basic startup without creating a simulation

    /// container holding all ressource units
    QList<RessourceUnit*> mRU;
    /// container holding all species sets
    QList<SpeciesSet*> mSpeciesSets;
};

#endif // MODEL_H
