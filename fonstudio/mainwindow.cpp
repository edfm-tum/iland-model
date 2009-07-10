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
#include "treespecies.h"
#include "tools/expression.h"
#include "tools/helper.h"

#include "paintarea.h"

// global settings
QDomDocument xmldoc;
QDomNode xmlparams;

// global Object pointers
LightRoom *lightroom = 0;
StampContainer *stamp_container=0;
StampContainer *reader_stamp_container=0;
QList<TreeSpecies*> tree_species;

double distance(const QPointF &a, const QPointF &b)
{
    return sqrt( (a.x()-b.x())*(a.x()-b.x()) + (a.y()-b.y())*(a.y()-b.y()) );
}

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

double nrandom(const float& p1, const float& p2)
{
    return p1 + (p2-p1)*(rand()/float(RAND_MAX));
}

void myMessageOutput(QtMsgType type, const char *msg)
 {
     switch (type) {
     case QtDebugMsg:
         MainWindow::logSpace()->appendPlainText(QString(msg));
         MainWindow::logSpace()->ensureCursorVisible();
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
     }


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
    mDomGrid=0;
    connect( ui->PaintWidget, SIGNAL(needsPainting(QPainter&)),
             this, SLOT(repaintArea(QPainter&)) );
    connect (ui->PaintWidget, SIGNAL(mouseClick(QPoint)),
             this, SLOT(mouseClick(const QPoint&)));
    connect(ui->PaintWidget, SIGNAL(mouseDrag(QPoint,QPoint)),
            this, SLOT(mouseDrag(const QPoint&, const QPoint &)));

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
            tree.setHeight(line.section(sep, iHeight, iHeight).toDouble()/100.); // convert from Picus-cm to m.

        tree.setSpecies( tree_species.first() ); // TODO: species specific!!!!
        tree.setup();

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

    // old code:
    /*
    mGrid->initialize(1.f); // set to unity...
    std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        (*tit).stampOnGrid(mStamp, *mGrid);
    } */
    mGrid->initialize(0.f); // set grid to zero.
    mDomGrid->initialize(0.f); // set dominance grid to zero.
    Tree::setGrid(mGrid, mDomGrid); // set static target grids
    Tree::resetStatistics();

    std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        (*tit).applyStamp(); // just do it ;)
    }
    //qDebug() << gridToString(*mGrid);
    qDebug() << "applied" << Tree::statPrints() << "stamps. tree #:" << mTrees.size();
    qDebug() << "dominance grid";
    qDebug() << gridToString(*mDomGrid);

    // read values...
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        (*tit).readStamp(); // just do it ;)
    }

    ui->PaintWidget->update();
    /*
    // debug output
    QString Line, Value;
    int ix,iy;
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
    */

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
    m_pixelpercell = sqsize / float(ccount);
    // get maximum value

    float maxval=0.;
    maxval = mGrid->max();
    if (maxval==0.)
        return;

    painter.fillRect(1,1,sqsize-2,sqsize-2, QColor("white"));
    int ix,iy;
    QColor fill_color;
    float value;

    QRectF cell(0,0,m_pixelpercell, m_pixelpercell);
    for (iy=0;iy<mGrid->sizeY();iy++) {
        for (ix=0;ix<mGrid->sizeX();ix++) {
            value = mGrid->valueAtIndex(QPoint(ix, iy));

            cell.moveTo(m_pixelpercell*ix, sqsize - m_pixelpercell*(iy+1));
            fill_color = Helper::colorFromValue(value, 0., maxval);
            painter.fillRect(cell, fill_color);
            //painter.drawRect(cell);

        }
    }
    Tree *t = (Tree*) ui->treeChange->property("tree").toInt();
    if (t) {
        QPointF pos = t->position();
        float pxpermeter = m_pixelpercell/ mGrid->cellsize();
        painter.setPen(Qt::black);
        painter.drawRect(int(pos.x()*pxpermeter-1), sqsize - int(pos.y()*pxpermeter-1), 3,3);
    }
    /*
    // paint trees...
    QPointF pos;
    float pxpermeter = pxpercell / mGrid->cellsize();

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
    } */

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

bool wantDrag=false;
void MainWindow::mouseClick(const QPoint& pos)
{
    wantDrag = false;
    ui->PaintWidget->setCursor(Qt::CrossCursor);
    ui->treeChange->setProperty("tree",0);
    if (!m_pixelpercell)
        return;
    int maxy = int( mGrid->sizeY()*m_pixelpercell );
    QPoint idx = QPoint(int (pos.x()/m_pixelpercell) , int((maxy-pos.y())/m_pixelpercell));
    QPointF coord = mGrid->cellCoordinates(idx);
    qDebug() << "click on" << pos << "are coords" << coord;
    // find adjactent tree

    std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        if(distance(tit->position(),coord)<2) {
            Tree *p = &(*tit);
            qDebug() << "found!" << tit->id() << "at" << tit->position()<<"value"<<p->impact();
            ui->treeChange->setProperty("tree", (int)p);
            ui->treeDbh->setValue(p->dbh());
            ui->treeHeight->setValue(p->height());
            wantDrag=true;
            ui->PaintWidget->setCursor(Qt::SizeAllCursor);
            ui->PaintWidget->update();
            break;
        }
   }

    /*std::vector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        if (tit->pxRect.contains(pos)) {
            qDebug() << "tree x/y:" << tit->position().x() << "/" << tit->position().y() << " impact-value: " << tit->impact();
            break;
        }
    }*/
}


void MainWindow::mouseDrag(const QPoint& from, const QPoint &to)
{
    ui->PaintWidget->setCursor(Qt::CrossCursor);
    if (!wantDrag)
        return;
    qDebug() << "drag from" << from << "to" << to;
    Tree *t = (Tree*) ui->treeChange->property("tree").toInt();
    if (!t)
        return;
    // calculate new position...
    int maxy = int( m_pixelpercell*mGrid->sizeY() );
    QPointF pos = QPointF(mGrid->cellsize()*to.x()/m_pixelpercell , mGrid->cellsize()*(maxy-to.y())/m_pixelpercell);
    t->setPosition(pos);
    on_stampTrees_clicked(); // restamp
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
    QString binaryStampFile =  xmlparams.firstChildElement("binaryStamp").text();
    QString binaryReaderStampFile =  xmlparams.firstChildElement("binaryReaderStamp").text();

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
        delete mDomGrid; mDomGrid=0;
    }
    mGrid = new FloatGrid(cellsize, cellcount, cellcount);
    mGrid->initialize(1.f); // set to unity...
    mDomGrid = new FloatGrid(cellsize*5, cellcount/5, cellcount/5);
    mDomGrid->initialize(0.f); // zero grid

    // setup of the binary stamps

    if (stamp_container) {
        delete stamp_container;
        delete reader_stamp_container;
    }

    stamp_container = new StampContainer();
    reader_stamp_container = new StampContainer();

    qDebug() << "loading stamps from" << binaryStampFile << "and readers from" << binaryReaderStampFile;
    QFile infile(binaryStampFile);
    infile.open(QIODevice::ReadOnly);
    QDataStream in(&infile);
    stamp_container->load(in);
    infile.close();

    QFile readerfile(binaryReaderStampFile);
    readerfile.open(QIODevice::ReadOnly);
    QDataStream rin(&readerfile);
    reader_stamp_container->load(rin);
    readerfile.close();

    qDebug() << "Loaded binary stamps from file. count:" << stamp_container->count();

    // Tree species...
    if (tree_species.isEmpty()) {
        TreeSpecies *ts = new TreeSpecies();
        tree_species.push_back(ts);
    }
    tree_species.first()->setStampContainer(stamp_container, reader_stamp_container); // start with the common single container

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
        tree.setSpecies( tree_species.first() );
        tree.setup();
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
    //int avg_cells = docElem.firstChildElement("lightroom").firstChildElement("size").attribute("average").toInt();

    double cut_threshold = docElem.firstChildElement("lightroom").firstChildElement("cutvalue").text().toDouble();
    qDebug() << "cutting stamps when averaged absoulte value of rings is below"<<cut_threshold;
    double crown, height, bhd;
    QString formula, name, result;

    LightRoomObject *lro = new LightRoomObject();
    lightroom->setLightRoomObject(lro);

    StampContainer readers;
    xmlparams = docElem.firstChildElement("params");
    QString binaryReaderStampFile =  xmlparams.firstChildElement("binaryReaderStamp").text();
    qDebug() << "reading binary stamp reader file" << binaryReaderStampFile;
    QFile infile(binaryReaderStampFile);
    infile.open(QIODevice::ReadOnly);
    QDataStream in(&infile);
    readers.load(in);
    infile.close();

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

        // averaged copy: FloatGrid gr = lightroom->result().averaged(avg_cells);
        const FloatGrid &gr = lightroom->result();
        // calculate sums...
        QVector<double> sums; // stores sum per ring (rectangle)
        QVector<double> rel_sum; // stores sum/px
        double sum;
        int ring_count;
        for (int o=0; o<gr.sizeX()/2; o++) {
            sum = 0; ring_count=0;
            // top and bottom
            for (int i=o; i<gr.sizeX()-o; i++) {
                sum += gr(i, o);
                sum += gr(i, gr.sizeX()-1-o);
                ring_count+=2;
            }
            // left, right :: do not calculate the edges two times....
            for (int i=o+1; i<gr.sizeX()-o-1; i++) {
                sum += gr(o, i);
                sum += gr(gr.sizeX()-1-o, i);
                ring_count+=2;
            }
            sums.push_back(sum);
            rel_sum.push_back(sum / double(ring_count) );
        }
        if (gr.sizeX()% 2) {
            sums.push_back(gr(gr.sizeX()/2, gr.sizeX()/2)); // center pixel for unevenly sized grids
            rel_sum.push_back(gr(gr.sizeX()/2, gr.sizeX()/2)); // center pixel for unevenly sized grids
        }
        int end_ring, target_grid_size;
        for (end_ring=0;end_ring<rel_sum.count();end_ring++)
            if (rel_sum[end_ring]>cut_threshold)
                break;
        end_ring = rel_sum.count() - end_ring; //
        target_grid_size = 2*end_ring - 1; // e.g. 3rd ring -> 5x5-matrix
        qDebug() << "break at ring" << end_ring;
        qDebug() << "circle;sum";
        for (int i=0;i<sums.count();i++)
            qDebug() << i << sums[i] << rel_sum[i];


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
        Stamp *stamp = stampFromGrid(gr, target_grid_size);
        if (!stamp)
            return;
        qDebug() << "created stamp with size (n x n)" << stamp->size() << "in an data-block of:" << stamp->dataSize();
        // process reading area
        double maxradius = lro->maxRadius();
        const Stamp *readerstamp = readers.readerStamp(maxradius);
        if (readerstamp) {
            int offset = stamp->offset() - readerstamp->offset();
            qDebug() << "fetching read-sum. offset stamp" << stamp->offset() << "offset readerstamp" << readerstamp->offset();
            double sum = 0;
            for (int x=0;x<readerstamp->size();x++)
                for (int y=0;y<readerstamp->size(); y++)
                    sum += *(readerstamp->data(x,y)) * *(stamp->data(x+offset, y+offset));
            qDebug() << "sum of reader-area over stamp" << sum;
            stamp->setReadSum(sum);
        } else qDebug() << "!!! no readerstamp available!!!";

        stamp->setDominanceValue( lightroom->centerValue() );
        double hd = qRound( height*100 / bhd );
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
    qDebug() << "current content of the container:";
    qDebug() << container.dump();
}

void MainWindow::on_lrLoadStamps_clicked()
{
    {
        QFile infile("E:\\Daten\\iLand\\Light\\fons\\stamps.bin");
        infile.open(QIODevice::ReadOnly);
        QDataStream in(&infile);
        StampContainer container;
        container.load(in);
        infile.close();
        qDebug() << "Dumping content of Stamp-container:";
        qDebug() << container.dump();
        // and the reader-stamps....
    }
    {
        QFile infile("E:\\Daten\\iLand\\Light\\fons\\readerstamp.bin");
        infile.open(QIODevice::ReadOnly);
        QDataStream in(&infile);
        StampContainer container;
        container.load(in);
        infile.close();
        qDebug() << "Dumping content of Reader-container:";
        qDebug() << container.dump();
        // and the reader-stamps....
    }

}

void MainWindow::on_treeChange_clicked()
{
    int pt = ui->treeChange->property("tree").toInt();
    if (!pt)
        return;
    Tree *t = (Tree*)pt;
    t->setDbh(    ui->treeDbh->value() );
    t->setHeight(ui->treeHeight->value() );
    t->setup();
    // changed, now recalc...
    on_stampTrees_clicked();
}

// create "reader" stamps....
void MainWindow::on_lrReadStamps_clicked()
{
    FloatGrid grid; // use copy ctor
    grid.setup(QRectF(-21., -21, 42, 42),2.);
    StampContainer container;
    int totcount=0;
    DebugTimer t;
    for (double radius=1.; radius<=8; radius+=0.1) {
        qDebug() << "creation of reader stamp for radius" << radius;
        grid.initialize(0.);
        float x,y;
        int tested=0, yes=0;
        for (x=-radius;x<=radius;x+=0.01)
            for (y=-radius;y<=radius;y+=0.01) {
                tested++;
                if ( x*x + y*y < radius*radius) {
                    grid.valueAt(x,y)++; yes++;
                }
            }
        qDebug() << "tested" << tested << "hit" << yes << "ratio" << yes/double(tested); // that should be pi, right?
        totcount+=tested;
        FloatGrid ngrid = grid.normalized(1.); // normalize with 1
        // create a stamp with a fitting size
        Stamp *stamp;
        int width;
        if (radius>=9.) {width=11;}
        else if (radius>=7.) { width=9; }
        else if (radius>=5.) { width=7; }
        else if (radius>=3.) { width=5; }
        else if (radius>=1.) { width=3; }
        stamp = stampFromGrid(ngrid,width);
        // save stamp
        container.addReaderStamp(stamp, radius);

    } // for (radius)
    qDebug() << "tested a total of" << totcount;
    QFile file("e:\\daten\\iland\\light\\fons\\readerstamp.bin");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);   // we will serialize the data into the file
    container.save(out);
    qDebug() << container.dump();


}
