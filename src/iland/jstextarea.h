#ifndef JSTEXTAREA_H
#define JSTEXTAREA_H

#include <QWidget>
#include <QTextEdit>

class JSTextArea : public QTextEdit
{
    Q_OBJECT
public:
    explicit JSTextArea(QWidget *parent = 0);

signals:
    void executeJS(QString code);

public slots:
protected:
    virtual void keyPressEvent ( QKeyEvent * event );
};

#endif // JSTEXTAREA_H
