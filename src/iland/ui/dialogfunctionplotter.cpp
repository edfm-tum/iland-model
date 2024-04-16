#include "dialogfunctionplotter.h"
#include "qboxlayout.h"
#include "qlineedit.h"
#include "ui_dialogfunctionplotter.h"
#include "QLineSeries"

#include "expression.h"
#include "exception.h"

DialogFunctionPlotter::DialogFunctionPlotter(const QString& funcExpression, const QString& itemLabel, bool standAlone, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogFunctionPlotter),
    mFuncExpression(funcExpression),
    mItemLabel(itemLabel),
    mStandAlone(standAlone)
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
    mEditFuncExpr->setToolTip("Enter iLand expressions, the first encountered symbol name is used as variable name");
    //setTitle(item->label);

    QPushButton* updateButton = this->findChild<QPushButton *>("buttonUpdate");
    connect(updateButton, &QPushButton::clicked, this, [=] {    mChart->removeAllSeries();
                                                                drawFunction(); });


    QDialogButtonBox* buttonBox = this->findChild<QDialogButtonBox *>("buttonBox");
    if (!mStandAlone) {
        buttonBox->addButton("Apply", QDialogButtonBox::AcceptRole);
        connect(buttonBox, &QDialogButtonBox::accepted, this,
                [=] { emit acceptFunction(mEditFuncExpr->text()); });
    }

    buttonBox->addButton("Close", QDialogButtonBox::RejectRole);

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
    double x, value;

    // setup expression and parse
    Expression expr(mFuncExpression);
    expr.setStrict(false);

    try {
        expr.parse();
        if (!expr.lastError().isEmpty()) {
            ui->labelStatus->setText(expr.lastError());
            return;
        }
    } catch (const IException &e) {
        qWarning() << e.message();
        ui->labelStatus->setText(QString("Error: %1").arg(e.message()));
    return;
    }


    std::array<double, 10> vars;
    QString plot_var("none");
    if (expr.variables().size() > 0) {
        plot_var = expr.variables()[0];
    }
    ui->labelStatus->setText(QString("Plot variable: %1.").arg(plot_var));
    if (expr.variables().size() > 1)
        ui->labelStatus->setText( ui->labelStatus->text() + QString(" - Warning: %1 unused variables!").arg(expr.variables().size()-1) );

    while (i <= mNumPoints)
    {
        x = mMinValue + i*delta;
        vars[0] = x;
        try {
            value = expr.execute(vars.data());
            if (std::isfinite(value))
                series->append(x, value);
        } catch (const IException &e) {
            ui->labelStatus->setText(QString("Error: %1").arg(e.message()));
            break;
        }

        //series->append(x, this->evaluateFunction(mFuncExpression, x));
        i ++;
    }

    mChart->addSeries(series);
    mChart->createDefaultAxes();
    mChart->axes(Qt::Horizontal).first()->setTitleText(plot_var);

}

double DialogFunctionPlotter::evaluateFunction(const QString& expression, double x)
{
    // WR: this uses the Javascript engine for plotting, not the iLand expression engine
    // WR: +: it is pretty wasteful and constructs a JS engine for *every* point that should be plotted!
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
