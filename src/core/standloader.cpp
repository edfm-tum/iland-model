#include "global.h"
#include "standloader.h"

#include "grid.h"
#include "model.h"
#include "resourceunit.h"
#include "speciesset.h"

#include "helper.h"
#include "expression.h"
#include "expressionwrapper.h"

#include <QtCore>

QStringList picusSpeciesIds = QStringList() << "0" << "1" << "17";
QStringList iLandSpeciesIds = QStringList() << "piab" << "piab" << "fasy";




void StandLoader::processInit()
{
    GlobalSettings *g = GlobalSettings::instance();
    XmlHelper xml(g->settings().node("model.initialization"));

    bool for_each_ru = xml.valueBool("foreach");

    QString mode = xml.value("type", ""); // now only "picus"
    QString  fileName = xml.value("file", "");
    if (!QFile::exists(fileName))
        throw IException(QString("File %1 does not exist!").arg(fileName));

    Tree::resetStatistics();
    if (for_each_ru) {
        loadFromPicus(fileName); // load in initial grid cell
        // we assume that all stands are equal, so wie simply COPY the trees and modify them afterwards
        const Grid<ResourceUnit*> &ruGrid=mModel->RUgrid();
        ResourceUnit **p = ruGrid.begin();
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
            //(*p)->trees()
            // loadFromPicus(fileName, rect.topLeft(), *p); -> do that for differing stands
            // copy
        }
        qDebug() << Tree::statCreated() << "trees loaded.";
    } else {
        // only one stand:
        loadFromPicus(fileName);
    }

    // evaluate debugging
    QString dbg_str = GlobalSettings::instance()->settings().paramValueString("debug_tree");
    if (!dbg_str.isEmpty()) {
       TreeWrapper tw;
       Expression dexp(dbg_str, &tw); // load expression dbg_str and enable external model variables
        //double *pid = dexp.addVar("id"); // binding
        //double *pru = dexp.addVar("ru"); // binding
        AllTreeIterator at(GlobalSettings::instance()->model());
        double result;
        while (Tree *t = at.next()) {
            tw.setTree(t);
            //*pid = t->id();
            //*pru = t->ru()->index();
            result = dexp.execute();
            if (result)
                t->enableDebugging();
        }
    }
}

void StandLoader::loadFromPicus(const QString &fileName, QPointF offset, ResourceUnit *ru)
{
    if (!ru)
        ru = mModel->ru();
    SpeciesSet *speciesSet = mModel->ru()->speciesSet(); // of default RU

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

    int iX = headers.indexOf("x");
    int iY = headers.indexOf("y");
    int iBhd = headers.indexOf("bhdfrom");
    int iHeight = headers.indexOf("treeheight");
    int iSpecies = headers.indexOf("species");
    int iAge = headers.indexOf("age");
    if (iX==-1 || iY==-1 || iBhd==-1 || iSpecies==-1 || iHeight==-1 || iAge==-1)
        throw IException(QString("Initfile %1 is not valid!").arg(fileName));

    double dbh;
    for (int i=1;i<lines.count();i++) {
        QString &line = lines[i];
        dbh = line.section(sep, iBhd, iBhd).toDouble();
        if (dbh<5.)
            continue;
        Tree &tree = ru->newTree();
        QPointF f;
        if (iX>=0 && iY>=0) {
           f.setX( line.section(sep, iX, iX).toDouble() );
           f.setY( line.section(sep, iY, iY).toDouble() );
           f+=offset;
           tree.setPosition(f);
        }

        tree.setDbh(dbh);
        tree.setHeight(line.section(sep, iHeight, iHeight).toDouble()/100.); // convert from Picus-cm to m.
        tree.setAge(line.section(sep, iAge, iAge).toInt());
        int idx = picusSpeciesIds.indexOf(line.section(sep, iSpecies, iSpecies));
        QString speciesid="piab";
        if (idx>0)
            speciesid = iLandSpeciesIds[idx];
        Species *s = speciesSet->species(speciesid);
        if (!ru || !s)
            throw IException("Loading init-file: either ressource unit or species invalid.");

        tree.setRU(ru);
        tree.setSpecies(s);
        tree.setup();
    }
    //qDebug() << "loaded init-file contained" << lines.count() <<"lines.";
    //qDebug() << "lines: " << lines;

}
