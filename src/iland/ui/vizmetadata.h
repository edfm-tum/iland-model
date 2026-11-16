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
#ifndef VIZMETADATA_H
#define VIZMETADATA_H

#include <QColor>
#include <QHash>
#include <QMap>
#include <QPair>
#include <QRectF>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include "colors.h"
#include "grid.h"

struct VizMetricSpec
{
    QString col;
    QString label;
    QString color;
    QString agg = "sum"; // "sum" (additive) or "none" (non-additive, e.g. dbh_avg_cm)
};

struct VizSpec
{
    QString id;
    QString name;
    QString category;
    QString description;
    QString table;
    QString type; // "line", "stacked_area", "bar", "stacked_bar", "boxplot", "spatial"
    QString sourceTag = "built-in"; // "built-in", "global", "project"
    bool enabled = true;
    bool allowPercentToggle = false;
    bool fillMissingYears = false;

    QString x = "year";
    QString xLabel;
    QString yLabel;
    QStringList xCategories;

    QString group;
    QString colorMode; // "species", "custom", "palette"
    QMap<QString, QString> groupLabels;
    QMap<QString, QString> groupColors;

    QStringList filters;
    QString metricsMode = "select"; // "select" or "all"
    QVector<VizMetricSpec> metrics;
    QString sql;

    // Spatial visualization options
    QString spatialKey = "rid"; // "rid" or "ru"
    QString palette = "turbo";  // "turbo", "greens", "rainbow", "blues", "reds", "terrain", "heat", "gray"
    QString speciesMode;        // "", "select", "select_with_aggregates", "dominant"

    bool isValid() const { return !id.isEmpty() && !name.isEmpty() && !type.isEmpty() && !sql.isEmpty(); }
    bool isSpatial() const { return type == QLatin1String("spatial"); }
    bool isStacked() const { return type == QLatin1String("stacked_area") || type == QLatin1String("stacked_bar"); }
};

struct VizFilterOption
{
    QVariant value;
    QString label;
};

struct VizSeriesData
{
    QString key;
    QString label;
    QColor color;
    QVector<double> y;
};

struct VizBoxPoint
{
    QString categoryLabel;
    double categoryValue = 0.0;
    double min = 0.0;
    double q25 = 0.0;
    double median = 0.0;
    double q75 = 0.0;
    double max = 0.0;
    int count = 0;
};

struct VizChartData
{
    QVector<double> xValues;
    QStringList xTickLabels;
    QVector<VizSeriesData> series;
    QVector<VizBoxPoint> boxes;
    QString xAxisTitle;
    QString yAxisTitle;
    QString errorMessage;

    bool hasError() const { return !errorMessage.isEmpty(); }
    QString toCsv() const;
};

class SpatialGridLayout
{
public:
    SpatialGridLayout() = default;

    void clear();
    bool isValid() const { return mValid; }
    QString sourceDescription() const { return mSourceDescription; }
    QRectF metricRect() const;
    const Grid<int> &gridForKey(const QString &spatialKey) const;
    const Grid<int> &ruIndexGrid() const { return mRuIndexGrid; }
    const Grid<int> &ridGrid() const { return mRidGrid; }
    bool hasRuIndexGrid() const { return !mRuIndexGrid.isEmpty(); }
    bool hasRidGrid() const { return !mRidGrid.isEmpty(); }

    void loadFromLiveModel();
    bool loadFromFile(const QString &path, QString *errorOut = nullptr);
    bool autoDiscoverForDb(const QString &dbPath, const QString &preferredXmlPath, QString *errorOut = nullptr);

private:
    bool loadFromXmlFile(const QString &xmlPath, QString *errorOut);
    bool loadFromGridFile(const QString &gridPath, QString *errorOut);

    Grid<int> mRuIndexGrid;
    Grid<int> mRidGrid;
    bool mValid = false;
    QString mSourceDescription;
};

struct SpatialFrameData
{
    QHash<int, double> valuesByKey;
    QHash<int, QString> dominantSpeciesByKey;
    QHash<QString, QPair<QString, QColor>> speciesInfo; // id -> (display name, color)
    QHash<int, double> rawValueForDominant;
    QHash<int, double> dominantShareByKey; // fraction (0..1) of dominant species relative to RU total
    QString metricLabel;
    QString speciesFilter;
    int year = 0;
    double minVal = 0.0;
    double maxVal = 0.0;
    double p98Val = 0.0;
    bool isDominantSpeciesMode = false;
    bool zeroFillValidCells = true;
    bool useRid = true;
    QString errorMessage;

    bool hasError() const { return !errorMessage.isEmpty(); }
    QString toCsv() const;
};

class VizRegistry
{
public:
    static QString stripJsonComments(const QString &jsonc);
    static QVector<VizSpec> loadAll(const QString &projectXmlPath, QStringList *warnings = nullptr);
    static bool parseFile(const QString &filePath, const QString &sourceTag, QVector<VizSpec> &outSpecs, QStringList *warnings = nullptr);

    static GridViewType paletteFromName(const QString &name);
    static QString paletteDisplayName(GridViewType palette);
};

class VizDataProvider
{
public:
    VizDataProvider();
    ~VizDataProvider();

    void setLiveMode();
    bool openExternalDatabase(const QString &filePath, QString *errorOut = nullptr);
    void closeExternalDatabase();

    bool isExternalMode() const { return mExternalMode; }
    QString externalDbPath() const { return mExternalDbPath; }
    QString sourceLabel() const;

    void setProjectXmlPath(const QString &xmlPath);
    QString projectXmlPath() const { return mProjectXmlPath; }

    bool hasValidDatabase() const;
    QSqlDatabase database() const;
    QSet<QString> availableTables() const;
    QStringList tableColumns(const QString &tableName) const;
    QString runInfoText() const;

    QPair<QString, QColor> resolveSpecies(const QString &speciesId);
    QColor resolveSeriesColor(const VizSpec &spec, const QString &key, int seriesIndex, const QString &explicitColor = QString());
    QString resolveGroupLabel(const VizSpec &spec, const QString &groupKey);

    QVector<VizFilterOption> fetchFilterOptions(const VizSpec &spec, const QString &filterCol);
    VizChartData fetchChartData(const VizSpec &spec, int metricIndex, const QMap<QString, QVariant> &filterValues,
                                bool normalizePercent, int liveCompletedYearHint = -1);

    QVector<int> fetchSpatialYears(const VizSpec &spec);
    QPair<double, double> fetchSpatialGlobalRange(const VizSpec &spec, int metricIndex, const QString &speciesFilter);
    SpatialFrameData fetchSpatialFrame(const VizSpec &spec, int year, int metricIndex, const QString &speciesFilter);

    static constexpr const char *kTotalSpeciesToken = "__ALL__";
    static constexpr const char *kDominantSpeciesToken = "__DOMINANT__";

private:
    void ensureSpeciesCache();
    void loadSpeciesFromInputDbPath(const QString &dbPath);
    static QString prepareFilterSql(const QString &rawSql, const QMap<QString, QVariant> &filterValues,
                                    const QSet<QString> &allowedWrapFilters, QStringList *usedPlaceholders);
    static void bindSqlParameters(class QSqlQuery &query, const QString &sql, const QMap<QString, QVariant> &params);
    static double computeQuantile(const QVector<double> &sorted, double p);

    bool mExternalMode = false;
    QString mExternalDbPath;
    QString mProjectXmlPath;

    bool mSpeciesCacheLoaded = false;
    bool mSpeciesCacheFromModel = false;
    QString mSpeciesCacheProjectPath;
    QHash<QString, QPair<QString, QColor>> mSpeciesCache;

    Colors mColorsHelper; // guarantees Colors static palettes are initialized
};

#endif // VIZMETADATA_H
