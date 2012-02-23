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


#include <QtGui>

#include "paintarea.h"

PaintArea::PaintArea(QWidget *parent)
     : QWidget(parent)
 {
     //QTimer *timer = new QTimer(this);
     //connect(timer, SIGNAL(timeout()), this, SLOT(update()));
     //timer->start(1000);

     //setWindowTitle(tr("Analog Clock"));
     //resize(200, 200);
     m_bitmap = QImage(this->size(), QImage::Format_ARGB32);
     // enable mouse tracking
     this->setMouseTracking(true);
     // enable keyboard focus
     setFocusPolicy(Qt::StrongFocus);

 }

void PaintArea::resizeEvent(QResizeEvent *event)
{
    m_bitmap = QImage(this->size(), QImage::Format_ARGB32);
    qDebug() << "paintarea resize" << this->size();
}
void PaintArea::paintEvent(QPaintEvent *)
{

    QPainter painter(this);
    QPainter pxpainter(&m_bitmap);

    //QPainter pxpainter(m_bitmap);
    //painter.drawRect(0, 0, width()-1, height()-1);

     emit needsPainting(pxpainter);
     painter.drawImage(rect(), m_bitmap);
     //painter.drawPixmap(rect(), m_bitmap);
     //painter.drawPixmap(0, 0,
//     QStylePainter spainter(this);
//     QStyleOption opt;
//     spainter.drawPrimitive(QStyle::PE_PanelButtonCommand, opt);

}

void PaintArea::mousePressEvent ( QMouseEvent * event )
{
    m_lastDown = event->pos();
    emit mouseClick(event->pos());
}

void PaintArea::mouseMoveEvent( QMouseEvent * event )
{
    emit mouseMove(event->pos());
}

void PaintArea::wheelEvent ( QWheelEvent * event )
{
    emit mouseWheel(event->pos(), event->delta() / 120);
}

void PaintArea::mouseReleaseEvent ( QMouseEvent * event )
{
    setCursor(Qt::CrossCursor);

    if ( (event->pos()-m_lastDown).manhattanLength() > 3) {
        emit mouseDrag(m_lastDown, event->pos(), event->button());
    }
    //emit mouseClick(event->pos());
     //emit needsPainting(painter);
}

void PaintArea::keyPressEvent(QKeyEvent *event)
{
    // emulate mouse actions with the keyboard
    QPoint mousepos = this->rect().center();
    switch (event->key()) {
        case Qt::Key_Plus:
        case Qt::Key_PageUp:  // zoom in
            emit mouseWheel(mousepos, 1);
            break;

        case Qt::Key_PageDown: // zoom out
        case Qt::Key_Minus:
            emit mouseWheel(mousepos, -1);
            break;
        // pan with cursor keys
        case Qt::Key_Left:
            emit mouseDrag(mousepos, mousepos + QPoint(20, 0), Qt::MidButton); break;
        case Qt::Key_Right:
            emit mouseDrag(mousepos, mousepos + QPoint(-20, 0), Qt::MidButton); break;

        case Qt::Key_Up:
            emit mouseDrag(mousepos, mousepos + QPoint(0, 20), Qt::MidButton); break;
        case Qt::Key_Down:
            emit mouseDrag(mousepos, mousepos + QPoint(0, -20), Qt::MidButton); break;
        }
}
