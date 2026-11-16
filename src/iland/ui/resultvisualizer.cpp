/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#include "resultvisualizer.h"
#include "spatialmapwidget.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QAreaSeries>
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QBoxPlotSeries>
#include <QBoxSet>
#include <QCategoryAxis>
#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QGraphicsLayout>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLineSeries>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QProxyStyle>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStackedBarSeries>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStyleOptionSlider>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QValueAxis>

#include <algorithm>
#include <cmath>
#include <limits>

#include "globalsettings.h"
#include "helper.h"
#include "model.h"

namespace {

class DirectJumpSliderStyle : public QProxyStyle
{
public:
    DirectJumpSliderStyle() : QProxyStyle(nullptr) {}

    int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons)
            return Qt::LeftButton | Qt::MiddleButton;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

bool isNearlyIntegerSeries(const QVector<double> &xs)
{
    for (double x : xs) {
        if (std::abs(x - std::round(x)) > 1e-6)
            return false;
    }
    return true;
}

QString formatNumberBrief(double v)
{
    const double av = std::abs(v);
    if (av == 0.0)
        return QStringLiteral("0");
    if (av >= 10000.0)
        return QString::number(v, 'f', 0);
    if (av >= 100.0)
        return QString::number(v, 'f', 1);
    if (av >= 1.0)
        return QString::number(v, 'f', 2);
    return QString::number(v, 'g', 3);
}

QIcon makeColorSwatchIcon(const QColor &color)
{
    QPixmap pm(14, 14);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.fillRect(QRect(1, 1, 12, 12), color);
    p.setPen(QColor(80, 85, 95));
    p.drawRect(QRect(1, 1, 11, 11));
    return QIcon(pm);
}

void clearChartContent(QChart *chart)
{
    if (!chart)
        return;
    chart->removeAllSeries();
    const QList<QAbstractAxis *> axes = chart->axes();
    for (QAbstractAxis *ax : axes) {
        chart->removeAxis(ax);
        delete ax;
    }
}

void computeNiceYBounds(double rawMin, double rawMax, bool includeZeroIfPositive,
                        double &outMin, double &outMax, int &outTickCount)
{
    if (!std::isfinite(rawMin) || !std::isfinite(rawMax)) {
        outMin = 0.0;
        outMax = 1.0;
        outTickCount = 6;
        return;
    }
    if (includeZeroIfPositive && rawMin > 0.0)
        rawMin = 0.0;

    if (rawMax <= rawMin) {
        if (std::abs(rawMin) <= 1e-12) {
            outMin = 0.0;
            outMax = 1.0;
            outTickCount = 6;
            return;
        }
        const double pad = std::abs(rawMin) * 0.2;
        rawMin -= pad;
        rawMax += pad;
    }

    const double span = rawMax - rawMin;
    const double roughStep = span / 5.0;
    const double mag = std::pow(10.0, std::floor(std::log10(roughStep)));
    static const double kMultipliers[] = { 1.0, 2.0, 2.5, 5.0, 10.0, 20.0 };

    double chosenMin = rawMin;
    double chosenMax = rawMax;
    int chosenIntervals = 5;
    bool found = false;

    for (double mult : kMultipliers) {
        const double step = mult * mag;
        if (step <= 0.0)
            continue;
        double nMin = std::floor(rawMin / step + 1e-9) * step;
        double nMax = std::ceil(rawMax / step - 1e-9) * step;
        if (nMax <= nMin)
            nMax = nMin + step;
        const int intervals = qRound((nMax - nMin) / step);
        if (intervals >= 2 && intervals <= 7) {
            chosenMin = (std::abs(nMin) < 1e-12) ? 0.0 : nMin;
            chosenMax = (std::abs(nMax) < 1e-12) ? 0.0 : nMax;
            chosenIntervals = intervals;
            found = true;
            break;
        }
    }

    if (!found) {
        const double step = 5.0 * mag;
        chosenMin = std::floor(rawMin / step) * step;
        chosenMax = std::ceil(rawMax / step) * step;
        if (chosenMax <= chosenMin)
            chosenMax = chosenMin + step;
        chosenIntervals = qBound(2, qRound((chosenMax - chosenMin) / step), 8);
    }

    outMin = chosenMin;
    outMax = chosenMax;
    outTickCount = chosenIntervals + 1;
}

void applyDeterministicYAxis(QValueAxis *axis, double minY, double maxY, bool includeZeroIfPositive = false)
{
    double nMin = 0.0;
    double nMax = 1.0;
    int ticks = 6;
    computeNiceYBounds(minY, maxY, includeZeroIfPositive, nMin, nMax, ticks);
    axis->setTickType(QValueAxis::TicksFixed);
    axis->setRange(nMin, nMax);
    axis->setTickCount(ticks);
}

void configureXAxis(QValueAxis *axX, const QVector<double> &xValues)
{
    if (!axX || xValues.isEmpty())
        return;

    const double minX = xValues.first();
    const double maxX = xValues.last();

    if (maxX <= minX) {
        if (isNearlyIntegerSeries(xValues))
            axX->setLabelFormat(QStringLiteral("%d"));
        axX->setTickType(QValueAxis::TicksFixed);
        axX->setRange(minX - 0.5, maxX + 0.5);
        axX->setTickCount(3);
        return;
    }

    if (isNearlyIntegerSeries(xValues)) {
        if (axX->labelFormat() != QLatin1String("%d"))
            axX->setLabelFormat(QStringLiteral("%d"));
        const double span = maxX - minX;
        if (span <= 10.0) {
            axX->setTickType(QValueAxis::TicksFixed);
            axX->setRange(minX, maxX);
            axX->setTickCount(qBound(2, static_cast<int>(std::round(span)) + 1, 11));
        } else {
            static const double kIntSteps[] = { 2, 5, 10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000 };
            double step = 2.0;
            for (double cand : kIntSteps) {
                if (span / cand <= 8.0) {
                    step = cand;
                    break;
                }
            }
            if (span / step > 8.0)
                step = std::ceil(span / 8.0);
            axX->setRange(minX, maxX);
            axX->setTickType(QValueAxis::TicksDynamic);
            axX->setTickAnchor(0.0);
            axX->setTickInterval(step);
        }
    } else {
        if (axX->labelFormat() != QLatin1String("%.1f"))
            axX->setLabelFormat(QStringLiteral("%.1f"));
        axX->setTickType(QValueAxis::TicksFixed);
        axX->setRange(minX, maxX);
        axX->setTickCount(6);
    }
}

} // namespace

ResultVisualizerWidget::ResultVisualizerWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    reloadMetadataCatalog();
    updateSourceButtons();
}

ResultVisualizerWidget::~ResultVisualizerWidget()
{
    if (mLegendPanel && !mLegendPanel->parentWidget())
        delete mLegendPanel.data();
    if (mOptionsPanel && !mOptionsPanel->parentWidget())
        delete mOptionsPanel.data();
}

void ResultVisualizerWidget::buildUi()
{
    // -----------------------------------------------------------------------
    // Left Dock Panel: Output legend (series list / spatial ramp)
    // -----------------------------------------------------------------------
    mLegendPanel = new QWidget();
    mLegendPanel->setMinimumWidth(180);
    auto *legLayout = new QVBoxLayout(mLegendPanel);
    legLayout->setContentsMargins(6, 6, 6, 6);
    legLayout->setSpacing(4);

    mLegendTitleLabel = new QLabel(QStringLiteral("Output legend"), mLegendPanel);
    QFont legTitleFont = mLegendTitleLabel->font();
    legTitleFont.setBold(true);
    mLegendTitleLabel->setFont(legTitleFont);
    mLegendTitleLabel->setWordWrap(true);
    legLayout->addWidget(mLegendTitleLabel);

    mLegendSubtitleLabel = new QLabel(mLegendPanel);
    mLegendSubtitleLabel->setStyleSheet(QStringLiteral("color: #555;"));
    mLegendSubtitleLabel->setWordWrap(true);
    legLayout->addWidget(mLegendSubtitleLabel);

    mLegendStack = new QStackedWidget(mLegendPanel);
    auto *chartLegendPage = new QWidget(mLegendStack);
    auto *chartLegendLayout = new QVBoxLayout(chartLegendPage);
    chartLegendLayout->setContentsMargins(0, 0, 0, 0);
    chartLegendLayout->setSpacing(4);

    mSeriesLegendList = new QListWidget(chartLegendPage);
    mSeriesLegendList->setSelectionMode(QAbstractItemView::NoSelection);
    mSeriesLegendList->setFocusPolicy(Qt::NoFocus);
    mSeriesLegendList->setIconSize(QSize(14, 14));
    mSeriesLegendList->setUniformItemSizes(true);
    chartLegendLayout->addWidget(mSeriesLegendList, 1);

    mLegendHintLabel = new QLabel(QStringLiteral("Check or uncheck items to show or hide series"), chartLegendPage);
    QFont hintFont = mLegendHintLabel->font();
    hintFont.setPointSizeF(qMax(7.5, hintFont.pointSizeF() - 1.5));
    mLegendHintLabel->setFont(hintFont);
    mLegendHintLabel->setStyleSheet(QStringLiteral("color: #666;"));
    mLegendHintLabel->setWordWrap(true);
    chartLegendLayout->addWidget(mLegendHintLabel);

    mSpatialLegend = new SpatialLegendWidget(mLegendStack);
    mLegendStack->addWidget(chartLegendPage); // index 0
    mLegendStack->addWidget(mSpatialLegend);  // index 1
    legLayout->addWidget(mLegendStack, 1);

    // -----------------------------------------------------------------------
    // Right Dock Panel: DB source + Catalog tree + Options + Actions
    // -----------------------------------------------------------------------
    mOptionsPanel = new QWidget();
    mOptionsPanel->setMinimumWidth(230);
    auto *optPanelLayout = new QVBoxLayout(mOptionsPanel);
    optPanelLayout->setContentsMargins(6, 6, 6, 6);
    optPanelLayout->setSpacing(5);

    mSourceLabel = new QLabel(QStringLiteral("No output DB"), mOptionsPanel);
    mSourceLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    mSourceLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    optPanelLayout->addWidget(mSourceLabel);

    auto *sourceBtnRow = new QHBoxLayout();
    sourceBtnRow->setSpacing(4);
    mOpenDbBtn = new QPushButton(QStringLiteral("Open DB..."), mOptionsPanel);
    mOpenDbBtn->setToolTip(QStringLiteral("Open an existing iLand SQLite output database for inspection"));
    sourceBtnRow->addWidget(mOpenDbBtn);

    mLiveBtn = new QPushButton(QStringLiteral("Live"), mOptionsPanel);
    mLiveBtn->setToolTip(QStringLiteral("Switch back to the currently running or loaded simulation output"));
    sourceBtnRow->addWidget(mLiveBtn);

    mReloadBtn = new QPushButton(QStringLiteral("Reload"), mOptionsPanel);
    mReloadBtn->setToolTip(QStringLiteral("Reload built-in and custom visualizations.json files"));
    sourceBtnRow->addWidget(mReloadBtn);

    mHelpBtn = new QPushButton(QStringLiteral("?"), mOptionsPanel);
    mHelpBtn->setMaximumWidth(28);
    mHelpBtn->setToolTip(QStringLiteral("Show documentation for custom visualizations.json definitions"));
    sourceBtnRow->addWidget(mHelpBtn);
    optPanelLayout->addLayout(sourceBtnRow);

    mSearchEdit = new QLineEdit(mOptionsPanel);
    mSearchEdit->setPlaceholderText(QStringLiteral("Filter visualizations..."));
    mSearchEdit->setClearButtonEnabled(true);
    optPanelLayout->addWidget(mSearchEdit);

    mActiveOnlyCheck = new QCheckBox(QStringLiteral("Active outputs only"), mOptionsPanel);
    mActiveOnlyCheck->setChecked(true);
    mActiveOnlyCheck->setToolTip(QStringLiteral("Hide visualizations whose output table is not present in the current database"));
    optPanelLayout->addWidget(mActiveOnlyCheck);

    mTree = new QTreeWidget(mOptionsPanel);
    mTree->setHeaderHidden(true);
    mTree->setRootIsDecorated(true);
    mTree->setIndentation(14);
    mTree->setMinimumHeight(140);
    optPanelLayout->addWidget(mTree, 1);

    // Options GroupBox
    mOptionsGroup = new QGroupBox(QStringLiteral("Options"), mOptionsPanel);
    auto *optGrid = new QGridLayout(mOptionsGroup);
    optGrid->setContentsMargins(6, 6, 6, 6);
    optGrid->setSpacing(4);
    optGrid->setColumnStretch(1, 1);

    auto configureOptionCombo = [](QComboBox *cb) {
        cb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        cb->setMinimumContentsLength(10);
        cb->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    };

    int row = 0;
    mMetricLabel = new QLabel(QStringLiteral("Metric:"), mOptionsGroup);
    mMetricCombo = new QComboBox(mOptionsGroup);
    configureOptionCombo(mMetricCombo);
    optGrid->addWidget(mMetricLabel, row, 0);
    optGrid->addWidget(mMetricCombo, row++, 1);

    mSpeciesLabel = new QLabel(QStringLiteral("Species:"), mOptionsGroup);
    mSpeciesCombo = new QComboBox(mOptionsGroup);
    configureOptionCombo(mSpeciesCombo);
    optGrid->addWidget(mSpeciesLabel, row, 0);
    optGrid->addWidget(mSpeciesCombo, row++, 1);

    mRuLabel = new QLabel(QStringLiteral("RU:"), mOptionsGroup);
    mRuCombo = new QComboBox(mOptionsGroup);
    configureOptionCombo(mRuCombo);
    mRuCombo->setEditable(true);
    mRuCombo->setInsertPolicy(QComboBox::NoInsert);
    auto *ruCompleter = new QCompleter(mRuCombo->model(), mRuCombo);
    ruCompleter->setFilterMode(Qt::MatchContains);
    ruCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ruCompleter->setCompletionMode(QCompleter::PopupCompletion);
    mRuCombo->setCompleter(ruCompleter);
    optGrid->addWidget(mRuLabel, row, 0);
    optGrid->addWidget(mRuCombo, row++, 1);

    mSecondaryLabel = new QLabel(QStringLiteral("Filter:"), mOptionsGroup);
    mSecondaryCombo = new QComboBox(mOptionsGroup);
    configureOptionCombo(mSecondaryCombo);
    optGrid->addWidget(mSecondaryLabel, row, 0);
    optGrid->addWidget(mSecondaryCombo, row++, 1);

    mPaletteLabel = new QLabel(QStringLiteral("Palette:"), mOptionsGroup);
    mPaletteCombo = new QComboBox(mOptionsGroup);
    configureOptionCombo(mPaletteCombo);
    mPaletteCombo->addItem(QStringLiteral("Turbo"), QStringLiteral("turbo"));
    mPaletteCombo->addItem(QStringLiteral("Greens"), QStringLiteral("greens"));
    mPaletteCombo->addItem(QStringLiteral("Rainbow"), QStringLiteral("rainbow"));
    mPaletteCombo->addItem(QStringLiteral("Blues"), QStringLiteral("blues"));
    mPaletteCombo->addItem(QStringLiteral("Reds"), QStringLiteral("reds"));
    mPaletteCombo->addItem(QStringLiteral("Heat"), QStringLiteral("heat"));
    mPaletteCombo->addItem(QStringLiteral("Gray"), QStringLiteral("gray"));
    optGrid->addWidget(mPaletteLabel, row, 0);
    optGrid->addWidget(mPaletteCombo, row++, 1);

    mScaleModeLabel = new QLabel(QStringLiteral("Scale:"), mOptionsGroup);
    mScaleModeCombo = new QComboBox(mOptionsGroup);
    configureOptionCombo(mScaleModeCombo);
    mScaleModeCombo->addItem(QStringLiteral("Auto (year)"), QStringLiteral("auto"));
    mScaleModeCombo->addItem(QStringLiteral("Auto 98% (year)"), QStringLiteral("auto_p98"));
    mScaleModeCombo->addItem(QStringLiteral("Global (all years)"), QStringLiteral("global"));
    mScaleModeCombo->addItem(QStringLiteral("Manual"), QStringLiteral("manual"));
    optGrid->addWidget(mScaleModeLabel, row, 0);
    optGrid->addWidget(mScaleModeCombo, row++, 1);

    mManualRangeLabel = new QLabel(QStringLiteral("Range:"), mOptionsGroup);
    mManualRangeWidget = new QWidget(mOptionsGroup);
    auto *rngRow = new QHBoxLayout(mManualRangeWidget);
    rngRow->setContentsMargins(0, 0, 0, 0);
    rngRow->setSpacing(4);
    mManualMinSpin = new QDoubleSpinBox(mManualRangeWidget);
    mManualMaxSpin = new QDoubleSpinBox(mManualRangeWidget);
    for (QDoubleSpinBox *sb : { mManualMinSpin, mManualMaxSpin }) {
        sb->setRange(-1e9, 1e9);
        sb->setDecimals(2);
        sb->setKeyboardTracking(false);
        sb->setMinimumWidth(68);
        sb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    rngRow->addWidget(mManualMinSpin, 1);
    rngRow->addWidget(new QLabel(QStringLiteral("–"), mManualRangeWidget));
    rngRow->addWidget(mManualMaxSpin, 1);
    optGrid->addWidget(mManualRangeLabel, row, 0);
    optGrid->addWidget(mManualRangeWidget, row++, 1);

    mPercentCheck = new QCheckBox(QStringLiteral("Normalize to 100%"), mOptionsGroup);
    mPercentCheck->setToolTip(QStringLiteral("Show relative shares (%) summing to 100%"));
    optGrid->addWidget(mPercentCheck, row++, 0, 1, 2);

    mGridFileBtn = new QPushButton(QStringLiteral("Spatial grid..."), mOptionsGroup);
    mGridFileBtn->setToolTip(QStringLiteral("Select a project XML or raster (.asc/.tif) defining resource unit positions"));
    optGrid->addWidget(mGridFileBtn, row++, 0, 1, 2);

    optPanelLayout->addWidget(mOptionsGroup);

    auto *actionRow = new QHBoxLayout();
    actionRow->setSpacing(4);
    mResetZoomBtn = new QPushButton(QStringLiteral("Reset zoom"), mOptionsPanel);
    mCopyImgBtn = new QPushButton(QStringLiteral("Copy image"), mOptionsPanel);
    mCopyCsvBtn = new QPushButton(QStringLiteral("Copy CSV"), mOptionsPanel);
    actionRow->addWidget(mResetZoomBtn);
    actionRow->addWidget(mCopyImgBtn);
    actionRow->addWidget(mCopyCsvBtn);
    optPanelLayout->addLayout(actionRow);

    // -----------------------------------------------------------------------
    // Central Tab Canvas: Year bar (spatial only) + Stacked Chart/Map + Footer
    // -----------------------------------------------------------------------
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->setSpacing(4);

    mSpatialYearBar = new QWidget(this);
    auto *spRow = new QHBoxLayout(mSpatialYearBar);
    spRow->setContentsMargins(4, 0, 4, 0);
    spRow->setSpacing(6);
    spRow->addWidget(new QLabel(QStringLiteral("Year:"), mSpatialYearBar));
    mYearSlider = new QSlider(Qt::Horizontal, mSpatialYearBar);
    auto *sliderStyle = new DirectJumpSliderStyle();
    sliderStyle->setParent(mYearSlider);
    mYearSlider->setStyle(sliderStyle);
    mYearSlider->setSingleStep(1);
    mYearSlider->setTickPosition(QSlider::TicksBelow);
    mYearSlider->setMinimumWidth(140);
    spRow->addWidget(mYearSlider, 1);

    mYearCombo = new QComboBox(mSpatialYearBar);
    mYearCombo->setMinimumWidth(72);
    spRow->addWidget(mYearCombo);

    mFollowLatestCheck = new QCheckBox(QStringLiteral("Follow"), mSpatialYearBar);
    mFollowLatestCheck->setChecked(true);
    mFollowLatestCheck->setToolTip(QStringLiteral("Automatically jump to the newest simulated year"));
    spRow->addWidget(mFollowLatestCheck);
    mSpatialYearBar->setVisible(false);
    rootLayout->addWidget(mSpatialYearBar);

    mCanvasStack = new QStackedWidget(this);
    mChart = new QChart();
    mChart->setAnimationOptions(QChart::NoAnimation);
    mChart->legend()->setVisible(false);
    QFont titleFont = mChart->titleFont();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() + 1.0);
    mChart->setTitleFont(titleFont);

    mChartView = new QChartView(mChart, mCanvasStack);
    mChartView->setRenderHint(QPainter::Antialiasing, true);
    mChartView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    mChartView->setRubberBand(QChartView::RectangleRubberBand);

    mSpatialMap = new SpatialMapWidget(mCanvasStack);
    mCanvasStack->addWidget(mChartView);  // index 0
    mCanvasStack->addWidget(mSpatialMap); // index 1
    rootLayout->addWidget(mCanvasStack, 1);

    auto *footerRow = new QHBoxLayout();
    footerRow->setSpacing(8);
    mDescLabel = new QLabel(this);
    mDescLabel->setStyleSheet(QStringLiteral("color: #555;"));
    mDescLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    footerRow->addWidget(mDescLabel, 1);

    mHoverLabel = new QLabel(this);
    mHoverLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    mHoverLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #222;"));
    footerRow->addWidget(mHoverLabel);
    rootLayout->addLayout(footerRow);

    configureControlsForSpec(nullptr);
    renderEmptyChart(QStringLiteral("Create a model or click 'Open DB...' to visualize outputs."));

    // Connections
    connect(mOpenDbBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onOpenExternalDb);
    connect(mLiveBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onSwitchToLiveDb);
    connect(mReloadBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onReloadMetadata);
    connect(mHelpBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onShowHelp);
    connect(mGridFileBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onSelectCustomGrid);

    connect(mSearchEdit, &QLineEdit::textChanged, this, &ResultVisualizerWidget::onCatalogFilterChanged);
    connect(mActiveOnlyCheck, &QCheckBox::toggled, this, &ResultVisualizerWidget::onCatalogFilterChanged);
    connect(mTree, &QTreeWidget::currentItemChanged, this, &ResultVisualizerWidget::onTreeSelectionChanged);
    connect(mSeriesLegendList, &QListWidget::itemChanged, this, &ResultVisualizerWidget::onLegendItemChanged);

    connect(mMetricCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ResultVisualizerWidget::onSpatialMetricOrSpeciesChanged);
    connect(mSpeciesCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ResultVisualizerWidget::onSpatialMetricOrSpeciesChanged);
    connect(mRuCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ResultVisualizerWidget::onControlValueChanged);
    if (mRuCombo->lineEdit())
        connect(mRuCombo->lineEdit(), &QLineEdit::editingFinished, this, &ResultVisualizerWidget::onControlValueChanged);
    connect(mSecondaryCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ResultVisualizerWidget::onControlValueChanged);
    connect(mPercentCheck, &QCheckBox::toggled, this, &ResultVisualizerWidget::onControlValueChanged);

    connect(mYearSlider, &QSlider::valueChanged, this, &ResultVisualizerWidget::onSpatialYearSliderChanged);
    connect(mYearCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ResultVisualizerWidget::onSpatialYearComboChanged);
    connect(mFollowLatestCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (mUpdatingControls || !checked || mSpatialYears.isEmpty())
            return;
        mUpdatingControls = true;
        const int lastIdx = mSpatialYears.size() - 1;
        mYearSlider->setValue(lastIdx);
        mYearCombo->setCurrentIndex(lastIdx);
        mUpdatingControls = false;
        if (const VizSpec *spec = selectedSpec()) {
            if (spec->isSpatial())
                renderSpatialFrame(*spec);
        }
    });
    connect(mPaletteCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ResultVisualizerWidget::onSpatialPaletteOrScaleChanged);
    connect(mScaleModeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ResultVisualizerWidget::onSpatialPaletteOrScaleChanged);
    connect(mManualMinSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ResultVisualizerWidget::onSpatialPaletteOrScaleChanged);
    connect(mManualMaxSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ResultVisualizerWidget::onSpatialPaletteOrScaleChanged);

    connect(mResetZoomBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onResetZoom);
    connect(mCopyImgBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onCopyImage);
    connect(mCopyCsvBtn, &QPushButton::clicked, this, &ResultVisualizerWidget::onCopyCsv);

    connect(mSpatialMap, &SpatialMapWidget::cellHovered, this, [this](const QString &txt) {
        mHoverLabel->setText(txt);
    });
    connect(mSpatialMap, &SpatialMapWidget::ruClicked, this, [this](int ruIdx, int rid) {
        if (ruIdx >= 0 && rid >= 0)
            mHoverLabel->setText(QString("Selected: RU %1 (rid %2)").arg(ruIdx).arg(rid));
    });
}

void ResultVisualizerWidget::setProjectXmlPath(const QString &xmlPath)
{
    if (mProjectXmlPath == xmlPath)
        return;
    mProjectXmlPath = xmlPath;
    mProvider.setProjectXmlPath(xmlPath);
}

void ResultVisualizerWidget::onModelCreated(const QString &projectXmlPath)
{
    mProjectXmlPath = projectXmlPath;
    mProvider.setProjectXmlPath(projectXmlPath);
    mProvider.setLiveMode();
    mCustomSpatialGridPath.clear();
    mLiveCompletedYearHint = 0; // beforeRun() writes Year 0 outputs
    mGlobalRangeValid = false;

    mSpatialLayout.loadFromLiveModel();
    if (!mSpatialLayout.isValid() && !mProjectXmlPath.isEmpty())
        mSpatialLayout.loadFromFile(mProjectXmlPath);
    mAvailableTables = mProvider.availableTables();

    reloadMetadataCatalog();
    updateSourceButtons();

    if (isVisible()) {
        refreshActiveVisualization();
        mDirty = false;
    } else {
        mDirty = true;
    }
}

void ResultVisualizerWidget::onYearSimulated(int year)
{
    // Model::runYear() increments currentYear right before emitting year(int),
    // so completed simulation data in SQLite extends up to (year - 1).
    mLiveCompletedYearHint = qMax(0, year - 1);
    mGlobalRangeValid = false;

    if (mProvider.isExternalMode())
        return;

    if (!isVisible()) {
        mDirty = true;
        return;
    }

    refreshActiveVisualization(true);
    mDirty = false;
}

void ResultVisualizerWidget::onModelDestroyed()
{
    mLiveCompletedYearHint = -1;
    mGlobalRangeValid = false;
    if (!mProvider.isExternalMode()) {
        mSpatialLayout.clear();
        mAvailableTables.clear();
        populateCatalog();
        updateSourceButtons();
        renderEmptyChart(QStringLiteral("Model destroyed. Create a model or click 'Open DB...' to inspect results."));
    } else {
        updateSourceButtons();
    }
}

void ResultVisualizerWidget::onTabActivated()
{
    if (mDirty) {
        mDirty = false;
        refreshActiveVisualization(true);
    }
}

void ResultVisualizerWidget::onOpenExternalDb()
{
    QString startDir;
    if (!mProvider.externalDbPath().isEmpty())
        startDir = QFileInfo(mProvider.externalDbPath()).absolutePath();
    else if (!mProjectXmlPath.isEmpty())
        startDir = QFileInfo(mProjectXmlPath).absolutePath();

    const QString fileName = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open iLand output database"),
        startDir,
        QStringLiteral("SQLite databases (*.sqlite *.db);;All files (*)"));

    if (fileName.isEmpty())
        return;

    QString err;
    if (!mProvider.openExternalDatabase(fileName, &err)) {
        QMessageBox::warning(this, QStringLiteral("Open output database"), err);
        return;
    }

    mLiveCompletedYearHint = -1;
    mGlobalRangeValid = false;
    mCustomSpatialGridPath.clear();
    mAvailableTables = mProvider.availableTables();

    // Auto-discover spatial grid layout from matching XML in the project
    QString layoutErr;
    mSpatialLayout.autoDiscoverForDb(fileName, mProjectXmlPath, &layoutErr);

    // Reload catalog in case project visualizations.json sits next to the discovered XML
    reloadMetadataCatalog();
    updateSourceButtons();
    refreshActiveVisualization();
}

void ResultVisualizerWidget::onSwitchToLiveDb()
{
    mProvider.closeExternalDatabase();
    mProvider.setLiveMode();
    mGlobalRangeValid = false;
    mCustomSpatialGridPath.clear();

    if (GlobalSettings::instance()->model() && GlobalSettings::instance()->model()->isSetup()) {
        mSpatialLayout.loadFromLiveModel();
        if (!mSpatialLayout.isValid() && !mProjectXmlPath.isEmpty())
            mSpatialLayout.loadFromFile(mProjectXmlPath);
        mLiveCompletedYearHint = qMax(0, GlobalSettings::instance()->currentYear() - 1);
    } else {
        mSpatialLayout.clear();
        mLiveCompletedYearHint = -1;
    }

    mAvailableTables = mProvider.availableTables();
    reloadMetadataCatalog();
    updateSourceButtons();
    refreshActiveVisualization();
}

void ResultVisualizerWidget::onReloadMetadata()
{
    mGlobalRangeValid = false;
    reloadMetadataCatalog();
    refreshActiveVisualization();
}

void ResultVisualizerWidget::onSelectCustomGrid()
{
    QString startDir;
    if (!mProjectXmlPath.isEmpty())
        startDir = QFileInfo(mProjectXmlPath).absolutePath();
    else if (!mProvider.externalDbPath().isEmpty())
        startDir = QFileInfo(mProvider.externalDbPath()).absolutePath();

    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select project XML or RU grid raster"),
        startDir,
        QStringLiteral("Project XML or GIS grid (*.xml *.asc *.tif *.tiff);;All files (*)"));

    if (path.isEmpty())
        return;

    QString err;
    if (!mSpatialLayout.loadFromFile(path, &err)) {
        QMessageBox::warning(this, QStringLiteral("Load spatial layout"), err);
        return;
    }
    mCustomSpatialGridPath = path;
    refreshActiveVisualization();
}

void ResultVisualizerWidget::onShowHelp()
{
    QFile f(QStringLiteral(":/README_visualizations.md"));
    QString md;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        md = QString::fromUtf8(f.readAll());
    else
        md = QStringLiteral("Documentation resource `:/README_visualizations.md` not found.");

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Custom output visualizations guide"));
    dlg.resize(780, 620);
    auto *lay = new QVBoxLayout(&dlg);

    auto *browser = new QTextBrowser(&dlg);
    browser->setOpenExternalLinks(true);
    browser->setMarkdown(md);
    lay->addWidget(browser, 1);

    auto *buttons = new QDialogButtonBox(&dlg);
    auto *saveBtn = buttons->addButton(QStringLiteral("Save guide / template..."), QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, [&]() {
        const QString outPath = QFileDialog::getSaveFileName(
            &dlg, QStringLiteral("Save README_visualizations.md"),
            QDir::home().filePath(QStringLiteral("README_visualizations.md")),
            QStringLiteral("Markdown (*.md);;All files (*)"));
        if (!outPath.isEmpty())
            Helper::saveToTextFile(outPath, md);
    });
    lay->addWidget(buttons);
    dlg.exec();
}

void ResultVisualizerWidget::updateSourceButtons()
{
    mSourceLabel->setText(mProvider.sourceLabel());
    const QString info = mProvider.runInfoText();
    mSourceLabel->setToolTip(mProvider.isExternalMode() ? (mProvider.externalDbPath() + (info.isEmpty() ? "" : "\n" + info)) : info);

    const bool liveDbOpen = QSqlDatabase::contains(QStringLiteral("out")) &&
                            QSqlDatabase::database(QStringLiteral("out"), false).isOpen();
    mLiveBtn->setEnabled(mProvider.isExternalMode() && liveDbOpen);
    updateSpatialControlsVisibility(selectedSpec());
}

void ResultVisualizerWidget::reloadMetadataCatalog()
{
    QStringList warnings;
    mSpecs = VizRegistry::loadAll(mProjectXmlPath, &warnings);
    for (const QString &w : warnings)
        qWarning() << "[ResultVisualizer]" << w;
    if (!warnings.isEmpty())
        mDescLabel->setText(warnings.first());

    mAvailableTables = mProvider.availableTables();
    populateCatalog();
    configureControlsForSpec(selectedSpec());
}

void ResultVisualizerWidget::populateCatalog()
{
    mTree->blockSignals(true);
    mTree->clear();

    const QString search = mSearchEdit->text().trimmed().toLower();
    const bool activeOnly = mActiveOnlyCheck->isChecked() && mProvider.hasValidDatabase();

    QMap<QString, QTreeWidgetItem *> catItems;
    QStringList categoryOrder;
    for (const VizSpec &spec : mSpecs) {
        if (!categoryOrder.contains(spec.category))
            categoryOrder.append(spec.category);
    }

    QTreeWidgetItem *toSelect = nullptr;
    QTreeWidgetItem *firstSelectable = nullptr;

    for (const QString &cat : categoryOrder) {
        QTreeWidgetItem *catItem = nullptr;
        for (const VizSpec &spec : mSpecs) {
            if (spec.category != cat)
                continue;

            const bool tableAvailable = !mProvider.hasValidDatabase() ||
                                        spec.table.isEmpty() ||
                                        mAvailableTables.contains(spec.table);

            if (activeOnly && !tableAvailable)
                continue;

            if (!search.isEmpty()) {
                const bool matches = spec.name.toLower().contains(search) ||
                                     spec.table.toLower().contains(search) ||
                                     spec.description.toLower().contains(search) ||
                                     spec.category.toLower().contains(search);
                if (!matches)
                    continue;
            }

            if (!catItem) {
                catItem = new QTreeWidgetItem(mTree, QStringList() << cat);
                QFont f = catItem->font(0);
                f.setBold(true);
                catItem->setFont(0, f);
                catItem->setFlags(Qt::ItemIsEnabled);
                catItems.insert(cat, catItem);
            }

            QString displayName = spec.name;
            if (spec.sourceTag != QLatin1String("built-in"))
                displayName += QString(" [%1]").arg(spec.sourceTag);

            auto *item = new QTreeWidgetItem(catItem, QStringList() << displayName);
            item->setData(0, Qt::UserRole, spec.id);

            QString tip = spec.description;
            if (!spec.table.isEmpty())
                tip += QString("\nTable: %1").arg(spec.table);
            if (!tableAvailable) {
                item->setForeground(0, QColor(145, 145, 145));
                tip += QString("\n(Not present in current database — enable output.%1 in project XML)").arg(spec.table);
            }
            item->setToolTip(0, tip);

            if (!firstSelectable && tableAvailable)
                firstSelectable = item;
            if (spec.id == mCurrentVizId)
                toSelect = item;
        }
    }

    mTree->expandAll();
    if (!toSelect)
        toSelect = firstSelectable;
    if (toSelect) {
        mTree->setCurrentItem(toSelect);
        mCurrentVizId = toSelect->data(0, Qt::UserRole).toString();
    } else {
        mCurrentVizId.clear();
        configureControlsForSpec(nullptr);
    }
    mTree->blockSignals(false);
}

const VizSpec *ResultVisualizerWidget::selectedSpec() const
{
    if (mCurrentVizId.isEmpty())
        return nullptr;
    for (const VizSpec &s : mSpecs) {
        if (s.id == mCurrentVizId)
            return &s;
    }
    return nullptr;
}

void ResultVisualizerWidget::onCatalogFilterChanged()
{
    const QString prevId = mCurrentVizId;
    populateCatalog();
    if (mCurrentVizId != prevId)
        configureControlsForSpec(selectedSpec());
    refreshActiveVisualization();
}

void ResultVisualizerWidget::onTreeSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    if (!current || current->data(0, Qt::UserRole).isNull())
        return;
    const QString id = current->data(0, Qt::UserRole).toString();
    if (id == mCurrentVizId)
        return;

    mCurrentVizId = id;
    mGlobalRangeValid = false;
    configureControlsForSpec(selectedSpec());
    refreshActiveVisualization();
}

void ResultVisualizerWidget::configureControlsForSpec(const VizSpec *spec)
{
    mUpdatingControls = true;
    mGlobalRangeValid = false;
    mManualRangeSeeded = false;
    mPinnedChartReadout.clear();

    if (!spec) {
        mMetricLabel->setVisible(false);
        mMetricCombo->setVisible(false);
        mSpeciesLabel->setVisible(false);
        mSpeciesCombo->setVisible(false);
        mRuLabel->setVisible(false);
        mRuCombo->setVisible(false);
        mSecondaryLabel->setVisible(false);
        mSecondaryCombo->setVisible(false);
        mPercentCheck->setVisible(false);
        mPaletteLabel->setVisible(false);
        mPaletteCombo->setVisible(false);
        mScaleModeLabel->setVisible(false);
        mScaleModeCombo->setVisible(false);
        mManualRangeLabel->setVisible(false);
        mManualRangeWidget->setVisible(false);
        mGridFileBtn->setVisible(false);
        mOptionsGroup->setVisible(false);
        mSpatialYearBar->setVisible(false);
        mDescLabel->setText(QString());
        mHoverLabel->setText(QString());
        updateLegendHeader(nullptr);
        mUpdatingControls = false;
        return;
    }

    mDescLabel->setText(spec->description);
    mHoverLabel->setText(QString());
    updateLegendHeader(spec);

    // 1. Metric selector
    const bool showMetric = (spec->metricsMode == QLatin1String("select") && spec->metrics.size() > 1);
    mMetricCombo->clear();
    for (int i = 0; i < spec->metrics.size(); ++i)
        mMetricCombo->addItem(spec->metrics[i].label, i);
    mMetricLabel->setVisible(showMetric);
    mMetricCombo->setVisible(showMetric);

    // 2. Percent toggle (stacked charts)
    const bool showPercent = spec->allowPercentToggle && spec->isStacked();
    mPercentCheck->setVisible(showPercent);
    if (!spec->allowPercentToggle)
        mPercentCheck->setChecked(false);

    // 3. Species selector (chart filter OR spatial species selector; dedicated "dominant" omits dropdown)
    const bool isDedicatedDominant = spec->isSpatial() && (spec->speciesMode == QLatin1String("dominant"));
    const bool hasSpeciesFilter = spec->filters.contains(QStringLiteral("species")) || (!spec->speciesMode.isEmpty() && !isDedicatedDominant);
    mSpeciesCombo->clear();
    if (hasSpeciesFilter) {
        if (spec->isSpatial() && spec->speciesMode == QLatin1String("select_with_aggregates"))
            mSpeciesCombo->addItem(QStringLiteral("<Total (all species)>"), QString::fromLatin1(VizDataProvider::kTotalSpeciesToken));
        VizSpec tmpSpec = *spec;
        const QVector<VizFilterOption> opts = mProvider.fetchFilterOptions(tmpSpec, QStringLiteral("species"));
        for (const VizFilterOption &opt : opts)
            mSpeciesCombo->addItem(opt.label, opt.value);
    }
    const bool showSpecies = hasSpeciesFilter && mSpeciesCombo->count() > 0;
    mSpeciesLabel->setVisible(showSpecies);
    mSpeciesCombo->setVisible(showSpecies);
    updateSpatialTotalAvailability(spec);

    // 4. RU / RID selector (for non-spatial per-RU charts)
    const bool hasRuFilter = !spec->isSpatial() && (spec->filters.contains(QStringLiteral("ru")) || spec->filters.contains(QStringLiteral("rid")));
    mRuCombo->clear();
    bool showRu = false;
    if (hasRuFilter) {
        const QVector<VizFilterOption> opts = mProvider.fetchFilterOptions(*spec, QStringLiteral("ru"));
        for (const VizFilterOption &opt : opts)
            mRuCombo->addItem(opt.label, opt.value);
        // Auto-hide RU dropdown if table only contains the landscape aggregate (ru = -1)
        const bool onlyLandscape = (opts.size() == 1 && opts.first().value.toInt() == -1);
        showRu = (!opts.isEmpty() && !onlyLandscape);
        if (!showRu && onlyLandscape) {
            mRuCombo->setCurrentIndex(0);
        } else if (showRu) {
            int firstRuIdx = 0;
            for (int i = 0; i < opts.size(); ++i) {
                if (opts[i].value.toInt() >= 0) {
                    firstRuIdx = i;
                    break;
                }
            }
            mRuCombo->setCurrentIndex(firstRuIdx);
        }
    }
    mRuLabel->setVisible(showRu);
    mRuCombo->setVisible(showRu);

    // 5. Secondary filter (e.g. "id" for abeUnit, "agent" for bite, "reason" for landscape_removed)
    mSecondaryFilterCol.clear();
    for (const QString &f : spec->filters) {
        if (f != QLatin1String("ru") && f != QLatin1String("rid") && f != QLatin1String("species")) {
            mSecondaryFilterCol = f;
            break;
        }
    }
    mSecondaryCombo->clear();
    bool showSecondary = false;
    if (!mSecondaryFilterCol.isEmpty()) {
        const QVector<VizFilterOption> opts = mProvider.fetchFilterOptions(*spec, mSecondaryFilterCol);
        for (const VizFilterOption &opt : opts)
            mSecondaryCombo->addItem(opt.label, opt.value);
        if (!opts.isEmpty()) {
            QString cap = mSecondaryFilterCol;
            cap[0] = cap[0].toUpper();
            mSecondaryLabel->setText(cap + QLatin1Char(':'));
            showSecondary = true;
        }
    }
    mSecondaryLabel->setVisible(showSecondary);
    mSecondaryCombo->setVisible(showSecondary);

    // 6. Spatial controls (in options dock & central year bar)
    const bool isSpatial = spec->isSpatial();
    mSpatialYearBar->setVisible(isSpatial);
    mCanvasStack->setCurrentIndex(isSpatial ? 1 : 0);
    mLegendStack->setCurrentIndex(isSpatial ? 1 : 0);
    if (isSpatial) {
        int palIdx = mPaletteCombo->findData(spec->palette);
        if (palIdx >= 0)
            mPaletteCombo->setCurrentIndex(palIdx);
    }
    updateSpatialControlsVisibility(spec);

    mUpdatingControls = false;
}

void ResultVisualizerWidget::updateSpatialControlsVisibility(const VizSpec *spec)
{
    const bool isSpatial = spec && spec->isSpatial();
    const bool isDominant = isSpatial &&
                            (spec->speciesMode == QLatin1String("dominant") ||
                             (!mSpeciesCombo->isHidden() &&
                              mSpeciesCombo->currentData().toString() == QLatin1String(VizDataProvider::kDominantSpeciesToken)));
    const bool showContinuousScale = isSpatial && !isDominant;
    const bool showManualRange = showContinuousScale &&
                                 (mScaleModeCombo->currentData().toString() == QLatin1String("manual"));
    const bool showGridBtn = isSpatial && mProvider.isExternalMode();

    mPaletteLabel->setVisible(showContinuousScale);
    mPaletteCombo->setVisible(showContinuousScale);
    mScaleModeLabel->setVisible(showContinuousScale);
    mScaleModeCombo->setVisible(showContinuousScale);
    mManualRangeLabel->setVisible(showManualRange);
    mManualRangeWidget->setVisible(showManualRange);
    mGridFileBtn->setVisible(showGridBtn);

    const bool anyOptionVisible = (spec != nullptr) &&
                                  (!mMetricCombo->isHidden() || !mSpeciesCombo->isHidden() || !mRuCombo->isHidden() ||
                                   !mSecondaryCombo->isHidden() || !mPercentCheck->isHidden() || showContinuousScale || showGridBtn);
    mOptionsGroup->setVisible(anyOptionVisible);
}

void ResultVisualizerWidget::updateSpatialTotalAvailability(const VizSpec *spec)
{
    if (!spec || !spec->isSpatial() || spec->metrics.isEmpty())
        return;

    int mIdx = mMetricCombo->currentData().isValid() ? mMetricCombo->currentData().toInt() : 0;
    mIdx = qBound(0, mIdx, spec->metrics.size() - 1);
    const bool allowTotal = (spec->metrics[mIdx].agg != QLatin1String("none"));

    const int totalRow = mSpeciesCombo->findData(QString::fromLatin1(VizDataProvider::kTotalSpeciesToken));
    if (totalRow >= 0) {
        if (auto *model = qobject_cast<QStandardItemModel *>(mSpeciesCombo->model())) {
            if (QStandardItem *item = model->item(totalRow)) {
                item->setEnabled(allowTotal);
            }
        }
        if (!allowTotal && mSpeciesCombo->currentIndex() == totalRow && mSpeciesCombo->count() > totalRow + 1)
            mSpeciesCombo->setCurrentIndex(totalRow + 1);
    }
}

void ResultVisualizerWidget::onControlValueChanged()
{
    if (mUpdatingControls)
        return;
    refreshActiveVisualization();
}

void ResultVisualizerWidget::onSpatialMetricOrSpeciesChanged()
{
    if (mUpdatingControls)
        return;
    const VizSpec *spec = selectedSpec();
    if (spec && spec->isSpatial()) {
        mUpdatingControls = true;
        updateSpatialTotalAvailability(spec);
        updateSpatialControlsVisibility(spec);
        mUpdatingControls = false;
        mGlobalRangeValid = false;
        mManualRangeSeeded = false;
    }
    refreshActiveVisualization();
}

void ResultVisualizerWidget::onSpatialYearSliderChanged(int index)
{
    if (mUpdatingControls)
        return;
    mUpdatingControls = true;
    if (index >= 0 && index < mYearCombo->count())
        mYearCombo->setCurrentIndex(index);
    if (!mSpatialYears.isEmpty() && index >= 0 && index < mSpatialYears.size() - 1 && mFollowLatestCheck->isChecked()) {
        QSignalBlocker blocker(mFollowLatestCheck);
        mFollowLatestCheck->setChecked(false);
    }
    mUpdatingControls = false;

    if (const VizSpec *spec = selectedSpec()) {
        if (spec->isSpatial())
            renderSpatialFrame(*spec);
    }
}

void ResultVisualizerWidget::onSpatialYearComboChanged(int index)
{
    if (mUpdatingControls)
        return;
    mUpdatingControls = true;
    if (index >= 0 && index <= mYearSlider->maximum())
        mYearSlider->setValue(index);
    if (!mSpatialYears.isEmpty() && index >= 0 && index < mSpatialYears.size() - 1 && mFollowLatestCheck->isChecked()) {
        QSignalBlocker blocker(mFollowLatestCheck);
        mFollowLatestCheck->setChecked(false);
    }
    mUpdatingControls = false;

    if (const VizSpec *spec = selectedSpec()) {
        if (spec->isSpatial())
            renderSpatialFrame(*spec);
    }
}

void ResultVisualizerWidget::onSpatialPaletteOrScaleChanged()
{
    if (mUpdatingControls)
        return;
    if (const VizSpec *spec = selectedSpec()) {
        if (spec->isSpatial()) {
            mUpdatingControls = true;
            updateSpatialControlsVisibility(spec);
            mUpdatingControls = false;
            renderSpatialFrame(*spec);
        }
    }
}

void ResultVisualizerWidget::onResetZoom()
{
    if (mCanvasStack->currentIndex() == 1)
        mSpatialMap->resetZoom();
    else
        mChart->zoomReset();
}

QImage ResultVisualizerWidget::grabCurrentImage()
{
    if (mCanvasStack->currentIndex() == 1)
        return mSpatialMap->grabImage();
    return mChartView->grab().toImage();
}

void ResultVisualizerWidget::onCopyImage()
{
    const QImage img = grabCurrentImage();
    if (!img.isNull())
        QApplication::clipboard()->setImage(img);
}

void ResultVisualizerWidget::onCopyCsv()
{
    QString csv;
    if (mCanvasStack->currentIndex() == 1)
        csv = mLastSpatialData.toCsv();
    else
        csv = mLastChartData.toCsv();

    if (!csv.isEmpty()) {
        QApplication::clipboard()->setText(csv);
        mHoverLabel->setText(QStringLiteral("Copied CSV data to clipboard."));
    }
}

void ResultVisualizerWidget::clearAndResetChart()
{
    mActiveChartSignature.clear();
    mPinnedChartReadout.clear();
    mLiveAxisX = nullptr;
    mLiveAxisY = nullptr;
    mLiveLineSeries.clear();
    mLiveAreaSeries.clear();
    clearChartContent(mChart);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void ResultVisualizerWidget::flushChartLayout()
{
    if (mChart->layout())
        mChart->layout()->activate();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    if (mChartView->scene())
        mChartView->scene()->update();
    mChartView->viewport()->update();
}

void ResultVisualizerWidget::updateLegendHeader(const VizSpec *spec, const QString &subtitle)
{
    mLegendTitleLabel->setText(spec ? spec->name : QStringLiteral("Output legend"));
    mLegendSubtitleLabel->setText(subtitle);
    mLegendSubtitleLabel->setVisible(!subtitle.isEmpty());
}

void ResultVisualizerWidget::populateChartLegend(const VizSpec &spec, const VizChartData &data)
{
    mLegendStack->setCurrentIndex(0);
    updateLegendHeader(&spec, data.yAxisTitle);

    if (spec.type == QLatin1String("boxplot")) {
        mActiveLegendSignature.clear();
        mSeriesLegendList->blockSignals(true);
        mSeriesLegendList->clear();
        const QIcon boxIcon = makeColorSwatchIcon(QColor(72, 145, 196, 200));
        for (const VizBoxPoint &bp : data.boxes) {
            const QString text = (bp.count > 0)
                                     ? QString("%1 (n=%2): med %3").arg(bp.categoryLabel).arg(bp.count).arg(formatNumberBrief(bp.median))
                                     : QString("%1: no data").arg(bp.categoryLabel);
            auto *item = new QListWidgetItem(boxIcon, text, mSeriesLegendList);
            item->setFlags(Qt::ItemIsEnabled);
            if (bp.count > 0) {
                item->setToolTip(QString("min=%1  Q25=%2  median=%3  Q75=%4  max=%5")
                                     .arg(formatNumberBrief(bp.min), formatNumberBrief(bp.q25),
                                          formatNumberBrief(bp.median), formatNumberBrief(bp.q75),
                                          formatNumberBrief(bp.max)));
            }
        }
        mSeriesLegendList->blockSignals(false);
        mLegendHintLabel->setText(QStringLiteral("Monthly quantiles (min / Q25 / median / Q75 / max)"));
        mLegendHintLabel->setVisible(true);
        return;
    }

    QStringList sigParts;
    sigParts << spec.id << spec.type;
    for (const VizSeriesData &sd : data.series)
        sigParts << QString("%1=%2@%3").arg(sd.key, sd.label, sd.color.name());
    const QString sig = sigParts.join(QLatin1Char('|'));

    if (sig == mActiveLegendSignature)
        return;
    mActiveLegendSignature = sig;

    const QSet<QString> &hidden = mHiddenSeriesByViz[spec.id];
    mSeriesLegendList->blockSignals(true);
    mSeriesLegendList->clear();
    for (const VizSeriesData &sd : data.series) {
        auto *item = new QListWidgetItem(makeColorSwatchIcon(sd.color), sd.label, mSeriesLegendList);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setData(Qt::UserRole, sd.key);
        item->setCheckState(hidden.contains(sd.key) ? Qt::Unchecked : Qt::Checked);
    }
    mSeriesLegendList->blockSignals(false);
    mLegendHintLabel->setText(QStringLiteral("Check or uncheck items to show or hide series"));
    mLegendHintLabel->setVisible(data.series.size() > 1);
}

void ResultVisualizerWidget::onLegendItemChanged(QListWidgetItem *item)
{
    if (!item)
        return;
    const VizSpec *spec = selectedSpec();
    if (!spec || spec->isSpatial() || spec->type == QLatin1String("boxplot"))
        return;

    const QString key = item->data(Qt::UserRole).toString();
    if (key.isEmpty())
        return;

    QSet<QString> &hidden = mHiddenSeriesByViz[spec->id];
    if (item->checkState() == Qt::Unchecked)
        hidden.insert(key);
    else
        hidden.remove(key);

    if (!mLastChartData.hasError() && !mLastChartData.series.isEmpty())
        renderChartData(*spec, mLastChartData);
}

void ResultVisualizerWidget::showEmptyChartState(const VizSpec *spec, const QString &chartTitle,
                                                 const QString &hoverError, bool clearLegendList)
{
    mCanvasStack->setCurrentIndex(0);
    clearAndResetChart();
    mChart->setTitle(chartTitle);
    mChart->legend()->setVisible(false);
    mHoverLabel->setText(hoverError);

    mLegendStack->setCurrentIndex(0);
    updateLegendHeader(spec);
    if (clearLegendList) {
        mActiveLegendSignature.clear();
        mSeriesLegendList->blockSignals(true);
        mSeriesLegendList->clear();
        mSeriesLegendList->blockSignals(false);
        mLegendHintLabel->setVisible(false);
    }
    flushChartLayout();
}

void ResultVisualizerWidget::refreshActiveVisualization(bool isLiveYearAdvance)
{
    const VizSpec *spec = selectedSpec();
    if (!spec) {
        renderEmptyChart(mProvider.hasValidDatabase()
                             ? QStringLiteral("No visualization selected (or matching table is inactive).")
                             : QStringLiteral("Create a model or click 'Open DB...' to visualize outputs."));
        return;
    }

    if (!mProvider.hasValidDatabase()) {
        if (spec->isSpatial()) {
            mCanvasStack->setCurrentIndex(1);
            mLegendStack->setCurrentIndex(1);
            updateLegendHeader(spec);
            const QString msg = QStringLiteral("Create a model or click 'Open DB...' to visualize spatial outputs.");
            mSpatialMap->setEmptyMessage(msg);
            mSpatialLegend->clearLegend(QStringLiteral("No active DB"));
        } else {
            renderEmptyChart(QStringLiteral("Create a model or click 'Open DB...' to visualize outputs."));
        }
        return;
    }

    if (!spec->table.isEmpty() && !mAvailableTables.contains(spec->table)) {
        const QString msg = QString("Output table '%1' is not present in the current database.\nEnable <output><%1><enabled>true</enabled> in the project XML.")
                                .arg(spec->table);
        if (spec->isSpatial()) {
            mCanvasStack->setCurrentIndex(1);
            mLegendStack->setCurrentIndex(1);
            updateLegendHeader(spec);
            mSpatialMap->setEmptyMessage(msg);
            mSpatialLegend->clearLegend(QString("Table '%1' disabled").arg(spec->table));
        } else {
            renderEmptyChart(msg);
        }
        return;
    }

    if (spec->isSpatial()) {
        mCanvasStack->setCurrentIndex(1);
        refreshSpatialYearsAndFrame(*spec, isLiveYearAdvance);
    } else {
        mCanvasStack->setCurrentIndex(0);
        renderChart(*spec);
    }
}

void ResultVisualizerWidget::renderEmptyChart(const QString &message)
{
    mLastChartData = VizChartData();
    showEmptyChartState(selectedSpec(), message, QString(), true);
}

void ResultVisualizerWidget::renderChart(const VizSpec &spec)
{
    // If filters were empty (e.g. at year 0 before first rows were written), retry populating once rows arrive
    const bool needsRu = spec.filters.contains(QStringLiteral("ru")) || spec.filters.contains(QStringLiteral("rid"));
    const bool needsSpecies = spec.filters.contains(QStringLiteral("species"));
    const bool needsSecondary = !mSecondaryFilterCol.isEmpty();
    if ((needsRu && mRuCombo->count() == 0) ||
        (needsSpecies && mSpeciesCombo->count() == 0) ||
        (needsSecondary && mSecondaryCombo->count() == 0)) {
        configureControlsForSpec(&spec);
    }

    if (needsRu && mRuCombo->count() == 0) {
        renderEmptyChart(QString("No per-RU data in '%1' (check conditionRU, or use the landscape visualization).").arg(spec.table));
        return;
    }
    if (needsSpecies && mSpeciesCombo->count() == 0) {
        renderEmptyChart(QString("No species data yet in '%1'.").arg(spec.table));
        return;
    }
    if (needsSecondary && mSecondaryCombo->count() == 0) {
        renderEmptyChart(QString("No data yet in '%1'.").arg(spec.table));
        return;
    }

    QMap<QString, QVariant> filters;
    if (mRuCombo->isVisible() || needsRu) {
        QVariant ruVal = mRuCombo->currentData();
        if (!ruVal.isValid() && mRuCombo->count() > 0) {
            int idx = mRuCombo->findText(mRuCombo->currentText(), Qt::MatchFixedString);
            if (idx >= 0)
                ruVal = mRuCombo->itemData(idx);
            else {
                bool ok = false;
                int typed = mRuCombo->currentText().trimmed().toInt(&ok);
                if (ok)
                    ruVal = typed;
            }
        }
        if (ruVal.isValid())
            filters.insert(QStringLiteral("ru"), ruVal);
    }

    if (mSpeciesCombo->isVisible() && mSpeciesCombo->currentData().isValid())
        filters.insert(QStringLiteral("species"), mSpeciesCombo->currentData());

    if (mSecondaryCombo->isVisible() && !mSecondaryFilterCol.isEmpty() && mSecondaryCombo->currentData().isValid())
        filters.insert(mSecondaryFilterCol, mSecondaryCombo->currentData());

    const int metricIdx = mMetricCombo->currentData().isValid() ? mMetricCombo->currentData().toInt() : 0;
    const bool normPct = mPercentCheck->isVisible() && mPercentCheck->isChecked();

    mLastChartData = mProvider.fetchChartData(spec, metricIdx, filters, normPct, mLiveCompletedYearHint);

    if (mLastChartData.hasError()) {
        showEmptyChartState(&spec, QString("%1 — Query error").arg(spec.name), mLastChartData.errorMessage, true);
        return;
    }

    if (spec.type == QLatin1String("boxplot")) {
        if (mLastChartData.boxes.isEmpty()) {
            showEmptyChartState(&spec, QString("%1 (no data yet)").arg(spec.name), QString(), true);
            return;
        }
        renderChartData(spec, mLastChartData);
        return;
    }

    if (mLastChartData.xValues.isEmpty() || mLastChartData.series.isEmpty()) {
        showEmptyChartState(&spec, QString("%1 (no data yet)").arg(spec.name), QString(), true);
        return;
    }

    renderChartData(spec, mLastChartData);
}

void ResultVisualizerWidget::renderChartData(const VizSpec &spec, const VizChartData &data)
{
    mChart->legend()->setVisible(false);
    populateChartLegend(spec, data);

    if (spec.type == QLatin1String("boxplot")) {
        renderBoxPlotChart(spec, data);
        return;
    }

    const QSet<QString> &hidden = mHiddenSeriesByViz[spec.id];
    QVector<const VizSeriesData *> visibleSeries;
    visibleSeries.reserve(data.series.size());
    for (const VizSeriesData &sd : data.series) {
        if (!hidden.contains(sd.key))
            visibleSeries.append(&sd);
    }

    if (visibleSeries.isEmpty()) {
        showEmptyChartState(&spec, QString("%1 (all series hidden)").arg(spec.name), QString(), false);
        return;
    }

    if (spec.type == QLatin1String("stacked_area"))
        renderStackedAreaChart(spec, data, visibleSeries);
    else if (spec.type == QLatin1String("bar") || spec.type == QLatin1String("stacked_bar"))
        renderBarChart(spec, data, visibleSeries);
    else
        renderLineChart(spec, data, visibleSeries);
}

void ResultVisualizerWidget::renderLineChart(const VizSpec &spec, const VizChartData &data,
                                             const QVector<const VizSeriesData *> &visibleSeries)
{
    QStringList keyParts;
    for (const VizSeriesData *sd : visibleSeries)
        keyParts << QString("%1@%2").arg(sd->key, sd->color.name());
    const QString sig = QString("%1|%2|%3|%4|%5|%6")
                            .arg(spec.id, spec.type, spec.name, data.xAxisTitle, data.yAxisTitle, keyParts.join(QLatin1Char(',')));

    const bool canUpdateInPlace = (sig == mActiveChartSignature && mLiveAxisX && mLiveAxisY &&
                                   mLiveLineSeries.size() == visibleSeries.size());

    if (!canUpdateInPlace) {
        clearAndResetChart();
        mChart->setTitle(spec.name);
        mLiveAxisX = new QValueAxis();
        mLiveAxisY = new QValueAxis();
        mLiveAxisX->setTitleText(data.xAxisTitle);
        mLiveAxisY->setTitleText(data.yAxisTitle);
        mChart->addAxis(mLiveAxisX, Qt::AlignBottom);
        mChart->addAxis(mLiveAxisY, Qt::AlignLeft);

        for (const VizSeriesData *sd : visibleSeries) {
            auto *ls = new QLineSeries();
            ls->setName(sd->label);
            QPen pen(sd->color);
            pen.setWidthF(2.0);
            ls->setPen(pen);

            const QString sLabel = sd->label;
            const QString xTitle = data.xAxisTitle;
            connect(ls, &QLineSeries::hovered, this, [this, sLabel, xTitle](const QPointF &pt, bool state) {
                if (state)
                    mHoverLabel->setText(QString("%1 | %2=%3: %4")
                                             .arg(sLabel, xTitle, formatNumberBrief(pt.x()), formatNumberBrief(pt.y())));
                else
                    mHoverLabel->clear();
            });

            mChart->addSeries(ls);
            ls->attachAxis(mLiveAxisX);
            ls->attachAxis(mLiveAxisY);
            mLiveLineSeries.append(ls);
        }
        mActiveChartSignature = sig;
    }

    double minY = std::numeric_limits<double>::max();
    double maxY = -std::numeric_limits<double>::max();
    bool hasAnyY = false;

    for (int sIdx = 0; sIdx < visibleSeries.size(); ++sIdx) {
        const VizSeriesData *sd = visibleSeries[sIdx];
        QList<QPointF> pts;
        pts.reserve(data.xValues.size());
        for (int i = 0; i < data.xValues.size(); ++i) {
            const double y = sd->y[i];
            if (!std::isfinite(y))
                continue;
            pts.append(QPointF(data.xValues[i], y));
            minY = qMin(minY, y);
            maxY = qMax(maxY, y);
            hasAnyY = true;
        }
        mLiveLineSeries[sIdx]->replace(pts);
    }

    configureXAxis(mLiveAxisX, data.xValues);
    if (hasAnyY)
        applyDeterministicYAxis(mLiveAxisY, minY, maxY, minY >= 0.0);
    else {
        mLiveAxisY->setTickType(QValueAxis::TicksFixed);
        mLiveAxisY->setRange(0.0, 1.0);
        mLiveAxisY->setTickCount(6);
    }

    flushChartLayout();
}

QString ResultVisualizerWidget::formatStackedAreaReadout(const QString &seriesKey, const QString &seriesLabel, double xCoord) const
{
    if (mLastChartData.xValues.isEmpty())
        return QString();

    int bestIdx = 0;
    double bestDist = std::abs(mLastChartData.xValues[0] - xCoord);
    for (int i = 1; i < mLastChartData.xValues.size(); ++i) {
        const double d = std::abs(mLastChartData.xValues[i] - xCoord);
        if (d < bestDist) {
            bestDist = d;
            bestIdx = i;
        }
    }

    const QSet<QString> hidden = mHiddenSeriesByViz.value(mCurrentVizId);
    double val = 0.0;
    double visibleTotal = 0.0;
    for (const VizSeriesData &sd : mLastChartData.series) {
        if (hidden.contains(sd.key) || bestIdx >= sd.y.size())
            continue;
        const double v = (std::isfinite(sd.y[bestIdx]) && sd.y[bestIdx] > 0.0) ? sd.y[bestIdx] : 0.0;
        visibleTotal += v;
        if (sd.key == seriesKey)
            val = std::isfinite(sd.y[bestIdx]) ? sd.y[bestIdx] : 0.0;
    }

    QString xStr;
    if (bestIdx < mLastChartData.xTickLabels.size() && !mLastChartData.xTickLabels[bestIdx].isEmpty())
        xStr = mLastChartData.xTickLabels[bestIdx];
    else if (isNearlyIntegerSeries(mLastChartData.xValues))
        xStr = QString::number(qRound(mLastChartData.xValues[bestIdx]));
    else
        xStr = formatNumberBrief(mLastChartData.xValues[bestIdx]);

    if (!mPercentCheck->isHidden() && mPercentCheck->isChecked()) {
        return QString("%1 | %2=%3: %4%")
            .arg(seriesLabel, mLastChartData.xAxisTitle, xStr, QString::number(val, 'f', 1));
    }

    const double pct = (visibleTotal > 0.0) ? (qMax(0.0, val) / visibleTotal * 100.0) : 0.0;
    return QString("%1 | %2=%3: %4 (%5%)")
        .arg(seriesLabel, mLastChartData.xAxisTitle, xStr, formatNumberBrief(val), QString::number(pct, 'f', 1));
}

void ResultVisualizerWidget::renderStackedAreaChart(const VizSpec &spec, const VizChartData &data,
                                                    const QVector<const VizSeriesData *> &visibleSeries)
{
    QStringList keyParts;
    for (const VizSeriesData *sd : visibleSeries)
        keyParts << QString("%1@%2").arg(sd->key, sd->color.name());
    const QString sig = QString("%1|%2|%3|%4|%5|%6")
                            .arg(spec.id, spec.type, spec.name, data.xAxisTitle, data.yAxisTitle, keyParts.join(QLatin1Char(',')));

    const bool canUpdateInPlace = (sig == mActiveChartSignature && mLiveAxisX && mLiveAxisY &&
                                   mLiveAreaSeries.size() == visibleSeries.size());

    if (!canUpdateInPlace) {
        clearAndResetChart();
        mChart->setTitle(spec.name);
        mLiveAxisX = new QValueAxis();
        mLiveAxisY = new QValueAxis();
        mLiveAxisX->setTitleText(data.xAxisTitle);
        mLiveAxisY->setTitleText(data.yAxisTitle);
        mChart->addAxis(mLiveAxisX, Qt::AlignBottom);
        mChart->addAxis(mLiveAxisY, Qt::AlignLeft);

        for (const VizSeriesData *sd : visibleSeries) {
            auto *area = new QAreaSeries();
            area->setName(sd->label);

            // Parent boundary series directly to QAreaSeries so removeAllSeries() cleans them up safely
            auto *lower = new QLineSeries(area);
            auto *upper = new QLineSeries(area);
            area->setLowerSeries(lower);
            area->setUpperSeries(upper);

            QColor fill = sd->color;
            fill.setAlpha(210);
            area->setBrush(QBrush(fill));
            area->setPen(QPen(sd->color.darker(115), 1.1));

            const QString sKey = sd->key;
            const QString sLabel = sd->label;
            connect(area, &QAreaSeries::hovered, this, [this, sKey, sLabel](const QPointF &pt, bool state) {
                if (state)
                    mHoverLabel->setText(formatStackedAreaReadout(sKey, sLabel, pt.x()));
                else
                    mHoverLabel->setText(mPinnedChartReadout);
            });
            connect(area, &QAreaSeries::clicked, this, [this, sKey, sLabel](const QPointF &pt) {
                mPinnedChartReadout = formatStackedAreaReadout(sKey, sLabel, pt.x());
                mHoverLabel->setText(mPinnedChartReadout);
            });

            mChart->addSeries(area);
            area->attachAxis(mLiveAxisX);
            area->attachAxis(mLiveAxisY);
            mLiveAreaSeries.append(area);
        }
        mActiveChartSignature = sig;
    }

    const int nx = data.xValues.size();
    QVector<double> cum(nx, 0.0);
    double maxY = 0.0;

    for (int sIdx = 0; sIdx < visibleSeries.size(); ++sIdx) {
        const VizSeriesData *sd = visibleSeries[sIdx];
        QList<QPointF> lowerPts;
        QList<QPointF> upperPts;
        lowerPts.reserve(nx);
        upperPts.reserve(nx);

        for (int i = 0; i < nx; ++i) {
            const double val = (std::isfinite(sd->y[i]) && sd->y[i] > 0.0) ? sd->y[i] : 0.0;
            lowerPts.append(QPointF(data.xValues[i], cum[i]));
            cum[i] += val;
            upperPts.append(QPointF(data.xValues[i], cum[i]));
            maxY = qMax(maxY, cum[i]);
        }

        mLiveAreaSeries[sIdx]->lowerSeries()->replace(lowerPts);
        mLiveAreaSeries[sIdx]->upperSeries()->replace(upperPts);
    }

    configureXAxis(mLiveAxisX, data.xValues);
    if (mPercentCheck->isVisible() && mPercentCheck->isChecked()) {
        mLiveAxisY->setTickType(QValueAxis::TicksFixed);
        mLiveAxisY->setRange(0.0, 100.0);
        mLiveAxisY->setTickCount(6);
    } else {
        applyDeterministicYAxis(mLiveAxisY, 0.0, maxY, true);
    }

    flushChartLayout();
}

void ResultVisualizerWidget::renderBarChart(const VizSpec &spec, const VizChartData &data,
                                            const QVector<const VizSeriesData *> &visibleSeries)
{
    clearAndResetChart();
    mChart->setTitle(spec.name);

    const bool isStacked = (spec.type == QLatin1String("stacked_bar"));
    QAbstractBarSeries *barSeries = isStacked
                                        ? static_cast<QAbstractBarSeries *>(new QStackedBarSeries())
                                        : static_cast<QAbstractBarSeries *>(new QBarSeries());

    const int nx = data.xValues.size();
    QVector<double> stackTotals(nx, 0.0);
    double maxY = 0.0;
    double minY = 0.0;

    for (const VizSeriesData *sd : visibleSeries) {
        auto *set = new QBarSet(sd->label);
        set->setColor(sd->color);
        set->setBorderColor(sd->color.darker(120));

        for (int i = 0; i < nx; ++i) {
            const double val = std::isfinite(sd->y[i]) ? sd->y[i] : 0.0;
            *set << val;
            if (isStacked) {
                stackTotals[i] += qMax(0.0, val);
                maxY = qMax(maxY, stackTotals[i]);
            } else {
                maxY = qMax(maxY, val);
                minY = qMin(minY, val);
            }
        }
        barSeries->append(set);
    }

    QStringList cats;
    cats.reserve(nx);
    const bool intX = isNearlyIntegerSeries(data.xValues);
    for (int i = 0; i < nx; ++i) {
        if (i < data.xTickLabels.size() && !data.xTickLabels[i].isEmpty())
            cats.append(data.xTickLabels[i]);
        else if (intX)
            cats.append(QString::number(qRound(data.xValues[i])));
        else
            cats.append(formatNumberBrief(data.xValues[i]));
    }

    const QString xTitle = data.xAxisTitle;
    const bool isPct = isStacked && !mPercentCheck->isHidden() && mPercentCheck->isChecked();
    auto formatBarReadout = [cats, xTitle, isStacked, stackTotals, isPct](int index, QBarSet *barset) -> QString {
        if (!barset || index < 0 || index >= cats.size())
            return QString();
        const double val = barset->at(index);
        if (isPct)
            return QString("%1 | %2=%3: %4%").arg(barset->label(), xTitle, cats[index], QString::number(val, 'f', 1));
        if (isStacked && index < stackTotals.size() && stackTotals[index] > 0.0) {
            const double pct = qMax(0.0, val) / stackTotals[index] * 100.0;
            return QString("%1 | %2=%3: %4 (%5%)")
                .arg(barset->label(), xTitle, cats[index], formatNumberBrief(val), QString::number(pct, 'f', 1));
        }
        return QString("%1 | %2=%3: %4").arg(barset->label(), xTitle, cats[index], formatNumberBrief(val));
    };
    connect(barSeries, &QAbstractBarSeries::hovered, this, [this, formatBarReadout](bool status, int index, QBarSet *barset) {
        if (status) {
            const QString msg = formatBarReadout(index, barset);
            if (!msg.isEmpty())
                mHoverLabel->setText(msg);
        } else {
            mHoverLabel->setText(mPinnedChartReadout);
        }
    });
    connect(barSeries, &QAbstractBarSeries::clicked, this, [this, formatBarReadout](int index, QBarSet *barset) {
        const QString msg = formatBarReadout(index, barset);
        if (!msg.isEmpty()) {
            mPinnedChartReadout = msg;
            mHoverLabel->setText(msg);
        }
    });

    mChart->addSeries(barSeries);

    QAbstractAxis *axX = nullptr;
    if (cats.size() <= 25) {
        auto *catAxis = new QBarCategoryAxis();
        catAxis->append(cats);
        axX = catAxis;
    } else {
        auto *sparseAxis = new QCategoryAxis();
        sparseAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
        sparseAxis->setStartValue(-0.5);
        sparseAxis->setRange(-0.5, cats.size() - 0.5);
        const int step = qMax(1, cats.size() / 9);
        for (int i = 0; i < cats.size(); i += step)
            sparseAxis->append(cats[i], static_cast<qreal>(i));
        if ((cats.size() - 1) % step > step / 2)
            sparseAxis->append(cats.last(), static_cast<qreal>(cats.size() - 1));
        axX = sparseAxis;
    }
    axX->setTitleText(data.xAxisTitle);

    auto *axY = new QValueAxis();
    axY->setTitleText(data.yAxisTitle);

    mChart->addAxis(axX, Qt::AlignBottom);
    mChart->addAxis(axY, Qt::AlignLeft);
    barSeries->attachAxis(axX);
    barSeries->attachAxis(axY);

    if (isStacked && mPercentCheck->isVisible() && mPercentCheck->isChecked()) {
        axY->setTickType(QValueAxis::TicksFixed);
        axY->setRange(0.0, 100.0);
        axY->setTickCount(6);
    } else {
        applyDeterministicYAxis(axY, minY, maxY, minY >= 0.0);
    }

    flushChartLayout();
}

void ResultVisualizerWidget::renderBoxPlotChart(const VizSpec &spec, const VizChartData &data)
{
    clearAndResetChart();
    mChart->setTitle(spec.name);

    auto *boxSeries = new QBoxPlotSeries();
    boxSeries->setBoxOutlineVisible(true);
    boxSeries->setBoxWidth(0.55);
    boxSeries->setBrush(QBrush(QColor(72, 145, 196, 165)));
    boxSeries->setPen(QPen(QColor(30, 80, 120), 1.4));

    QStringList cats;
    double minY = std::numeric_limits<double>::max();
    double maxY = -std::numeric_limits<double>::max();
    bool hasAny = false;
    QHash<QBoxSet *, VizBoxPoint> metaBySet;

    for (const VizBoxPoint &bp : data.boxes) {
        cats.append(bp.categoryLabel);
        auto *set = new QBoxSet(bp.min, bp.q25, bp.median, bp.q75, bp.max, bp.categoryLabel);
        if (bp.count == 0) {
            set->setBrush(Qt::transparent);
            set->setPen(QPen(Qt::transparent));
        } else {
            minY = qMin(minY, bp.min);
            maxY = qMax(maxY, bp.max);
            hasAny = true;
        }
        metaBySet.insert(set, bp);
        boxSeries->append(set);
    }

    connect(boxSeries, &QBoxPlotSeries::hovered, this, [this, metaBySet](bool status, QBoxSet *boxset) {
        if (!status || !boxset || !metaBySet.contains(boxset)) {
            mHoverLabel->clear();
            return;
        }
        const VizBoxPoint &bp = metaBySet.value(boxset);
        if (bp.count == 0) {
            mHoverLabel->setText(QString("%1 | no data").arg(bp.categoryLabel));
            return;
        }
        mHoverLabel->setText(QString("%1 (n=%2) | min=%3  Q25=%4  med=%5  Q75=%6  max=%7")
                                 .arg(bp.categoryLabel)
                                 .arg(bp.count)
                                 .arg(formatNumberBrief(bp.min),
                                      formatNumberBrief(bp.q25),
                                      formatNumberBrief(bp.median),
                                      formatNumberBrief(bp.q75),
                                      formatNumberBrief(bp.max)));
    });

    mChart->addSeries(boxSeries);

    auto *axX = new QBarCategoryAxis();
    axX->append(cats);
    axX->setTitleText(data.xAxisTitle);

    auto *axY = new QValueAxis();
    axY->setTitleText(data.yAxisTitle);

    mChart->addAxis(axX, Qt::AlignBottom);
    mChart->addAxis(axY, Qt::AlignLeft);
    boxSeries->attachAxis(axX);
    boxSeries->attachAxis(axY);

    if (hasAny) {
        applyDeterministicYAxis(axY, minY, maxY, minY >= 0.0);
    } else {
        axY->setTickType(QValueAxis::TicksFixed);
        axY->setRange(0.0, 1.0);
        axY->setTickCount(6);
    }

    flushChartLayout();
}

void ResultVisualizerWidget::refreshSpatialYearsAndFrame(const VizSpec &spec, bool isLiveYearAdvance)
{
    const QVector<int> newYears = mProvider.fetchSpatialYears(spec);

    int prevSelectedYear = mYearCombo->currentData().isValid() ? mYearCombo->currentData().toInt() : -1;
    mSpatialYears = newYears;

    mUpdatingControls = true;

    // Lazy-refresh species list once spatial rows arrive (e.g. if spec opened at year 0 before first rows)
    const bool usesSpeciesCombo = spec.filters.contains(QStringLiteral("species")) ||
                                  (!spec.speciesMode.isEmpty() && spec.speciesMode != QLatin1String("dominant"));
    const int expectedAggregateCount = (spec.speciesMode == QLatin1String("select_with_aggregates")) ? 1 : 0;
    if (!mSpatialYears.isEmpty() && usesSpeciesCombo && mSpeciesCombo->count() <= expectedAggregateCount) {
        const QVariant prevSpecies = mSpeciesCombo->currentData();
        mSpeciesCombo->clear();
        if (spec.speciesMode == QLatin1String("select_with_aggregates"))
            mSpeciesCombo->addItem(QStringLiteral("<Total (all species)>"), QString::fromLatin1(VizDataProvider::kTotalSpeciesToken));
        const QVector<VizFilterOption> opts = mProvider.fetchFilterOptions(spec, QStringLiteral("species"));
        for (const VizFilterOption &opt : opts)
            mSpeciesCombo->addItem(opt.label, opt.value);
        int restoreIdx = mSpeciesCombo->findData(prevSpecies);
        if (restoreIdx >= 0)
            mSpeciesCombo->setCurrentIndex(restoreIdx);
        const bool showSpecies = mSpeciesCombo->count() > 0;
        mSpeciesLabel->setVisible(showSpecies);
        mSpeciesCombo->setVisible(showSpecies);
        updateSpatialTotalAvailability(&spec);
        updateSpatialControlsVisibility(&spec);
    }

    mYearCombo->clear();
    for (int yr : mSpatialYears)
        mYearCombo->addItem(QString::number(yr), yr);

    if (mSpatialYears.isEmpty()) {
        mYearSlider->setRange(0, 0);
        mYearSlider->setEnabled(false);
        mYearCombo->setEnabled(false);
        mUpdatingControls = false;
        mLegendStack->setCurrentIndex(1);
        updateLegendHeader(&spec);
        mSpatialMap->setLayout(&mSpatialLayout, spec.spatialKey);
        mSpatialMap->setEmptyMessage(QString("No resource unit spatial rows available yet in '%1'.\n(Check conditionRU in project XML or simulate more years.)").arg(spec.table));
        mSpatialLegend->clearLegend(QStringLiteral("No spatial data yet"));
        return;
    }

    mYearSlider->setEnabled(mSpatialYears.size() > 1);
    mYearCombo->setEnabled(true);
    mYearSlider->setRange(0, mSpatialYears.size() - 1);
    const int step = qMax(1, mSpatialYears.size() / 10);
    mYearSlider->setPageStep(step);
    mYearSlider->setTickInterval(step);

    int targetIdx = mSpatialYears.size() - 1;
    if (prevSelectedYear >= 0) {
        const bool followAdvance = isLiveYearAdvance && mFollowLatestCheck->isChecked();
        if (!followAdvance) {
            const int found = mSpatialYears.indexOf(prevSelectedYear);
            if (found >= 0)
                targetIdx = found;
        }
    }
    mYearCombo->setCurrentIndex(targetIdx);
    mYearSlider->setValue(targetIdx);
    mUpdatingControls = false;

    renderSpatialFrame(spec);
}

void ResultVisualizerWidget::renderSpatialFrame(const VizSpec &spec)
{
    mLegendStack->setCurrentIndex(1);

    if (!mSpatialLayout.isValid() && !mProvider.isExternalMode() && !mProjectXmlPath.isEmpty())
        mSpatialLayout.loadFromFile(mProjectXmlPath);

    mSpatialMap->setLayout(&mSpatialLayout, spec.spatialKey);

    if (!mSpatialLayout.isValid()) {
        updateLegendHeader(&spec);
        const QString msg = mProvider.isExternalMode()
                                ? QStringLiteral("No spatial RU grid layout loaded.\nClick 'Spatial grid...' in Output options to choose the project XML or environment raster (.asc/.tif).")
                                : QStringLiteral("No spatial RU grid layout available from the active model or project XML.");
        mSpatialMap->setEmptyMessage(msg);
        mSpatialLegend->clearLegend(QStringLiteral("No spatial grid layout"));
        return;
    }

    if (mSpatialYears.isEmpty() || !mYearCombo->currentData().isValid()) {
        updateLegendHeader(&spec);
        mSpatialMap->setEmptyMessage(QStringLiteral("No spatial data available for the selected table yet."));
        mSpatialLegend->clearLegend(QStringLiteral("No spatial data"));
        return;
    }

    const int year = mYearCombo->currentData().toInt();
    const int metricIdx = mMetricCombo->currentData().isValid() ? mMetricCombo->currentData().toInt() : 0;
    QString speciesFilter;
    QString speciesText;
    if (spec.speciesMode == QLatin1String("dominant")) {
        speciesFilter = QString::fromLatin1(VizDataProvider::kDominantSpeciesToken);
    } else if (!mSpeciesCombo->isHidden()) {
        speciesFilter = mSpeciesCombo->currentData().toString();
        speciesText = mSpeciesCombo->currentText();
    }

    mLastSpatialData = mProvider.fetchSpatialFrame(spec, year, metricIdx, speciesFilter);
    if (mLastSpatialData.hasError()) {
        updateLegendHeader(&spec, mLastSpatialData.metricLabel);
        mSpatialMap->setEmptyMessage(mLastSpatialData.errorMessage);
        mSpatialLegend->clearLegend(mLastSpatialData.errorMessage);
        return;
    }

    const QString palName = mPaletteCombo->currentData().toString();
    const GridViewType palette = VizRegistry::paletteFromName(palName);

    double scaleMin = mLastSpatialData.minVal;
    double scaleMax = mLastSpatialData.maxVal;
    QString scaleModeLabel;

    if (!mLastSpatialData.isDominantSpeciesMode) {
        const QString scaleMode = mScaleModeCombo->currentData().toString();
        if (scaleMode == QLatin1String("global")) {
            if (!mGlobalRangeValid) {
                mCachedGlobalRange = mProvider.fetchSpatialGlobalRange(spec, metricIdx, speciesFilter);
                mGlobalRangeValid = true;
            }
            scaleMin = mCachedGlobalRange.first;
            scaleMax = mCachedGlobalRange.second;
            scaleModeLabel = QStringLiteral("Scale: global (all years)");
        } else if (scaleMode == QLatin1String("auto_p98")) {
            scaleMin = mLastSpatialData.minVal;
            scaleMax = mLastSpatialData.p98Val;
            scaleModeLabel = QStringLiteral("Scale: auto 98% (year)");
        } else if (scaleMode == QLatin1String("manual")) {
            if (!mManualRangeSeeded) {
                QSignalBlocker bMin(mManualMinSpin);
                QSignalBlocker bMax(mManualMaxSpin);
                const double span = qMax(1.0, mLastSpatialData.maxVal - mLastSpatialData.minVal);
                const double step = qMax(0.01, span / 20.0);
                mManualMinSpin->setSingleStep(step);
                mManualMaxSpin->setSingleStep(step);
                mManualMinSpin->setValue(mLastSpatialData.minVal);
                mManualMaxSpin->setValue(mLastSpatialData.maxVal);
                mManualRangeSeeded = true;
            }
            scaleMin = mManualMinSpin->value();
            scaleMax = qMax(scaleMin + 1e-6, mManualMaxSpin->value());
            scaleModeLabel = QStringLiteral("Scale: manual");
        } else {
            scaleModeLabel = QStringLiteral("Scale: auto (year)");
        }
    }

    updateLegendHeader(&spec, mLastSpatialData.metricLabel);
    mSpatialMap->setFrame(mLastSpatialData, palette, scaleMin, scaleMax, speciesText);
    mSpatialLegend->setLegendData(mLastSpatialData, palette, scaleMin, scaleMax, speciesText,
                                  mSpatialLayout.sourceDescription(), scaleModeLabel);
}
