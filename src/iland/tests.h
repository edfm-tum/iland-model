#ifndef TESTS_H
#define TESTS_H

class Climate;
class Tests
{
public:
    Tests(QObject *wnd);
    void speedOfExpression();
    void clearTrees();
    void killTrees();
    // climate
    void climate();
    void climateResponse();
    // light based tests for multiple stands
    void multipleLightRuns(const QString &fileName);
    private:
    QString dumpTreeList();
    QObject *mParent;
 private:
    void testSun();
    void testPheno(Climate *clim);

};

#endif // TESTS_H
