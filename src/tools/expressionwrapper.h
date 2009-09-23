#ifndef EXPRESSIONWRAPPER_H
#define EXPRESSIONWRAPPER_H


class ExpressionWrapper
{
public:
    ExpressionWrapper();
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);
    virtual double valueByName(const QString &variableName);
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

private:
    const Tree *mTree;
};

class ResourceUnit;
class RUWrapper: public ExpressionWrapper
{
public:
    RUWrapper() : mRU(0) {}
    RUWrapper(const ResourceUnit* ressourceUnit) : mRU(ressourceUnit) {}
    void setResourceUnit(ResourceUnit* ressourceUnit) { mRU = ressourceUnit; }
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    const ResourceUnit *mRU;
};

#endif // EXPRESSIONWRAPPER_H
