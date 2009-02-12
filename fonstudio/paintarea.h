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


 protected:
     void paintEvent(QPaintEvent *event);
 };


#endif // PAINTAREA_H
