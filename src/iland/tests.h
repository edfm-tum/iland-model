#ifndef TESTS_H
#define TESTS_H
#include <QtCore>

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
    void testWater();
    void testCSVFile();
    void testXml();
    private:
    QString dumpTreeList();
    QObject *mParent;
 private:
    void testSun();
    void testPheno(const Climate *clim);


};

#endif // TESTS_H
