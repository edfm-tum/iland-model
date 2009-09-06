#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTGui>
#include <QTXml>

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



double distance(const QPointF &a, const QPointF &b)
{
    return sqrt( (a.x()-b.x())*(a.x()-b.x()) + (a.y()-b.y())*(a.y()-b.y()) );
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

    // dock windows
    ui->menuView->addAction( ui->dockEditor->toggleViewAction() );
    ui->menuView->addAction( ui->dockLogviewer->toggleViewAction() );
    ui->menuView->addAction( ui->dockWidget->toggleViewAction() );


    ui->pbResults->setMenu(ui->menuOutput_menu);

    mLogSpace = ui->logOutput;
    qInstallMsgHandler(myMessageOutput);

    // load xml file

    QString argText = QApplication::arguments().last();
    if (QApplication::arguments().count()>1 && !argText.isEmpty())
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

    checkModelState();

}

MainWindow::~MainWindow()
{
    delete ui;
}

// control GUI actions
void MainWindow::checkModelState()
{
    ui->actionModelCreate->setEnabled(mRemoteControl.canCreate());
    ui->actionModelDestroy->setEnabled(mRemoteControl.canDestroy());
    ui->actionModelRun->setEnabled(mRemoteControl.canRun());
}


void MainWindow::on_saveFile_clicked()
{
    QString content = ui->iniEdit->toPlainText();
    if (!content.isEmpty())
         Helper::saveToTextFile(ui->initFileName->text(), content);

}


void MainWindow::readwriteCycle()
{

    if (!mRemoteControl.canRun())
        return;

    mRemoteControl.runYear();

}




QString MainWindow::dumpTreelist()
{
    if (!mRemoteControl.isRunning())
        return "";

    Model *model = mRemoteControl.model();
    RessourceUnit *ru = model->ru();
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

    if (!mRemoteControl.canRun())
        return;
    Model *model = mRemoteControl.model();

    FloatGrid *grid = model->grid();
    FloatGrid *domGrid = model->heightGrid();
    // do the actual painting
    if (!grid)
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
        maxval =grid->max();
    if (maxval==0.)
        return;
    // clear background
    painter.fillRect(ui->PaintWidget->rect(), Qt::white);
    // draw rectangle around the grid
    QRectF r = grid->metricRect();
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
                if (grid->coordValid(world)) {
                   value = grid->valueAt(world);
                   col = Helper::colorFromValue(value, 0., maxval, true).rgb();
                   img.setPixel(x,y,col);
               }
           }

    } else {
        // for rather small grids, the inverse tactic (draw each pixel!)
        if (show_fon) {

            //QRectF cell(0,0,m_pixelpercell, m_pixelpercell);
            QPointF k;
            float hdom;
            for (iy=0;iy<grid->sizeY();iy++) {
                for (ix=0;ix<grid->sizeX();ix++) {

                    value = grid->valueAtIndex(QPoint(ix, iy));
                    if (show_scale40) {
                        k = grid->cellCenterPoint(QPoint(ix, iy));
                        hdom = qMax(2.f, domGrid->valueAt(k)); // 2m: as in stamp grid
                        value = 1.f -  (1.f - value) * hdom / scale_value;
                    }
                    QRectF f = grid->cellRect(QPoint(ix,iy));
                    QRect r = vp.toScreen(f);

                    fill_color = Helper::colorFromValue(value, 0., maxval, true);
                    painter.fillRect(r, fill_color);

                }
            }
        } // if (show_fon)

        if (show_dom) {
            // paint the lower-res-grid;
            for (iy=0;iy<domGrid->sizeY();iy++) {
                for (ix=0;ix<domGrid->sizeX();ix++) {
                    QPoint p(ix,iy);
                    value = domGrid->valueAtIndex(p);
                    QRect r = vp.toScreen(domGrid->cellRect(p));
                    fill_color = Helper::colorFromValue(value, 0., 50.); // 0..50m
                    painter.fillRect(r, fill_color);
                }
            }

        } // if (show_dem)
    }
    if (show_impact) {
        AllTreeIterator treelist(model);
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
     paintFON(painter, ui->PaintWidget->rect());
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
    if (!mRemoteControl.isRunning())
        return;

    // test ressource units...
    Model *model = mRemoteControl.model();
    RessourceUnit *ru = model->ru(coord);
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

    if (!mRemoteControl.isRunning())
        return;
    FloatGrid *grid = mRemoteControl.model()->grid();
    QPointF p = vp.toWorld(pos);
    if (grid->coordValid(p)) {
        if (ui->visFon->isChecked())
           ui->fonValue->setText(QString("%1 / %2\n%3").arg(p.x()).arg(p.y()).arg((*grid).valueAt(p)));
        if( ui->visDomGrid->isChecked())
            ui->fonValue->setText(QString("%1 / %2\n%3").arg(p.x()).arg(p.y()).arg((*mRemoteControl.model()->heightGrid()).valueAt(p)));
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
    readwriteCycle();
}



void MainWindow::on_actionEdit_XML_settings_triggered()
{
    ui->editStack->setCurrentIndex(0);
    ui->PaintWidget->update();
}




void MainWindow::setupModel()
{
    // load project xml file to global xml settings structure
    GlobalSettings::instance()->loadProjectFile(ui->initFileName->text());
    // create the model
    mRemoteControl.create();
    Model *model = mRemoteControl.model();
    if (model) {
        // set viewport of paintwidget
        vp = Viewport(model->grid()->metricRect(), ui->PaintWidget->rect());
        // debug mode
        GlobalSettings::instance()->setDebugOutput(GlobalSettings::dTreeGrowth);
    }
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
    if (!mRemoteControl.isRunning()) return;
    QString gr = gridToString(*mRemoteControl.model()->grid());
    QApplication::clipboard()->setText(gr);
    qDebug() << "grid copied to clipboard.";
}




void MainWindow::on_actionModelCreate_triggered()
{
    // create model
    setupModel();
    checkModelState();
}

void MainWindow::on_actionModelDestroy_triggered()
{
    mRemoteControl.destroy();
    checkModelState();
}

void MainWindow::on_actionModelRun_triggered()
{
   if (!mRemoteControl.canRun())
        return;
   mRemoteControl.runYear();
   checkModelState();
   ui->PaintWidget->update();
}

QStringList MainWindow::debugDataTable(GlobalSettings::DebugOutputs type, const QString separator)
{
    if (!mRemoteControl.isRunning())
        return QStringList();

    GlobalSettings *g = GlobalSettings::instance();
    QList<DebugList> ddl = g->debugLists(-1, type); // get all debug data

    QStringList result;
    result << g->debugListCaptions(type).join(separator);
    foreach (const DebugList &l, ddl) {
        QString line;
        int c=0;
        foreach(const QVariant &value, l) {
            if (c++)
                line+=separator;
            line += value.toString();
        }
        result << line;
    }
    return result;
}



void MainWindow::on_actionTree_Partition_triggered()
{
    QStringList result = debugDataTable(GlobalSettings::dTreePartition, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}

void MainWindow::on_actionTree_Growth_triggered()
{
    QStringList result = debugDataTable(GlobalSettings::dTreeGrowth, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}

void MainWindow::on_actionSelect_Data_Types_triggered()
{
    int value = 0;
    int newvalue = QInputDialog::getInt(this, "QInputDialog::getText()",
                                        "Enter code for desired outputs: add\n" \
                                        "1 ... Tree grwoth\n" \
                                        "2 ... Tree partition\n" \
                                        "4 ... Standlevel NPP\n" \
                                        "(e.g.: 5 = NPP + tree growth) or 0 for no debug outputs.", value);
     GlobalSettings::instance()->setDebugOutput(newvalue);
}
