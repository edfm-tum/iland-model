#ifndef DIALOGFUNCTIONPLOTTER_H
#define DIALOGFUNCTIONPLOTTER_H

#include "QChart"
#include <QDialog>
#include "QJSEngine"
#include "qchartview.h"
#include "qlineedit.h"

namespace Ui {
class DialogFunctionPlotter;
}

class DialogFunctionPlotter : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFunctionPlotter(const QString& funcExpression, const QString& itemLabel, bool standAlone=false, QWidget *parent = nullptr);
    ~DialogFunctionPlotter();

    //functions
    //void setTitle(const QString& title);

private:
    Ui::DialogFunctionPlotter *ui;
    //QGraphicsView* mGraphicView;
    QChart* mChart;
    QChartView* mChartView;
    QLineEdit* mEditMaxValue;
    QLineEdit* mEditMinValue;
    QLineEdit* mEditNumPoints;
    QLineEdit* mEditFuncExpr;

    QString mFuncExpression;
    QString mItemLabel;
    double mMinValue = 0;
    double mMaxValue = 1;
    int mNumPoints = 50;
    QString funcName;

    //QJSEngine funcEngine;
    bool mStandAlone;

   // functions
    void drawFunction();
    double evaluateFunction(const QString& expression, double x);

signals:
    void acceptFunction(const QString& funcExpr);
};

#endif // DIALOGFUNCTIONPLOTTER_H
