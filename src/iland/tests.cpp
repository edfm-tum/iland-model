#include "global.h"
#include "tests.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "expressionwrapper.h"
#include "expression.h"
///
#include "climate.h"
//
#include "standloader.h"

Tests::Tests(QObject *wnd)
{
    //QObject::connect(this, SIGNAL(needRepaint),wnd, SLOT(repaint));
    mParent=wnd;

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
        if (drandom() < 0.20) {
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

void Tests::climate()
{
    Climate clim;
    try {
    clim.setup();
    DebugTimer t("climate 100yrs", true);
    ClimateDay *begin, *end;
    int mon=0;
    for (int i=0;i<100;i++) {
        clim.monthRange(mon, &begin, &end);
        clim.nextYear();
        mon=(mon+1)%12;
    }
    } catch (IException &e) {
        Helper::msg(e.toString());
    }
}


void Tests::multipleLightRuns(const QString &fileName)
{
    XmlHelper xml(fileName);
    Model *model = GlobalSettings::instance()->model();
    if (!model || !model->ru())
        return;
    QVector<Tree> &mTrees =  model->ru()->trees();

    QString outPath = xml.value("outputpath");
    QString inPath = xml.value("inputpath");
    QString inFile = xml.value("stands");
    qDebug() << "standlist:" << inFile << "inpath:"<<inPath << "save to:"<<outPath;
    QStringList fileList = Helper::loadTextFile(inFile).remove('\r').split('\n', QString::SkipEmptyParts);

    StandLoader loader(model);
    try {
        foreach (QString file, fileList) {
            file = inPath + "\\" + file;
            qDebug() << "processing" << file;
            mTrees.clear();
            loader.loadFromPicus(file);
            // do a cycle...
            model->runYear();

            // create output file
            QFileInfo fi(file);
            QString outFileName = QString("%1\\out_%2.csv").arg(outPath, fi.baseName());
            Helper::saveToTextFile(outFileName, dumpTreeList() );
            qDebug() << mTrees.size() << "trees loaded, saved to" << outFileName;
            mParent->metaObject()->invokeMethod(mParent,"repaint");
            QApplication::processEvents();

        }
    } catch (IException &e) {
        Helper::msg(e.toString());
    }
}

QString Tests::dumpTreeList()
{

    Model *model = GlobalSettings::instance()->model();

    AllTreeIterator at(model);
    DebugList treelist;
    QString line;
    QStringList result;
    result << "id;species;dbh;height;x;y;RU#;LRI;mWoody;mRoot;mFoliage;LA";

    while (Tree *tree = at.next()) {
        treelist.clear();
        tree->dumpList(treelist);
        line = "";
        foreach(QVariant value, treelist)
            line+=value.toString() + ";";
        result << line;
    }

    QString resStr = result.join("\n");
    return resStr;
}
