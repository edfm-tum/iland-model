/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include <QtCore>
#include <QtGui>
#include <QtXml>

#include <signal.h>

#include "global.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"

#include "model.h"
#include "standloader.h"
#include "stampcontainer.h"
#include "resourceunit.h"
#include "speciesset.h"
#include "tree.h"
#include "species.h"

#include "exception.h"

#include "paintarea.h"

#include "expression.h"
#include "expressionwrapper.h"
#include "management.h"
#include "outputmanager.h"

#include "tests.h"
#include "mapgrid.h"
#include "layeredgrid.h"

// global settings
QDomDocument xmldoc;
QDomNode xmlparams;

/** @class MainWindow
   @ingroup GUI
   The main window of the iLand viewer.


  */


double distance(const QPointF &a, const QPointF &b)
{
    return sqrt( (a.x()-b.x())*(a.x()-b.x()) + (a.y()-b.y())*(a.y()-b.y()) );
}


double nrandom(const float& p1, const float& p2)
{
    return p1 + (p2-p1)*(rand()/float(RAND_MAX));
}

bool showDebugMessages=true;
QStringList bufferedMessages;
bool doBufferMessages = false;
bool doLogToWindow = false;
void logToWindow(bool mode)
{
   doLogToWindow = mode;
}
class LogToWindow
{
public:
    LogToWindow() { logToWindow(true);}
    ~LogToWindow() { logToWindow(false);}
};


QMutex qdebug_mutex;
void dumpMessages();
void myMessageOutput(QtMsgType type, const char *msg)
 {

    QMutexLocker m(&qdebug_mutex);
    switch (type) {
     case QtDebugMsg:
        if (showDebugMessages) {
            bufferedMessages.append(QString(msg));
        }

         break;
     case QtWarningMsg:
         //MainWindow::logSpace()->appendPlainText(QString("WARNING: %1").arg(msg));
         //MainWindow::logSpace()->ensureCursorVisible();
          bufferedMessages.append(QString(msg));
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         bufferedMessages.append(QString(msg));

         QString file_name = GlobalSettings::instance()->path("fatallog.txt","log");
         Helper::msg(QString("Fatal message encountered:\n%1\nFatal-Log-File: %2").arg(msg, file_name));
         dumpMessages();
         Helper::saveToTextFile(file_name, MainWindow::logSpace()->toPlainText() + bufferedMessages.join("\n"));

     }
     if (!doBufferMessages || bufferedMessages.count()>5000)
             dumpMessages();
 }

QMutex dump_message_mutex;
void dumpMessages()
{
    QMutexLocker m(&dump_message_mutex); // serialize access
    // 2011-03-08: encountered "strange" crashes
    // when a warning within Qt lead to a infinite loop/deadlock (also caused by mutex locking)
    // now we prevent this by installing temporarily a 0-handler
    qInstallMsgHandler(0);

    if (MainWindow::logStream() && !doLogToWindow) {
        foreach(const QString &s, bufferedMessages)
            *MainWindow::logStream() << s << endl;
        MainWindow::logStream()->flush();

    } else {
        foreach(const QString &s, bufferedMessages)
            MainWindow::logSpace()->appendPlainText(s);

        // it does *not* work to just MainWindow::logSpace()->textCursor().movePosition()!
        // you have to "setTextCursor()".
        QTextCursor cursor = MainWindow::logSpace()->textCursor();
        cursor.movePosition(QTextCursor::End);
        MainWindow::logSpace()->setTextCursor(cursor);
        MainWindow::logSpace()->ensureCursorVisible();
    }

    bufferedMessages.clear();
    qInstallMsgHandler(myMessageOutput);
}


// handle signal...
// source: http://cplusplus.com/forum/unices/13455/
void handle_signal( int signo ) {
    Helper::msg(QString("Received Signal:\n%1").arg(signo));
    qDebug() << "*** Received signal "<< signo << "****";
    dumpMessages();

}

void MainWindow::bufferedLog(bool bufferLog)
{
    doBufferMessages = bufferLog;
    if (bufferLog==false)
        dumpMessages();
}

void MainWindow::setupFileLogging(const bool do_start)
{
    if (mLogStream) {
        if (mLogStream->device())
            delete mLogStream->device();
        delete mLogStream;
        mLogStream = NULL;
    }
    if (!do_start)
        return;

    if (GlobalSettings::instance()->settings().value("system.logging.logTarget", "console") == "file") {
        QString fname = GlobalSettings::instance()->settings().value("system.logging.logFile", "logfile.txt");
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        fname.replace("$date$", timestamp);
        fname = GlobalSettings::instance()->path(fname, "log");
        QFile *file = new QFile(fname);

        if (!file->open(QIODevice::WriteOnly)) {
            qDebug() << "cannot open logfile" << fname;
        } else {
            qDebug() << "Log output is redirected to logfile" << fname;
            mLogStream = new QTextStream(file);
        }
    }

}


QPlainTextEdit *MainWindow::mLogSpace=NULL;
QTextStream *MainWindow::mLogStream=NULL;


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
    ui->menuView->addAction( ui->dockModelDrill->toggleViewAction() );

    ui->pbResults->setMenu(ui->menuOutput_menu);

    mLogSpace = ui->logOutput;
    mLogSpace->setMaximumBlockCount(10000); // set a maximum for the in-GUI size of messages.
    qInstallMsgHandler(myMessageOutput);
    // install signal handler
    signal( SIGSEGV, handle_signal );

    readSettings();

    // create global scripting context
    GlobalSettings::instance()->resetScriptEngine();

    // load xml file: use either a command-line argument (if present), or load the content of a small text file....
    QString argText = QApplication::arguments().last();
    if (QApplication::arguments().count()>1 && !argText.isEmpty()) {
        ui->initFileName->setText(argText);
    } else {
        QString lastXml = QSettings().value("project/lastxmlfile").toString();
        //QString lastXml = Helper::loadTextFile( QCoreApplication::applicationDirPath()+ "/lastxmlfile.txt" );
        if (!lastXml.isEmpty() && QFile::exists(lastXml))
            ui->initFileName->setText(lastXml);
    }
    QString xmlFile = Helper::loadTextFile(ui->initFileName->text());

    if (!xmlFile.isEmpty()) {
        ui->iniEdit->setPlainText(xmlFile);
        QString errMsg;
        int errLine, errCol;
        if (!xmldoc.setContent(xmlFile, &errMsg, &errLine, &errCol)) {
            QMessageBox::information(this, "title text", QString("Cannot set content of XML file %1. \nat line %2 col %3: %4 ")
                                     .arg(ui->initFileName->text()).arg(errLine).arg(errCol).arg(errMsg));
            return;
        }
    }

    on_actionEdit_XML_settings_triggered();

    qDebug() << "threadcount: " << QThread::idealThreadCount();

    // load window settings
    QString fileName = QDir::current().filePath("gui.txt");
    if (QFile::exists(fileName)) {

        QByteArray state = Helper::loadFile(fileName);
        restoreState(state);
    }
    checkModelState();
    ui->statusBar->addPermanentWidget(ui->modelRunProgress);
    ui->modelRunProgress->setValue(0);
    mStatusLabel = new QLabel(this);
    labelMessage("no model created.");
    ui->statusBar->addWidget(mStatusLabel);
    // remote control of model
    connect(&mRemoteControl, SIGNAL(year(int)),this,SLOT(yearSimulated(int)));
    connect(&mRemoteControl, SIGNAL(finished(QString)), this, SLOT(modelFinished(QString)));
    connect(&mRemoteControl, SIGNAL(stateChanged()), this, SLOT(checkModelState()));

    // log levels
    ui->actionDebug->setProperty("logLevel", QVariant(0));
    ui->actionInfo->setProperty("logLevel", QVariant(1));
    ui->actionWarning->setProperty("logLevel", QVariant(2));
    ui->actionError->setProperty("logLevel", QVariant(3));

    // species filter
    connect( ui->speciesFilterBox, SIGNAL(currentIndexChanged(int)), SLOT(repaint()));
    connect( ui->paintGridBox, SIGNAL(currentIndexChanged(int)), SLOT(repaint()));
    updatePaintGridList();

    // model controller
    mRemoteControl.setMainWindow( this );
    mRemoteControl.connectSignals();
    GlobalSettings::instance()->setModelController( &mRemoteControl );

    // automatic run
    if (QApplication::arguments().contains("run")) {
        QTimer::singleShot(3000, this, SLOT(automaticRun()));
    }

}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::batchLog(const QString s)
{
    QFile outfile(QCoreApplication::applicationDirPath()+ "/batchlog.txt");
    if (outfile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream str(&outfile);
        str << s << endl;
    }
}

/// automatically start a simulation...
void MainWindow::automaticRun()
{
    // start a simulation
    setWindowTitle("iLand viewer --- batch mode");
    batchLog("*** iland batch mode ***");
    batchLog(QString("%1 Loading project %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate),
           ui->initFileName->text()));
    batchLog(QString("%1 Loading the model...").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    setupModel();

    int count = QCoreApplication::arguments()[QCoreApplication::arguments().count()-2].toInt();
    if (count==0) {
        qDebug() << "invalid number of years....";
        return;
    }
    ui->modelRunProgress->setMaximum(count-1);
    batchLog(QString("%1 Running the model (%2 years)...").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(count));
    // note the "+1": this is similar to the normal way of starting
    // "1" means in Globals.year that we are in the 1st year.
    // the simulation stops when reaching the count+1 year.
    mRemoteControl.run(count + 1);
    // process debug outputs...
    saveDebugOutputs();

    // see the finsished() slot
}

// simply command an update of the painting area
void MainWindow::repaint()
{
    ui->PaintWidget->update();
    QCoreApplication::processEvents();
}

// control GUI actions
void MainWindow::checkModelState()
{
    ui->actionModelCreate->setEnabled(mRemoteControl.canCreate()&& !mRemoteControl.isRunning());
    ui->actionModelDestroy->setEnabled(mRemoteControl.canDestroy() && !mRemoteControl.isRunning());
    ui->actionModelRun->setEnabled(mRemoteControl.canRun( )&& !mRemoteControl.isPaused() && !mRemoteControl.isRunning());
    ui->actionRun_one_year->setEnabled(mRemoteControl.canRun() && !mRemoteControl.isPaused()&& !mRemoteControl.isRunning());
    ui->actionReload->setEnabled(mRemoteControl.canDestroy()&& !mRemoteControl.isRunning());
    ui->actionStop->setEnabled(mRemoteControl.isRunning());
    ui->actionPause->setEnabled(mRemoteControl.isRunning());
    ui->actionPause->setText(mRemoteControl.isPaused()?"Continue":"Pause");
    dumpMessages();
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

    Model *model = mRemoteControl.model();
    model->onlyApplyLightPattern();
}




QString MainWindow::dumpTreelist()
{
    if (!mRemoteControl.isRunning())
        return "";

    Model *model = mRemoteControl.model();

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
        result << line;
    }
    QString resStr = result.join("\n");
    return resStr;
}

void MainWindow::updatePaintGridList()
{
    ui->paintGridBox->clear();
    ui->paintGridBox->addItem("<none>", "");
    QHash<QString, PaintObject>::const_iterator i = mPaintList.begin();
    while (i!=mPaintList.constEnd()) {
        ui->paintGridBox->addItem(i.key(),i.key());
        ++i;
    }
}

void MainWindow::addLayers(const LayeredGridBase *layer, const QString &name)
{
    const QStringList names = layer->names();
    int layer_id = 0;
    foreach (const QString &layername, names) {
        QString comb_name = QString("%1 - %2").arg(name, layername);
        PaintObject po;
        po.what = PaintObject::PaintLayers;
        po.layered = layer;
        po.layer_id = layer_id++;
        po.auto_range = true;
        mPaintList[comb_name] = po;
    }
    updatePaintGridList();
}


void MainWindow::paintGrid(MapGrid *map_grid, const QString &name,
               const GridViewType view_type,
               double min_val, double max_val)
{
    if (map_grid==0 && !name.isEmpty()) {
        // remove the grid from the list
        mPaintList.remove(name);
        updatePaintGridList();
        return;
    }
    mPaintNext.what=PaintObject::PaintMapGrid;
    mPaintNext.min_value=min_val; mPaintNext.max_value=max_val;
    mPaintNext.map_grid = map_grid; mPaintNext.view_type = view_type;
    if (!name.isEmpty()) {
        updatePaintGridList();
        mPaintList[name] = mPaintNext;
    }
    ui->visOtherGrid->setChecked(true);
    repaint();
}

void MainWindow::paintGrid(const FloatGrid *grid, const QString &name,
                           const GridViewType view_type,
                           double min_val, double max_val)
{
    if (grid==0 && !name.isEmpty()) {
        // remove the grid from the list
        mPaintList.remove(name);
        updatePaintGridList();
        return;
    }
    mPaintNext.what=PaintObject::PaintFloatGrid;
    mPaintNext.min_value=min_val;
    mPaintNext.max_value=max_val;
    mPaintNext.float_grid = grid;
    mPaintNext.view_type = view_type;
    if (!name.isEmpty()) {
        mPaintList[name] = mPaintNext;
        updatePaintGridList();
    }
    ui->visOtherGrid->setChecked(true);
    repaint();
}

void MainWindow::paintFON(QPainter &painter, QRect rect)
{
    DebugTimer drawtimer("painting");
    drawtimer.setSilent();

    if (!mRemoteControl.canRun())
        return;
    Model *model = mRemoteControl.model();

    FloatGrid *grid = model->grid();
    HeightGrid *domGrid = model->heightGrid();
    // do the actual painting
    if (!grid)
        return;
    bool auto_scale_color = ui->visAutoScale->isChecked();
    bool show_fon = ui->visFon->isChecked();
    bool show_dom = ui->visDomGrid->isChecked();
    bool show_trees = ui->visImpact->isChecked();
    bool species_color = ui->visSpeciesColor->isChecked();
    bool show_ru = ui->visResourceUnits->isChecked();
    bool show_regeneration = ui->visRegeneration->isChecked();
    bool other_grid = ui->visOtherGrid->isChecked();

    if (other_grid) {
        // return; // TODO TEST
        if (ui->paintGridBox->currentIndex()>-1) {
            QString name = ui->paintGridBox->itemData(ui->paintGridBox->currentIndex()).toString();
            if (!name.isEmpty())
                mPaintNext = mPaintList[name];
        }

        if (mPaintNext.what != PaintObject::PaintNothing) {
            if (mPaintNext.what == PaintObject::PaintMapGrid)
                paintMapGrid(painter, mPaintNext.map_grid, 0, mPaintNext.view_type, mPaintNext.min_value, mPaintNext.max_value);

            if (mPaintNext.what == PaintObject::PaintFloatGrid)
                paintMapGrid(painter, 0, mPaintNext.float_grid, mPaintNext.view_type, mPaintNext.min_value, mPaintNext.max_value);

            if (mPaintNext.what == PaintObject::PaintLayers)
                paintGrid(painter, mPaintNext);

            return;
        }
    }


    // clear background
    painter.fillRect(ui->PaintWidget->rect(), Qt::white);
    // draw rectangle around the grid
    QRectF r = grid->metricRect();
    QRect rs = vp.toScreen(r);
    painter.setPen(Qt::black);
    painter.drawRect(rs);

    // what to paint??

    float maxval=1.f; // default maximum
    if (!auto_scale_color)
        maxval =grid->max();
    if (maxval==0.)
        return;
    QString species;
    if (ui->speciesFilterBox->currentIndex()>-1)
        species = ui->speciesFilterBox->itemData(ui->speciesFilterBox->currentIndex()).toString();

    int ix,iy;
    QColor fill_color;
    float value;

    if (show_fon ) {

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

    }

    if (show_regeneration ) {

        if (mRegenerationGrid.isEmpty())
            mRegenerationGrid.setup(*model->grid()); // copy
        static int last_year=0;
        static QString last_species="";
        if (last_year!=GlobalSettings::instance()->currentYear() || species!=last_species) {
            last_year=GlobalSettings::instance()->currentYear();
            last_species=species;
            // fill grid...
            DebugTimer t("create regeneration map...");
            mRegenerationGrid.wipe(0.f);
            foreach(const ResourceUnit *ru, model->ruList()) {
                foreach(const ResourceUnitSpecies *rus, ru->ruSpecies()) {
                    if (species.isEmpty() || rus->species()->id() == species)
                        rus->visualGrid(mRegenerationGrid);
                }
            }
        }
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
                if (mRegenerationGrid.coordValid(world)) {
                    value = mRegenerationGrid.valueAt(world);
                    col = Helper::colorFromValue(value, 0., 4., false).rgb(); // 0..4m
                    img.setPixel(x,y,col);
                }
            }
    }

    if (show_dom) {
        // paint the lower-res-grid;
        float max_val = 50.;
        if (auto_scale_color) {
            max_val = 0.;
            for (HeightGridValue *v = domGrid->begin(); v!=domGrid->end(); ++v)
                max_val = qMax(max_val, v->height);
        }
        for (iy=0;iy<domGrid->sizeY();iy++) {
            for (ix=0;ix<domGrid->sizeX();ix++) {
                QPoint p(ix,iy);
                const HeightGridValue &hgv = domGrid->valueAtIndex(p);
                if (hgv.isValid()) {
                    value = domGrid->valueAtIndex(p).height;
                    QRect r = vp.toScreen(domGrid->cellRect(p));
                    fill_color = Helper::colorFromValue(value, 0., max_val); // 0..50m
                    painter.fillRect(r, fill_color);
                }
                // areas "outside" are drawn as gray.
                if (hgv.isForestOutside()) {
                    QRect r = vp.toScreen(domGrid->cellRect(p));
                    if (hgv.isRadiating())
                        painter.fillRect(r, Qt::darkGray);
                    else
                        painter.fillRect(r, Qt::gray);

                }
            }
        }

    } // if (show_dom)

    if (show_ru){
        QString ru_expr = ui->lTreeExpr->text();
        if (ru_expr.isEmpty())
            ru_expr = "id";
        RUWrapper ru_wrapper;

        Expression ru_value(ru_expr, &ru_wrapper);
        ru_value.setCatchExceptions(); // silent catching...
        double min_value = 0.;
        double max_value = 1.; // defaults
        double value;
        if (auto_scale_color) {
            min_value = 9999999999999999999.;
            max_value = -999999999999999999.;
            foreach (const ResourceUnit *ru, model->ruList()) {
                ru_wrapper.setResourceUnit(ru);
                value = ru_value.execute();
                min_value = qMin(min_value, value);
                max_value = qMax(max_value, value);
            }
            qDebug() << "scale colors: min" << min_value << "max:" << max_value;
        }

        // paint resource units
        foreach (const ResourceUnit *ru, model->ruList()) {
            ru_wrapper.setResourceUnit(ru);
            QRect r = vp.toScreen(ru->boundingBox());
            value = ru_value.execute();
            fill_color = Helper::colorFromValue(value, min_value, max_value);
            painter.fillRect(r, fill_color);
        }
        if (!ru_value.lastError().isEmpty())
            qDebug() << "Expression error while painting: " << ru_value.lastError();
    }

    if (show_trees) {
        QString single_tree_expr = ui->lTreeExpr->text();
        if (single_tree_expr.isEmpty())
            single_tree_expr = "1-lri";
        TreeWrapper tw;

        Expression tree_value(single_tree_expr, &tw);    // get maximum value
        tree_value.setCatchExceptions(); // silent catching...

        AllTreeIterator treelist(model);
        Tree *tree;
        painter.setPen(Qt::gray);
        while ((tree = treelist.next())) {
            if ( !vp.isVisible(treelist.currentRU()->boundingBox()) ) {
                continue;
            }
            // filter species...
            if (!species.isEmpty())
                if (tree->species()->id() != species)
                    continue;

            // filter out dead trees
            if (tree->isDead())
                continue;

            QPointF pos = tree->position();
            QPoint p = vp.toScreen(pos);
            if (species_color) {
                // use species specific color....
                fill_color = tree->species()->displayColor();
            } else {
                // calculate expression
                tw.setTree(tree);
                value = tree_value.execute();
                fill_color = Helper::colorFromValue(value, 0., 1., false);
            }
            painter.setBrush(fill_color);
            int diameter = qMax(1,vp.meterToPixel( tree->crownRadius()));
            painter.drawEllipse(p, diameter, diameter);
        }
        if (!tree_value.lastError().isEmpty())
            qDebug() << "Expression error while painting: " << tree_value.lastError();

    } // if (show_trees)

    // highlight selected tree
    Tree *t = (Tree*) ui->treeChange->property("tree").toInt();
    if (t) {
        QPointF pos = t->position();
        painter.setPen(Qt::black);
        QPoint p = vp.toScreen(pos);
        painter.drawRect( p.x()-1, p.y()-1, 3,3);
    }
}

void MainWindow::paintGrid(QPainter &painter, PaintObject &object)
{
    painter.fillRect(ui->PaintWidget->rect(), object.background_color);
    bool clip_with_stand_grid = ui->visClipStandGrid->isChecked();

    int sx=0, sy=0;
    QRect total_rect;
    object.cur_min_value = object.min_value;
    object.cur_max_value = object.max_value;
    switch (object.what) {
    case PaintObject::PaintMapGrid:
        sx = object.map_grid->grid().sizeX();
        sy = object.map_grid->grid().sizeY();
        total_rect = vp.toScreen(object.map_grid->grid().metricRect());
        break;
    case PaintObject::PaintFloatGrid:
        sx = object.float_grid->sizeX();
        sy = object.float_grid->sizeY();
        total_rect = vp.toScreen(object.float_grid->metricRect());
        break;
    case PaintObject::PaintLayers:
        sx = object.layered->sizeX();
        sy = object.layered->sizeY();
        total_rect = vp.toScreen(object.layered->metricRect());
        if (object.auto_range) {
            object.layered->range( object.cur_min_value, object.cur_max_value, object.layer_id );
        }
        break;
    case PaintObject::PaintNothing:
        return;
    }



    painter.setPen(Qt::black);
    painter.drawRect(total_rect);

    bool reverse = object.view_type == GridViewRainbowReverse || object.view_type == GridViewGrayReverse;
    bool black_white = object.view_type == GridViewGray || object.view_type == GridViewGrayReverse;

    int ix,iy;
    double value=0.;
    QRect r;
    QColor fill_color;
    QPointF pmetric;
    for (iy=0;iy<sy;iy++) {
        for (ix=0;ix<sx;ix++) {
            QPoint p(ix,iy);
            switch(object.what) {
            case PaintObject::PaintMapGrid:
                value = object.map_grid->grid().constValueAtIndex(p);
                pmetric = object.map_grid->grid().cellRect(p).center();
                r = vp.toScreen(object.map_grid->grid().cellRect(p));

                break;
            case PaintObject::PaintFloatGrid:
                value = object.float_grid->constValueAtIndex(p);
                pmetric = object.float_grid->cellRect(p).center();
                r = vp.toScreen(object.float_grid->cellRect(p));
                break;
            case PaintObject::PaintLayers:
                value = object.layered->value(ix, iy, object.layer_id);
                pmetric = object.layered->cellRect(p).center();
                r = vp.toScreen(object.layered->cellRect(p));
                break;
            default: ;
            }
            if (clip_with_stand_grid && !GlobalSettings::instance()->model()->heightGrid()->valueAt(pmetric).isValid()) {
                fill_color = Qt::white;
            } else {
                fill_color = Helper::colorFromValue(value, object.cur_min_value, object.cur_max_value, reverse,black_white);
            }
            painter.fillRect(r, fill_color);
        }
    }

}

// paint the values of the MapGrid
void MainWindow::paintMapGrid(QPainter &painter,
                              MapGrid *map_grid, const FloatGrid *float_grid,
                              const GridViewType view_type,
                              double min_val, double max_val)
{
    // clear background
    painter.fillRect(ui->PaintWidget->rect(), Qt::white);
    const Grid<int> *int_grid = 0;

    int sx, sy;
    QRect total_rect;

    if (map_grid) {
        int_grid = &map_grid->grid();
        sx = int_grid->sizeX();
        sy = int_grid->sizeY();
        total_rect = vp.toScreen(int_grid->metricRect());
    } else {
        if (!float_grid)
            return;
        sx = float_grid->sizeX();
        sy = float_grid->sizeY();
        total_rect = vp.toScreen(float_grid->metricRect());
    }

    // draw rectangle around the grid
    painter.setPen(Qt::black);
    painter.drawRect(total_rect);
    // paint the lower-res-grid;
    int ix,iy;
    double value;
    QRect r;
    QColor fill_color;
    bool reverse = view_type == GridViewRainbowReverse || view_type == GridViewGrayReverse;
    bool black_white = view_type == GridViewGray || view_type == GridViewGrayReverse;
    for (iy=0;iy<sy;iy++) {
        for (ix=0;ix<sx;ix++) {
            QPoint p(ix,iy);
            if (int_grid){
                value = int_grid->constValueAtIndex(p);
                r = vp.toScreen(int_grid->cellRect(p));
            } else {
                value = float_grid->constValueAtIndex(p);
                r = vp.toScreen(float_grid->cellRect(p));
            }
            fill_color = Helper::colorFromValue(value, min_val, max_val, reverse,black_white);
            painter.fillRect(r, fill_color);
        }
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
    //qDebug() << "to world:" << coord;
    wantDrag = false;
    ui->PaintWidget->setCursor(Qt::CrossCursor);
    Model *model = mRemoteControl.model();
    ResourceUnit *ru = model->ru(coord);
    // find adjactent tree
    if (!mRemoteControl.canRun())
        return;

    if (ui->visResourceUnits->isChecked()) {
        if (!ru) return;
        showResourceUnitDetails(ru);
        return;
    }
    // test ressource units...

    //qDebug() << "coord:" << coord << "RU:"<< ru << "ru-rect:" << ru->boundingBox();
    ui->treeChange->setProperty("tree",0);
    QVector<Tree> &mTrees =  ru->trees();
    QVector<Tree>::iterator tit;
    Tree *closestTree=0;
    double min_distance = 100000000, current_dist;
    for (tit=mTrees.begin(); tit!=mTrees.end(); ++tit) {
        current_dist = distance(tit->position(),coord);
        if (current_dist<min_distance) {
            closestTree = &(*tit);
            min_distance = current_dist;
        }
    }
    if (min_distance<5 && closestTree) {
            Tree *p = closestTree;
            //qDebug() << "found!" << tit->id() << "at" << tit->position()<<"value"<<p->lightResourceIndex();
            //qDebug() <<p->dump();
            showTreeDetails(p);

            ui->treeChange->setProperty("tree", (int)p);
            ui->treeDbh->setValue(p->dbh());
            ui->treeHeight->setValue(p->height());
            ui->treePosX->setValue(p->position().x());
            ui->treePosY->setValue(p->position().y());
            ui->treeImpact->setText(QString("#:%1 - %2").arg(p->id()).arg(p->lightResourceIndex(),5));
            wantDrag=true;
            ui->PaintWidget->setCursor(Qt::SizeAllCursor);
            ui->PaintWidget->update();
    }

}

void MainWindow::showResourceUnitDetails(const ResourceUnit *ru)
{
    ui->dataTree->clear();
    RUWrapper ruw;
    ruw.setResourceUnit(ru);
    const QStringList &names = ruw.getVariablesList();
    QList<QTreeWidgetItem *> items;
    foreach(QString name, names) {
        items.append(new QTreeWidgetItem(QStringList()<<name<<QString::number(ruw.valueByName(name)) ));
    }
    QList<QPair<QString, QVariant> > dbgdata = GlobalSettings::instance()->debugValues(-ru->index()); // hack: use negative values for resource units

    QList<QPair<QString, QVariant> >::const_iterator i = dbgdata.constBegin();
    while (i != dbgdata.constEnd()) {
     //cout << i.key() << ": " << i.value() << endl;
        items.append(new QTreeWidgetItem(QStringList()
                                         << (*i).first
                                         << (*i).second.toString()) );
        ++i;
     }
    ui->dataTree->addTopLevelItems(items);
}


void MainWindow::showTreeDetails(Tree *tree)
{
    ui->dataTree->clear();
    TreeWrapper tw;
    tw.setTree(tree);
    const QStringList &names = tw.getVariablesList();
    QList<QTreeWidgetItem *> items;
    foreach(QString name, names) {
        items.append(new QTreeWidgetItem(QStringList()<<name<<QString::number(tw.valueByName(name)) ));
    }
    QList<QPair<QString, QVariant> > dbgdata = GlobalSettings::instance()->debugValues(tree->id());

    QList<QPair<QString, QVariant> >::const_iterator i = dbgdata.constBegin();
    while (i != dbgdata.constEnd()) {
     //cout << i.key() << ": " << i.value() << endl;
        items.append(new QTreeWidgetItem(QStringList()
                                         << (*i).first
                                         << (*i).second.toString()) );
        ++i;
     }
    ui->dataTree->addTopLevelItems(items);
}

void MainWindow::mouseMove(const QPoint& pos)
{

    if (!mRemoteControl.canRun())
        return;
    FloatGrid *grid = mRemoteControl.model()->grid();
    QPointF p = vp.toWorld(pos);
    bool has_value = true;
    double value;
    if (grid->coordValid(p)) {
        QString location=QString("%1 / %2").arg(p.x()).arg(p.y());
        if (ui->visOtherGrid->isChecked()) {
            switch (mPaintNext.what) {
            case PaintObject::PaintFloatGrid:
                value = mPaintNext.float_grid->isEmpty()?0: mPaintNext.float_grid->constValueAt(p);
                break;
            case PaintObject::PaintMapGrid:
                value = mPaintNext.map_grid->grid().constValueAt(p);
                break;
            case PaintObject::PaintLayers:
                value = mPaintNext.layered->value(p, mPaintNext.layer_id);
                break;
            default: has_value = false;
            }
            if (has_value) {
                location += QString("\n %1").arg(value);
                ui->fonValue->setText(location);
                return;
            }
        }

        if (ui->visFon->isChecked() || ui->visImpact->isChecked()) {
            if (mPaintNext.what == PaintObject::PaintFloatGrid && mPaintNext.float_grid)
                location += QString("\n %1").arg(mPaintNext.float_grid->constValueAt(p));
            else
                location += QString("\n %1").arg((*grid).valueAt(p));
        }
        if( ui->visDomGrid->isChecked())
            location += QString("\n %1").arg((*mRemoteControl.model()->heightGrid()).valueAt(p).height);
        if( ui->visRegeneration->isChecked())
            location += QString("\n %1").arg(mRegenerationGrid.valueAt(p));

        ui->fonValue->setText(location);
    }
}

void MainWindow::mouseWheel(const QPoint& pos, int steps)
{
    //qDebug() << "mouse-wheel" << steps;
    vp.zoomTo(pos, qMax(1-(2*steps/10.),0.2));
    ui->PaintWidget->update();
}

void MainWindow::mouseDrag(const QPoint& from, const QPoint &to, Qt::MouseButton button)
{
    qDebug() << "drag" << button;
    ui->PaintWidget->setCursor(Qt::CrossCursor);
    // move view area if not dedicately moving around a tree
    if (!wantDrag) {
        vp.moveTo(from, to);
        ui->PaintWidget->update();
        return;
    }
    wantDrag = false;
    qDebug() << "drag from" << from << "to" << to;
    Tree *t = (Tree*) ui->treeChange->property("tree").toInt();
    if (!t)
        return;
    QPointF pos = vp.toWorld(to);
    // calculate new position...
    t->setPosition(pos);
    readwriteCycle();
    ui->PaintWidget->update();
}



void MainWindow::on_actionEdit_XML_settings_triggered()
{
    ui->editStack->setCurrentIndex(0);
    ui->PaintWidget->update();
}

QMutex mutex_yearSimulated;
void MainWindow::yearSimulated(int year)
{
    QMutexLocker mutex_locker(&mutex_yearSimulated);
    checkModelState();
    ui->modelRunProgress->setValue(year);
    labelMessage(QString("Running.... year %1 of %2.").arg(year).arg(mRemoteControl.totalYears()));
    ui->PaintWidget->update();
    QApplication::processEvents();
}

void MainWindow::modelFinished(QString errorMessage)
{
    qDebug() << "Finished!";
    labelMessage("Finished!!");

    checkModelState();
    if (windowTitle().contains("batch")) {
        // we are in automatic batch mode.
        // we should therefore close down the application.
        batchLog(QString("%1 Finished!!! shutting down...").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));

        qDebug() << "****************************";
        qDebug() << "Finished automated model run: " << errorMessage;
        qDebug() << "****************************";

        close(); // shut down the application....

    }
}

/// creates the iLand model
void MainWindow::setupModel()
{
    // load project xml file to global xml settings structure
    mRemoteControl.setFileName(ui->initFileName->text());
    //GlobalSettings::instance()->loadProjectFile(ui->initFileName->text());
    labelMessage("Creating model...");

    // setup logging
    setupFileLogging(true);

    // create the model
    mRemoteControl.create();
    if (!mRemoteControl.canRun())
        return;

    Model *model = mRemoteControl.model();
    if (model && model->isSetup()) {
        // set viewport of paintwidget
        vp = Viewport(model->grid()->metricRect(), ui->PaintWidget->rect());
        ui->PaintWidget->update();
    }
    ui->treeChange->setProperty("tree",0);

    // setup dynamic output
    QString dout = GlobalSettings::instance()->settings().value("output.dynamic.columns");
    mRemoteControl.setupDynamicOutput(dout);
    mRemoteControl.setDynamicOutputEnabled(GlobalSettings::instance()->settings().valueBool("output.dynamic.enabled",false));

    ui->modelRunProgress->setValue(0);
    QSettings().setValue("project/lastxmlfile", ui->initFileName->text());
    // magic debug output number
    GlobalSettings::instance()->setDebugOutput((int) GlobalSettings::instance()->settings().valueDouble("system.settings.debugOutput"));

    // populate the tree species filter list
    ui->speciesFilterBox->clear();
    ui->speciesFilterBox->addItem("<all species>", "");
    QHash<QString, QString> list = mRemoteControl.availableSpecies();
    QHash<QString, QString>::const_iterator i = list.begin();
    while (i!=list.constEnd()) {
        ui->speciesFilterBox->addItem(i.value(), i.key());
        ++i;
    }

    // retrieve the active management script file
    if (mRemoteControl.model()->management())
        ui->scriptActiveScriptFile->setText(QString("loaded: %1").arg(mRemoteControl.model()->management()->scriptFile()));
    labelMessage("Model created. Ready to run.");
    checkModelState();
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
    QString fileName = Helper::fileDialog("select XML-project file", ui->initFileName->text(), "*.xml");
    if (fileName.isEmpty())
        return;
    ui->initFileName->setText(fileName);
    QString xmlFile = Helper::loadTextFile(ui->initFileName->text());
    ui->iniEdit->setPlainText(xmlFile);
    checkModelState();
}

void MainWindow::on_actionTreelist_triggered()
{
    QApplication::clipboard()->setText(dumpTreelist());
    qDebug() << "treelist copied to clipboard.";
}

void MainWindow::on_actionFON_grid_triggered()
{
    //if (!mRemoteControl.isRunning()) return;
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
   QString msg = QString("How many years to run?\nCurrent year: %1.").arg(mRemoteControl.currentYear());
   bool ok;
   int count = QInputDialog::getInt(this, "input value",
                                        msg, 10, 0, 10000, 1, &ok);
   if (!ok)
       return;
   count = count + mRemoteControl.currentYear();
   ui->modelRunProgress->setMaximum(count-1);
   mRemoteControl.run(count);
   // process debug outputs...
   saveDebugOutputs();

}

void MainWindow::on_actionRun_one_year_triggered()
{
   if (!mRemoteControl.canRun())
        return;
   mRemoteControl.runYear();
   GlobalSettings::instance()->outputManager()->save(); // save output tables when stepping single year by year
   labelMessage(QString("Simulated a single year. year %1.").arg(mRemoteControl.currentYear()));

   ui->PaintWidget->update();
   checkModelState();
}

void MainWindow::on_actionReload_triggered()
{
    if (!mRemoteControl.canDestroy())
        return;
    mRemoteControl.destroy();
    setupModel();
}

void MainWindow::on_actionPause_triggered()
{
    mRemoteControl.pause();
    if (!mRemoteControl.isRunning())
        labelMessage("Model execution paused...");

    checkModelState();

    if (!mRemoteControl.isPaused())
        mRemoteControl.continueRun();
}

void MainWindow::on_actionStop_triggered()
{
    mRemoteControl.cancel();
    labelMessage("Model stopped.");
}

void MainWindow::on_actionTree_Partition_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dTreePartition, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}

void MainWindow::on_actionTree_Growth_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dTreeGrowth, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}

void MainWindow::on_actionTree_NPP_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dTreeNPP, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}

void MainWindow::on_actionWater_Output_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dWaterCycle, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}
void MainWindow::on_actionDaily_responses_Output_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dDailyResponses, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}

void MainWindow::on_action_debugEstablishment_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dEstablishment, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";
}

void MainWindow::on_actionSnag_Dynamics_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dCarbonCycle, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";

}

void MainWindow::on_actionPerformance_triggered()
{
    QStringList result = GlobalSettings::instance()->debugDataTable(GlobalSettings::dPerformance, ";");
    QApplication::clipboard()->setText(result.join("\n"));
    qDebug() << "copied" <<  result.count() << "lines of debug data to clipboard.";

}

QImage MainWindow::screenshot()
{
    return ui->PaintWidget->drawImage();
}

/// set the viewport of the main viewing window
/// @p center_point is the point to zoom to (world coordinates), and @p scael_px_per_m is the
/// pixel/m scaling.
void MainWindow::setViewport(QPointF center_point, double scale_px_per_m)
{
    vp.setViewPoint(center_point, scale_px_per_m);

//    double current_px = vp.pixelToMeter(1); // number of meters covered by one pixel
//    if (current_px==0)
//        return;

//    vp.setCenterPoint(center_point);
//    QPoint screen = vp.toScreen(center_point); // screen coordinates of the target point
//    double target_scale = scale_px_per_m / current_px;

//    vp.zoomTo(screen, target_scale);
    ui->PaintWidget->update();
    QCoreApplication::processEvents();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void MainWindow::on_actionImageToClipboard_triggered()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setImage( screenshot() );
    qDebug() << "copied image to clipboard.";
}

void MainWindow::saveDebugOutputs()
{
    // save to files if switch is true
    if (!GlobalSettings::instance()->settings().valueBool("system.settings.debugOutputAutoSave"))
        return;

    QString p = GlobalSettings::instance()->path("debug_", "temp");

    GlobalSettings::instance()->debugDataTable(GlobalSettings::dTreePartition, ";", p + "tree_partition.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dTreeGrowth, ";", p + "tree_growth.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dTreeNPP, ";", p + "tree_npp.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dStandNPP, ";", p + "stand_npp.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dWaterCycle, ";", p + "water_cycle.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dDailyResponses, ";", p + "daily_responses.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dEstablishment, ";", p + "establishment.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dCarbonCycle, ";", p + "carboncycle.csv");
    GlobalSettings::instance()->debugDataTable(GlobalSettings::dPerformance, ";", p + "performance.csv");
    Helper::saveToTextFile(p+"dynamic.csv", mRemoteControl.dynamicOutput());

    qDebug() << "saved debug outputs to" << p;
}

void MainWindow::on_actionSelect_Data_Types_triggered()
{
    int value = GlobalSettings::instance()->currentDebugOutput();
    int newvalue = QInputDialog::getInt(this, "QInputDialog::getText()",
                                        "Enter code for desired outputs: add\n" \
                                        "1 ... Tree NPP\n" \
                                        "2 ... Tree partition\n" \
                                        "4 ... Tree growth (dbh,h)\n" \
                                        "8 ... Standlevel NPP\n" \
                                        "16...Water Cycle\n" \
                                        "32...Daily responses\n" \
                                        "64...Establishment\n" \
                                        "128...Carbon cycle\n" \
                                        "256...Performance\n"
                                        "(e.g.: 5 = NPP + tree growth) or 0 for no debug outputs.", value);
     GlobalSettings::instance()->setDebugOutput(newvalue);
}




// Expression test
void MainWindow::on_pbCalculateExpression_clicked()
{
    QString expr_text=ui->expressionText->text();
    QString expr_filter=ui->expressionFilter->text();
    if (expr_text == "test") {
        Tests t(this);
        int which = QInputDialog::getInt(this, "Which test",
                                        "which test?\n0: expression speed\n1: tree clear\n" \
                                        "2:kill trees\n3: climate\n4: multiple light automation\n" \
                                        "5: species response\n" \
                                        "6: watercycle\n" \
                                        "7: CSV File\n" \
                                        "8: Xml setters\n" \
                                        "9: random functions\n" \
                                        "10: seed dispersal.\n" \
                                        "11: multiple thread expression\n" \
                                        "12: linearized expressions\n" \
                                        "13: establishment\n" \
                                        "14: GridRunner\n" \
                                         "15: Soil (ICBM/2N)\n" \
                                         "16: load Map \n" \
                                         "17: test DEM \n" \
                                         "18: test fire module \n" \
                                         "19: test wind module\n" \
                                         "20: test rumple index",-1);
        switch (which) {
        case 0: t.speedOfExpression();break;
        case 1: t.clearTrees(); break;
        case 2: t.killTrees(); break;
        case 3: t.climate(); break;
        case 4: t.multipleLightRuns(GlobalSettings::instance()->path("automation.xml", "home"));
        case 5: t.climateResponse(); break;
        case 6: t.testWater(); break;
        case 7: t.testCSVFile(); break;
        case 8: t.testXml(); break;
        case 9: t.testRandom(); break;
        case 10: t.testSeedDispersal(); break;
        case 11: t.testMultithreadExecute(); break;
        case 12: t.testLinearExpressions(); break;
        case 13: t.testEstablishment(); break;
        case 14: t.testGridRunner(); break;
        case 15: t.testSoil(); break;
        case 16: t.testMap(); break;
        case 17: t.testDEM(); break;
        case 18: t.testFire(); break;
        case 19: t.testWind(); break;
        case 20: t.testRumple(); break;
        }
        return;
    }
    if (expr_filter.isEmpty())
        expr_filter = "1"; // a constant true expression
    TreeWrapper wrapper;
    Expression expr(expr_text, &wrapper);
    Expression filter(expr_filter, &wrapper);
    AllTreeIterator at(GlobalSettings::instance()->model());
    int totalcount=0;
    QVector<double> datavector;
    try {

        while (Tree *tree=at.next()) {
            wrapper.setTree(tree);
            if (filter.execute()) {
                datavector << expr.execute();
            }
            totalcount++;
        }
    } catch (IException &e) {
        Helper::msg(e.message());
    }
    StatData stats(datavector);
    qDebug() << "Expression:" << expr_text << "filtered" << datavector.count() << "of" << totalcount;
    qDebug() << "sum:" << stats.sum() << "min" << stats.min() << "max" << stats.max() << "average" << stats.mean();
    qDebug() << "P25" << stats.percentile25() << "median" << stats.median() << "P75" << stats.percentile75() << "P90" << stats.percentile(90);

    //qDebug() << "Expression:" << expr_text << "results: count of total: " << count << "/" << totalcount
    //        << "sum:" << sum  << "average:" << (count>0?sum/double(count):0.) << "minval:" << minval << "maxval:" << maxval;
    // add to history
    if (!ui->expressionHistory->currentItem() || ui->expressionHistory->currentItem()->text() != expr_text) {
        ui->expressionHistory->insertItem(0, expr_text);
        ui->expressionHistory->setCurrentRow(0);
    }
}

void MainWindow::on_pbExecExpression_clicked()
{
    // just repaint...
    ui->PaintWidget->update();
}

void MainWindow::on_actionDynamic_Output_triggered()
{
    QApplication::clipboard()->setText(mRemoteControl.dynamicOutput());
    qDebug() << "copied dynamic output to clipboard";
}

void MainWindow::on_actionShow_Debug_Messages_triggered(bool checked)
{
    // enable/disble debug messages
    showDebugMessages=checked;
}

void MainWindow::on_reloadJavaScript_clicked()
{
    if (!GlobalSettings::instance()->model())
        MSGRETURN("no model available.");
    Management *mgmt = GlobalSettings::instance()->model()->management();
    if (!mgmt)
        MSGRETURN("Error: no valid Management object available! (no model created).");
    if (mgmt->scriptFile().isEmpty())
        Helper::msg("no mangement script file specified");
    mgmt->loadScript(mgmt->scriptFile());
    qDebug() << "reloaded" << mgmt->scriptFile();
    ScriptGlobal::scriptOutput = ui->scriptResult;
}

void MainWindow::on_selectJavaScript_clicked()
{
    if (!GlobalSettings::instance()->model())
        return;
    QString fileName = Helper::fileDialog("select a Javascript file:");
    if (fileName.isEmpty())
        return;
    ScriptGlobal::loadScript(fileName);

    ui->scriptActiveScriptFile->setText(QString("loaded: %1").arg(fileName));
    qDebug() << "loaded Javascript file" << fileName;
    ScriptGlobal::scriptOutput = ui->scriptResult;

}

void MainWindow::on_scriptCommand_returnPressed()
{
    QString command = ui->scriptCommand->text();
    if (ui->scriptCommandHistory->currentText() != command) {
        ui->scriptCommandHistory->insertItem(0, command);
        ui->scriptCommandHistory->setCurrentIndex(0);
    }

    qDebug() << "executing" << command;
    try {

        QString result = ScriptGlobal::executeScript(command);
        if (!result.isEmpty()) {
            ui->scriptResult->append(result);
            qDebug() << result;
        }
    } catch(const IException &e) {
        Helper::msg(e.message());
    }
}





void MainWindow::on_actionOutput_table_description_triggered()
{
    QString txt = GlobalSettings::instance()->outputManager()->wikiFormat();
    QApplication::clipboard()->setText(txt);
    qDebug() << "Description copied to clipboard!";
}

void MainWindow::on_actionTimers_triggered()
{
    LogToWindow l;
    DebugTimer::printAllTimers();
}

void MainWindow::on_actionOnline_ressources_triggered()
{
    QDesktopServices::openUrl(QUrl("http://iland.boku.ac.at/"));
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog;
    dialog.exec();
}


/* Logging and filtering of logging */
void MainWindow::on_pbLogToClipboard_clicked()
{
    // copy content of log window to clipboard
    QApplication::clipboard()->setText(ui->logOutput->toPlainText());

}

void MainWindow::on_pbLogClearText_clicked()
{
    ui->logOutput->clear();
    ui->logOutput->setProperty("fullText","");
    ui->pbLogFilterClear->setEnabled(false);
}

void MainWindow::on_pbFilterExecute_clicked()
{
    QStringList lines;
    QString search_for = ui->logFilterExpression->text();
    if (search_for.isEmpty())
        return;
    QString full_content;
    if (ui->logOutput->property("fullText").toString().isEmpty()) {
        full_content = ui->logOutput->toPlainText();
        ui->logOutput->setProperty("fullText",full_content);
    } else
        full_content = ui->logOutput->property("fullText").toString();
    QStringList debugLines = full_content.split("\n");
    int i=0;
    foreach(const QString &line, debugLines) {
        i++; // line counter
        if (line.contains(search_for))
            lines.push_back(QString("%1: %2").arg(i).arg(line) );
    }
    if (lines.count()>0) {
        ui->logOutput->setPlainText(lines.join("\n"));
    } else {
        ui->logOutput->setPlainText("Search term not found!");
    }
    ui->pbLogFilterClear->setEnabled(true);
}

void MainWindow::on_pbLogFilterClear_clicked()
{
    QString text = ui->logOutput->property("fullText").toString();
    if (text.isEmpty())
        return;
    //QString sel = ui->logOutput->textCursor().selectedText();
    //int line = sel.toInt();
    int line = atoi(ui->logOutput->textCursor().block().text().toLocal8Bit());
    ui->logOutput->setPlainText(text);
    ui->logOutput->setProperty("fullText","");
    //int bl = ui->logOutput->document()->findBlockByNumber(line).position();
    ui->logOutput->setTextCursor(QTextCursor(ui->logOutput->document()->findBlockByNumber(line)));
    ui->logOutput->ensureCursorVisible();
    ui->pbLogFilterClear->setEnabled(false);

}

void MainWindow::on_actionClearDebugOutput_triggered()
{
    GlobalSettings::instance()->clearDebugLists();
}


void MainWindow::on_actionDebug_triggered()
{
    //
    QObject *o = QObject::sender();
    int level = o->property("logLevel").toInt();
    ui->actionDebug->setChecked( level == 0);
    ui->actionInfo->setChecked( level == 1);
    ui->actionWarning->setChecked( level == 2);
    ui->actionError->setChecked( level == 3);

    setLogLevel(level);
}



void MainWindow::on_scriptCommandHistory_currentIndexChanged(int index)
{
    if (index>=0)
        ui->scriptCommand->setText(ui->scriptCommandHistory->itemText(index));
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.beginGroup("MainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.endGroup();
    // javascript commands
    settings.beginWriteArray("javascriptCommands");
    int size = qMin(ui->scriptCommandHistory->count(), 15); // max 15 entries in the history
    for (int i=0;i<size; ++i) {
        settings.setArrayIndex(i);
        settings.setValue("item", ui->scriptCommandHistory->itemText(i));
    }
    settings.endArray();
    settings.beginGroup("project");
    settings.setValue("lastxmlfile", ui->initFileName->text());
    settings.endGroup();
}
void MainWindow::readSettings()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCoreApplication::setOrganizationName("iLand");
    QCoreApplication::setOrganizationDomain("iland.boku.ac.at");
    QCoreApplication::setApplicationName("iLand");
    QSettings settings;
    qDebug() << "reading settings from" << settings.fileName();

    // window state and
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());

    // read javascript commands
    int size = settings.beginReadArray("javascriptCommands");
    for (int i=0;i<size; ++i) {
        settings.setArrayIndex(i);
        ui->scriptCommandHistory->addItem(settings.value("item").toString());
    }
    settings.endArray();

}


void MainWindow::on_paintGridBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    ui->visOtherGrid->setChecked(true);
}
