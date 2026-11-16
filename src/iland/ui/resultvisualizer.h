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
#ifndef RESULTVISUALIZER_H
#define RESULTVISUALIZER_H

#include <QHash>
#include <QImage>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QWidget>

#include "vizmetadata.h"

class QAreaSeries;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QLineSeries;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSlider;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QChart;
class QChartView;
class QValueAxis;
class SpatialLegendWidget;
class SpatialMapWidget;

class ResultVisualizerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ResultVisualizerWidget(QWidget *parent = nullptr);
    ~ResultVisualizerWidget() override;

    QWidget *legendPanel() const { return mLegendPanel; }
    QWidget *optionsPanel() const { return mOptionsPanel; }
    QImage grabCurrentImage();

public slots:
    void setProjectXmlPath(const QString &xmlPath);
    void onModelCreated(const QString &projectXmlPath);
    void onYearSimulated(int year);
    void onModelDestroyed();
    void onTabActivated();

private slots:
    void onOpenExternalDb();
    void onSwitchToLiveDb();
    void onReloadMetadata();
    void onShowHelp();
    void onSelectCustomGrid();

    void onCatalogFilterChanged();
    void onTreeSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void onLegendItemChanged(QListWidgetItem *item);

    void onControlValueChanged();
    void onSpatialMetricOrSpeciesChanged();
    void onSpatialYearComboChanged(int index);
    void onSpatialYearSliderChanged(int index);
    void onSpatialPaletteOrScaleChanged();

    void onResetZoom();
    void onCopyImage();
    void onCopyCsv();

private:
    void buildUi();
    void reloadMetadataCatalog();
    void populateCatalog();
    const VizSpec *selectedSpec() const;

    void configureControlsForSpec(const VizSpec *spec);
    void updateSpatialTotalAvailability(const VizSpec *spec);
    void updateSpatialControlsVisibility(const VizSpec *spec);

    void clearAndResetChart();
    void flushChartLayout();
    void updateLegendHeader(const VizSpec *spec, const QString &subtitle = QString());
    void populateChartLegend(const VizSpec &spec, const VizChartData &data);
    void showEmptyChartState(const VizSpec *spec, const QString &chartTitle,
                             const QString &hoverError = QString(), bool clearLegendList = true);

    void refreshActiveVisualization(bool isLiveYearAdvance = false);
    void renderChart(const VizSpec &spec);
    void renderChartData(const VizSpec &spec, const VizChartData &data);
    QString formatStackedAreaReadout(const QString &seriesKey, const QString &seriesLabel, double xCoord) const;
    void renderLineChart(const VizSpec &spec, const VizChartData &data, const QVector<const VizSeriesData *> &visibleSeries);
    void renderStackedAreaChart(const VizSpec &spec, const VizChartData &data, const QVector<const VizSeriesData *> &visibleSeries);
    void renderBarChart(const VizSpec &spec, const VizChartData &data, const QVector<const VizSeriesData *> &visibleSeries);
    void renderBoxPlotChart(const VizSpec &spec, const VizChartData &data);
    void renderEmptyChart(const QString &message);

    void refreshSpatialYearsAndFrame(const VizSpec &spec, bool isLiveYearAdvance = false);
    void renderSpatialFrame(const VizSpec &spec);
    void updateSourceButtons();

    // Data & metadata state
    VizDataProvider mProvider;
    SpatialGridLayout mSpatialLayout;
    QVector<VizSpec> mSpecs;
    QSet<QString> mAvailableTables;
    QString mCurrentVizId;
    QString mProjectXmlPath;
    QString mCustomSpatialGridPath;
    int mLiveCompletedYearHint = -1;
    bool mDirty = false;
    bool mUpdatingControls = false;

    // Cached state for CSV export, legend toggling, and spatial range
    VizChartData mLastChartData;
    SpatialFrameData mLastSpatialData;
    QVector<int> mSpatialYears;
    QPair<double, double> mCachedGlobalRange = qMakePair(0.0, 1.0);
    bool mGlobalRangeValid = false;
    QHash<QString, QSet<QString>> mHiddenSeriesByViz;
    QString mActiveLegendSignature;
    QString mPinnedChartReadout;
    bool mManualRangeSeeded = false;

    // Live chart state for flicker/overplot-free in-place updates
    QString mActiveChartSignature;
    QValueAxis *mLiveAxisX = nullptr;
    QValueAxis *mLiveAxisY = nullptr;
    QVector<QLineSeries *> mLiveLineSeries;
    QVector<QAreaSeries *> mLiveAreaSeries;

    // External side-dock panels (reparented by MainWindow into QDockWidgets)
    QPointer<QWidget> mLegendPanel;
    QPointer<QWidget> mOptionsPanel;

    // Legend dock widgets (left)
    QLabel *mLegendTitleLabel = nullptr;
    QLabel *mLegendSubtitleLabel = nullptr;
    QStackedWidget *mLegendStack = nullptr;
    QListWidget *mSeriesLegendList = nullptr;
    QLabel *mLegendHintLabel = nullptr;
    SpatialLegendWidget *mSpatialLegend = nullptr;

    // Options dock widgets (right)
    QLabel *mSourceLabel = nullptr;
    QPushButton *mOpenDbBtn = nullptr;
    QPushButton *mLiveBtn = nullptr;
    QPushButton *mReloadBtn = nullptr;
    QPushButton *mHelpBtn = nullptr;
    QLineEdit *mSearchEdit = nullptr;
    QCheckBox *mActiveOnlyCheck = nullptr;
    QTreeWidget *mTree = nullptr;

    QGroupBox *mOptionsGroup = nullptr;
    QLabel *mMetricLabel = nullptr;
    QComboBox *mMetricCombo = nullptr;
    QLabel *mSpeciesLabel = nullptr;
    QComboBox *mSpeciesCombo = nullptr;
    QLabel *mRuLabel = nullptr;
    QComboBox *mRuCombo = nullptr;
    QLabel *mSecondaryLabel = nullptr;
    QComboBox *mSecondaryCombo = nullptr;
    QString mSecondaryFilterCol;
    QCheckBox *mPercentCheck = nullptr;
    QLabel *mPaletteLabel = nullptr;
    QComboBox *mPaletteCombo = nullptr;
    QLabel *mScaleModeLabel = nullptr;
    QComboBox *mScaleModeCombo = nullptr;
    QLabel *mManualRangeLabel = nullptr;
    QWidget *mManualRangeWidget = nullptr;
    QDoubleSpinBox *mManualMinSpin = nullptr;
    QDoubleSpinBox *mManualMaxSpin = nullptr;
    QPushButton *mGridFileBtn = nullptr;

    QPushButton *mResetZoomBtn = nullptr;
    QPushButton *mCopyImgBtn = nullptr;
    QPushButton *mCopyCsvBtn = nullptr;

    // Central canvas widgets
    QWidget *mSpatialYearBar = nullptr;
    QSlider *mYearSlider = nullptr;
    QComboBox *mYearCombo = nullptr;
    QCheckBox *mFollowLatestCheck = nullptr;

    QStackedWidget *mCanvasStack = nullptr;
    QChart *mChart = nullptr;
    QChartView *mChartView = nullptr;
    SpatialMapWidget *mSpatialMap = nullptr;

    QLabel *mDescLabel = nullptr;
    QLabel *mHoverLabel = nullptr;
};

#endif // RESULTVISUALIZER_H
