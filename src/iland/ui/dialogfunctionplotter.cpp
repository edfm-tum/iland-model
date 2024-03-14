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
    this->setWindowTitle("Function Plotter");
    //mGraphicView = this->findChild<QGraphicsView *>();
    mChart = new QChart();
    mChart->legend()->hide();

    QVBoxLayout* chartLayout = this->findChild<QVBoxLayout *>("chartLayout");
    mChartView = new QChartView(mChart);
    //mChartView->setRenderHint(QPainter::Antialiasing);

    chartLayout->addWidget(mChartView);

    this->findChild<QLineEdit *>("inputFunction")->setText(mFuncExpression);

    //setTitle(item->label);

    QPushButton* updateButton = this->findChild<QPushButton *>("buttonUpdate");

    connect(updateButton, &QPushButton::clicked, this, [=] {    mChart->removeAllSeries();
                                                                drawFunction(); });
    drawFunction();
}

DialogFunctionPlotter::~DialogFunctionPlotter()
{
    delete ui;
}


void DialogFunctionPlotter::drawFunction()
{
    auto series = new QLineSeries;
    mMaxValue = this->findChild<QLineEdit *>("inputMaxValue")->text().toDouble();
    mMinValue = this->findChild<QLineEdit *>("inputMinValue")->text().toDouble();
    mFuncExpression = this->findChild<QLineEdit *>("inputFunction")->text();

    double delta = (mMaxValue - mMinValue) / mNumPoints;
    double x = mMinValue;

    while (x <= mMaxValue)
    {
        series->append(x, this->evaluateFunction(mFuncExpression, x));
        x += delta;
    }

    mChart->addSeries(series);
    mChart->createDefaultAxes();

}

double DialogFunctionPlotter::evaluateFunction(const QString& expression, double x)
{
    QJSEngine engine;
    engine.globalObject().setProperty("x", x);
    //QString evalString = QString("(%1)").arg(expression).arg(x); // Construct evaluation string
    QJSValue result = engine.evaluate(expression);
    if (result.isError()) {
        qWarning() << "Evaluation error:" << result.toString();
        return std::numeric_limits<double>::quiet_NaN(); // Return NaN on error
    }
    return result.toNumber();

}
