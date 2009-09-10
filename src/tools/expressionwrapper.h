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
    void setTree(Tree* tree) { mTree = tree; }
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);
    virtual double value(const QString &variableName);
    virtual int variableIndex(const QString &variableName);


private:
    const Tree *mTree;

};

#endif // EXPRESSIONWRAPPER_H
