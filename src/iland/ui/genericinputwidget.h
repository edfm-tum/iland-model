#ifndef GENERICINPUTWIDGET_H
#define GENERICINPUTWIDGET_H

#include <QWidget>

class genericInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit genericInputWidget(QWidget *parent = nullptr, QString inputType = "string", QString inputName = "default");
    //~genericInputWidget();

private:
    QString inputWidget;
    QString variableName;
    void connectFileDialog(QString variableName, QLineEdit *lineEdit);

signals:

};

#endif // GENERICINPUTWIDGET_H
