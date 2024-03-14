#ifndef DIALOGFUNCTIONPLOTTER_H
#define DIALOGFUNCTIONPLOTTER_H

#include "QChart"
#include <QDialog>
#include "QJSEngine"
#include "qchartview.h"

namespace Ui {
class DialogFunctionPlotter;
}

class DialogFunctionPlotter : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFunctionPlotter(const QString& funcExpression, QWidget *parent = nullptr);
    ~DialogFunctionPlotter();

    //functions
    //void setTitle(const QString& title);

private:
    Ui::DialogFunctionPlotter *ui;
    //QGraphicsView* mGraphicView;
    QChart* mChart;
    QChartView* mChartView;

    QString mFuncExpression;
    double mMinValue = 0;
    double mMaxValue = 1;
    int mNumPoints = 500;
    QString funcName;

    //QJSEngine funcEngine;

   // functions
    void drawFunction();
    double evaluateFunction(const QString& expression, double x);
};

#endif // DIALOGFUNCTIONPLOTTER_H
