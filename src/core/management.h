#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <QObject>
#include <QList>
#include <QtCore/QVariantList>

class Tree;
class QScriptEngine;
class Management : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count)
public:
    Management();
    ~Management();
    void run();
    void loadScript(const QString &fileName);
    QString scriptFile() const { return mScriptFile; }
    QString executeScript(QString cmd="");
    static QObject *scriptOutput;
public slots:
    void remain(int number); ///< remove randomly number of trees
    int kill(int pctfrom, int pctto, int number); ///< remove "number" in the percentile interval "from" - "to". remove all if "number" is higher than the count. return the number of removed trees.
    void kill(); ///< kill all trees in the list
    double percentile(int pct); ///< get value for the pct th percentile (1..100)
    int load() { return load(QString()); } ///< load all trees, return number of trees
    int load(QString filter); ///< load all trees passing the filter in a list, return number of trees
    int load(int ruindex); ///< load all trees of a resource index
    void sort(QString statement); ///< sort trees in the list according to a criterion
    int filter(QString filter); ///< apply a filter on the list of trees (expression), return number of remaining trees.
    int filter(QVariantList idList); ///< apply filter in form of a list of ids, return number of remaining trees
    int count() const {return mTrees.count();} ///< return number of trees curerntly in list
private:
    QString mScriptFile;
    QList<QPair<Tree*, double> > mTrees;
    QScriptEngine *mEngine;
    int mRemoved;
};

#endif // MANAGEMENT_H
