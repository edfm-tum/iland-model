#include "dialogfunctionplotter.h"
#include "qboxlayout.h"
#include "qlineedit.h"
#include "ui_dialogfunctionplotter.h"
#include "QLineSeries"

DialogFunctionPlotter::DialogFunctionPlotter(const QString& funcExpression, const QString& itemLabel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogFunctionPlotter),
    mItemLabel(itemLabel),
    mFuncExpression(funcExpression)
{
    ui->setupUi(this);
    this->setWindowTitle("Function Plotter: " + mItemLabel);
    //mGraphicView = this->findChild<QGraphicsView *>();
    mChart = new QChart();
    mChart->legend()->hide();

    QVBoxLayout* chartLayout = this->findChild<QVBoxLayout *>("chartLayout");
    mChartView = new QChartView(mChart);
    //mChartView->setRenderHint(QPainter::Antialiasing);

    chartLayout->addWidget(mChartView);

    mEditMaxValue = this->findChild<QLineEdit *>("inputMaxValue");
    mEditMaxValue->setText(QString::number(mMaxValue));
    mEditMinValue = this->findChild<QLineEdit *>("inputMinValue");
    mEditMinValue->setText(QString::number(mMinValue));
    mEditNumPoints = this->findChild<QLineEdit *>("inputNumPoints");
    mEditNumPoints->setText(QString::number(mNumPoints));
    mEditFuncExpr = this->findChild<QLineEdit *>("inputFunction");
    mEditFuncExpr->setText(mFuncExpression);
    mEditFuncExpr->setToolTip("Expressions are based on Javascript. You can use all the functions that are included in the Math module, e. g.: Math.sin, Math.exp, Math.log");
    //setTitle(item->label);

    QPushButton* updateButton = this->findChild<QPushButton *>("buttonUpdate");
    QDialogButtonBox* buttonBox = this->findChild<QDialogButtonBox *>("buttonBox");
    buttonBox->addButton("Apply", QDialogButtonBox::AcceptRole);
    buttonBox->addButton("Close", QDialogButtonBox::RejectRole);

    connect(updateButton, &QPushButton::clicked, this, [=] {    mChart->removeAllSeries();
                                                                drawFunction(); });

    connect(buttonBox, &QDialogButtonBox::accepted, this,
            [=] { emit acceptFunction(mEditFuncExpr->text()); });
    drawFunction();
}

DialogFunctionPlotter::~DialogFunctionPlotter()
{
    delete ui;
}


void DialogFunctionPlotter::drawFunction()
{
    auto series = new QLineSeries;
    mMaxValue = mEditMaxValue->text().toDouble();
    mMinValue = mEditMinValue->text().toDouble();
    mNumPoints = mEditNumPoints->text().toDouble();
    mFuncExpression = mEditFuncExpr->text();

    double delta = (mMaxValue - mMinValue) / (mNumPoints);
    int i = 0;
    double x;

    while (i <= mNumPoints)
    {
        x = mMinValue + i*delta;
        series->append(x, this->evaluateFunction(mFuncExpression, x));
        i ++;
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
