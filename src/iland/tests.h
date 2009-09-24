#ifndef TESTS_H
#define TESTS_H

class Tests
{
public:
    Tests(QObject *wnd);
    void speedOfExpression();
    void clearTrees();
    void killTrees();
    // climate
    void climate();
    // light based tests for multiple stands
    void multipleLightRuns(const QString &fileName);
    private:
    QString dumpTreeList();
    QObject *mParent;

};

#endif // TESTS_H
