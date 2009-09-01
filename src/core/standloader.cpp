#include "global.h"
#include "standloader.h"

#include "grid.h"
#include "model.h"
#include "ressourceunit.h"
#include "speciesset.h"

#include "helper.h"

#include <QtCore>

QStringList picusSpeciesIds = QStringList() << "0" << "17";
QStringList iLandSpeciesIds = QStringList() << "piab" << "fasy";




void StandLoader::processInit()
{
    GlobalSettings *g = GlobalSettings::instance();
    const XmlHelper &xml = g->settings();
    bool forEachCell = xml.hasNode("initialization.foreach");
    QString mode = xml.value("initialization.type", ""); // now only "picus"
    QString  fileName = xml.value("initialization.file", "");
    if (!QFile::exists(fileName))
        throw IException(QString("File %1 does not exist!").arg(fileName));

    Tree::resetStatistics();
    if (forEachCell) {
        loadFromPicus(fileName); // load in initial grid cell
        // we assume that all stands are equal, so wie simply COPY the trees and modify them afterwards
        const Grid<RessourceUnit*> &ruGrid=mModel->RUgrid();
        RessourceUnit **p = ruGrid.begin();
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
        return;
    }
    // only one stand:
    loadFromPicus(fileName);
}

void StandLoader::loadFromPicus(const QString &fileName, QPointF offset, RessourceUnit *ru)
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
    QStringList headers = lines[0].split(sep);
    //int iSpecies = headers.indexOf("species");
    //int iCount = headers.indexOf("count");
    int iX = headers.indexOf("x");
    int iY = headers.indexOf("y");
    int iBhd = headers.indexOf("bhdfrom");
    int iHeight = headers.indexOf("treeheight");
    int iSpecies = headers.indexOf("species");

    for (int i=1;i<lines.count();i++) {
        QString &line = lines[i];
        //qDebug() << "line" << i << ":" << line;
        Tree &tree = ru->newTree();
        QPointF f;
        if (iX>=0 && iY>=0) {
           f.setX( line.section(sep, iX, iX).toDouble() );
           f.setY( line.section(sep, iY, iY).toDouble() );
           f+=offset;
           tree.setPosition(f);
        }
        if (iBhd>=0)
            tree.setDbh(line.section(sep, iBhd, iBhd).toDouble());
        if (tree.dbh() < 5)
            continue; // 5cm: lower threshold for the moment
        if (iHeight>=0)
            tree.setHeight(line.section(sep, iHeight, iHeight).toDouble()/100.); // convert from Picus-cm to m.

        if (iSpecies>=0) {
            int idx = picusSpeciesIds.indexOf(line.section(sep, iSpecies, iSpecies));
            QString speciesid="piab";
            if (idx>0)
                speciesid = iLandSpeciesIds[idx];
            Species *s = speciesSet->species(speciesid);

            tree.setRU(ru);
            tree.setSpecies(s);
        }

        tree.setup();
    }
    //qDebug() << "loaded init-file contained" << lines.count() <<"lines.";
    //qDebug() << "lines: " << lines;

}
