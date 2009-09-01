#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTGui>
#include <QTXml>

#include <imagestamp.h>
#include "lightroom.h"

#include "model.h"
#include "standloader.h"
#include "stampcontainer.h"
#include "ressourceunit.h"
#include "speciesset.h"
#include "tree.h"

#include "exception.h"

#include "paintarea.h"


// global settings
QDomDocument xmldoc;
QDomNode xmlparams;

// global Object pointers
LightRoom *lightroom = 0;
StampContainer *stamp_container=0;
StampContainer *reader_stamp_container=0;
QList<Species*> tree_species;

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
         MainWindow::logSpace()->appendPlainText(QString("WARNING: %1").arg(msg));
         MainWindow::logSpace()->ensureCursorVisible();
         fprintf(stderr, "WARNING: %s\n", msg);
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
    mModel = 0;
    connect( ui->PaintWidget, SIGNAL(needsPainting(QPainter&)),
             this, SLOT(repaintArea(QPainter&)) );
    connect (ui->PaintWidget, SIGNAL(mouseClick(QPoint)),
             this, SLOT(mouseClick(const QPoint&)));
    connect(ui->PaintWidget, SIGNAL(mouseDrag(QPoint,QPoint,Qt::MouseButton)),
            this, SLOT(mouseDrag(const QPoint&, const QPoint &, const Qt::MouseButton)));
    connect(ui->PaintWidget, SIGNAL(mouseMove(QPoint)),
            this, SLOT(mouseMove(const QPoint&)));
    connect(ui->PaintWidget, SIGNAL(mouseWheel(QPoint, int)),
            this, SLOT(mouseWheel(const QPoint&, int)));


    ui->pbResults->setMenu(ui->menuOutput_menu);

    mLogSpace = ui->logOutput;
    qInstallMsgHandler(myMessageOutput);
    // load xml file
    xmldoc.clear();
    QString argText = QApplication::arguments().last();
    if (!argText.isEmpty())
        ui->initFileName->setText(argText);

    QString xmlFile = Helper::loadTextFile(ui->initFileName->text());

    if (!xmlFile.isEmpty()) {
        ui->iniEdit->setPlainText(xmlFile);

        if (!xmldoc.setContent(xmlFile)) {
            QMessageBox::information(this, "title text", "Cannot set content of XML file " + ui->initFileName->text());
            return;
        }
    }

    on_actionEdit_XML_settings_triggered();
    // update

    qDebug() << "threadcount: " << QThread::idealThreadCount();

}

MainWindow::~MainWindow()
{
    delete ui;
    if (lightroom)
        delete lightroom;
    if (mModel)
        delete mModel;
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

    Globals->loadSettingsMetaDataFromXml(xmldoc.documentElement().firstChildElement("globalsettings"));

}


void MainWindow::on_saveFile_clicked()
{
    QString content = ui->iniEdit->toPlainText();
    if (!content.isEmpty())
         Helper::saveToTextFile(ui->initFileName->text(), content);

}


void MainWindow::readwriteCycle()
{

    if (!mModel || !mModel->ru())
        return;
    mModel->runYear();

    /*
    RessourceUnit *ru = mModel->ru();
    QVector<Tree> &mTrees = ru->trees();
    if (mTrees.size()==0 || !mGrid)
        return;

    //mGrid->initialize(0.f); // set grid to zero.
    DebugTimer t_print("apply/read-cycle");
    mDomGrid->initialize(0.f); // set dominance grid to zero.
    mGrid->initialize(1.f); // 1 for multiplicative
    //mDomGrid->initialize(1000.); //0: "active",  1000: no influence (trees are always below this height)
    Tree::setGrid(mGrid, mDomGrid); // set static target grids
    Tree::resetStatistics();

    QVector<Tree>::iterator tit;

    DebugTimer t2;
    // height dominance grid
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        (*tit).heightGrid(); // just do it ;)
    }
    t2.interval("height grid");

    // grid for horizontal concurrence
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        (*tit).applyStamp(); // just do it ;)
    }
    t2.interval("apply stamp");
    //qDebug() << gridToString(*mGrid);
    qDebug() << "applied" << Tree::statPrints() << "stamps. (no. of trees):" << mTrees.size();
    //qDebug() << gridToString(*mDomGrid);

    // read values...
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        //(*tit).readStamp(); // just do it ;)
        (*tit).readStampMul(); // multipliactive approach
    }
    t2.interval("read stamp");
    t_print.showElapsed();

    //qDebug() << Tree::statAboveZ() << "above dominance grid";
    */
    ui->PaintWidget->update(); // repaint...

}

void MainWindow::applyCycles(int cycle_count)
{
    if (!mModel || !mModel->ru() || mModel->ru()->trees().size()==0)
        return;
    RessourceUnit *ru = mModel->ru();
    QVector<Tree> &mTrees = ru->trees();
    // (1) init grid
    DebugTimer t2(QString("application of %1 cycles").arg(cycle_count));
    int i;
    QVector<Tree>::iterator tit;
    {
        DebugTimer t("initialize");
        for (i=0;i<cycle_count;i++) {
            mDomGrid->initialize(0.f); // set dominance grid to zero.
            mGrid->initialize(1.f); // 1 for multiplicative
        }
    }

    {
        DebugTimer t("dominance grid");
        for (i=0;i<cycle_count;i++) {
            // height dominance grid
            for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
                (*tit).heightGrid(); // just do it ;)
            }
        }
    }

    Tree::setGrid(mGrid, mDomGrid); // set static target grids
    Tree::resetStatistics();
    {
        DebugTimer t("apply grid");
        for (i=0;i<cycle_count;i++) {
            // height dominance grid
            if (cycle_count>1 && i==cycle_count-1)
                mGrid->initialize(1.f); // reset before the ultimate application

            for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
                (*tit).applyStamp(); // just do it ;)
            }
        }
    }

    {
        DebugTimer t("read grid");
        for (i=0;i<cycle_count;i++) {
            for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
                //(*tit).readStamp(); // just do it ;)
                (*tit).readStampMul(); // multipliactive approach
            }
        }
    }
    qDebug() << "ms/cycle (avg):" << t2.elapsed() / double(cycle_count) << "total time" << t2.elapsed();
}

void MainWindow::on_stampTrees_clicked()
{
    readwriteCycle();
}

QString MainWindow::dumpTreelist()
{
    if (!mModel || !mModel->ru() || mModel->ru()->trees().size()==0)
        return "";
    RessourceUnit *ru = mModel->ru();
    QVector<Tree> &mTrees = ru->trees();
    QStringList result;
    result+=QString("id;x;y;dbh;height;fonvalue");
    QVector<Tree>::iterator tit;
    Tree *tree;

    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        tree = &(*tit);
        result+=QString("%1;%2;%3;%4;%5;%6")
                .arg(tree->id())
                .arg(tree->position().x())
                .arg(tree->position().y())
                .arg(tree->dbh())
                .arg(tree->height())
                .arg(tree->lightRessourceIndex());
    }
    QString resStr = result.join("\n");
    return resStr;
}


void MainWindow::paintFON(QPainter &painter, QRect rect)
{
    DebugTimer drawtimer("painting");
    if (!mModel || !mModel->ru())
        return;


    // do the actual painting
    if (!mGrid)
        return;
    bool auto_scale_color = ui->visAutoScale->isChecked();
    bool show_fon = ui->visFon->isChecked();
    bool show_dom = ui->visDomGrid->isChecked();
    bool show_impact = ui->visImpact->isChecked();
    bool show_scale40 = ui->visDomHeight->isChecked();
    float scale_value = ui->visScaleValue->currentText().toFloat();

    //    // get maximum value

    float maxval=1.f; // default maximum
    if (!auto_scale_color)
        maxval = mGrid->max();
    if (maxval==0.)
        return;
    // clear background
    painter.fillRect(ui->PaintWidget->rect(), Qt::white);
    // draw rectangle around the grid
    QRectF r = mGrid->metricRect();
    QRect rs = vp.toScreen(r);
    painter.setPen(Qt::black);
    painter.drawRect(rs);

    int ix,iy;
    QColor fill_color;
    float value;

    if (show_fon ) { // !=0: no sense!
        // start from each pixel and query value in grid for the pixel
        int x,y;
        int sizex = rect.width();
        int sizey = rect.height();
        QPointF world;
        QRgb col;
        QImage &img = ui->PaintWidget->drawImage();
        for (x=0;x<sizex;x++)
            for (y=0;y<sizey;y++) {
                world = vp.toWorld(QPoint(x,y));
                if (mGrid->coordValid(world)) {
                   value = mGrid->valueAt(world);
                   col = Helper::colorFromValue(value, 0., maxval, true).rgb();
                   img.setPixel(x,y,col);
               }
           }

    } else {
        // for rather small grids, the inverse tactic (draw each pixel!)
        if (show_fon) {

            QRectF cell(0,0,m_pixelpercell, m_pixelpercell);
            QPointF k;
            float hdom;
            for (iy=0;iy<mGrid->sizeY();iy++) {
                for (ix=0;ix<mGrid->sizeX();ix++) {

                    value = mGrid->valueAtIndex(QPoint(ix, iy));
                    if (show_scale40) {
                        k = mGrid->cellCenterPoint(QPoint(ix, iy));
                        hdom = qMax(2.f, mDomGrid->valueAt(k)); // 2m: as in stamp grid
                        value = 1.f -  (1.f - value) * hdom / scale_value;
                    }
                    QRectF f = mGrid->cellRect(QPoint(ix,iy));
                    QRect r = vp.toScreen(f);

                    fill_color = Helper::colorFromValue(value, 0., maxval, true);
                    painter.fillRect(r, fill_color);

                }
            }
        } // if (show_fon)

        if (show_dom) {
            // paint the lower-res-grid;

            for (iy=0;iy<mDomGrid->sizeY();iy++) {
                for (ix=0;ix<mDomGrid->sizeX();ix++) {
                    QPoint p(ix,iy);
                    value = mDomGrid->valueAtIndex(p);
                    QRect r = vp.toScreen(mDomGrid->cellRect(p));
                    fill_color = Helper::colorFromValue(value, 0., 50.); // 0..50m
                    painter.fillRect(r, fill_color);
                }
            }

        } // if (show_dem)
    }
    if (show_impact) {
        AllTreeIterator treelist(mModel);
        Tree *tree;
        while (tree = treelist.next()) {
            if ( !vp.isVisible(treelist.currentRU()->boundingBox()) ) {
                continue;
            }
            QPointF pos = tree->position();
            QPoint p = vp.toScreen(pos);
            value = tree->lightRessourceIndex();
            fill_color = Helper::colorFromValue(value, 0., 1., true);
            painter.setBrush(fill_color);
            int diameter = qMax(1,vp.meterToPixel( tree->dbh()/100. * 5.));
            painter.drawEllipse(p, diameter, diameter);
        }

    } // if (show_impact)
    // show selected tree
    Tree *t = (Tree*) ui->treeChange->property("tree").toInt();
    if (t) {
        QPointF pos = t->position();
        painter.setPen(Qt::black);
        QPoint p = vp.toScreen(pos);
        painter.drawRect( p.x()-1, p.y()-1, 3,3);
    }
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
    // fix viewpoint
    vp.setScreenRect(ui->PaintWidget->rect());
}

void MainWindow::on_visFon_toggled() { ui->PaintWidget->update(); }
void MainWindow::on_visDomGrid_toggled() { on_visFon_toggled(); }
void MainWindow::on_visImpact_toggled() { on_visFon_toggled(); }
bool wantDrag=false;
void MainWindow::mouseClick(const QPoint& pos)
{
    QPointF coord=vp.toWorld(pos);
    qDebug() << "to world:" << coord;
    wantDrag = false;
    ui->PaintWidget->setCursor(Qt::CrossCursor);
    ui->treeChange->setProperty("tree",0);

    // find adjactent tree
    if (!mModel || !mModel->ru() || mModel->ru()->trees().size()==0)
        return;
    // test ressource units...
    RessourceUnit *ru = mModel->ru(coord);
    qDebug() << "coord:" << coord << "RU:"<< ru << "ru-rect:" << ru->boundingBox();
    QVector<Tree> &mTrees =  ru->trees();
    QVector<Tree>::iterator tit;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        if(distance(tit->position(),coord)<2) {
            Tree *p = &(*tit);
            qDebug() << "found!" << tit->id() << "at" << tit->position()<<"value"<<p->lightRessourceIndex();
            ui->treeChange->setProperty("tree", (int)p);
            ui->treeDbh->setValue(p->dbh());
            ui->treeHeight->setValue(p->height());
            ui->treePosX->setValue(p->position().x());
            ui->treePosY->setValue(p->position().y());
            ui->treeImpact->setText(QString("#:%1 - %2").arg(p->id()).arg(p->lightRessourceIndex(),5));
            wantDrag=true;
            ui->PaintWidget->setCursor(Qt::SizeAllCursor);
            ui->PaintWidget->update();
            break;
        }
   }
}

void MainWindow::mouseMove(const QPoint& pos)
{
    if (!mGrid || mGrid->isEmpty())
        return;
    QPointF p = vp.toWorld(pos);
    if (mGrid->coordValid(p)) {
        if (ui->visFon->isChecked())
           ui->fonValue->setText(QString("%1 / %2\n%3").arg(p.x()).arg(p.y()).arg((*mGrid).valueAt(p)));
        if( ui->visDomGrid->isChecked())
            ui->fonValue->setText(QString("%1 / %2\n%3").arg(p.x()).arg(p.y()).arg((*mDomGrid).valueAt(p)));
    }
}

void MainWindow::mouseWheel(const QPoint& pos, int steps)
{
    qDebug() << "mouse-wheel" << steps;
    vp.zoomTo(pos, qMax(1-(2*steps/10.),0.2));
    ui->PaintWidget->update();
}

void MainWindow::mouseDrag(const QPoint& from, const QPoint &to, Qt::MouseButton button)
{
    qDebug() << "drag" << button;
    ui->PaintWidget->setCursor(Qt::CrossCursor);
    // padding
    if (button == Qt::MidButton) {
        vp.moveTo(from, to);
        ui->PaintWidget->update();
        return;
    }
    if (!wantDrag)
        return;
    qDebug() << "drag from" << from << "to" << to;
    Tree *t = (Tree*) ui->treeChange->property("tree").toInt();
    if (!t)
        return;
    QPointF pos = vp.toWorld(to);
    // calculate new position...
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
    if (!mModel || !mModel->ru() || mModel->ru()->trees().size()==0)
        return;
    QVector<Tree> &mTrees =  mModel->ru()->trees();
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




void MainWindow::addTrees(const double dbh, const int count)
{
    // add a number of trees
    if (!mModel || !mModel->ru() || mModel->ru()->trees().size()==0)
        return;
    QVector<Tree> &mTrees =  mModel->ru()->trees();
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
    /*
    QStringList dbhs = setting("seriesDbh").split(" ");
    QStringList ns = setting("seriesStemnumber").split(" ");
    QStringList result, line;
    result.append("result;" + dbhs.join(";"));
    int stemCount, dbh;
    double val=0.;
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
            //stampTrees(); // dropped func
            //val = retrieveFon();
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
*/
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



void MainWindow::setupModel()
{
    try {
        if (mModel)
            delete mModel;

        mModel = new Model();
        GlobalSettings::instance()->loadProjectFile(ui->initFileName->text());
        mModel->loadProject();
        mGrid = mModel->grid();
        mDomGrid= mModel->heightGrid();
        // set viewport of paintwidget
        vp = Viewport(mModel->grid()->metricRect(), ui->PaintWidget->rect());
    } catch(const IException &e) {
        Helper::msg(e.toString());
    }
}

void MainWindow::on_fonRun_clicked()
{
    try {
        setupModel();
        QVector<Tree> &mTrees =  mModel->ru()->trees();
        Tree::lafactor = 0.8;
        // Load Trees
        mTrees.clear();

        mModel->beforeRun(); // load stand

        // start first cycle...
        readwriteCycle();
    } catch(const IException &e) {
        Helper::msg(e.toString());
    }
}

void MainWindow::on_lrProcess_clicked()
{
    QDomElement docElem = xmldoc.documentElement();
    QDomElement trees = docElem.firstChildElement("lightroom").firstChildElement("trees");
            //getNode("lightroom.trees");
    QString path =  docElem.firstChildElement("lightroom").firstChildElement("trees").firstChildElement("path").text();
    QString target_name = docElem.firstChildElement("lightroom").firstChildElement("trees").firstChildElement("file").text();
    qDebug() << "store to " << path;
    QDomElement tree = docElem.firstChildElement("lightroom").firstChildElement("trees").firstChildElement("tree");
    //int avg_cells = docElem.firstChildElement("lightroom").firstChildElement("size").attribute("average").toInt();

    double cut_threshold = docElem.firstChildElement("lightroom").firstChildElement("cutvalue").text().toDouble();
    QString stamp_desc = docElem.firstChildElement("lightroom").firstChildElement("desc").text();
    qDebug() << "cutting stamps when averaged absoulte value of rings is below"<<cut_threshold;
    float crown, height, bhd;
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

    DebugTimer creation_time("total creation of stamps");

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

        // store as textfile:
        //result = gridToString( lightroom->result() );
        //Helper::saveToTextFile(path + "\\ " + name + ".txt", result);

        // save stamp as image:
        //QImage img = gridToImage( lightroom->result() );
        //img.save(path + "\\ " + name + ".jpg", "JPG", 100);

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
        container.addStamp(stamp,bhd, hd, lro->maxRadius()); // 3rd param was: ,
        ///////////////////////////
        tree = tree.nextSiblingElement("tree");
    }
    qDebug() << "finished!!!";
    // write container to a file....
    QFile file(path + "\\" + target_name);
    file.open(QIODevice::WriteOnly);
    container.setDescription(stamp_desc); // provided in xml...
    QDataStream out(&file);   // we will serialize the data into the file
    container.save(out);
    file.close();
    qDebug() << "current content of the container:";
    qDebug() << container.dump();
}

void MainWindow::on_lrLoadStamps_clicked()
{
    {
        QString fileName = Helper::fileDialog("Name for binary stamp file");
        if (fileName.isEmpty())
            return;
        QFile infile(fileName);
        infile.open(QIODevice::ReadOnly);
        QDataStream in(&infile);
        StampContainer container;
        container.load(in);
        infile.close();
        qDebug() << "Dumping content of Stamp-container" << fileName;
        qDebug() << container.dump();
        // and the reader-stamps....
    }
    {
        QString fileName = Helper::fileDialog("Name for reader stamp file");
        if (fileName.isEmpty())
            return;
        QFile infile(fileName);
        infile.open(QIODevice::ReadOnly);
        QDataStream in(&infile);
        StampContainer container;
        container.load(in);
        infile.close();
        qDebug() << "Dumping content of Reader-container" << fileName;
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
    QPointF newPos(ui->treePosX->value(), ui->treePosY->value() );
    t->setPosition(newPos);
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
    for (double radius=0.5; radius<=8; radius+=0.1) {
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
        int width=11;
        if (radius>=7.) { width=9; }
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
    container.setDescription("Reader-stamps for crown-radii ranging from 0.5m to 8m, stepwidth is 0.1m.");
    container.save(out);
    qDebug() << container.dump();


}

void MainWindow::on_pbSetAsDebug_clicked()
{
    int pt = ui->treeChange->property("tree").toInt();
    if (!pt)
        return;
    Tree *t = (Tree*)pt;
    t->enableDebugging();

}

void MainWindow::on_openFile_clicked()
{
    QString fileName = Helper::fileDialog("select XML-ini file for FonStudio...");
    if (fileName.isEmpty())
        return;
    ui->initFileName->setText(fileName);
    QString xmlFile = Helper::loadTextFile(ui->initFileName->text());
    ui->iniEdit->setPlainText(xmlFile);
}


void MainWindow::on_actionTreelist_triggered()
{
    QApplication::clipboard()->setText(dumpTreelist());
    qDebug() << "treelist copied to clipboard.";
}

void MainWindow::on_actionFON_grid_triggered()
{
    if (!mGrid)
        return;
    QString gr = gridToString(*mGrid);
    QApplication::clipboard()->setText(gr);
    qDebug() << "grid copied to clipboard.";
}

void MainWindow::on_execManyStands_clicked()
{
    if (!mModel || !mModel->ru())
        return;
    QVector<Tree> &mTrees =  mModel->ru()->trees();
    QDomElement xmlAuto = xmldoc.documentElement().firstChildElement("automation");
    QString outPath = xmlAuto.firstChildElement("outputpath").text();
    QString inPath = xmlAuto.firstChildElement("inputpath").text();
    QString inFile = xmlAuto.firstChildElement("stands").text();
    qDebug() << "standlist:" << inFile << "inpath:"<<inPath << "save to:"<<outPath;
    QStringList fileList = Helper::loadTextFile(inFile).remove('\r').split('\n', QString::SkipEmptyParts);

    StandLoader loader(mModel);

    foreach (QString file, fileList) {
        file = inPath + "\\" + file;
        qDebug() << "processing" << file;
        mTrees.clear();
        loader.loadFromPicus(file);
        // do a cycle...
        readwriteCycle();

        // create output file
        QFileInfo fi(file);
        QString outFileName = QString("%1\\out_%2.csv").arg(outPath, fi.baseName());
        Helper::saveToTextFile(outFileName, dumpTreelist() );
        qDebug() << mTrees.size() << "trees loaded, saved to" << outFileName;
        QApplication::processEvents();

    }


}

void MainWindow::on_pbMultipleApplication_clicked()
{
    int c=ui->lApplyManyCount->text().toInt();
    applyCycles(c);
}
