#include "global.h"
#include "standloader.h"

#include "grid.h"
#include "model.h"
#include "resourceunit.h"
#include "speciesset.h"

#include "helper.h"
#include "expression.h"
#include "expressionwrapper.h"
#include "environment.h"
#include "csvfile.h"

#include <QtCore>

QStringList picusSpeciesIds = QStringList() << "0" << "1" << "17";
QStringList iLandSpeciesIds = QStringList() << "piab" << "piab" << "fasy";


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
        return;
    }

    // copy trees from first unit to all other units:
    if (copy_mode=="copy") {
        loadInitFile(fileName, type);
        copyTrees();
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
            loadInitFile(fileName, type, ru);
            qDebug() << "loaded" << fileName << "on" << ru->boundingBox() << "," << ru->trees().count() << "trees.";
        }
        return;
    }
    throw IException("StandLoader::processInit: invalid initalization.mode!");
}

void StandLoader::evaluateDebugTrees()
{
    // evaluate debugging
    QString dbg_str = GlobalSettings::instance()->settings().paramValueString("debug_tree");
    if (!dbg_str.isEmpty()) {
       TreeWrapper tw;
       Expression dexp(dbg_str, &tw); // load expression dbg_str and enable external model variables
        AllTreeIterator at(GlobalSettings::instance()->model());
        double result;
        while (Tree *t = at.next()) {
            tw.setTree(t);
            result = dexp.execute();
            if (result)
                t->enableDebugging();
        }
    }
}

void StandLoader::loadInitFile(const QString &fileName, const QString &type, ResourceUnit *ru)
{
    QString pathFileName = GlobalSettings::instance()->path(fileName, "init");
    if (!QFile::exists(pathFileName))
        throw IException(QString("StandLoader::loadInitFile: File %1 does not exist!").arg(pathFileName));

    if (type=="picus")
        return loadFromPicus(pathFileName, ru);
    if (type=="iland")
        return loadiLandFile(pathFileName, ru);

    throw IException("StandLoader::loadInitFile: unknown initalization.type!");
}

void StandLoader::loadFromPicus(const QString &fileName, ResourceUnit *ru)
{
    if (!ru)
        ru = mModel->ru();
    Q_ASSERT(ru!=0);

    QPointF offset = ru->boundingBox().topLeft();

    SpeciesSet *speciesSet = ru->speciesSet(); // of default RU

    QString text = Helper::loadTextFile(fileName);
    if (text.isEmpty()) {
        qDebug() << "file not found: " + fileName;
        return;
    }

    // cut out the <trees> </trees> part....
    QRegExp rx(".*<trees>(.*)</trees>.*");
    rx.indexIn(text, 0);
    if (rx.capturedTexts().count()<1)
        return;
    text = rx.cap(1).trimmed();
    QStringList lines=text.split('\n');
    if (lines.count()<2)
        return;
    char sep='\t';
    if (!lines[0].contains(sep))
        sep=';';
    QStringList headers = lines[0].trimmed().split(sep);

    int iID = headers.indexOf("id");
    int iX = headers.indexOf("x");
    int iY = headers.indexOf("y");
    int iBhd = headers.indexOf("bhdfrom");
    int iHeight = headers.indexOf("treeheight");
    int iSpecies = headers.indexOf("species");
    int iAge = headers.indexOf("age");
    if (iX==-1 || iY==-1 || iBhd==-1 || iSpecies==-1 || iHeight==-1)
        throw IException(QString("Initfile %1 is not valid!").arg(fileName));

    double dbh;
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
        tree.setHeight(line.section(sep, iHeight, iHeight).toDouble()/100.); // convert from Picus-cm to m.
        if (iAge>=0)
           tree.setAge(line.section(sep, iAge, iAge).toInt());
        else
            tree.setAge(10);
        QString speciesid = line.section(sep, iSpecies, iSpecies);
        bool ok;
        int picusid = speciesid.toInt(&ok);
        if (ok) {
            int idx = picusSpeciesIds.indexOf(line.section(sep, iSpecies, iSpecies));
            if (idx==-1)
                throw IException(QString("Loading init-file: invalid Picus-species-id. Species: %1").arg(picusid));
            speciesid = iLandSpeciesIds[idx];
        }
        Species *s = speciesSet->species(speciesid);
        if (!ru || !s)
            throw IException(QString("Loading init-file: either ressource unit or species invalid. Species: %1").arg(speciesid));

        tree.setRU(ru);
        tree.setSpecies(s);
        tree.setup();
    }
    //qDebug() << "loaded init-file contained" << lines.count() <<"lines.";
    //qDebug() << "lines: " << lines;

}

struct InitFileItem
{
    Species *species;
    int count;
    double dbh_from, dbh_to;
    double hd;
    int age;
};
 QVector<InitFileItem> init_items;
 void executeiLandInit(ResourceUnit *ru);
void StandLoader::loadiLandFile(const QString &fileName, ResourceUnit *ru)
{
    if (!ru)
        ru = mModel->ru();
    Q_ASSERT(ru!=0);
    SpeciesSet *speciesSet = ru->speciesSet(); // of default RU
    Q_ASSERT(speciesSet!=0);

    if (!QFile::exists(fileName))
        throw IException(QString("load-ini-file: file '%1' does not exist.").arg(fileName));
    DebugTimer t("StandLoader::loadiLandFile");
    CSVFile infile(fileName);
    int icount = infile.columnIndex("count");
    int ispecies = infile.columnIndex("species");
    int idbh_from = infile.columnIndex("dbh_from");
    int idbh_to = infile.columnIndex("dbh_to");
    int ihd = infile.columnIndex("hd");
    int iage = infile.columnIndex("age");
    if (icount<0 || ispecies<0 || idbh_from<0 || idbh_to<0 || ihd<0 || iage<0)
        throw IException(QString("load-ini-file: file '%1' containts not all required fields (count, species, dbh_from, dbh_to, hd, age).").arg(fileName));

    init_items.clear(); // init_items: declared as a global
    InitFileItem item;
    for (int row=0;row<infile.rowCount();row++) {
         item.count = infile.value(row, icount).toInt();
         item.dbh_from = infile.value(row, idbh_from).toDouble();
         item.dbh_to = infile.value(row, idbh_to).toDouble();
         item.hd = infile.value(row, ihd).toDouble();
         item.age = infile.value(row, iage).toInt();
         item.species = speciesSet->species(infile.value(row, ispecies).toString());
         if (!item.species) {
             throw IException(QString("load-ini-file: unknown speices '%1' in file '%2', line %3.")
                              .arg(infile.value(row, ispecies).toString())
                              .arg(fileName)
                              .arg(row));
         }
         init_items.push_back(item);
    }
    // exeucte the
    executeiLandInit(ru);

}

// evenlist: tentative order of pixel-indices (within a 5x5 grid) used as tree positions.
// e.g. 12 = centerpixel, 0: upper left corner, ...
int evenlist[25] = { 12, 6, 18, 16, 8, 22, 2, 10, 14, 0, 24, 20, 4,
                     1, 13, 15, 19, 21, 3, 7, 11, 17, 23, 5, 9};
// sort function
bool sortPairLessThan(const QPair<int, double> &s1, const QPair<int, double> &s2)
 {
     return s1.second < s2.second;
 }
void executeiLandInit(ResourceUnit *ru)
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
    double r;
    const double exponent = 2.;
    foreach(const InitFileItem &item, init_items) {
        for (int i=0;i<item.count;i++) {
            // create trees
            r = drandom(); // 0..1
            key = 99 - int(100 * pow(r, exponent)); // more "hits" for lower indices = higher likelihood to go to pixels with lower basal area
            key = limit(key, 0, 99);
            int tree_idx = ru->newTreeIndex();
            Tree &tree = ru->trees()[tree_idx]; // get reference to modify tree
            tree.setDbh(nrandom(item.dbh_from, item.dbh_to));
            tree.setHeight(tree.dbh()/100. * item.hd); // dbh from cm->m, *hd-ratio -> meter height
            tree.setSpecies(item.species);
            tree.setAge(item.age);
            tree.setRU(ru);
            tree.setup();
            // key: rank of target pixel
            // first: index of target pixel
            // second: sum of target pixel
            tree_map.insert(tcount[key].first, tree_idx); // store tree in map
            tcount[key].second+=tree.basalArea(); // aggregate the basal area for each 10m pixel
            if (i%20==0)
                qSort(tcount.begin(), tcount.end(), sortPairLessThan);
        }
        qSort(tcount.begin(), tcount.end(), sortPairLessThan);
    }

    int bits, index, pos;
    int c;
    QList<int> trees;
    QPoint tree_pos;
    int tree_no=0;
    for (int i=0;i<100;i++) {
        trees = tree_map.values(i);
        c = trees.count();
        bits = 0;
        index = -1;
        foreach(int tree_idx, trees) {
            if (c>18) {
                index = (index + 1)%25;
            } else {
                int stop=1000;
                do {
                    index = limit(int(25 * pow(drandom(), exponent)), 0, 24);
                } while (isBitSet(bits, index)==true && stop--);
                if (!stop)
                    qDebug() << "executeiLandInit: found no free bit.";
                setBit(bits, index, true); // mark position as used
            }
            pos = evenlist[index];
            tree_pos = offsetIdx  // position of resource unit
                       + QPoint(5*(i/10), 5*(i%10)) // relative position of 10x10m pixel
                       + QPoint(pos/5, pos%5); // relative position within 10x10m pixel
            //qDebug() << tree_no++ << "to" << index;
            ru->trees()[tree_idx].setPosition(tree_pos);
        }
    }
}
