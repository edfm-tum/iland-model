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

#include "vizmetadata.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <algorithm>
#include <cmath>
#include <limits>

#include "csvfile.h"
#include "gisgrid.h"
#include "globalsettings.h"
#include "helper.h"
#include "model.h"
#include "modelcontroller.h"
#include "resourceunit.h"
#include "species.h"
#include "speciesset.h"
#include "xmlhelper.h"

namespace {

const char *kExternalConnectionName = "viz_ext";

QString stripTrailingSemicolon(QString sql)
{
    sql = sql.trimmed();
    while (sql.endsWith(QLatin1Char(';')))
        sql.chop(1);
    return sql.trimmed();
}

bool tableOmitsZeroRuRows(const QString &table)
{
    return table == QLatin1String("stand") || table == QLatin1String("sapling") ||
           table == QLatin1String("standdead") || table == QLatin1String("management");
}

QColor defaultSpeciesFallbackColor(const QString &speciesId)
{
    static const QHash<QString, QString> fallbackHex = {
        { "piab", "#1b7837" }, { "abal", "#00441b" }, { "lade", "#fdb863" },
        { "pisy", "#d95f02" }, { "fasy", "#33a02c" }, { "quro", "#6a3d9a" },
        { "qupe", "#8073ac" }, { "qupu", "#b2abd2" }, { "acps", "#b2df8a" },
        { "acpl", "#a6dba0" }, { "acca", "#c7e9c0" }, { "bepe", "#a6cee3" },
        { "cabe", "#74add1" }, { "frex", "#fb9a99" }, { "algl", "#1f78b4" },
        { "alin", "#4393c3" }, { "pice", "#01665e" }, { "pimu", "#8c510a" },
        { "soau", "#e31a1c" }, { "soar", "#f46d43" }, { "potr", "#fdbf6f" },
        { "poni", "#fee08b" }, { "tico", "#de77ae" }, { "tipl", "#f1b6da" },
        { "ulgl", "#bf812d" }, { "saca", "#dfc27d" }, { "casa", "#9970ab" },
        { "psme", "#238b45" }, { "pico", "#fe9929" }, { "pien", "#006837" },
        { "abla", "#41ab5d" }, { "tshe", "#2b8cbe" }, { "thpl", "#7bccc4" },
        { "pipo", "#ec7014" }, { "alru", "#3690c0" }
    };
    const QString key = speciesId.toLower();
    if (fallbackHex.contains(key))
        return QColor(fallbackHex.value(key));

    // Deterministic index from species string
    uint hash = 0;
    for (QChar ch : speciesId)
        hash = hash * 31 + ch.unicode();
    return Colors::colorFromPalette(static_cast<int>(hash % 60), GridViewBrewerQual);
}

QString resolveXmlRelativePath(const QString &path, const QString &homeDir, const QString &xmlDir)
{
    if (path.isEmpty())
        return QString();
    if (QFileInfo(path).isAbsolute())
        return QDir::cleanPath(path);
    const QString base = homeDir.isEmpty() ? xmlDir : homeDir;
    return QDir::cleanPath(QDir(base).filePath(path));
}

} // namespace

// ---------------------------------------------------------------------------
// VizChartData / SpatialFrameData CSV helpers
// ---------------------------------------------------------------------------

QString VizChartData::toCsv() const
{
    if (hasError())
        return QString();

    QStringList lines;
    if (!boxes.isEmpty()) {
        lines << QStringLiteral("category;count;min;q25;median;q75;max");
        for (const VizBoxPoint &b : boxes) {
            lines << QString("%1;%2;%3;%4;%5;%6;%7")
                         .arg(b.categoryLabel)
                         .arg(b.count)
                         .arg(b.min, 0, 'g', 8)
                         .arg(b.q25, 0, 'g', 8)
                         .arg(b.median, 0, 'g', 8)
                         .arg(b.q75, 0, 'g', 8)
                         .arg(b.max, 0, 'g', 8);
        }
        return lines.join(QLatin1Char('\n'));
    }

    if (series.isEmpty())
        return QString();

    QStringList header;
    header << (xAxisTitle.isEmpty() ? QStringLiteral("x") : xAxisTitle);
    for (const VizSeriesData &s : series)
        header << s.label;
    lines << header.join(QLatin1Char(';'));

    for (int i = 0; i < xValues.size(); ++i) {
        QStringList row;
        if (i < xTickLabels.size() && !xTickLabels[i].isEmpty())
            row << xTickLabels[i];
        else
            row << QString::number(xValues[i], 'g', 8);

        for (const VizSeriesData &s : series) {
            double v = (i < s.y.size()) ? s.y[i] : std::numeric_limits<double>::quiet_NaN();
            row << (std::isfinite(v) ? QString::number(v, 'g', 8) : QString());
        }
        lines << row.join(QLatin1Char(';'));
    }
    return lines.join(QLatin1Char('\n'));
}

QString SpatialFrameData::toCsv() const
{
    if (hasError())
        return QString();
    QStringList lines;
    const QString keyCol = useRid ? QStringLiteral("rid") : QStringLiteral("ru");
    if (isDominantSpeciesMode) {
        lines << QString("%1;year;dominant_species;value;share").arg(keyCol);
        QList<int> keys = dominantSpeciesByKey.keys();
        std::sort(keys.begin(), keys.end());
        for (int k : keys) {
            lines << QString("%1;%2;%3;%4;%5")
                         .arg(k)
                         .arg(year)
                         .arg(dominantSpeciesByKey.value(k))
                         .arg(rawValueForDominant.value(k, 0.0), 0, 'g', 8)
                         .arg(dominantShareByKey.value(k, 1.0), 0, 'g', 6);
        }
    } else {
        lines << QString("%1;year;value").arg(keyCol);
        QList<int> keys = valuesByKey.keys();
        std::sort(keys.begin(), keys.end());
        for (int k : keys) {
            lines << QString("%1;%2;%3")
                         .arg(k)
                         .arg(year)
                         .arg(valuesByKey.value(k), 0, 'g', 8);
        }
    }
    return lines.join(QLatin1Char('\n'));
}

// ---------------------------------------------------------------------------
// SpatialGridLayout
// ---------------------------------------------------------------------------

void SpatialGridLayout::clear()
{
    mRuIndexGrid.clear();
    mRidGrid.clear();
    mValid = false;
    mSourceDescription.clear();
}

QRectF SpatialGridLayout::metricRect() const
{
    if (!mRidGrid.isEmpty())
        return mRidGrid.metricRect();
    if (!mRuIndexGrid.isEmpty())
        return mRuIndexGrid.metricRect();
    return QRectF();
}

const Grid<int> &SpatialGridLayout::gridForKey(const QString &spatialKey) const
{
    if (spatialKey == QLatin1String("ru") && !mRuIndexGrid.isEmpty())
        return mRuIndexGrid;
    if (!mRidGrid.isEmpty())
        return mRidGrid;
    return mRuIndexGrid;
}

void SpatialGridLayout::loadFromLiveModel()
{
    clear();
    Model *m = GlobalSettings::instance()->model();
    if (!m || !m->isSetup())
        return;

    const Grid<ResourceUnit *> &ruGrid = m->RUgrid();
    if (ruGrid.isEmpty())
        return;

    mRuIndexGrid.setMetricRect(ruGrid.metricRect());
    mRidGrid.setMetricRect(ruGrid.metricRect());
    if (!mRuIndexGrid.setup(ruGrid.cellsize(), ruGrid.sizeX(), ruGrid.sizeY()) ||
        !mRidGrid.setup(ruGrid.cellsize(), ruGrid.sizeX(), ruGrid.sizeY()) ||
        mRuIndexGrid.isEmpty() || mRidGrid.isEmpty()) {
        clear();
        return;
    }
    mRuIndexGrid.initialize(-1);
    mRidGrid.initialize(-1);

    const int count = qMin(ruGrid.count(), qMin(mRuIndexGrid.count(), mRidGrid.count()));
    for (int i = 0; i < count; ++i) {
        const ResourceUnit *ru = ruGrid.constValueAtIndex(i);
        if (ru && ru->id() != -1) {
            mRuIndexGrid[i] = ru->index();
            mRidGrid[i] = ru->id();
        }
    }
    mValid = true;
    mSourceDescription = QStringLiteral("Live model RU grid");
}

bool SpatialGridLayout::loadFromFile(const QString &path, QString *errorOut)
{
    if (path.isEmpty() || !QFile::exists(path)) {
        if (errorOut)
            *errorOut = QString("Layout file does not exist: %1").arg(path);
        return false;
    }
    if (path.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive))
        return loadFromXmlFile(path, errorOut);
    return loadFromGridFile(path, errorOut);
}

bool SpatialGridLayout::loadFromGridFile(const QString &gridPath, QString *errorOut)
{
    Grid<int> loaded;
    try {
        if (!loaded.loadGridFromFile(gridPath) || loaded.isEmpty()) {
            if (errorOut)
                *errorOut = QString("Failed to load spatial grid: %1").arg(gridPath);
            return false;
        }
    } catch (const IException &e) {
        if (errorOut)
            *errorOut = e.message();
        return false;
    }

    clear();
    mRidGrid.setup(loaded.metricRect(), loaded.cellsize());
    mRuIndexGrid.setup(loaded.metricRect(), loaded.cellsize());
    mRidGrid.initialize(-1);
    mRuIndexGrid.initialize(-1);

    int ruSeq = 0;
    for (int i = 0; i < loaded.count(); ++i) {
        const int val = loaded.constValueAtIndex(i);
        if (!loaded.isNull(val) && val >= 0) {
            mRidGrid[i] = val;
            mRuIndexGrid[i] = ruSeq++;
        }
    }
    mValid = true;
    mSourceDescription = QFileInfo(gridPath).fileName();
    return true;
}

bool SpatialGridLayout::loadFromXmlFile(const QString &xmlPath, QString *errorOut)
{
    try {
        XmlHelper xml(xmlPath);
        const QString xmlDir = QFileInfo(xmlPath).absolutePath();
        QString homeDir = xml.value("system.path.home", "", false);
        if (homeDir.isEmpty())
            homeDir = xmlDir;
        else if (QFileInfo(homeDir).isRelative())
            homeDir = QDir::cleanPath(QDir(xmlDir).filePath(homeDir));

        const double width = xml.valueDouble("model.world.width", 0.0, false);
        const double height = xml.valueDouble("model.world.height", 0.0, false);
        if (width <= 0.0 || height <= 0.0) {
            if (errorOut)
                *errorOut = QString("Invalid world width/height in %1").arg(xmlPath);
            return false;
        }

        // Only update global GIS transform if no live model owns it
        if (!GlobalSettings::instance()->model() && xml.hasNode("model.world.location")) {
            double loc_x = xml.valueDouble("model.world.location.x", 0.0, false);
            double loc_y = xml.valueDouble("model.world.location.y", 0.0, false);
            double loc_z = xml.valueDouble("model.world.location.z", 0.0, false);
            double loc_rot = xml.valueDouble("model.world.location.rotation", 0.0, false);
            setupGISTransformation(loc_x, loc_y, loc_z, loc_rot);
        } else if (!GlobalSettings::instance()->model()) {
            setupGISTransformation(0.0, 0.0, 0.0, 0.0);
        }

        const bool envEnabled = xml.valueBool("model.world.environmentEnabled", false, false);
        const QString envMode = xml.value("model.world.environmentMode", "", false);
        const QString envGridPath = resolveXmlRelativePath(xml.value("model.world.environmentGrid", "", false), homeDir, xmlDir);
        const QString envFilePath = resolveXmlRelativePath(xml.value("model.world.environmentFile", "", false), homeDir, xmlDir);
        const bool standGridEnabled = xml.valueBool("model.world.standGrid.enabled", false, false);
        const QString standGridPath = resolveXmlRelativePath(xml.value("model.world.standGrid.fileName", "", false), homeDir, xmlDir);

        Grid<int> tmpRu;
        Grid<int> tmpRid;
        tmpRu.setup(QRectF(0.0, 0.0, width, height), 100.0);
        tmpRid.setup(QRectF(0.0, 0.0, width, height), 100.0);
        tmpRu.initialize(-1);
        tmpRid.initialize(-1);

        // Optional stand grid mask (using GisGrid directly so no Model instance is required)
        QVector<bool> hasStandCell(tmpRu.count(), true);
        if (standGridEnabled && !standGridPath.isEmpty() && QFile::exists(standGridPath)) {
            GisGrid sg;
            if (sg.loadFromFile(standGridPath)) {
                hasStandCell.fill(false);
                const double step = qBound(5.0, sg.cellSize(), 25.0);
                for (int i = 0; i < tmpRu.count(); ++i) {
                    const QRectF r = tmpRu.cellRect(tmpRu.indexOf(i));
                    bool validInRu = false;
                    for (double py = r.top() + step * 0.5; py < r.bottom() && !validInRu; py += step) {
                        for (double px = r.left() + step * 0.5; px < r.right(); px += step) {
                            const double gv = sg.value(px, py);
                            if (gv > -1.0 && gv != sg.noDataValue()) {
                                validInRu = true;
                                break;
                            }
                        }
                    }
                    hasStandCell[i] = validInRu;
                }
            }
        }

        GisGrid envGrid;
        bool hasEnvGrid = false;
        if (envEnabled && envMode == QLatin1String("grid") && !envGridPath.isEmpty() && QFile::exists(envGridPath))
            hasEnvGrid = envGrid.loadFromFile(envGridPath);

        CSVFile envCsv;
        QHash<QString, int> coordToId;
        QSet<int> validEnvIds;
        bool hasCoordRows = false;
        bool hasIdCol = false;
        if (envEnabled && !envFilePath.isEmpty() && QFile::exists(envFilePath)) {
            if (envCsv.loadFile(envFilePath)) {
                int idCol = envCsv.columnIndex("id");
                int ixCol = envCsv.columnIndex("x");
                int iyCol = envCsv.columnIndex("y");
                hasIdCol = (idCol >= 0);
                if (hasEnvGrid && hasIdCol) {
                    for (int r = 0; r < envCsv.rowCount(); ++r)
                        validEnvIds.insert(envCsv.value(r, idCol).toInt());
                } else if (!hasEnvGrid && ixCol >= 0 && iyCol >= 0) {
                    hasCoordRows = true;
                    for (int r = 0; r < envCsv.rowCount(); ++r) {
                        QString key = QString("%1_%2")
                                          .arg(envCsv.value(r, ixCol).toString())
                                          .arg(envCsv.value(r, iyCol).toString());
                        int rid = hasIdCol ? envCsv.value(r, idCol).toInt() : (r + 1);
                        coordToId.insert(key, rid);
                    }
                }
            }
        }

        int ruIdx = 0;
        int seqId = 0;
        // Iterate in Grid memory order (row-major by y then x), matching Model::setupSpace()
        for (int i = 0; i < tmpRu.count(); ++i) {
            if (!hasStandCell[i])
                continue;
            QPoint pos = tmpRu.indexOf(i);
            QPointF center = tmpRu.cellCenterPoint(pos);
            if (hasEnvGrid) {
                double gv = envGrid.value(center);
                int rid = qRound(gv);
                if (rid >= 0 && (validEnvIds.isEmpty() || validEnvIds.contains(rid))) {
                    tmpRu[i] = ruIdx++;
                    tmpRid[i] = rid;
                }
            } else if (hasCoordRows) {
                int ix = static_cast<int>(center.x() / 100.0);
                int iy = static_cast<int>(center.y() / 100.0);
                QString key = QString("%1_%2").arg(ix).arg(iy);
                if (coordToId.contains(key)) {
                    tmpRu[i] = ruIdx++;
                    tmpRid[i] = coordToId.value(key);
                }
            } else {
                tmpRu[i] = ruIdx++;
                tmpRid[i] = seqId++;
            }
        }

        clear();
        mRuIndexGrid.setup(tmpRu);
        mRuIndexGrid.copy(tmpRu);
        mRidGrid.setup(tmpRid);
        mRidGrid.copy(tmpRid);
        mValid = (ruIdx > 0);
        mSourceDescription = QString("XML: %1 (%2 RUs)").arg(QFileInfo(xmlPath).fileName()).arg(ruIdx);
        return mValid;
    } catch (const IException &e) {
        if (errorOut)
            *errorOut = e.message();
        return false;
    }
}

bool SpatialGridLayout::autoDiscoverForDb(const QString &dbPath, const QString &preferredXmlPath, QString *errorOut)
{
    QStringList candidates;
    if (!preferredXmlPath.isEmpty() && QFile::exists(preferredXmlPath))
        candidates << preferredXmlPath;

    if (!dbPath.isEmpty()) {
        QDir dbDir = QFileInfo(dbPath).absoluteDir();
        for (const QString &f : dbDir.entryList({ "*.xml" }, QDir::Files))
            candidates << dbDir.filePath(f);
        QDir parentDir = dbDir;
        if (parentDir.cdUp()) {
            for (const QString &f : parentDir.entryList({ "*.xml" }, QDir::Files))
                candidates << parentDir.filePath(f);
        }
    }

    candidates.removeDuplicates();
    for (const QString &xmlPath : candidates) {
        if (loadFromXmlFile(xmlPath, errorOut))
            return true;
    }
    if (errorOut && errorOut->isEmpty())
        *errorOut = QStringLiteral("No matching project XML or spatial grid file found.");
    return false;
}

// ---------------------------------------------------------------------------
// VizRegistry
// ---------------------------------------------------------------------------

QString VizRegistry::stripJsonComments(const QString &jsonc)
{
    QString out;
    out.reserve(jsonc.size());
    bool inString = false;
    bool escaped = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    const int n = jsonc.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = jsonc[i];
        const QChar next = (i + 1 < n) ? jsonc[i + 1] : QChar();

        if (inLineComment) {
            if (c == QLatin1Char('\n')) {
                inLineComment = false;
                out.append(c);
            } else if (c == QLatin1Char('\r')) {
                out.append(c);
            } else {
                out.append(QLatin1Char(' '));
            }
            continue;
        }

        if (inBlockComment) {
            if (c == QLatin1Char('*') && next == QLatin1Char('/')) {
                inBlockComment = false;
                out.append(QStringLiteral("  "));
                ++i;
            } else if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
                out.append(c);
            } else {
                out.append(QLatin1Char(' '));
            }
            continue;
        }

        if (inString) {
            out.append(c);
            if (escaped) {
                escaped = false;
            } else if (c == QLatin1Char('\\')) {
                escaped = true;
            } else if (c == QLatin1Char('"')) {
                inString = false;
            }
            continue;
        }

        if (c == QLatin1Char('"')) {
            inString = true;
            out.append(c);
            continue;
        }

        if (c == QLatin1Char('/') && next == QLatin1Char('/')) {
            inLineComment = true;
            out.append(QStringLiteral("  "));
            ++i;
            continue;
        }

        if (c == QLatin1Char('/') && next == QLatin1Char('*')) {
            inBlockComment = true;
            out.append(QStringLiteral("  "));
            ++i;
            continue;
        }

        out.append(c);
    }
    return out;
}

bool VizRegistry::parseFile(const QString &filePath, const QString &sourceTag, QVector<VizSpec> &outSpecs, QStringList *warnings)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (warnings)
            warnings->append(QString("Could not open visualization metadata file: %1").arg(filePath));
        return false;
    }

    const QString raw = QString::fromUtf8(file.readAll());
    const QString stripped = stripJsonComments(raw);

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(stripped.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        int line = stripped.left(err.offset).count(QLatin1Char('\n')) + 1;
        if (warnings)
            warnings->append(QString("JSON parse error in %1 (line %2): %3").arg(filePath).arg(line).arg(err.errorString()));
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonArray arr = root.value(QStringLiteral("visualizations")).toArray();
    for (int i = 0; i < arr.size(); ++i) {
        if (!arr[i].isObject())
            continue;
        const QJsonObject obj = arr[i].toObject();
        VizSpec spec;
        spec.sourceTag = sourceTag;
        spec.id = obj.value(QStringLiteral("id")).toString().trimmed();
        spec.name = obj.value(QStringLiteral("name")).toString().trimmed();
        spec.category = obj.value(QStringLiteral("category")).toString(QStringLiteral("Custom")).trimmed();
        spec.description = obj.value(QStringLiteral("description")).toString().trimmed();
        spec.table = obj.value(QStringLiteral("table")).toString().trimmed();
        spec.type = obj.value(QStringLiteral("type")).toString(QStringLiteral("line")).trimmed().toLower();
        spec.enabled = obj.value(QStringLiteral("enabled")).toBool(true);
        spec.allowPercentToggle = obj.value(QStringLiteral("allowPercentToggle")).toBool(false);
        spec.fillMissingYears = obj.value(QStringLiteral("fillMissingYears")).toBool(false);

        spec.x = obj.value(QStringLiteral("x")).toString(QStringLiteral("year")).trimmed();
        spec.xLabel = obj.value(QStringLiteral("xLabel")).toString();
        spec.yLabel = obj.value(QStringLiteral("yLabel")).toString();
        const QJsonArray xCats = obj.value(QStringLiteral("xCategories")).toArray();
        for (const QJsonValue &v : xCats)
            spec.xCategories.append(v.toString());

        spec.group = obj.value(QStringLiteral("group")).toString().trimmed();
        spec.colorMode = obj.value(QStringLiteral("colorMode")).toString().trimmed().toLower();
        if (spec.colorMode.isEmpty() && spec.group == QLatin1String("species"))
            spec.colorMode = QStringLiteral("species");

        const QJsonObject gLabels = obj.value(QStringLiteral("groupLabels")).toObject();
        for (auto it = gLabels.begin(); it != gLabels.end(); ++it)
            spec.groupLabels.insert(it.key(), it.value().toString());

        const QJsonObject gColors = obj.value(QStringLiteral("groupColors")).toObject();
        for (auto it = gColors.begin(); it != gColors.end(); ++it)
            spec.groupColors.insert(it.key(), it.value().toString());

        const QJsonArray filtersArr = obj.value(QStringLiteral("filters")).toArray();
        for (const QJsonValue &v : filtersArr) {
            const QString f = v.toString().trimmed();
            if (!f.isEmpty())
                spec.filters.append(f);
        }

        spec.metricsMode = obj.value(QStringLiteral("metricsMode")).toString(QStringLiteral("select")).trimmed().toLower();
        const QJsonArray metricsArr = obj.value(QStringLiteral("metrics")).toArray();
        for (const QJsonValue &mv : metricsArr) {
            if (mv.isObject()) {
                QJsonObject mo = mv.toObject();
                VizMetricSpec ms;
                ms.col = mo.value(QStringLiteral("col")).toString().trimmed();
                ms.label = mo.value(QStringLiteral("label")).toString(ms.col).trimmed();
                ms.color = mo.value(QStringLiteral("color")).toString().trimmed();
                ms.agg = mo.value(QStringLiteral("agg")).toString(QStringLiteral("sum")).trimmed().toLower();
                if (!ms.col.isEmpty())
                    spec.metrics.append(ms);
            } else if (mv.isString()) {
                VizMetricSpec ms;
                ms.col = mv.toString().trimmed();
                ms.label = ms.col;
                if (!ms.col.isEmpty())
                    spec.metrics.append(ms);
            }
        }

        const QJsonValue sqlVal = obj.value(QStringLiteral("sql"));
        if (sqlVal.isArray()) {
            QStringList sqlLines;
            for (const QJsonValue &lv : sqlVal.toArray())
                sqlLines.append(lv.toString());
            spec.sql = sqlLines.join(QLatin1Char('\n')).trimmed();
        } else {
            spec.sql = sqlVal.toString().trimmed();
        }

        spec.spatialKey = obj.value(QStringLiteral("spatialKey")).toString(QStringLiteral("rid")).trimmed().toLower();
        spec.palette = obj.value(QStringLiteral("palette")).toString(QStringLiteral("turbo")).trimmed().toLower();
        spec.speciesMode = obj.value(QStringLiteral("speciesMode")).toString().trimmed().toLower();

        if (spec.id.isEmpty()) {
            if (warnings)
                warnings->append(QString("Skipping visualization entry #%1 in %2: missing 'id'.").arg(i).arg(filePath));
            continue;
        }
        if (spec.enabled && !spec.isValid()) {
            if (warnings)
                warnings->append(QString("Skipping visualization '%1' in %2: missing name/type/sql.").arg(spec.id, filePath));
            continue;
        }
        outSpecs.append(spec);
    }
    return true;
}

QVector<VizSpec> VizRegistry::loadAll(const QString &projectXmlPath, QStringList *warnings)
{
    QVector<VizSpec> merged;
    QHash<QString, int> indexById;

    auto mergeTier = [&](const QString &path, const QString &tag) {
        if (path.isEmpty() || !QFile::exists(path))
            return;
        QVector<VizSpec> tierSpecs;
        if (!parseFile(path, tag, tierSpecs, warnings))
            return;
        for (const VizSpec &spec : tierSpecs) {
            if (indexById.contains(spec.id)) {
                int idx = indexById.value(spec.id);
                merged[idx] = spec;
            } else {
                indexById.insert(spec.id, merged.size());
                merged.append(spec);
            }
        }
    };

    // Tier 1: Built-in compiled resource
    mergeTier(QStringLiteral(":/visualizations.json"), QStringLiteral("built-in"));

    // Tier 2: Executable directory
    const QString exeFile = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("visualizations.json"));
    mergeTier(exeFile, QStringLiteral("global"));

    // Tier 3: Project XML directory (and home directory if distinct)
    QStringList projectCandidates;
    if (!projectXmlPath.isEmpty()) {
        QFileInfo xmlInfo(projectXmlPath);
        if (xmlInfo.exists())
            projectCandidates << QDir(xmlInfo.absolutePath()).filePath(QStringLiteral("visualizations.json"));
    }
    if (GlobalSettings::instance()->model()) {
        QString homeViz = GlobalSettings::instance()->path(QStringLiteral("visualizations.json"), QStringLiteral("home"));
        if (!homeViz.isEmpty())
            projectCandidates << homeViz;
    }

    QSet<QString> seenCanonical;
    seenCanonical.insert(QFileInfo(exeFile).canonicalFilePath());
    for (const QString &p : projectCandidates) {
        QFileInfo fi(p);
        QString canon = fi.exists() ? fi.canonicalFilePath() : fi.absoluteFilePath();
        if (!seenCanonical.contains(canon)) {
            seenCanonical.insert(canon);
            mergeTier(p, QStringLiteral("project"));
        }
    }

    // Filter out entries disabled via overrides ("enabled": false)
    QVector<VizSpec> result;
    result.reserve(merged.size());
    for (const VizSpec &s : merged) {
        if (s.enabled)
            result.append(s);
    }
    return result;
}

GridViewType VizRegistry::paletteFromName(const QString &name)
{
    const QString key = name.trimmed().toLower();
    if (key == QLatin1String("greens"))
        return GridViewGreens;
    if (key == QLatin1String("rainbow"))
        return GridViewRainbow;
    if (key == QLatin1String("rainbow_reverse"))
        return GridViewRainbowReverse;
    if (key == QLatin1String("blues"))
        return GridViewBlues;
    if (key == QLatin1String("reds"))
        return GridViewReds;
    if (key == QLatin1String("heat"))
        return GridViewHeat;
    if (key == QLatin1String("gray"))
        return GridViewGray;
    if (key == QLatin1String("gray_reverse"))
        return GridViewGrayReverse;
    if (key == QLatin1String("terrain"))
        return GridViewTerrain;
    return GridViewTurbo;
}

QString VizRegistry::paletteDisplayName(GridViewType palette)
{
    switch (palette) {
    case GridViewTurbo: return QStringLiteral("Turbo");
    case GridViewGreens: return QStringLiteral("Greens");
    case GridViewRainbow: return QStringLiteral("Rainbow");
    case GridViewRainbowReverse: return QStringLiteral("Rainbow (rev)");
    case GridViewBlues: return QStringLiteral("Blues");
    case GridViewReds: return QStringLiteral("Reds");
    case GridViewHeat: return QStringLiteral("Heat");
    case GridViewGray: return QStringLiteral("Gray");
    case GridViewGrayReverse: return QStringLiteral("Gray (rev)");
    case GridViewTerrain: return QStringLiteral("Terrain");
    default: return QStringLiteral("Turbo");
    }
}

// ---------------------------------------------------------------------------
// VizDataProvider
// ---------------------------------------------------------------------------

VizDataProvider::VizDataProvider() = default;

VizDataProvider::~VizDataProvider()
{
    closeExternalDatabase();
}

void VizDataProvider::setLiveMode()
{
    mExternalMode = false;
    mSpeciesCacheLoaded = false;
}

bool VizDataProvider::openExternalDatabase(const QString &filePath, QString *errorOut)
{
    closeExternalDatabase();

    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        if (errorOut)
            *errorOut = QString("Database file does not exist: %1").arg(filePath);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QLatin1String(kExternalConnectionName));
    db.setDatabaseName(filePath);
    db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
    if (!db.open()) {
        if (errorOut)
            *errorOut = QString("Failed to open SQLite database '%1': %2").arg(filePath, db.lastError().text());
        QSqlDatabase::removeDatabase(QLatin1String(kExternalConnectionName));
        return false;
    }

    mExternalMode = true;
    mExternalDbPath = filePath;
    mSpeciesCacheLoaded = false;
    return true;
}

void VizDataProvider::closeExternalDatabase()
{
    if (QSqlDatabase::contains(QLatin1String(kExternalConnectionName))) {
        {
            QSqlDatabase db = QSqlDatabase::database(QLatin1String(kExternalConnectionName), false);
            if (db.isOpen())
                db.close();
        }
        QSqlDatabase::removeDatabase(QLatin1String(kExternalConnectionName));
    }
    mExternalMode = false;
    mExternalDbPath.clear();
    mSpeciesCacheLoaded = false;
}

QString VizDataProvider::sourceLabel() const
{
    if (mExternalMode)
        return QString("File: %1").arg(QFileInfo(mExternalDbPath).fileName());
    if (hasValidDatabase())
        return QStringLiteral("Live model");
    return QStringLiteral("No output DB");
}

void VizDataProvider::setProjectXmlPath(const QString &xmlPath)
{
    if (mProjectXmlPath != xmlPath) {
        mProjectXmlPath = xmlPath;
        if (!mSpeciesCacheFromModel)
            mSpeciesCacheLoaded = false;
    }
}

bool VizDataProvider::hasValidDatabase() const
{
    QSqlDatabase db = database();
    return db.isValid() && db.isOpen();
}

QSqlDatabase VizDataProvider::database() const
{
    if (mExternalMode) {
        if (QSqlDatabase::contains(QLatin1String(kExternalConnectionName)))
            return QSqlDatabase::database(QLatin1String(kExternalConnectionName), false);
        return QSqlDatabase();
    }
    if (QSqlDatabase::contains(QStringLiteral("out")))
        return QSqlDatabase::database(QStringLiteral("out"), false);
    return QSqlDatabase();
}

QSet<QString> VizDataProvider::availableTables() const
{
    QSet<QString> tables;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
        return tables;

    QSqlQuery q(db);
    if (q.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table'"))) {
        while (q.next())
            tables.insert(q.value(0).toString());
    }
    return tables;
}

QStringList VizDataProvider::tableColumns(const QString &tableName) const
{
    QStringList cols;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || tableName.isEmpty())
        return cols;
    const QSqlRecord rec = db.record(tableName);
    for (int i = 0; i < rec.count(); ++i)
        cols.append(rec.fieldName(i));
    return cols;
}

QString VizDataProvider::runInfoText() const
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
        return QString();
    QSqlQuery q(db);
    if (q.exec(QStringLiteral("SELECT timestamp, version FROM runinfo LIMIT 1")) && q.next()) {
        return QString("Run: %1 | %2").arg(q.value(0).toString(), q.value(1).toString());
    }
    return QString();
}

void VizDataProvider::loadSpeciesFromInputDbPath(const QString &dbPath)
{
    if (dbPath.isEmpty() || !QFile::exists(dbPath))
        return;

    const QString connName = QStringLiteral("viz_species_tmp");
    {
        QSqlDatabase spDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        spDb.setDatabaseName(dbPath);
        spDb.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (spDb.open()) {
            QSqlQuery q(spDb);
            if (q.exec(QStringLiteral("SELECT shortName, name, displayColor FROM species"))) {
                while (q.next()) {
                    const QString id = q.value(0).toString();
                    const QString name = q.value(1).toString();
                    QString hex = q.value(2).toString().trimmed();
                    if (!hex.isEmpty() && !hex.startsWith(QLatin1Char('#')))
                        hex.prepend(QLatin1Char('#'));
                    QColor col = QColor::fromString(hex);
                    if (!col.isValid())
                        col = defaultSpeciesFallbackColor(id);
                    mSpeciesCache.insert(id, qMakePair(name.isEmpty() ? id : name, col));
                }
            }
            spDb.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
}

void VizDataProvider::ensureSpeciesCache()
{
    Model *m = GlobalSettings::instance()->model();
    const bool modelReady = (m && m->isSetup() && m->speciesSet());

    if (mSpeciesCacheLoaded) {
        if (mSpeciesCacheFromModel == modelReady && (modelReady || mSpeciesCacheProjectPath == mProjectXmlPath))
            return;
    }

    mSpeciesCache.clear();
    mSpeciesCacheFromModel = modelReady;
    mSpeciesCacheProjectPath = mProjectXmlPath;
    mSpeciesCacheLoaded = true;

    if (modelReady) {
        const SpeciesSet *set = m->speciesSet();
        for (const Species *s : set->activeSpecies()) {
            if (s)
                mSpeciesCache.insert(s->id(), qMakePair(s->name(), s->displayColor()));
        }
        return;
    }

    if (QSqlDatabase::contains(QStringLiteral("in"))) {
        QSqlDatabase dbin = QSqlDatabase::database(QStringLiteral("in"), false);
        if (dbin.isValid() && dbin.isOpen()) {
            QSqlQuery q(dbin);
            if (q.exec(QStringLiteral("SELECT shortName, name, displayColor FROM species"))) {
                while (q.next()) {
                    const QString id = q.value(0).toString();
                    const QString name = q.value(1).toString();
                    QString hex = q.value(2).toString().trimmed();
                    if (!hex.isEmpty() && !hex.startsWith(QLatin1Char('#')))
                        hex.prepend(QLatin1Char('#'));
                    QColor col = QColor::fromString(hex);
                    if (!col.isValid())
                        col = defaultSpeciesFallbackColor(id);
                    mSpeciesCache.insert(id, qMakePair(name.isEmpty() ? id : name, col));
                }
                return;
            }
        }
    }

    // Fallback: locate species input DB from project XML
    QString xmlCandidate = mProjectXmlPath;
    if ((xmlCandidate.isEmpty() || !QFile::exists(xmlCandidate)) && mExternalMode && !mExternalDbPath.isEmpty()) {
        QDir dir = QFileInfo(mExternalDbPath).absoluteDir();
        QStringList xmls = dir.entryList({ "*.xml" }, QDir::Files);
        if (xmls.isEmpty() && dir.cdUp())
            xmls = dir.entryList({ "*.xml" }, QDir::Files);
        if (!xmls.isEmpty())
            xmlCandidate = dir.filePath(xmls.first());
    }

    if (!xmlCandidate.isEmpty() && QFile::exists(xmlCandidate)) {
        try {
            XmlHelper xml(xmlCandidate);
            const QString xmlDir = QFileInfo(xmlCandidate).absolutePath();
            QString homeDir = xml.value("system.path.home", "", false);
            if (homeDir.isEmpty())
                homeDir = xmlDir;
            else if (QFileInfo(homeDir).isRelative())
                homeDir = QDir::cleanPath(QDir(xmlDir).filePath(homeDir));

            const QString dbDirRel = xml.value("system.path.database", "database", false);
            const QString dbDir = resolveXmlRelativePath(dbDirRel, homeDir, xmlDir);
            const QString inDbFile = xml.value("system.database.in", "", false);
            const QString inDbPath = resolveXmlRelativePath(inDbFile, dbDir, homeDir);
            loadSpeciesFromInputDbPath(inDbPath);
        } catch (const IException &) {
            // Silently ignore unreadable XML; fallback colors will be used
        }
    }
}

QPair<QString, QColor> VizDataProvider::resolveSpecies(const QString &speciesId)
{
    ensureSpeciesCache();
    if (mSpeciesCache.contains(speciesId))
        return mSpeciesCache.value(speciesId);
    QColor c = defaultSpeciesFallbackColor(speciesId);
    return qMakePair(speciesId, c);
}

QColor VizDataProvider::resolveSeriesColor(const VizSpec &spec, const QString &key, int seriesIndex, const QString &explicitColor)
{
    if (!explicitColor.isEmpty()) {
        QColor c(explicitColor);
        if (c.isValid())
            return c;
    }
    if (spec.groupColors.contains(key)) {
        QColor c(spec.groupColors.value(key));
        if (c.isValid())
            return c;
    }
    if (spec.colorMode == QLatin1String("species") || spec.group == QLatin1String("species"))
        return resolveSpecies(key).second;

    return Colors::colorFromPalette(seriesIndex, GridViewBrewerQual);
}

QString VizDataProvider::resolveGroupLabel(const VizSpec &spec, const QString &groupKey)
{
    if (spec.groupLabels.contains(groupKey))
        return spec.groupLabels.value(groupKey);
    if (spec.colorMode == QLatin1String("species") || spec.group == QLatin1String("species")) {
        QString spName = resolveSpecies(groupKey).first;
        if (!spName.isEmpty() && spName != groupKey)
            return QString("%1 (%2)").arg(spName, groupKey);
    }
    return groupKey;
}

QVector<VizFilterOption> VizDataProvider::fetchFilterOptions(const VizSpec &spec, const QString &filterCol)
{
    QVector<VizFilterOption> options;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || spec.table.isEmpty())
        return options;

    const QStringList cols = tableColumns(spec.table);
    if (!cols.contains(filterCol))
        return options;

    if (filterCol == QLatin1String("ru")) {
        const bool hasRid = cols.contains(QStringLiteral("rid"));
        const bool hasYear = cols.contains(QStringLiteral("year"));

        QSqlQuery q(db);
        QString sql;
        if (hasRid && hasYear) {
            sql = QString("SELECT DISTINCT ru, rid FROM \"%1\" WHERE ru >= 0 AND year = (SELECT MIN(year) FROM \"%1\" WHERE ru >= 0) ORDER BY ru").arg(spec.table);
        } else if (hasRid) {
            sql = QString("SELECT DISTINCT ru, rid FROM \"%1\" WHERE ru >= 0 ORDER BY ru").arg(spec.table);
        } else {
            sql = QString("SELECT DISTINCT ru FROM \"%1\" WHERE ru >= 0 ORDER BY ru").arg(spec.table);
        }

        if (q.exec(sql)) {
            while (q.next()) {
                int ruVal = q.value(0).toInt();
                int ridVal = hasRid ? q.value(1).toInt() : -1;
                QString label;
                if (ruVal == -1)
                    label = QStringLiteral("Landscape (-1)");
                else if (hasRid && ridVal >= 0)
                    label = QString("RU %1 (rid %2)").arg(ruVal).arg(ridVal);
                else
                    label = QString("RU %1").arg(ruVal);
                options.append({ ruVal, label });
            }
        }
        return options;
    }

    if (filterCol == QLatin1String("species")) {
        QStringList dbSpecies;
        QSqlQuery q(db);
        if (q.exec(QString("SELECT DISTINCT species FROM \"%1\" ORDER BY species").arg(spec.table))) {
            while (q.next()) {
                const QString sp = q.value(0).toString();
                if (!sp.isEmpty())
                    dbSpecies.append(sp);
            }
        }
        if (dbSpecies.isEmpty() && !mExternalMode && GlobalSettings::instance()->model()) {
            for (const Species *s : GlobalSettings::instance()->controller()->availableSpecies())
                dbSpecies.append(s->id());
        }
        for (const QString &sp : dbSpecies) {
            const QString spName = resolveSpecies(sp).first;
            const QString label = (spName != sp) ? QString("%1 (%2)").arg(spName, sp) : sp;
            options.append({ sp, label });
        }
        return options;
    }

    // Generic discrete column filter ("id", "agent", "reason", "stage", etc.)
    QSqlQuery q(db);
    if (q.exec(QString("SELECT DISTINCT \"%1\" FROM \"%2\" ORDER BY \"%1\"").arg(filterCol, spec.table))) {
        while (q.next()) {
            QVariant val = q.value(0);
            QString key = val.toString();
            QString label = spec.groupLabels.contains(key) ? QString("%1 (%2)").arg(spec.groupLabels.value(key), key) : key;
            options.append({ val, label });
        }
    }
    return options;
}

QString VizDataProvider::prepareFilterSql(const QString &rawSql, const QMap<QString, QVariant> &filterValues,
                                          const QSet<QString> &allowedWrapFilters, QStringList *usedPlaceholders)
{
    QString sql = stripTrailingSemicolon(rawSql);
    QStringList extraWheres;

    for (auto it = filterValues.constBegin(); it != filterValues.constEnd(); ++it) {
        if (!it.value().isValid())
            continue;
        const QString placeholder = QLatin1Char(':') + it.key();
        const QRegularExpression rx(QStringLiteral(":%1\\b").arg(QRegularExpression::escape(it.key())));
        if (rx.match(sql).hasMatch()) {
            if (usedPlaceholders)
                usedPlaceholders->append(placeholder);
        } else if (allowedWrapFilters.contains(it.key())) {
            extraWheres.append(QString("\"%1\" = %2").arg(it.key(), placeholder));
            if (usedPlaceholders)
                usedPlaceholders->append(placeholder);
        }
    }

    if (!extraWheres.isEmpty()) {
        sql = QString("SELECT * FROM (%1) _viz_sub WHERE %2").arg(sql, extraWheres.join(QStringLiteral(" AND ")));
    }
    return sql;
}

void VizDataProvider::bindSqlParameters(QSqlQuery &query, const QString &sql, const QMap<QString, QVariant> &params)
{
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        if (!it.value().isValid())
            continue;
        const QRegularExpression rx(QStringLiteral(":%1\\b").arg(QRegularExpression::escape(it.key())));
        if (rx.match(sql).hasMatch())
            query.bindValue(QLatin1Char(':') + it.key(), it.value());
    }
}

double VizDataProvider::computeQuantile(const QVector<double> &sorted, double p)
{
    if (sorted.isEmpty())
        return 0.0;
    if (sorted.size() == 1)
        return sorted.first();
    const double pos = p * (sorted.size() - 1);
    const int idx = static_cast<int>(std::floor(pos));
    const double frac = pos - idx;
    if (idx + 1 >= sorted.size())
        return sorted.last();
    return sorted[idx] * (1.0 - frac) + sorted[idx + 1] * frac;
}

VizChartData VizDataProvider::fetchChartData(const VizSpec &spec, int metricIndex, const QMap<QString, QVariant> &filterValues,
                                             bool normalizePercent, int liveCompletedYearHint)
{
    VizChartData result;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        result.errorMessage = QStringLiteral("No active output database.");
        return result;
    }

    QSet<QString> wrapAllowed;
    for (const QString &f : spec.filters)
        wrapAllowed.insert(f);

    const QString sql = prepareFilterSql(spec.sql, filterValues, wrapAllowed, nullptr);
    QSqlQuery q(db);
    if (!q.prepare(sql)) {
        result.errorMessage = QString("SQL prepare error: %1\nQuery: %2").arg(q.lastError().text(), sql);
        return result;
    }
    bindSqlParameters(q, sql, filterValues);

    if (!q.exec()) {
        result.errorMessage = QString("SQL execution error: %1\nQuery: %2").arg(q.lastError().text(), sql);
        return result;
    }

    const QSqlRecord rec = q.record();
    const int xColIdx = rec.indexOf(spec.x);
    if (xColIdx < 0) {
        result.errorMessage = QString("Column '%1' (x-axis) not found in query result.").arg(spec.x);
        return result;
    }

    result.xAxisTitle = spec.xLabel.isEmpty() ? spec.x : spec.xLabel;

    // -----------------------------------------------------------------------
    // Boxplot mode
    // -----------------------------------------------------------------------
    if (spec.type == QLatin1String("boxplot")) {
        if (spec.metrics.isEmpty()) {
            result.errorMessage = QStringLiteral("Boxplot visualization defines no metrics.");
            return result;
        }
        const int clampedIdx = qBound(0, metricIndex, spec.metrics.size() - 1);
        const VizMetricSpec &metric = spec.metrics[clampedIdx];
        const int yColIdx = rec.indexOf(metric.col);
        if (yColIdx < 0) {
            result.errorMessage = QString("Metric column '%1' not found in query result.").arg(metric.col);
            return result;
        }
        result.yAxisTitle = spec.yLabel.isEmpty() ? metric.label : spec.yLabel;

        QMap<double, QVector<double>> groupedNumeric;
        QMap<QString, QVector<double>> groupedString;
        QStringList stringOrder;
        bool numericCategories = !spec.xCategories.isEmpty();

        while (q.next()) {
            const QVariant xVal = q.value(xColIdx);
            const QVariant yVal = q.value(yColIdx);
            if (xVal.isNull() || yVal.isNull())
                continue;
            bool yOk = false;
            const double y = yVal.toDouble(&yOk);
            if (!yOk || !std::isfinite(y))
                continue;

            bool xOk = false;
            const double xNum = xVal.toDouble(&xOk);
            if (xOk && (numericCategories || groupedString.isEmpty())) {
                numericCategories = true;
                groupedNumeric[xNum].append(y);
            } else {
                numericCategories = false;
                const QString key = xVal.toString();
                if (!groupedString.contains(key))
                    stringOrder.append(key);
                groupedString[key].append(y);
            }
        }

        if (numericCategories) {
            if (!spec.xCategories.isEmpty() && spec.x == QLatin1String("month")) {
                // Always show months 1..12 in calendar order
                for (int m = 1; m <= spec.xCategories.size(); ++m) {
                    VizBoxPoint bp;
                    bp.categoryValue = m;
                    bp.categoryLabel = spec.xCategories[m - 1];
                    QVector<double> vals = groupedNumeric.value(static_cast<double>(m));
                    if (!vals.isEmpty()) {
                        std::sort(vals.begin(), vals.end());
                        bp.count = vals.size();
                        bp.min = vals.first();
                        bp.q25 = computeQuantile(vals, 0.25);
                        bp.median = computeQuantile(vals, 0.50);
                        bp.q75 = computeQuantile(vals, 0.75);
                        bp.max = vals.last();
                    }
                    result.boxes.append(bp);
                }
            } else {
                for (auto it = groupedNumeric.constBegin(); it != groupedNumeric.constEnd(); ++it) {
                    VizBoxPoint bp;
                    bp.categoryValue = it.key();
                    int catIdx = qRound(it.key()) - 1;
                    bp.categoryLabel = (catIdx >= 0 && catIdx < spec.xCategories.size())
                                           ? spec.xCategories[catIdx]
                                           : QString::number(it.key());
                    QVector<double> vals = it.value();
                    std::sort(vals.begin(), vals.end());
                    bp.count = vals.size();
                    bp.min = vals.first();
                    bp.q25 = computeQuantile(vals, 0.25);
                    bp.median = computeQuantile(vals, 0.50);
                    bp.q75 = computeQuantile(vals, 0.75);
                    bp.max = vals.last();
                    result.boxes.append(bp);
                }
            }
        } else {
            for (const QString &key : stringOrder) {
                VizBoxPoint bp;
                bp.categoryLabel = key;
                QVector<double> vals = groupedString.value(key);
                if (!vals.isEmpty()) {
                    std::sort(vals.begin(), vals.end());
                    bp.count = vals.size();
                    bp.min = vals.first();
                    bp.q25 = computeQuantile(vals, 0.25);
                    bp.median = computeQuantile(vals, 0.50);
                    bp.q75 = computeQuantile(vals, 0.75);
                    bp.max = vals.last();
                }
                result.boxes.append(bp);
            }
        }
        return result;
    }

    // -----------------------------------------------------------------------
    // Line / Stacked Area / Bar / Stacked Bar modes
    // -----------------------------------------------------------------------
    const bool hasGroup = !spec.group.isEmpty();
    const int groupColIdx = hasGroup ? rec.indexOf(spec.group) : -1;
    if (hasGroup && groupColIdx < 0) {
        result.errorMessage = QString("Group column '%1' not found in query result.").arg(spec.group);
        return result;
    }

    QVector<int> activeMetricIndices;
    QVector<int> activeMetricColIdx;
    if (spec.metricsMode == QLatin1String("all")) {
        for (int i = 0; i < spec.metrics.size(); ++i) {
            int cIdx = rec.indexOf(spec.metrics[i].col);
            if (cIdx >= 0) {
                activeMetricIndices.append(i);
                activeMetricColIdx.append(cIdx);
            }
        }
        if (activeMetricIndices.isEmpty()) {
            result.errorMessage = QStringLiteral("None of the configured metric columns were found in query result.");
            return result;
        }
        result.yAxisTitle = spec.yLabel;
    } else {
        if (spec.metrics.isEmpty()) {
            result.errorMessage = QStringLiteral("Visualization defines no metrics.");
            return result;
        }
        const int clampedIdx = qBound(0, metricIndex, spec.metrics.size() - 1);
        int cIdx = rec.indexOf(spec.metrics[clampedIdx].col);
        if (cIdx < 0) {
            result.errorMessage = QString("Metric column '%1' not found in query result.").arg(spec.metrics[clampedIdx].col);
            return result;
        }
        activeMetricIndices.append(clampedIdx);
        activeMetricColIdx.append(cIdx);
        result.yAxisTitle = spec.yLabel.isEmpty() ? spec.metrics[clampedIdx].label : spec.yLabel;
    }

    // Read rows into map: seriesKey -> (x -> value)
    QStringList seriesKeys;
    QMap<QString, int> seriesKeyToIdx;
    QVector<QMap<double, double>> rawSeriesValues;
    QVector<VizMetricSpec> seriesMetricMeta;
    QSet<double> uniqueXSet;

    auto ensureSeries = [&](const QString &key, const VizMetricSpec &ms) -> int {
        auto it = seriesKeyToIdx.constFind(key);
        if (it != seriesKeyToIdx.constEnd())
            return it.value();
        int idx = seriesKeys.size();
        seriesKeys.append(key);
        seriesKeyToIdx.insert(key, idx);
        rawSeriesValues.append(QMap<double, double>());
        seriesMetricMeta.append(ms);
        return idx;
    };

    if (!hasGroup) {
        // Pre-seed all metrics in configured order so legend order stays fixed even before rows arrive
        for (int i = 0; i < activeMetricIndices.size(); ++i) {
            const VizMetricSpec &ms = spec.metrics[activeMetricIndices[i]];
            ensureSeries(ms.col, ms);
        }
    }

    while (q.next()) {
        bool xOk = false;
        const double x = q.value(xColIdx).toDouble(&xOk);
        if (!xOk || !std::isfinite(x))
            continue;
        uniqueXSet.insert(x);

        if (hasGroup) {
            const QString gKey = q.value(groupColIdx).toString();
            const VizMetricSpec &ms = spec.metrics[activeMetricIndices.first()];
            const QVariant yVar = q.value(activeMetricColIdx.first());
            if (!yVar.isNull()) {
                bool yOk = false;
                double y = yVar.toDouble(&yOk);
                if (yOk && std::isfinite(y)) {
                    int sIdx = ensureSeries(gKey, ms);
                    rawSeriesValues[sIdx][x] = y;
                }
            }
        } else {
            for (int i = 0; i < activeMetricIndices.size(); ++i) {
                const VizMetricSpec &ms = spec.metrics[activeMetricIndices[i]];
                const QVariant yVar = q.value(activeMetricColIdx[i]);
                if (!yVar.isNull()) {
                    bool yOk = false;
                    double y = yVar.toDouble(&yOk);
                    if (yOk && std::isfinite(y)) {
                        int sIdx = ensureSeries(ms.col, ms);
                        rawSeriesValues[sIdx][x] = y;
                    }
                }
            }
        }
    }

    // Build sorted X vector (optionally zero-filling sparse integer years)
    QList<double> sortedX = uniqueXSet.values();
    std::sort(sortedX.begin(), sortedX.end());

    if (spec.fillMissingYears) {
        int minY = sortedX.isEmpty() ? 1 : qMin(1, qRound(sortedX.first()));
        int maxY = sortedX.isEmpty() ? 0 : qRound(sortedX.last());
        if (liveCompletedYearHint > maxY)
            maxY = liveCompletedYearHint;
        if (maxY >= minY && (maxY - minY) <= 100000) {
            result.xValues.reserve(maxY - minY + 1);
            for (int yr = minY; yr <= maxY; ++yr)
                result.xValues.append(static_cast<double>(yr));
        } else {
            result.xValues = sortedX.toVector();
        }
    } else {
        result.xValues = sortedX.toVector();
    }

    if (result.xValues.isEmpty())
        return result;

    const bool alignGrid = spec.isStacked() || spec.type == QLatin1String("bar") || spec.fillMissingYears;
    const double missingFill = alignGrid ? 0.0 : std::numeric_limits<double>::quiet_NaN();

    // For grouped queries, sort group keys nicely (by species or alphabetically)
    QVector<int> order(seriesKeys.size());
    for (int i = 0; i < seriesKeys.size(); ++i)
        order[i] = i;
    if (hasGroup) {
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            return seriesKeys[a] < seriesKeys[b];
        });
    }

    result.series.reserve(order.size());
    for (int pos = 0; pos < order.size(); ++pos) {
        const int idx = order[pos];
        VizSeriesData sd;
        sd.key = seriesKeys[idx];
        if (hasGroup) {
            sd.label = resolveGroupLabel(spec, sd.key);
            sd.color = resolveSeriesColor(spec, sd.key, pos);
        } else {
            sd.label = seriesMetricMeta[idx].label;
            sd.color = resolveSeriesColor(spec, sd.key, pos, seriesMetricMeta[idx].color);
        }

        sd.y.resize(result.xValues.size());
        const QMap<double, double> &valMap = rawSeriesValues[idx];
        for (int xi = 0; xi < result.xValues.size(); ++xi) {
            auto it = valMap.constFind(result.xValues[xi]);
            sd.y[xi] = (it != valMap.constEnd()) ? it.value() : missingFill;
        }
        result.series.append(sd);
    }

    if (normalizePercent && spec.isStacked() && !result.series.isEmpty()) {
        for (int xi = 0; xi < result.xValues.size(); ++xi) {
            double total = 0.0;
            for (const VizSeriesData &sd : result.series) {
                if (std::isfinite(sd.y[xi]) && sd.y[xi] > 0.0)
                    total += sd.y[xi];
            }
            if (total > 0.0) {
                for (VizSeriesData &sd : result.series) {
                    if (std::isfinite(sd.y[xi]))
                        sd.y[xi] = 100.0 * sd.y[xi] / total;
                }
            }
        }
        result.yAxisTitle = QStringLiteral("Share (%)");
    }

    return result;
}

QVector<int> VizDataProvider::fetchSpatialYears(const VizSpec &spec)
{
    QVector<int> years;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
        return years;

    const QRegularExpression yearParamRx(QStringLiteral(":year\\b"));
    QString sql;
    if (!yearParamRx.match(spec.sql).hasMatch()) {
        sql = QString("SELECT DISTINCT year FROM (%1) _viz_y ORDER BY year").arg(stripTrailingSemicolon(spec.sql));
    } else if (!spec.table.isEmpty()) {
        const QStringList cols = tableColumns(spec.table);
        if (cols.contains(QStringLiteral("ru")))
            sql = QString("SELECT DISTINCT year FROM \"%1\" WHERE ru >= 0 ORDER BY year").arg(spec.table);
        else
            sql = QString("SELECT DISTINCT year FROM \"%1\" ORDER BY year").arg(spec.table);
    } else {
        return years;
    }

    QSqlQuery q(db);
    if (q.exec(sql)) {
        while (q.next()) {
            bool ok = false;
            int yr = q.value(0).toInt(&ok);
            if (ok)
                years.append(yr);
        }
    }
    return years;
}

QPair<double, double> VizDataProvider::fetchSpatialGlobalRange(const VizSpec &spec, int metricIndex, const QString &speciesFilter)
{
    QPair<double, double> range(0.0, 1.0);
    if (spec.metrics.isEmpty() ||
        spec.speciesMode == QLatin1String("dominant") ||
        speciesFilter == QLatin1String(kDominantSpeciesToken))
        return range;

    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen())
        return range;

    const int clampedIdx = qBound(0, metricIndex, spec.metrics.size() - 1);
    const VizMetricSpec &metric = spec.metrics[clampedIdx];
    const QString metricCol = metric.col;
    const bool isAdditive = (metric.agg != QLatin1String("none"));
    const bool zeroBaseline = isAdditive && (!speciesFilter.isEmpty() || tableOmitsZeroRuRows(spec.table));

    const QStringList cols = spec.table.isEmpty() ? QStringList() : tableColumns(spec.table);
    QString keyCol = (spec.spatialKey == QLatin1String("ru")) ? QStringLiteral("ru") : QStringLiteral("rid");
    if (!cols.isEmpty() && !cols.contains(keyCol)) {
        const QString altKey = (keyCol == QLatin1String("rid")) ? QStringLiteral("ru") : QStringLiteral("rid");
        if (cols.contains(altKey))
            keyCol = altKey;
    }

    const QRegularExpression yearParamRx(QStringLiteral(":year\\b"));
    if (!yearParamRx.match(spec.sql).hasMatch()) {
        const QString baseSql = stripTrailingSemicolon(spec.sql);
        QString sql;
        QSqlQuery q(db);
        if (speciesFilter == QLatin1String(kTotalSpeciesToken)) {
            sql = QString("SELECT MIN(s), MAX(s) FROM (SELECT SUM(\"%1\") AS s FROM (%2) _viz_g GROUP BY year, \"%3\")")
                      .arg(metricCol, baseSql, keyCol);
            q.prepare(sql);
        } else if (!speciesFilter.isEmpty()) {
            sql = QString("SELECT MIN(\"%1\"), MAX(\"%1\") FROM (%2) _viz_g WHERE species = :species")
                      .arg(metricCol, baseSql);
            q.prepare(sql);
            q.bindValue(QStringLiteral(":species"), speciesFilter);
        } else {
            sql = QString("SELECT MIN(\"%1\"), MAX(\"%1\") FROM (%2) _viz_g")
                      .arg(metricCol, baseSql);
            q.prepare(sql);
        }

        if (q.exec() && q.next()) {
            bool okMin = false, okMax = false;
            double mn = q.value(0).toDouble(&okMin);
            double mx = q.value(1).toDouble(&okMax);
            if (okMin && okMax && std::isfinite(mn) && std::isfinite(mx)) {
                if (zeroBaseline)
                    mn = qMin(0.0, mn); // additive single-species / omitted-row tables include 0 baseline
                if (mx <= mn)
                    mx = mn + 1.0;
                return qMakePair(mn, mx);
            }
        }
    }

    // Fallback if fast aggregate query failed or user SQL explicitly embeds :year
    const QVector<int> years = fetchSpatialYears(spec);
    bool initialized = false;
    double globalMin = 0.0;
    double globalMax = 0.0;
    for (int yr : years) {
        SpatialFrameData frame = fetchSpatialFrame(spec, yr, metricIndex, speciesFilter);
        if (frame.hasError() || frame.valuesByKey.isEmpty())
            continue;
        for (double v : frame.valuesByKey) {
            if (!initialized) {
                globalMin = globalMax = v;
                initialized = true;
            } else {
                globalMin = qMin(globalMin, v);
                globalMax = qMax(globalMax, v);
            }
        }
    }
    if (initialized) {
        if (zeroBaseline)
            globalMin = qMin(0.0, globalMin);
        if (globalMax <= globalMin)
            globalMax = globalMin + 1.0;
        range = qMakePair(globalMin, globalMax);
    }
    return range;
}

SpatialFrameData VizDataProvider::fetchSpatialFrame(const VizSpec &spec, int year, int metricIndex, const QString &speciesFilter)
{
    const QString effectiveSpecies = (spec.speciesMode == QLatin1String("dominant"))
                                         ? QString::fromLatin1(kDominantSpeciesToken)
                                         : speciesFilter;

    SpatialFrameData frame;
    frame.year = year;
    frame.speciesFilter = effectiveSpecies;
    frame.useRid = (spec.spatialKey != QLatin1String("ru"));

    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        frame.errorMessage = QStringLiteral("No active output database.");
        return frame;
    }
    if (spec.metrics.isEmpty()) {
        frame.errorMessage = QStringLiteral("Spatial visualization defines no metrics.");
        return frame;
    }

    const int clampedIdx = qBound(0, metricIndex, spec.metrics.size() - 1);
    const VizMetricSpec &metric = spec.metrics[clampedIdx];
    const bool isAdditive = (metric.agg != QLatin1String("none"));
    frame.metricLabel = metric.label;
    frame.isDominantSpeciesMode = (effectiveSpecies == QLatin1String(kDominantSpeciesToken));
    frame.zeroFillValidCells = isAdditive && (!effectiveSpecies.isEmpty() || tableOmitsZeroRuRows(spec.table));

    QString sql = stripTrailingSemicolon(spec.sql);
    const QRegularExpression yearParamRx(QStringLiteral(":year\\b"));
    const bool hasExplicitYearParam = yearParamRx.match(sql).hasMatch();
    if (!hasExplicitYearParam) {
        sql = QString("SELECT * FROM (%1) _viz_sp WHERE year = :year").arg(sql);
    }

    QSqlQuery q(db);
    if (!q.prepare(sql)) {
        frame.errorMessage = QString("SQL prepare error: %1\nQuery: %2").arg(q.lastError().text(), sql);
        return frame;
    }
    q.bindValue(QStringLiteral(":year"), year);

    if (!q.exec()) {
        frame.errorMessage = QString("SQL execution error: %1\nQuery: %2").arg(q.lastError().text(), sql);
        return frame;
    }

    const QSqlRecord rec = q.record();
    int keyColIdx = rec.indexOf(frame.useRid ? QStringLiteral("rid") : QStringLiteral("ru"));
    if (keyColIdx < 0) {
        // Graceful fallback between rid and ru if only one is selected
        keyColIdx = rec.indexOf(frame.useRid ? QStringLiteral("ru") : QStringLiteral("rid"));
        if (keyColIdx >= 0)
            frame.useRid = !frame.useRid;
    }
    if (keyColIdx < 0) {
        frame.errorMessage = QString("Neither 'rid' nor 'ru' column found in spatial query.");
        return frame;
    }

    const int metricColIdx = rec.indexOf(metric.col);
    if (metricColIdx < 0) {
        frame.errorMessage = QString("Metric column '%1' not found in spatial query.").arg(metric.col);
        return frame;
    }
    const int speciesColIdx = rec.indexOf(QStringLiteral("species"));

    QHash<int, double> totalForDominant;
    while (q.next()) {
        bool kOk = false;
        const int key = q.value(keyColIdx).toInt(&kOk);
        if (!kOk || key < 0)
            continue;

        bool vOk = false;
        const double val = q.value(metricColIdx).toDouble(&vOk);
        if (!vOk || !std::isfinite(val))
            continue;

        const QString rowSpecies = (speciesColIdx >= 0) ? q.value(speciesColIdx).toString() : QString();

        if (frame.isDominantSpeciesMode) {
            if (rowSpecies.isEmpty() || val <= 0.0)
                continue;
            totalForDominant[key] = totalForDominant.value(key, 0.0) + val;
            if (!frame.rawValueForDominant.contains(key) || val > frame.rawValueForDominant.value(key)) {
                frame.rawValueForDominant[key] = val;
                frame.dominantSpeciesByKey[key] = rowSpecies;
                if (!frame.speciesInfo.contains(rowSpecies))
                    frame.speciesInfo.insert(rowSpecies, resolveSpecies(rowSpecies));
            }
        } else if (effectiveSpecies == QLatin1String(kTotalSpeciesToken)) {
            frame.valuesByKey[key] = frame.valuesByKey.value(key, 0.0) + val;
        } else if (!effectiveSpecies.isEmpty() && speciesColIdx >= 0) {
            if (rowSpecies == effectiveSpecies)
                frame.valuesByKey[key] = val;
        } else {
            frame.valuesByKey[key] = val;
        }
    }

    if (frame.isDominantSpeciesMode) {
        for (auto it = frame.rawValueForDominant.constBegin(); it != frame.rawValueForDominant.constEnd(); ++it) {
            const double tot = totalForDominant.value(it.key(), 0.0);
            frame.dominantShareByKey.insert(it.key(), tot > 0.0 ? (it.value() / tot) : 1.0);
        }
    } else if (!frame.valuesByKey.isEmpty()) {
        QVector<double> vals;
        vals.reserve(frame.valuesByKey.size());
        for (double v : frame.valuesByKey)
            vals.append(v);
        std::sort(vals.begin(), vals.end());

        double mn = vals.first();
        double mx = vals.last();
        double p98 = computeQuantile(vals, 0.98);
        if (frame.zeroFillValidCells)
            mn = qMin(0.0, mn);
        if (mx <= mn)
            mx = mn + 1.0;
        if (p98 <= mn)
            p98 = mx;
        frame.minVal = mn;
        frame.maxVal = mx;
        frame.p98Val = p98;
    }

    return frame;
}
