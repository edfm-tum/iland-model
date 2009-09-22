#ifndef EXPRESSIONWRAPPER_H
#define EXPRESSIONWRAPPER_H


class ExpressionWrapper
{
public:
    ExpressionWrapper();
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);
    virtual double value(const QString &variableName);
    virtual int variableIndex(const QString &variableName);
};

class Tree;
class TreeWrapper: public ExpressionWrapper
{
public:
    TreeWrapper() : mTree(0) {}
    TreeWrapper(const Tree* tree) : mTree(tree) {}
    void setTree(const Tree* tree) { mTree = tree; }
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);
    virtual double value(const QString &variableName) { return value(variableName); }

private:
    const Tree *mTree;
};

class RessourceUnit;
class RUWrapper: public ExpressionWrapper
{
public:
    RUWrapper() : mRU(0) {}
    RUWrapper(const RessourceUnit* ressourceUnit) : mRU(ressourceUnit) {}
    void setRessourceUnit(RessourceUnit* ressourceUnit) { mRU = ressourceUnit; }
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);
    virtual double value(const QString &variableName) { return value(variableName); }
private:
    const RessourceUnit *mRU;
};

#endif // EXPRESSIONWRAPPER_H
