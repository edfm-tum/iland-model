#ifndef SCRIPTTREE_H
#define SCRIPTTREE_H

#include <QObject>
#include "tree.h"
#include "species.h"

class ScriptTree : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double x READ x)
    Q_PROPERTY(double y READ y)
    Q_PROPERTY(double dbh READ dbh)
    Q_PROPERTY(double height READ height)
    Q_PROPERTY(QString species READ species)
public:
    explicit ScriptTree(QObject *parent = nullptr);
    void setTree(Tree *t) { mTree = t; }
    void clear() { mTree = nullptr; }

    double x() const {  return mTree ? mTree->position().x() : -1.;    }
    double y() const {  return mTree ? mTree->position().y() : -1;    }
    double dbh() const {  return mTree ? mTree->dbh() : -1.;    }
    double height() const {  return mTree ? mTree->height() : -1.;    }
    QString species() const { return mTree ? mTree->species()->id() : QStringLiteral("invalid"); }

signals:

public slots:
    QString info();

private:
    Tree *mTree;
};

#endif // SCRIPTTREE_H
