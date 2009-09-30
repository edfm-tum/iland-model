#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <QObject>


class Tree;
class QScriptEngine;
class Management : public QObject
{
    Q_OBJECT
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
    double percentile(int pct); ///< get value for the pct th percentile (1..100)
    int load() { return load(QString()); } ///< load all trees
    int load(QString filter); ///< load all trees passing the filter in a list
    void sort(QString statement); ///< sort trees in the list according to a criterion
private:
    QString mScriptFile;
    QList<QPair<Tree*, double> > mTrees;
    QScriptEngine *mEngine;
    int mRemoved;
};

#endif // MANAGEMENT_H
