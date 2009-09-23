#include "global.h"
#include "tests.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "expressionwrapper.h"
#include "expression.h"

Tests::Tests()
{
}


void Tests::speedOfExpression()
{
    // (1) for each
    double sum;
    int count;
    {
        DebugTimer t("plain loop");
        for (int i=0;i<10;i++) {
            sum=0;
            count=0;
            foreach(const ResourceUnit *ru,GlobalSettings::instance()->model()->ruList()) {
                foreach(const Tree &tree, ru->constTrees()) {
                    sum+=tree.volume();
                    count++;
                }
            }

        }
        qDebug() << "Sum of volume" << sum << "count" << count;
    }
    {
        DebugTimer t("plain loop (iterator)");
        for (int i=0;i<10;i++) {
            AllTreeIterator at(GlobalSettings::instance()->model());
            sum = 0.;
            count = 0;
            while (Tree *tree=at.next()) {
                sum += pow(tree->dbh(),2.1); count++;
            }
        }
        qDebug() << "Sum of volume" << sum << "count" << count;
    }
    {
        TreeWrapper tw;
        Expression expr("dbh^2.1", &tw);
        DebugTimer t("Expression loop");
        for (int i=0;i<10;i++) {

            AllTreeIterator at(GlobalSettings::instance()->model());
            sum = 0.;
            while (Tree *tree=at.next()) {
                tw.setTree(tree);
                sum += expr.execute();
            }
        }
        qDebug() << "Sum of volume" << sum;
    }
}

void Tests::clearTrees()
{
    ResourceUnit *ru = GlobalSettings::instance()->model()->ru();
    int tc = ru->trees().count();
    // kill n percent...
    ru->trees().last().die();
    ru->trees().first().die();
    if (tc>20) {
        ru->trees()[15].die();
        ru->trees()[16].die();
        ru->trees()[17].die();
        ru->trees()[18].die();
        ru->trees()[19].die();
        ru->trees()[20].die();
    }
    qDebug() << "killed 8 trees";
    ru->cleanTreeList();
    ru->cleanTreeList();
}

void Tests::killTrees()
{
    AllTreeIterator at(GlobalSettings::instance()->model());
    int count=0, totalcount=0, idc=0;
    while (Tree *t = at.next()) {
        totalcount++;
        if (random() < 0.20) {
            t->die();
            count++;
        }
    }

    qDebug() << "killed" << count << "of" << totalcount << "left:" << totalcount-count;
    { DebugTimer t("count living");
      at.reset();
      count=0;
      idc=0;
      while (at.nextLiving()) {
          count++; idc+=at.current()->id();
          //qDebug() << at.current()->id();
      }
    }
    qDebug() << count << "living trees idsum" << idc;

    {
    DebugTimer t("clear trees");
    foreach(ResourceUnit *ru,GlobalSettings::instance()->model()->ruList())
        ru->cleanTreeList();
    }
    // count all trees
    at.reset();
    count=0;
    idc=0;
    while (at.next())
    {
        count++; idc+=at.current()->id();
        //qDebug() << (*at)->id();
    }
    qDebug() << count << "trees left idsum" << idc;
}
