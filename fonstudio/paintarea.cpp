
#include <QtGui>

#include "paintarea.h"

PaintArea::PaintArea(QWidget *parent)
     : QWidget(parent)
 {
     //QTimer *timer = new QTimer(this);
     //connect(timer, SIGNAL(timeout()), this, SLOT(update()));
     //timer->start(1000);

     //setWindowTitle(tr("Analog Clock"));
     resize(200, 200);
 }

void PaintArea::paintEvent(QPaintEvent *)
{
     QPainter painter(this);

     painter.drawRect(0, 0, width()-1, height()-1);
     emit needsPainting(painter);
     //painter.drawPixmap(0, 0,

}

void PaintArea::mousePressEvent ( QMouseEvent * event )
{

    emit mouseClick(event->pos());
     //emit needsPainting(painter);


}
