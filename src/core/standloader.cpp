#include "global.h"
#include "standloader.h"


#include "grid.h"
#include "model.h"
#include "resourceunit.h"
#include "speciesset.h"

#include "helper.h"
#include "random.h"
#include "expression.h"
#include "expressionwrapper.h"
#include "environment.h"
#include "csvfile.h"

// provide a mapping between "Picus"-style and "iLand"-style species Ids
QVector<int> picusSpeciesIds = QVector<int>() << 0 << 1 << 17;
QStringList iLandSpeciesIds = QStringList() << "piab" << "piab" << "fasy";
StandLoader::~StandLoader()
{
    if (mRandom)
        delete mRandom;
}

void StandLoader::loadForUnit()
{

}

void StandLoader::copyTrees()
{
    // we assume that all stands are equal, so wie simply COPY the trees and modify them afterwards
    const Grid<ResourceUnit*> &ruGrid=mModel->RUgrid();
    ResourceUnit **p = ruGrid.begin();
    if (!p)
        throw IException("Standloader: invalid resource unit pointer!");
    ++p; // skip the first...
    const QVector<Tree> &tocopy = mModel->ru()->trees();
    for (; p!=ruGrid.end(); ++p) {
        QRectF rect = (*p)->boundingBox();
        foreach(const Tree& tree, tocopy) {
            Tree &newtree = (*p)->newTree();
            newtree = tree; // copy tree data...
            newtree.setPosition(tree.position()+(*p)->boundingBox().topLeft());
            newtree.setRU(*p);
            newtree.setNewId();
        }
    }
    qDebug() << Tree::statCreated() << "trees loaded / copied.";
}

/** main routine of the stand setup.
*/
void StandLoader::processInit()
{
    GlobalSettings *g = GlobalSettings::instance();
    XmlHelper xml(g->settings().node("model.initialization"));

    QString copy_mode = xml.value("mode", "copy");
    QString type = xml.value("type", "");
    QString  fileName = xml.value("file", "");

    Tree::resetStatistics();

    // one global init-file for the whole area:
    if (copy_mode=="single") {
        loadInitFile(fileName, type);
        evaluateDebugTrees();
        return;
    }

    // copy trees from first unit to all other units:
    if (copy_mode=="copy") {
        loadInitFile(fileName, type);
        copyTrees();
        evaluateDebugTrees();
        return;
    }

    // call a single tree init for each resource unit
    if (copy_mode=="unit") {
        foreach( const ResourceUnit *const_ru, g->model()->ruList()) {
            ResourceUnit *ru = const_cast<ResourceUnit*>(const_ru);
            // set environment
            g->model()->environment()->setPosition(ru->boundingBox().center());
            type = xml.value("type", "");
            fileName = xml.value("file", "");
            if (fileName.isEmpty())
                continue;
            loadInitFile(fileName, type, ru);
            qDebug() << "loaded" << fileName << "on" << ru->boundingBox() << "," << ru->trees().count() << "trees.";
        }
        evaluateDebugTrees();
        return;
    }

    throw IException("StandLoader::processInit: invalid initalization.mode!");
}

void StandLoader::evaluateDebugTrees()
{
    // evaluate debugging
    QString dbg_str = GlobalSettings::instance()->settings().paramValueString("debug_tree");
    int counter=0;
    if (!dbg_str.isEmpty()) {
       TreeWrapper tw;
       Expression dexp(dbg_str, &tw); // load expression dbg_str and enable external model variables
        AllTreeIterator at(GlobalSettings::instance()->model());
        double result;
        while (Tree *t = at.next()) {
            tw.setTree(t);
            result = dexp.execute();
            if (result) {
                t->enableDebugging();
                counter++;
            }
        }
    }
    qDebug() << "evaluateDebugTrees: enabled debugging for" << counter << "trees.";
}

void StandLoader::loadInitFile(const QString &fileName, const QString &type, ResourceUnit *ru)
{
    QString pathFileName = GlobalSettings::instance()->path(fileName, "init");
    if (!QFile::exists(pathFileName))
        throw IException(QString("StandLoader::loadInitFile: File %1 does not exist!").arg(pathFileName));

    if (type=="picus" || type=="single")
        return loadPicusFile(pathFileName, ru);
    if (type=="iland" || type=="distribution")
        return loadiLandFile(pathFileName, ru);

    throw IException(QLatin1String("StandLoader::loadInitFile: unknown initalization.type:")+type);
}

void StandLoader::loadPicusFile(const QString &fileName, ResourceUnit *ru)
{
    QString content = Helper::loadTextFile(fileName);
    if (content.isEmpty()) {
        qDebug() << "file not found: " + fileName;
        return;
    }
    loadSingleTreeList(content, ru, fileName);
}

void StandLoader::loadSingleTreeList(const QString &content, ResourceUnit*ru, const QString &fileName)
{
    if (!ru)
        ru = mModel->ru();
    Q_ASSERT(ru!=0);

    QPointF offset = ru->boundingBox().topLeft();
    SpeciesSet *speciesSet = ru->speciesSet(); // of default RU

    QString my_content(content);
    // cut out the <trees> </trees> part if present
    if (content.contains("<trees>")) {
        QRegExp rx(".*<trees>(.*)</trees>.*");
        rx.indexIn(content, 0);
        if (rx.capturedTexts().count()<1)
            return;
        my_content = rx.cap(1).trimmed();
    }

    QStringList lines=my_content.split('\n');
    if (lines.count()<2)
        return;
    // drop comments
    while (!lines.isEmpty() && lines.front().startsWith('#') )
        lines.pop_front();
    while (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();

    char sep='\t';
    if (!lines[0].contains(sep))
        sep=';';
    QStringList headers = lines[0].trimmed().split(sep);

    int iID = headers.indexOf("id");
    int iX = headers.indexOf("x");
    int iY = headers.indexOf("y");
    int iBhd = headers.indexOf("bhdfrom");
    if (iBhd<0)
        iBhd = headers.indexOf("dbh");
    double height_conversion = 100.;
    int iHeight = headers.indexOf("treeheight");
    if (iHeight<0) {
        iHeight = headers.indexOf("height");
        height_conversion = 1.; // in meter
    }
    int iSpecies = headers.indexOf("species");
    int iAge = headers.indexOf("age");
    if (iX==-1 || iY==-1 || iBhd==-1 || iSpecies==-1 || iHeight==-1)
        throw IException(QString("Initfile %1 is not valid!\nObligatory columns are: x,y, bhdfrom or dbh, species, treeheight or height.").arg(fileName));

    double dbh;
    bool ok;
    QString speciesid;
    for (int i=1;i<lines.count();i++) {
        QString &line = lines[i];
        dbh = line.section(sep, iBhd, iBhd).toDouble();
        if (dbh<5.)
            continue;

        QPointF f;
        if (iX>=0 && iY>=0) {
           f.setX( line.section(sep, iX, iX).toDouble() );
           f.setY( line.section(sep, iY, iY).toDouble() );
           f+=offset;

        }
        // position valid?
        if (!mModel->heightGrid()->valueAt(f).isValid())
            continue;
        Tree &tree = ru->newTree();
        tree.setPosition(f);
        if (iID>=0)
            tree.setId(line.section(sep, iID, iID).toInt() );

        tree.setDbh(dbh);
        tree.setHeight(line.section(sep, iHeight, iHeight).toDouble()/height_conversion); // convert from Picus-cm to m if necessary

        speciesid = line.section(sep, iSpecies, iSpecies);
        int picusid = speciesid.toInt(&ok);
        if (ok) {
            int idx = picusSpeciesIds.indexOf(picusid);
            if (idx==-1)
                throw IException(QString("Loading init-file: invalid Picus-species-id. Species: %1").arg(picusid));
            speciesid = iLandSpeciesIds[idx];
        }
        Species *s = speciesSet->species(speciesid);
        if (!ru || !s)
            throw IException(QString("Loading init-file: either ressource unit or species invalid. Species: %1").arg(speciesid));
        tree.setSpecies(s);

        ok = true;
        if (iAge>=0)
           tree.setAge(line.section(sep, iAge, iAge).toInt(&ok), tree.height()); // this is a *real* age
        if (iAge<0 || !ok || tree.age()==0)
           tree.setAge(0, tree.height()); // no real tree age available

        tree.setRU(ru);
        tree.setup();
    }
    //qDebug() << "loaded init-file contained" << lines.count() <<"lines.";
    //qDebug() << "lines: " << lines;
}

void StandLoader::loadDistributionList(const QString &content, ResourceUnit *ru, const QString &fileName)
{
    if (!ru)
        ru = mModel->ru();
    Q_ASSERT(ru!=0);
    SpeciesSet *speciesSet = ru->speciesSet(); // of default RU
    Q_ASSERT(speciesSet!=0);

    DebugTimer t("StandLoader::loadiLandFile");
    CSVFile infile;
    infile.loadFromString(content);

    int icount = infile.columnIndex("count");
    int ispecies = infile.columnIndex("species");
    int idbh_from = infile.columnIndex("dbh_from");
    int idbh_to = infile.columnIndex("dbh_to");
    int ihd = infile.columnIndex("hd");
    int iage = infile.columnIndex("age");
    int idensity = infile.columnIndex("density");
    if (icount<0 || ispecies<0 || idbh_from<0 || idbh_to<0 || ihd<0 || iage<0)
        throw IException(QString("load-ini-file: file '%1' containts not all required fields (count, species, dbh_from, dbh_to, hd, age).").arg(fileName));

    mInitItems.clear();
    InitFileItem item;
    bool ok;
    for (int row=0;row<infile.rowCount();row++) {
         item.count = infile.value(row, icount).toInt();
         item.dbh_from = infile.value(row, idbh_from).toDouble();
         item.dbh_to = infile.value(row, idbh_to).toDouble();
         item.hd = infile.value(row, ihd).toDouble();
         ok = true;
         if (iage>=0)
             item.age = infile.value(row, iage).toInt(&ok);
         if (iage<0 || !ok)
             item.age = 0;

         item.species = speciesSet->species(infile.value(row, ispecies).toString());
         if (idensity>=0)
             item.density = infile.value(row, idensity).toDouble();
         else
             item.density = 0.;
         if (item.density<-1 || item.density>1)
             throw IException(QString("load-ini-file: invalid value for density. Allowed range is -1..1: '%1' in file '%2', line %3.")
                              .arg(item.density)
                              .arg(fileName)
                              .arg(row));
         if (!item.species) {
             throw IException(QString("load-ini-file: unknown speices '%1' in file '%2', line %3.")
                              .arg(infile.value(row, ispecies).toString())
                              .arg(fileName)
                              .arg(row));
         }
         mInitItems.push_back(item);
    }
    // setup the random distribution
    QString density_func = GlobalSettings::instance()->settings().value("model.initialization.randomFunction", "1-x^2");
    qDebug() << "density function:" << density_func;
    if (!mRandom || (mRandom->densityFunction()!= density_func)) {
        if (mRandom)
            delete mRandom;
        mRandom=new RandomCustomPDF(density_func);
        qDebug() << "new probabilty density function:" << density_func;
    }

    // exeucte the
    executeiLandInit(ru);
    ru->cleanTreeList();

}

void StandLoader::loadiLandFile(const QString &fileName, ResourceUnit *ru)
{
    if (!QFile::exists(fileName))
        throw IException(QString("load-ini-file: file '%1' does not exist.").arg(fileName));
    QString content = Helper::loadTextFile(fileName);
    loadDistributionList(content, ru, fileName);
}

// evenlist: tentative order of pixel-indices (within a 5x5 grid) used as tree positions.
// e.g. 12 = centerpixel, 0: upper left corner, ...
int evenlist[25] = { 12, 6, 18, 16, 8, 22, 2, 10, 14, 0, 24, 20, 4,
                     1, 13, 15, 19, 21, 3, 7, 11, 17, 23, 5, 9};
int unevenlist[25] = { 11,13,7,17, 1,19,5,21, 9,23,3,15,
                       6,18,2,10,4,24,12,0,8,14,20,22};


// sort function
bool sortPairLessThan(const QPair<int, double> &s1, const QPair<int, double> &s2)
{
    return s1.second < s2.second;
}

/**
*/
void StandLoader::executeiLandInit(ResourceUnit *ru)
{

    QPointF offset = ru->boundingBox().topLeft();
    QPoint offsetIdx = GlobalSettings::instance()->model()->grid()->indexAt(offset);

    // a multimap holds a list for all trees.
    // key is the index of a 10x10m pixel within the resource unit
    QMultiMap<int, int> tree_map;
    QVector<QPair<int, double> > tcount; // counts
    for (int i=0;i<100;i++)
        tcount.push_back(QPair<int,double>(i,0.));

    int key;
    double rand_val, rand_fraction;
    int total_count = 0;
    foreach(const InitFileItem &item, mInitItems) {
        rand_fraction = fabs(double(item.density));
        for (int i=0;i<item.count;i++) {
            // create trees
            int tree_idx = ru->newTreeIndex();
            Tree &tree = ru->trees()[tree_idx]; // get reference to modify tree
            tree.setDbh(nrandom(item.dbh_from, item.dbh_to));
            tree.setHeight(tree.dbh()/100. * item.hd); // dbh from cm->m, *hd-ratio -> meter height
            tree.setSpecies(item.species);
            if (item.age<=0)
                tree.setAge(0,tree.height());
            else
                tree.setAge(item.age, tree.height());
            tree.setRU(ru);
            tree.setup();
            total_count++;

            // calculate random value. "density" is from 1..-1.
            rand_val = mRandom->get();
            if (item.density<0)
                rand_val = 1. - rand_val;
            rand_val = rand_val * rand_fraction + drandom()*(1.-rand_fraction);

            // key: rank of target pixel
            // first: index of target pixel
            // second: sum of target pixel
            key = limit(int(100*rand_val), 0, 99); // get from random number generator
            tree_map.insert(tcount[key].first, tree_idx); // store tree in map
            tcount[key].second+=tree.basalArea(); // aggregate the basal area for each 10m pixel
            if ( (total_count < 20 && i%2==0)
                || (total_count<100 && i%10==0 )
                || (i%30==0) ) {
                qSort(tcount.begin(), tcount.end(), sortPairLessThan);
            }
        }
        qSort(tcount.begin(), tcount.end(), sortPairLessThan);
    }

    int bits, index, pos;
    int c;
    QList<int> trees;
    QPoint tree_pos;

    for (int i=0;i<100;i++) {
        trees = tree_map.values(i);
        c = trees.count();
        QPointF pixel_center = ru->boundingBox().topLeft() + QPointF((i/10)*10. + 5., (i%10)*10. + 5.);
        if (!mModel->heightGrid()->valueAt(pixel_center).isValid()) {
            // no trees on that pixel: let trees die
            foreach(int tree_idx, trees) {
                ru->trees()[tree_idx].die();
            }
            continue;
        }

        bits = 0;
        index = -1;
        double r;
        foreach(int tree_idx, trees) {
            if (c>18) {
                index = (index + 1)%25;
            } else {
                int stop=1000;
                index = 0;
                do {
                    //r = drandom();
                    //if (r<0.5)  // skip position with a prob. of 50% -> adds a little "noise"
                    //    index++;
                    //index = (index + 1)%25; // increase and roll over

                    // search a random position
                    r = drandom();
                    index = limit(int(25 *  r*r), 0, 24); // use rnd()^2 to search for locations -> higher number of low indices (i.e. 50% of lookups in first 25% of locations)
                } while (isBitSet(bits, index)==true && stop--);
                if (!stop)
                    qDebug() << "executeiLandInit: found no free bit.";
                setBit(bits, index, true); // mark position as used
            }
            // get position from fixed lists (one for even, one for uneven resource units)
            pos = ru->index()%2?evenlist[index]:unevenlist[index];
            tree_pos = offsetIdx  // position of resource unit
                       + QPoint(5*(i/10), 5*(i%10)) // relative position of 10x10m pixel
                       + QPoint(pos/5, pos%5); // relative position within 10x10m pixel
            //qDebug() << tree_no++ << "to" << index;
            ru->trees()[tree_idx].setPosition(tree_pos);
        }
    }
}
