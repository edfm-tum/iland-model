#include "scripttree.h"

ScriptTree::ScriptTree(QObject *parent) : QObject(parent)
{
    mTree = nullptr;
}

QString ScriptTree::info()
{
    // return some information
    if (!mTree) return QString("invalid tree");
    QString s;
    s.sprintf("%08p", mTree);
    return QString("%1 (%7): %2 (%3cm, %4m, at %5/%6)").arg(s).arg(species()).arg(dbh()).arg(height()).arg(x()).arg(y()).arg(mTree->id());
}
