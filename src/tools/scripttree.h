#ifndef SCRIPTTREE_H
#define SCRIPTTREE_H

#include <QObject>
#include "tree.h"
#include "species.h"
#include "expressionwrapper.h"

class ScriptTree : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid)
    Q_PROPERTY(double x READ x)
    Q_PROPERTY(double y READ y)
    Q_PROPERTY(double dbh READ dbh)
    Q_PROPERTY(double height READ height)
    Q_PROPERTY(QString species READ species)
public:
    explicit ScriptTree(QObject *parent = nullptr);
    void setTree(Tree *t) { mTree = t; }
    const Tree *tree() { return mTree; }
    void clear() { mTree = nullptr; }

    bool valid() const { return mTree != nullptr; }
    double x() const {  return mTree ? mTree->position().x() : -1.;    }
    double y() const {  return mTree ? mTree->position().y() : -1;    }
    double dbh() const {  return mTree ? static_cast<double>(mTree->dbh()) : -1.;    }
    double height() const {  return mTree ? static_cast<double>(mTree->height()) : -1.;    }
    QString species() const { return mTree ? mTree->species()->id() : QStringLiteral("invalid"); }

signals:

public slots:
    QString info();
    double expr(QString expr_str);

private:
    Tree *mTree;
};

// Expression class
class ScriptTreeExpr : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString expression READ expression WRITE setExpression)
public:
    Q_INVOKABLE ScriptTreeExpr(QString expr);
    static void addToScriptEngine(QJSEngine &engine);
public slots:
    QString expression() { return mExpression.expression(); }
    void setExpression(QString expr) {  mExpression.setExpression(expr); mExpression.setModelObject(&mTW); }
    double value(ScriptTree *script_tree );
private:
    Expression mExpression;
    TreeWrapper mTW;
};

#endif // SCRIPTTREE_H
