#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTGui>
#include <QTXml>

#include <imagestamp.h>
#include "lightroom.h"
#include "core/grid.h"
#include "core/stamp.h"
#include "core/stampcontainer.h"
#include "tree.h"
#include "paintarea.h"
#include "tools/expression.h"
#include "tools/helper.h"

// global settings
QDomDocument xmldoc;
QDomNode xmlparams;

// global Object pointers
LightRoom *lightroom = 0;

QString setting(const QString& paramname)
{
    if (!xmlparams.isNull())
        return xmlparams.firstChildElement(paramname).text();
    else
        return "ERROR";
}

QDomElement getNode(const QString xmlkey)
{
    QDomElement docElem = xmldoc.documentElement(); // top element
    QStringList keys = xmlkey.split(".");
    QDomElement next = docElem;
    foreach(QString k, keys) {
        QDomElement next = next.firstChildElement(k);
        if (next.isNull()) {
            qDebug() << "XML key " << xmlkey << " not valid!";
            return docElem;
        }
    }
    return next;
}

QString gridToString(const FloatGrid &grid)
{
    QString res;
    for (int x=0;x<grid.sizeX();x++){
        for (int y=0;y<grid.sizeY();y++) {
            res+=QString::number(grid.constValueAtIndex(QPoint(x,y))) + ";";
        }
        res+="\r\n";
    }
    return res;
}
QImage gridToImage(const FloatGrid &grid,
                   bool black_white=false,
                   double min_value=0., double max_value=1.,
                   bool reverse=false)
{
    QImage res(grid.sizeX(), grid.sizeY(), QImage::Format_ARGB32);
    QRgb col;
    QColor qcol;
    int grey;
    double rval;
    for (int x=0;x<grid.sizeX();x++){
        for (int y=0;y<grid.sizeY();y++) {
            rval = grid.constValueAtIndex(QPoint(x,y));
            rval = std::max(min_value, rval);
            rval = std::min(max_value, rval);
            if (reverse) rval = max_value - rval;
            if (black_white) {
                grey = int(255*rval);
                col = QColor(grey,grey,grey).rgb();
            } else {
                col = QColor::fromHsvF(0.66666666666*rval, 0.95, 0.95).rgb();
            }
            res.setPixel(x,y,col);
            //res+=QString::number(grid.constValueAtIndex(QPoint(x,y))) + ";";
        }
        //res+="\r\n";
    }
    return res;
}

// Helper functions
//QString loadFromFile(const QString& fileName)
//{
//    QFile file(fileName);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
//         return "";
//
//     QTextStream s(&file);
//     return s.readAll();
//}
//
//bool writeToFile(const QString& fileName, const QString& content)
//{
//    QFile data(fileName);
//    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
//        QTextStream out(&data);
//        out << content;
//        return true;
//    }
//    return false;
//}

double nrandom(const float& p1, const float& p2)
{
    return p1 + (p2-p1)*(rand()/float(RAND_MAX));
}

void myMessageOutput(QtMsgType type, const char *msg)
 {
     /*switch (type) {
     case QtDebugMsg:
         fprintf(stderr, "Debug: %s\n", msg);
         break;
     case QtWarningMsg:
         fprintf(stderr, "Warning: %s\n", msg);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         abort();
     }*/

     MainWindow::logSpace()->appendPlainText(QString(msg));
     MainWindow::logSpace()->ensureCursorVisible();
 }

QPlainTextEdit *MainWindow::mLogSpace=NULL;

QPlainTextEdit* MainWindow::logSpace()
{
   return mLogSpace;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindowClass)
{
    ui->setupUi(this);
    mGrid=0;
    connect( ui->PaintWidget, SIGNAL(needsPainting(QPainter&)),
             this, SLOT(repaintArea(QPainter&)) );
    connect (ui->PaintWidget, SIGNAL(mouseClick(QPoint)),
             this, SLOT(mouseClick(const QPoint&)));
    //connect(&a, SIGNAL(valueChanged(int)),
    //                  &b, SLOT(setValue(int)));
    mLogSpace = ui->logOutput;
    qInstallMsgHandler(myMessageOutput);
    // load xml file
    xmldoc.clear();
    QString xmlFile = Helper::loadTextFile(ui->initFileName->text());

    ui->iniEdit->setPlainText(xmlFile);

    if (!xmldoc.setContent(xmlFile)) {
        QMessageBox::information(this, "title text", "Cannot set content of XML file " + ui->initFileName->text());
        return;
    }

    on_actionEdit_XML_settings_triggered();


}

MainWindow::~MainWindow()
{
    delete ui;
    if (lightroom)
        delete lightroom;
}

void MainWindow::on_applyXML_clicked()
{
    xmldoc.clear();
    // load for edit
    QString xml = ui->iniEdit->toPlainText();
    QString errMsg;
    int errLine, errCol;
    if (!xmldoc.setContent(xml, &errMsg, &errLine, &errCol)) {
        QMessageBox::information(this, "Error applying XML",
                                 QString("Error applying xml line %1, col %2.\nMessage: %3").arg(errLine).arg(errCol).arg(errMsg));
        return;
    }
}

/// load a Picus ini file formatted file.
void MainWindow::loadPicusIniFile(const QString &fileName)
{
    QString text = Helper::loadTextFile(fileName);

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
    for (int i=1;i<lines.count();i++) {
        QString &line = lines[i];
        qDebug() << "line" << i << ":" << line;
        Tree tree;
        QPointF f;
        if (iX>=0 && iY>=0) {
           f.setX( line.section(sep, iX, iX).toDouble() );
           f.setY( line.section(sep, iY, iY).toDouble() );
           tree.setPosition(f);
        }
        if (iBhd>=0)
            tree.setDbh(line.section(sep, iBhd, iBhd).toDouble());
        if (iHeight>=0)
            tree.setHeight(line.section(sep, iHeight, iHeight).toDouble());
        mTrees.push_back(tree);
    }
    qDebug() << "lines: " << lines;
}

void MainWindow::on_saveFile_clicked()
{
    QString content = ui->iniEdit->toPlainText();
    if (!content.isEmpty())
         Helper::saveToTextFile(ui->initFileName->text(), content);

}

//float dist_and_direction(const QPointF &PStart, const QPointF &PEnd, float &rAngle)
//{
//    float dx = PEnd.x() - PStart.x();
//    float dy = PEnd.y() - PStart.y();
//    float d = sqrt(dx*dx + dy*dy);
//    // direction:
//    rAngle = atan2(dx, dy);
//    return d;
//}
/**************************************
**
***************************************/
void MainWindow::on_stampTrees_clicked()
{
    if (mTrees.size()==0 || !mGrid)
        return;
    int ix,iy;
    //QRect posRect;
    //QPoint cell;
    //QPointF cellcoord;
    mGrid->initialize(1.f); // set to unity...
    std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        (*tit).stampOnGrid(mStamp, *mGrid);
    }

    // debug output
    QString Line, Value;
    for (iy=0;iy<mGrid->sizeY();iy++) {
        Line="";
        for (ix=0;ix<mGrid->sizeX();ix++) {
            Line = QString("%1 %2").arg(Line).arg(mGrid->valueAtIndex(QPoint(ix, iy)));
            //Value.setNum(mGrid->valueAtIndex(QPoint(ix, iy)));
            //Line+=Value;
        }
        qDebug() << "y" << iy << ":" << Line;
    }
    on_pbRetrieve_clicked();
    //ui->PaintWidget->update();

}

/**************************************
**
***************************************/
void MainWindow::on_pbRetrieve_clicked()
{
    if (mTrees.size()==0 || !mGrid)
        return;

    std::vector<Tree>::iterator tit;
    float treevalue;
    float sum_bhd=0.f;
    float sum_impact=0.f;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        treevalue = (*tit).retrieveValue(mStamp, *mGrid);
        sum_bhd+=tit->dbh();
        sum_impact+=tit->impact();
        //qDebug() << "tree x/y:" << tit->position().x() << "/" << tit->position().y() << " value: " << tit->impact();
    }
    qDebug() << "N="<<mTrees.size()<<"avg.dbh="<<sum_bhd/float(mTrees.size())<<"avg.value="<<sum_impact/float(mTrees.size());
    ui->PaintWidget->update();
}

void MainWindow::paintFON(QPainter &painter, QRect rect)
{
    // do the actual painting
    if (!mGrid)
        return;
    // draw grid...

    int sqsize = qMin(ui->PaintWidget->width(), ui->PaintWidget->height())-1;
    int ccount = mGrid->sizeX();
    float pxpercell = sqsize / float(ccount);
    float pxpermeter = pxpercell / mGrid->cellsize();
    // get maximum value
    float maxval=0.;
    for( float *p = mGrid->begin();p!=mGrid->end(); ++p)
        maxval=qMax(maxval, *p);

    if (maxval==0.)
        return;

    painter.fillRect(1,1,sqsize-2,sqsize-2, QColor("white"));
    int ix,iy;
    QColor fill_color;
    float value;
    QRectF cell(0,0,pxpercell, pxpercell);
    for (iy=0;iy<mGrid->sizeY();iy++) {
        for (ix=0;ix<mGrid->sizeX();ix++) {
            value = mGrid->valueAtIndex(QPoint(ix, iy));
            if (value<1.) {
                cell.moveTo(pxpercell*ix, pxpercell*iy);
                // scale color in hsv from 0..240
                fill_color.setHsv( int(value*240./maxval), 200, 200);
                painter.fillRect(cell, fill_color);
                //painter.drawRect(cell);
            }
        }
    }

    // paint trees...
    QPointF pos;
    std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        pos = (*tit).position();
        fill_color.setHsv(int((*tit).impact()*240.), 200, 200);
        painter.drawRect(int(pxpermeter*pos.x()-3), int(pxpermeter*pos.y()-3), 7, 7);
        painter.fillRect(int(pxpermeter*pos.x()-2), int(pxpermeter*pos.y()-2), 4, 4, fill_color);
        tit->pxRect = QRect(int(pxpermeter*pos.x()-3), int(pxpermeter*pos.y()-3), 7, 7);
        float r = int(tit->impactRadius() * pxpermeter);

        painter.setPen(Qt::black);
        painter.drawEllipse(QPointF(pxpermeter*pos.x(),pxpermeter*pos.y()),r,r);
    }

    qDebug() << "repaintArea. maxval:" << maxval;

}

void MainWindow::repaintArea(QPainter &painter)
{
    // select drawing type...
    switch (m_gfxtype) {
        case 0: // paint FON cells
            paintFON(painter, ui->PaintWidget->rect()); break;
        case 1:  // paint Lightroom - studio --- painting is done on the background image of PaintArea
            break;
        case 2: // paint Lightroom
        default: break; // no painting
    }
}

void MainWindow::mouseClick(const QPoint& pos)
{
    //qDebug() << "click on" << pos;
    std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        if (tit->pxRect.contains(pos)) {
            qDebug() << "tree x/y:" << tit->position().x() << "/" << tit->position().y() << " impact-value: " << tit->impact();
            break;
        }
    }
}





void MainWindow::on_calcFormula_clicked()
{
    QString expression = ui->formula->text();
    Expression expr(expression);
    // add vars to expression
    double *v1 = expr.addVar("x");
    double *v2 = expr.addVar("y");

    // set current values
    *v1 = ui->lVar1->text().toDouble();
    *v2 = ui->lVar2->text().toDouble();

    double result = expr.execute();

    ui->lCalcResult->setText(QString("%1").arg(result));
}



void MainWindow::on_lCalcResult_linkActivated(QString link)
{
    qDebug() << "link activated:" << link;
}

void MainWindow::on_pbAddTrees_clicked()
{
    // add a number of trees
    int n = ui->eTreeCount->text().toInt();
    float dbh = ui->eTreeDBH->text().toFloat();
    for (int i=0;i<n; i++) {
        Tree t;
        t.setDbh(nrandom(dbh-2., dbh+2));
        t.setHeight(t.dbh());
        QPointF p(nrandom(0.,mGrid->metricSizeX()), nrandom(0., mGrid->metricSizeY()));
        t.setPosition(p);
        mTrees.push_back(t);
    }

    on_stampTrees_clicked();

}


void MainWindow::stampTrees()
{
    mGrid->initialize(1.f); // set to unity...
    std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        (*tit).stampOnGrid(mStamp, *mGrid);
    }
}

double MainWindow::retrieveFon()
{
    std::vector<Tree>::iterator tit;
    float treevalue;
    float sum_bhd=0.f;
    float sum_impact=0.f;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        treevalue = (*tit).retrieveValue(mStamp, *mGrid);
        sum_bhd+=tit->dbh();
        sum_impact+=tit->impact();
        //qDebug() << "tree x/y:" << tit->position().x() << "/" << tit->position().y() << " value: " << tit->impact();
    }
    qDebug() << "N="<<mTrees.size()<<"avg.dbh="<<sum_bhd/float(mTrees.size())<<"avg.value="<<sum_impact/float(mTrees.size());
    return sum_impact/float(mTrees.size());

}

void MainWindow::addTrees(const double dbh, const int count)
{
    // add a number of trees
    for (int i=0;i<count; i++) {
        Tree t;
        t.setDbh(nrandom(dbh-2., dbh+2));
        t.setHeight(t.dbh());
        QPointF p(nrandom(0.,mGrid->metricSizeX()), nrandom(0., mGrid->metricSizeY()));
        t.setPosition(p);
        mTrees.push_back(t);
    }
}

void MainWindow::on_calcMatrix_clicked()
{
    QStringList dbhs = setting("seriesDbh").split(" ");
    QStringList ns = setting("seriesStemnumber").split(" ");
    QStringList result, line;
    result.append("result;" + dbhs.join(";"));
    int stemCount, dbh;
    double val;
    QTime timer;
    timer.start();
    for (int n=0; n<ns.count(); ++n) {
        line.clear(); line.append(ns[n]);
        for (int d=0; d<dbhs.count(); ++d) {
            stemCount = ns[n].toInt();
            dbh = dbhs.at(d).toInt();
            // clear trees
            mTrees.clear();
            addTrees(dbh, stemCount);
            // stamp
            stampTrees();
            val = retrieveFon();
            line.append( QString::number(val) );
            qDebug() << "dbh" << dbh << "n" << stemCount << "result" << val;
        }
        result.append(line.join(";"));
        QApplication::processEvents();
    }
    // calculate iso-fones
    double isoresult;

    float n_min, n_max, below_val;
    QStringList isofones = setting("seriesIsofones").split(" ");
    QStringList isoresults;
    isoresults.append("isofon;" + dbhs.join(";") );
    for (int i=0; i<isofones.count(); ++i) {
        double isofon = isofones[i].toFloat();
        isoresult=0;
        line.clear(); line.append(isofones[i]);
        for (int d=0; d<dbhs.count(); d++) {
            isoresult = result[ns.count()-1].section(';',0,0).toFloat(); // in case no result found (greter than maximum)
            for (int n=0; n<ns.count(); ++n) {
                val = result[n+1].section(';',d+1, d+1).toFloat();
                if (val < isofon && n<ns.count()-1 ) {
                    continue;
                }
                if (n==0) {
                    // class break before --> use min
                    isoresult=result[1].section(';',0,0).toFloat();
                }  else {
                    // break in between...
                    n_min = result[n].section(';',0,0).toFloat();
                    n_max = result[n+1].section(';',0,0).toFloat();
                    below_val = result[n].section(';',d+1,d+1).toFloat();
                    // linear interpolation
                    isoresult = n_max + (isofon-val)/(val - below_val) * (n_max - n_min);
                }
                break;
            } // stemnumbers
            line.append(QString::number(isoresult) );
        } // dbhs
        isoresults.append(line.join(";") );
    } // isoresults
    result.append("");
    result.append(isoresults);
    qDebug() << "finished, elapsed " << timer.elapsed();
    QApplication::clipboard()->setText(result.join("\n"));

}

void MainWindow::on_actionLightroom_triggered()
{
    ui->editStack->setCurrentIndex(1);
    ui->headerCaption->setText("Lightroom");
    m_gfxtype = 1;
    ui->PaintWidget->update();
}

void MainWindow::on_actionEdit_XML_settings_triggered()
{
    ui->editStack->setCurrentIndex(0);
    ui->headerCaption->setText("Edit XML file");
    m_gfxtype = -1;
    ui->PaintWidget->update();
}

void MainWindow::on_actionFON_action_triggered()
{
    ui->editStack->setCurrentIndex(2);
    ui->headerCaption->setText("Field Of Neighborhood test environment");
    m_gfxtype = 0;
    ui->PaintWidget->update();

}

void MainWindow::on_pbCreateLightroom_clicked()
{
    if (xmldoc.isNull()) MSGRETURN("XML not loaded");
    QDomElement docElem = xmldoc.documentElement(); // top element
    QDomElement docLightroom = docElem.firstChildElement("lightroom");
    if (docLightroom.isNull()) MSGRETURN("cant find node 'lightroom' in xml");
    int x,y,z;
    x = docLightroom.firstChildElement("size").attribute("x").toInt();
    y = docLightroom.firstChildElement("size").attribute("y").toInt();
    z = docLightroom.firstChildElement("size").attribute("z").toInt();
    double cellsize = docLightroom.firstChildElement("size").attribute("cellsize").toDouble();

    int hemisize = docLightroom.firstChildElement("hemigrid").attribute("size").toInt();
    double lat = docLightroom.firstChildElement("hemigrid").attribute("latitude").toFloat();
    double diffus = docLightroom.firstChildElement("hemigrid").attribute("diffus").toFloat();

    // create a lightroom object...
    if (!lightroom)
        lightroom = new LightRoom();

    lightroom->setup(x,y,z,cellsize,
                    hemisize,lat,diffus);
    LightRoomObject *lro = new LightRoomObject();
    lro->setuptree(40., 20., "5*(1-x*x)");
    lightroom->setLightRoomObject(lro);

    qDebug() << "Lightroom setup complete";
}

void MainWindow::on_testLRO_clicked()
{
        // setup a lightroom object, and do some tests...
    double x = ui->lr_x->text().toDouble();
    double y = ui->lr_y->text().toDouble();
    double z = ui->lr_z->text().toDouble();
    double azi = ui->lrAzim->text().toDouble();
    double elev = ui->lrElevation->text().toDouble();

    LightRoomObject lro;
    lro.setuptree(40., 10., "5*(1-x*x)");
    qDebug()<<"x:"<<x<<"y:"<<y<<"z:"<<z<<"azimuth:"<<azi<<"elevation:"<<elev << lro.hittest(x,y,z,RAD(azi), RAD(elev));
    //qDebug()<<"-10,-10,0 - azimuth 42, elev: 45:" << lro.hittest(-10,-10,0,RAD(42), RAD(45));
}

void MainWindow::on_lroTestHemi_clicked()
{
    double x = double(ui->lrSliderX->value());// ui->lr_x->text().toDouble();
    double y = ui->lrSliderY->value();
    double z = ui->lrSliderZ->value();
    ui->lr_x->setText(QString::number(x));
    ui->lr_y->setText(QString::number(y));
    ui->lr_z->setText(QString::number(z));
    if (!lightroom)
        MSGRETURN("Lightroom NULL!");
    DebugTimer t("single point");
    double res = lightroom->calculateGridAtPoint(x,y,z);
    ui->lrHemiValue->setText(QString::number(res));
    // now paint...
    //ui->PaintWidget->drawImage();
    lightroom->shadowGrid().paintGrid(ui->PaintWidget->drawImage());
    ui->PaintWidget->update(); // repaint
    //qDebug() << lightroom->shadowGrid().dumpGrid();

}
void MainWindow::on_lrLightGrid_clicked()
{
    lightroom->solarGrid().paintGrid(ui->PaintWidget->drawImage());
    ui->PaintWidget->update(); // repaint
    qDebug() << lightroom->solarGrid().dumpGrid();
}

void MainWindow::on_lrCalcFullGrid_clicked()
{
    if (!lightroom)
        MSGRETURN("Lightroom NULL!");
    lightroom->calculateFullGrid();
    float maxvalue = lightroom->result().max();
    qDebug() << "maxvalue" << maxvalue;
    const FloatGrid &result = lightroom->result();
    QString res;
    for (int x=0;x<result.sizeX();x++){
        for (int y=0;y<result.sizeY();y++) {
            res+=QString::number(result.constValueAtIndex(QPoint(x,y))) + ";";
        }
        res+="\n";
    }
    qDebug()<< res;
}



void MainWindow::on_fonRun_clicked()
{
    // print out the element names of all elements that are direct children
    // of the outermost element.
    QDomElement docElem = xmldoc.documentElement();

    xmlparams = docElem.firstChildElement("params");
    QDomNode trees = docElem.firstChildElement("trees");
    // get a parameter...
    //int p1 = params.firstChildElement("param1").text().toInt();
    QString stampFile =  xmlparams.firstChildElement("stampFile").text();


    // Here we append a new element to the end of the document
    //QDomElement elem = doc.createElement("img");
    //elem.setAttribute("src", "myimage.png");
    //docElem.appendChild(elem);

    // Load stamp
    if (!mStamp.load(stampFile)) {
        QMessageBox::information(NULL, "info", "image not found!");
        return;

    }

    float phi=0.f;
    for (float r=0.; r<1.1; r+=0.1f) {
        qDebug() << "r: " << r << " phi: " << phi << " col: " << mStamp.get(r, phi);
        phi+=0.1;
        qDebug() << "r: " << r << " phi: " << phi << " col: " << mStamp.get(r, phi);
    }

    // atan2-test
    //qDebug() << "1/1:" << atan2(1., 1.) << "-1/1:" << atan2(1., -1.) << "-1/-1" << atan2(-1., -1.) << "1/-1:" << atan2(-1., 1.);

    // expression test
    QString expr_text=xmlparams.firstChildElement("expression").text();
    Expression expr(expr_text);
    double value = expr.execute();
    qDebug() << "expression:" << expr_text << "->" <<value;

    QString expr_hScale=xmlparams.firstChildElement("hScale").text();
    Tree::hScale.setExpression(expr_hScale);
    Tree::hScale.addVar("height");
    Tree::hScale.addVar("dbh");

    QString expr_rScale=xmlparams.firstChildElement("rScale").text();
    Tree::rScale.setExpression(expr_rScale);
    Tree::rScale.addVar("height");
    Tree::rScale.addVar("dbh");

    // setup grid
    int cellsize = xmlparams.firstChildElement("cellSize").text().toInt();
    int cellcount = xmlparams.firstChildElement("cells").text().toInt();
    if (mGrid) {
        delete mGrid; mGrid=0;
    }
    mGrid = new FloatGrid(cellsize, cellcount, cellcount);
    mGrid->initialize(1.f); // set to unity...

    // Load Trees
    mTrees.clear();
    QDomElement treelist = docElem.firstChildElement("treeinit");
    if (!treelist.isNull()) {
        QString fname = treelist.text();
        loadPicusIniFile(fname);

    }
    QDomElement n = trees.firstChildElement("tree");
    while (!n.isNull()) {
        Tree tree;
        tree.setDbh( n.attributeNode("dbh").value().toFloat() );
        tree.setHeight( n.attributeNode("height").value().toFloat() );
        tree.setPosition( QPointF(n.attributeNode("x").value().toFloat(), n.attributeNode("y").value().toFloat() ));
        mTrees.push_back(tree);

/*        ui->logMessages->append("tree: " \
           " dbh: " + n.attributeNode("dbh").value() +
           "\nheight: " + n.attributeNode("height").value() );*/
        n = n.nextSiblingElement();
    }
    qDebug() << mTrees.size() << "trees loaded.";
    // test stylesheets...
    QString style = setting("style");
    ui->PaintWidget->setStyleSheet( style );

}

void MainWindow::on_lrProcess_clicked()
{
    QDomElement docElem = xmldoc.documentElement();
    QDomElement trees = docElem.firstChildElement("lightroom").firstChildElement("trees");
            //getNode("lightroom.trees");
    QString path =  docElem.firstChildElement("lightroom").firstChildElement("trees").firstChildElement("path").text();
    qDebug() << "store to " << path;
    QDomElement tree = docElem.firstChildElement("lightroom").firstChildElement("trees").firstChildElement("tree");
    double crown, height, bhd;
    QString formula, name, result;

    LightRoomObject *lro = new LightRoomObject();
    lightroom->setLightRoomObject(lro);

    StampContainer container;
    container.useLookup(false); // disable lookup table (not necessary for creation)

    while (!tree.isNull()) {
        name = tree.attribute("name");
        height = tree.attribute("h").toDouble();
        crown = tree.attribute("crown").toDouble();
        bhd = tree.attribute("bhd").toDouble();
        formula = tree.attribute("formula");
        ///////////////////////////
        lro->setuptree(height, crown, formula);
        qDebug() << "start" << name;
        lightroom->calculateFullGrid();
        // store
        result = gridToString( lightroom->result() );
        Helper::saveToTextFile(path + "\\ " + name + ".txt", result);
        QImage img = gridToImage( lightroom->result() );
        img.save(path + "\\ " + name + ".jpg", "JPG", 100);
        // test: use subpixel averages ....
        /*
        FloatGrid gr3x3 = lightroom->result().averaged(3);
        QImage img3x3 = gridToImage( gr3x3 );
        img3x3.save(path + "\\ " + name + "_3x3.jpg", "JPG", 100);
        Helper::saveToTextFile(path + "\\ " + name + "_3x3.txt", gridToString(gr3x3));
        result="";
        for (int x=0;x<3;x++)
            for (int y=0;y<3;y++) {
            FloatGrid gr3x3 = lightroom->result().averaged(3,x,y);
            result+="\r\n" + gridToString(gr3x3);

        }
        Helper::saveToTextFile(QString("%1\\%2_shift.txt").arg(path, name), result); */

        // store to container
        Stamp *stamp = stampFromGrid(lightroom->result(), 23);
        double hd = qRound( height*100 / hd );
        container.addStamp(stamp,bhd, hd);
        ///////////////////////////
        tree = tree.nextSiblingElement("tree");
    }
    qDebug() << "finished!!!";
    // write container to a file....
    QFile file(path + "\\stamps.bin");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // we will serialize the data into the file
    container.save(out);
    file.close();

}
