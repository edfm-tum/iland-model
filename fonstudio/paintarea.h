#ifndef PAINTAREA_H
#define PAINTAREA_H

#include <QWidget>

class PaintArea : public QWidget
 {
     Q_OBJECT

 public:
     PaintArea(QWidget *parent = 0);

signals:
     void needsPainting(QPainter &painter);
     void mouseClick(const QPoint &pos);

 protected:
     void paintEvent(QPaintEvent *event);
     void mousePressEvent ( QMouseEvent * event );
 };


#endif // PAINTAREA_H
