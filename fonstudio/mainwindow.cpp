#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTGui>
#include <QTXml>

#include <stamp.h>
#include "../core/grid.h"
#include "tree.h"
#include "paintarea.h"
#include "logicexpression.h"

// Helper functions
QString loadFromFile(const QString& fileName)
{
    QFile file(fileName);
     if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
         return "";

     QTextStream s(&file);
     return s.readAll();
}

bool writeToFile(const QString& fileName, const QString& content)
{
    QFile data(fileName);
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);
        out << content;
        return true;
    }
    return false;
}

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

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_applyXML_clicked()
{
    QDomDocument doc;
    QFile file(ui->initFileName->text());
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, "title text", "Cannot open file!");
        return;
    }
    // load for edit
    QString xmlFile = loadFromFile(ui->initFileName->text());
    ui->iniEdit->setPlainText(xmlFile);

    if (!doc.setContent(&file)) {
        file.close();
        QMessageBox::information(this, "title text", "Cannot set content!");
        return;
    }
    file.close();

    // print out the element names of all elements that are direct children
    // of the outermost element.
    QDomElement docElem = doc.documentElement();

    QDomNode params = docElem.firstChildElement("params");
    QDomNode trees = docElem.firstChildElement("trees");
    // get a parameter...
    //int p1 = params.firstChildElement("param1").text().toInt();
    QString stampFile =  params.firstChildElement("stampFile").text();


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
    QString expr_text=params.firstChildElement("expression").text();
    LogicExpression expr(expr_text);
    double value = expr.execute();
    qDebug() << "expression:" << expr_text << "->" <<value;

    QString expr_hScale=params.firstChildElement("hScale").text();
    Tree::hScale.setExpression(expr_hScale);
    Tree::hScale.addVar("height");
    Tree::hScale.addVar("dbh");

    QString expr_rScale=params.firstChildElement("rScale").text();
    Tree::rScale.setExpression(expr_rScale);
    Tree::rScale.addVar("height");
    Tree::rScale.addVar("dbh");

    // setup grid
    int cellsize = params.firstChildElement("cellSize").text().toInt();
    int cellcount = params.firstChildElement("cells").text().toInt();
    if (mGrid) {
        delete mGrid; mGrid=0;
    }
    mGrid = new FloatGrid(cellsize, cellcount, cellcount);
    mGrid->initialize(1.f); // set to unity...

    // Load Trees
    Trees.clear();
    QDomElement n = trees.firstChildElement("tree");
    while (!n.isNull()) {
        Tree tree;
        tree.setDbh( n.attributeNode("dbh").value().toFloat() );
        tree.setHeight( n.attributeNode("height").value().toFloat() );
        tree.setPosition( QPointF(n.attributeNode("x").value().toFloat(), n.attributeNode("y").value().toFloat() ));
        Trees.push_back(tree);

/*        ui->logMessages->append("tree: " \
           " dbh: " + n.attributeNode("dbh").value() +
           "\nheight: " + n.attributeNode("height").value() );*/
        n = n.nextSiblingElement();
    }
    qDebug() << Trees.size() << "trees loaded.";


}

void MainWindow::on_saveFile_clicked()
{
    QString content = ui->iniEdit->toPlainText();
    if (!content.isEmpty())
        writeToFile(ui->initFileName->text(), content);

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
    if (Trees.size()==0 || !mGrid)
        return;
    int ix,iy;
    //QRect posRect;
    //QPoint cell;
    //QPointF cellcoord;
    mGrid->initialize(1.f); // set to unity...
    std::vector<Tree>::iterator tit;
    for (tit=Trees.begin(); tit!=Trees.end(); ++tit) {
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
    if (Trees.size()==0 || !mGrid)
        return;

    std::vector<Tree>::iterator tit;
    float treevalue;
    for (tit=Trees.begin(); tit!=Trees.end(); ++tit) {
        treevalue = (*tit).retrieveValue(mStamp, *mGrid);
        qDebug() << "tree x/y:" << tit->position().x() << "/" << tit->position().y() << " value: " << tit->impact();
    }
    ui->PaintWidget->update();
}


void MainWindow::repaintArea(QPainter &painter)
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
    for (tit=Trees.begin(); tit!=Trees.end(); ++tit) {
        pos = (*tit).position();
        fill_color.setHsv(int((*tit).impact()*240.), 200, 200);
        painter.drawRect(int(pxpermeter*pos.x()-3), int(pxpermeter*pos.y()-3), 7, 7);
        painter.fillRect(int(pxpermeter*pos.x()-2), int(pxpermeter*pos.y()-2), 4, 4, fill_color);
        tit->pxRect = QRect(int(pxpermeter*pos.x()-3), int(pxpermeter*pos.y()-3), 7, 7);
    }

    qDebug() << "repaintArea. maxval:" << maxval;
}

void MainWindow::mouseClick(const QPoint& pos)
{
    //qDebug() << "click on" << pos;
    std::vector<Tree>::iterator tit;
    for (tit=Trees.begin(); tit!=Trees.end(); ++tit) {
        if (tit->pxRect.contains(pos)) {
            qDebug() << "tree x/y:" << tit->position().x() << "/" << tit->position().y() << " impact-value: " << tit->impact();
            break;
        }
    }
}





void MainWindow::on_calcFormula_clicked()
{
    QString expression = ui->formula->text();
    LogicExpression expr(expression);
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
        Trees.push_back(t);
    }

    on_stampTrees_clicked();

}
