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
        return;

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
