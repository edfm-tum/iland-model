#include "global.h"
#include "tests.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "expressionwrapper.h"
#include "expression.h"
///
#include "climate.h"
#include "species.h"
#include "speciesresponse.h"

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
    DebugTimer t("climate 100yrs");
    t.setAsWarning();
    const ClimateDay *begin, *end;
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


void Tests::testSun()
{
    // solar radiation
    Sun sun;
    sun.setup(RAD(47));
    qDebug()<<sun.dump();
    sun.setup(RAD(70));
    qDebug()<<sun.dump();
    sun.setup(RAD(-20));
    qDebug()<<sun.dump();
}

void Tests::testPheno(Climate *clim)
{
    Phenology pheno(1,clim,0.9, 4.1, 10., 11., 2. ,9.);
    pheno.calculate();
    qDebug() << "Phenology is the key:";
    for (int i=0;i<12;i++)
        qDebug() << i << pheno.month()[i];
}

void Tests::climateResponse()
{

    try {

    DebugTimer t("climate Responses");
    t.setAsWarning();

    // get a climate response object....
    Model *model = GlobalSettings::instance()->model();
    //model->ru()->climate()->setup(); // force setup

    const ResourceUnitSpecies &rus = model->ru()->ruSpecies().first();

    const_cast<ResourceUnitSpecies&>(rus).calculateResponses();
    const SpeciesResponse *sr = rus.speciesResponse();
    QString line;
    for (int mon=0;mon<12;mon++) {
        line = QString("%1;%2;%3").arg(mon)
               .arg(sr->vpdResponse()[mon])
               .arg(sr->tempResponse()[mon]);
        qDebug() << line;
    }
    // test nitrogen response
    line="Nitrogen response\n";
    for (double navailable=0.; navailable<100;navailable+=10) {
        for (double cls=1.; cls<=3.; cls+=0.2) {
            double res = model->ru()->speciesSet()->nitrogenResponse(navailable,cls);
            line+=QString("%1;%2;%3\n").arg(navailable).arg(cls).arg(res);
        }
    }
    qDebug() << line;

    line="CO2 response\n";
    for (double nresponse=0.; nresponse<=1;nresponse+=0.1) {
        for (double h2o=0.; h2o<=1; h2o+=0.1) {
            for (double co2=250; co2<600; co2+=50) {
                double res = model->ru()->speciesSet()->co2Response(co2, nresponse, h2o);
                line+=QString("%1;%2;%3;%4\n").arg(nresponse).arg(h2o).arg(co2).arg(res);
            }
        }
    }
    qDebug() << line;

    // sun
    //testSun();
    testPheno(model->ru()->climate());

    // test phenology
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
            Tree::resetStatistics();
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
        *(line.end()-1)=' ';
        result << line;
    }

    QString resStr = result.join("\n");
    return resStr;
}
