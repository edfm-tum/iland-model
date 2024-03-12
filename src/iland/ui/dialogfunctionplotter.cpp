#include "dialogfunctionplotter.h"
#include "qlineedit.h"
#include "ui_dialogfunctionplotter.h"
#include "QLineSeries"

DialogFunctionPlotter::DialogFunctionPlotter(const QString& funcExpression, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogFunctionPlotter),
    mFuncExpression(funcExpression)
{
    ui->setupUi(this);

//    mGraphicView = this->findChild<QGraphicsView *>();
//    mChart = new QChart();
//    mChart->legend()->hide();

//    this->findChild<QLineEdit *>("inputFunction")->setText(mFuncExpression);

//    drawFunction();
}

DialogFunctionPlotter::~DialogFunctionPlotter()
{
    delete ui;
}

//void DialogFunctionPlotter::drawFunction()
//{
//    auto series = new QLineSeries;
//    double delta = (mMaxValue - mMinValue) / mNumPoints;
//    double x = mMinValue;

//    while (x <= mMaxValue)
//    {
//        series->append(x, evaluateFunction(mFuncExpression, x));
//        x += delta;
//    }

//    mChart->addSeries(series);
//    mChart->createDefaultAxes();
//    mChartView = new QChartView(mChart);
//    mChartView->setRenderHint(QPainter::Antialiasing);

//}

//double evaluateFunction(const QString& expression, double x)
//{
//    QJSEngine engine;
//    QString evalString = QString("(%1)").arg(expression).arg(x); // Construct evaluation string
//    QJSValue result = engine.evaluate(evalString);
//    if (result.isError()) {
//        qWarning() << "Evaluation error:" << result.toString();
//        return std::numeric_limits<double>::quiet_NaN(); // Return NaN on error
//    }
//    return result.toNumber();
//}
