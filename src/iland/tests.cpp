#include "global.h"
#include "tests.h"

#include "helper.h"
#include "random.h"
#include "model.h"
#include "resourceunit.h"
#include "expressionwrapper.h"
#include "expression.h"
///
#include "climate.h"
#include "species.h"
#include "speciesresponse.h"
#include "watercycle.h"
#include "csvfile.h"
#include "xmlhelper.h"
#include "environment.h"
#include "exception.h"
#include "seeddispersal.h"
#include "establishment.h"
//
#include "standloader.h"
#include "soil.h"
#include "gisgrid.h"

Tests::Tests(QObject *wnd)
{
    //QObject::connect(this, SIGNAL(needRepaint),wnd, SLOT(repaint));
    mParent=wnd;

}


void Tests::testXml()
{
    XmlHelper xml( GlobalSettings::instance()->settings().node("model.species") );
    qDebug() << xml.value("source");
    QDomElement e = xml.node("source");
    xml.setNodeValue(e, "chief wiggum");
    qDebug() <<  xml.value("source"); // should print chief wiggum
    xml.setNodeValue("model.species.source","species");
    qDebug() << "and back:" << xml.value("source");

    // Test of environment
    Environment environment;
    environment.loadFromFile(GlobalSettings::instance()->path("envtest.txt"));
    try {
        environment.setPosition(QPointF(34,10)); // 0/0
        environment.setPosition(QPointF(150,10)); // 1/0
        environment.setPosition(QPointF(250,10)); // 2/0
        Climate *c = environment.climate();
        SpeciesSet *s = environment.speciesSet();
        qDebug() << s->count();
        qDebug() << c->daylength_h(160);
        environment.setPosition(QPointF(20,200)); // 0/2
        environment.setPosition(QPointF(-34,10)); // exception
    } catch (const IException &ex) {
        qDebug() << ex.toString();
    }
}

void Tests::speedOfExpression()
{
    Q_ASSERT(1==0);
//    int *p;
//    p = 0;
//    *p = 1; --> signal handler does not really work
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

void Tests::testWater()
{
    Model *model = GlobalSettings::instance()->model();
    model->createStandStatistics(); // force this!
    WaterCycle wc;
    wc.setup(model->ru());
    wc.run();
}

void Tests::testPheno(const Climate *clim)
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

    // get a climate response object....
    Model *model = GlobalSettings::instance()->model();
    //model->ru()->climate()->setup(); // force setup

    ResourceUnitSpecies *rus = model->ru()->ruSpecies().first();

    rus->calculate();
    const SpeciesResponse *sr = rus->speciesResponse();
    QString line;
    for (int mon=0;mon<12;mon++) {
        line = QString("%1;%2").arg(mon)
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
            loader.loadPicusFile(file);
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

void Tests::testCSVFile()
{
    CSVFile file;
    file.loadFile("e:\\csvtest.txt");
    for (int row=0;row<file.rowCount(); row++)
        for (int col=0;col<file.colCount(); col++)
            qDebug() << "row col" << row << file.columnName(col) << "value" << file.value(row, col);
}


void Tests::testRandom()
{
    RandomCustomPDF pdf("x^2");
    RandomCustomPDF *pdf2 = new RandomCustomPDF("x^3");
    QStringList list;
    for (int i=0;i<1000;i++)
        list << QString::number(pdf.get());
    qDebug() << list.join("\n");
    delete pdf2;
    // simple random test
    list.clear();
    for (int i=0;i<1000;i++)
        list << QString::number(irandom(0,5));
    qDebug() << "irandom test (0,5): " << list;

}

void Tests::testGridRunner()
{
    Grid<float> &lif = *GlobalSettings::instance()->model()->grid();
    //QRectF box = GlobalSettings::instance()->model()->ru(0)->boundingBox();
    QRectF box2 = QRectF(10,10,10,10);
    GridRunner<float> runner(lif, box2);
    int i=0;
    while (float *p=runner.next()) {
        QPoint point = lif.indexOf(p);
        qDebug() << i++ << point.x() << point.y() << *p << p;
    }
}

void Tests::testSeedDispersal()
{
    SeedDispersal sd;
    sd.setup();
    sd.loadFromImage("e:\\temp\\test.jpg");
    QImage img = gridToImage(sd.seedMap(), true, -1., 1.);
    img.save("e:\\temp\\seedmap.png");
    sd.execute();
//    sd.edgeDetection();
//    gridToImage(sd.seedMap(), true, -1., 1.).save("seedmap_edge.png");
//    sd.distribute();
    img = gridToImage(sd.seedMap(), true, -1., 1.);
    img.save("e:\\temp\\seedmap_e.png");

}

Expression tme_exp;
int tme_count;
double tme_test1(const double &x) {
    double result = tme_exp.calculate(x);
    if (result!=1)
        tme_count++;
    return result;
}
double tme_test2(const double &x) {
    tme_exp.setVar("x",x);
    double result = tme_exp.execute();
    if (result!=1)
        tme_count++;
    return result;
}
QMutex tme_mutex;
double tme_test3(const double &x) {
    QMutexLocker l(&tme_mutex);
    tme_exp.setVar("x",x);
    double result = tme_exp.execute();
    if (result!=1)
        tme_count++;
    return result;
}
void Tests::testMultithreadExecute()
{
    tme_exp.setExpression("(x+x+x+x)/(sqrt(x*x)*4)");
    tme_exp.setStrict(false);
    try {
        tme_exp.parse();
    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
    QVector<double> data;
    for (int i=1;i<1000000;i++)
        data.push_back(nrandom(1,2000));
    qDebug() << "test multiple threads, part 1 (thread safe):";
    tme_count = 0;
    { DebugTimer t("threadsafe");
        QtConcurrent::blockingMap(data,(*tme_test1)); }
    qDebug() << "finished: errors: " << tme_count;

    tme_exp.setStrict(false);
    tme_exp.addVar("x");
    try {
        tme_exp.parse();
    } catch(const IException &e) {
        QString error_msg = e.toString();
        Helper::msg(error_msg);
        qDebug() << error_msg;
    }
    qDebug() << "test multiple threads, part 2 (non thread safe):";
    tme_count = 0;
    { DebugTimer t("no_thread");
    QtConcurrent::blockingMap(data,(*tme_test2));
    }
    qDebug() << "finished: errors: " << tme_count;

    qDebug() << "test multiple threads, part 3 (mutex locker):";
    tme_count = 0;
    { DebugTimer t("mutex");
        QtConcurrent::blockingMap(data,(*tme_test3));
    }
    qDebug() << "finished: errors: " << tme_count;
}

void Tests::testEstablishment()
{
    Model *model = GlobalSettings::instance()->model();
    //model->ru(0)
    Establishment est(model->ru(0)->climate(),model->ru(0)->ruSpecies().first());
    est.calculate();
}

void Tests::testLinearExpressions()
{
    Expression::setLinearizationEnabled(true); // enable
    Expression a("40*(1-(1-(x/40)^(1/3))*exp(-0.03))^3"); // test function: sapling growth
    Expression b("40*(1-(1-(x/40)^(1/3))*exp(-0.03))^3");

    b.linearize(2., 140.);
    b.calculate(58.);
    for (double x=5.; x<150.; x++) {
        double ra = a.calculate(x);
        double rb = b.calculate(x);
        qDebug() << x << ra << rb << rb-ra;
    }
    // test b
    { DebugTimer t("straight");
        for (int i=0;i<1000000;i++) {
            a.calculate(3.4);
        }
    }
    { DebugTimer t("linear");
        for (int i=0;i<1000000;i++) {
            b.calculate(3.4);
        }
    }
    { DebugTimer t("nativ");
        double x=3;
        double y=0;
        for (int i=0;i<1000000;i++) {
           y+= 40.*pow(1.-(1.-pow((x/40.),1./3.))*exp(-0.03),3.);
        }
        qDebug() << y;
    }
    /// test matrix-case
//    Expression c("x*x+exp(y*y)");
//    Expression d("x*x+exp(y*y)");
    Expression c("exp(2*ln(x)*(1-y/2))");
    Expression d("exp(2*ln(x)*(1-y/2))");
    d.linearize2d(0., 1., 0., 1.);
    qDebug() << d.calculate(0, 0.8);
    for (double x=0.; x<1.; x+=0.1) {
        for (double y=0.; y<1.; y+=0.1) {
            double ra = c.calculate(x,y);
            double rb = d.calculate(x,y);
            qDebug() << x << y << ra << rb << rb-ra;
        }
    }
    qDebug() << "random test";
    for (int i=0;i<100;i++) {
        double x = drandom();
        double y = drandom();
        double ra = c.calculate(x,y);
        double rb = d.calculate(x,y);
        qDebug() << x << y << ra << rb << rb-ra;

    }
    qDebug() << "performance";
    { DebugTimer t("straight");
        for (int i=0;i<1000000;i++) {
            c.calculate(0.33, 0.454);
        }
    }
    { DebugTimer t("linear");
        for (int i=0;i<1000000;i++) {
            d.calculate(0.33, 0.454);
        }
    }
}

void Tests::testSoil()
{
//    Y0l=33 # young C labile, i.e. litter
//    Y0Nl=0.45 # young N labile
//    Y0r=247 # young C refractory , i.e. dwd
//    Y0Nr=0.99 # young N refractory
//    O0=375 # old C, i.e soil organic matter (SOM)
//    O0N=15 # old N
//    kyl=0.15 # litter decomposition rate
//    kyr=0.0807 # downed woody debris (dwd) decomposition rate
    Soil soil;
    qDebug() << "soil object has" << sizeof(soil) << "bytes.";
    // values from RS R script script_snagsoil_v5.r
    soil.setInitialState(CNPool(33000., 450., 0.15), // young lab
                         CNPool(247000., 990., 0.0807), // young ref
                         CNPair(375000., 15000.)); // SOM
    CNPool in_l(3040, 3040/75., 0.15);
    CNPool in_r(11970, 11970./250., 0.0807);
    double re = 1.1;

    QStringList out;
    out << "year;iLabC;iLabN;ikyl;iRefC;iRefN;ikyr;RE;kyl;kyr;ylC;ylN;yrC;yrN;somC;somN;NAvailable";

    for (int i=0;i<100;i++) {
        // run the soil model
        soil.setClimateFactor(re);
        if (i==1) {
           soil.setSoilInput(in_l*5., in_r*5.); // pulse in year 2
        } else if (i>1 && i<10) {
            soil.setSoilInput(CNPool(), CNPool()); // no input for years 3..10
        } else{
            soil.setSoilInput(in_l, in_r); // normal input
        }

        soil.calculateYear();
        QList<QVariant> list = soil.debugList();
        QString line=QString::number(i)+";";
        foreach(QVariant v, list)
            line+=v.toString() + ";";
        line.chop(1);
        out << line;
    }
    if (fabs(soil.availableNitrogen() - 77.2196456179288) < 0.000001)
        qDebug() << "PASSED.";
    else
        qDebug() << "ERROR";
    qDebug() << "ICBM/2N run: saved to e:/soil.txt";
    //qDebug() << out;
    Helper::saveToTextFile("e:/soil.txt", out.join("\r\n"));

    qDebug()<< "test weighting of CNPool (C/N/rate):";
    CNPool c(1,1,1);
    for (int i=0;i<10;i++) {
        c.addBiomass(1,1,2);
        qDebug() << c.C << c.N << c.parameter();
    }

}

void Tests::testMap()
{
    QString fileName = GlobalSettings::instance()->path("gis/montafon_grid.txt");
    GisGrid gis_grid;
    HeightGrid *hgrid = GlobalSettings::instance()->model()->heightGrid();
    if (gis_grid.loadFromFile(fileName)) {
        Grid<int> *sgrid = gis_grid.create10mGrid();
        // setup of the mask...
        for (int i=0;i<sgrid->count();i++)
            hgrid->valueAtIndex(i).height = sgrid->valueAtIndex(i);
            //mHeightGrid->valueAtIndex(i).setValid( sgrid->valueAtIndex(i)!=-1 );
        delete sgrid;
    }

}
