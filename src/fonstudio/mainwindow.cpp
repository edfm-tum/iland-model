#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui>
#include <QtXml>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <algorithm>

#include "lightroom.h"
#include "stampcontainer.h"
#include "exception.h"
#include "paintarea.h"
#include "globalsettings.h"
#include "helper.h"
#include "debugtimer.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (!MainWindow::logSpace())
        return;

    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
        MainWindow::logSpace()->appendPlainText(msg);
        MainWindow::logSpace()->ensureCursorVisible();
        break;
    case QtWarningMsg:
        MainWindow::logSpace()->appendPlainText(QString("WARNING: %1").arg(msg));
        MainWindow::logSpace()->ensureCursorVisible();
        fprintf(stderr, "WARNING: %s\n", msg.toLocal8Bit().data());
        break;
    case QtCriticalMsg:
        MainWindow::logSpace()->appendPlainText(QString("CRITICAL: %1").arg(msg));
        MainWindow::logSpace()->ensureCursorVisible();
        fprintf(stderr, "Critical: %s\n", msg.toLocal8Bit().data());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg.toLocal8Bit().data());
        abort();
    }
}

QPlainTextEdit *MainWindow::mLogSpace = nullptr;

QPlainTextEdit* MainWindow::logSpace()
{
    return mLogSpace;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindowClass),
      m_lightroom(nullptr),
      m_viewContainer(nullptr),
      m_cancelRequested(false)
{
    ui->setupUi(this);

    // Configure splitter proportions
    ui->mainVerticalSplitter->setStretchFactor(0, 4);
    ui->mainVerticalSplitter->setStretchFactor(1, 1);
    ui->viewSplitter->setStretchFactor(0, 2);
    ui->viewSplitter->setStretchFactor(1, 3);

    mLogSpace = ui->logOutput;
    qInstallMessageHandler(myMessageOutput);

    connect(ui->iniEdit, &QPlainTextEdit::textChanged, this, &MainWindow::onXmlTextChanged);

    // Determine initial file from command line or default simplefon.xml
    QString initialFile;
    if (QApplication::arguments().count() > 1) {
        QString arg = QApplication::arguments().last();
        if (QFileInfo::exists(arg))
            initialFile = arg;
    }
    if (initialFile.isEmpty() && QFileInfo::exists("simplefon.xml")) {
        initialFile = "simplefon.xml";
    }

    if (!initialFile.isEmpty()) {
        if (initialFile.endsWith(".bin", Qt::CaseInsensitive)) {
            ui->mainTabWidget->setCurrentWidget(ui->tabView);
            loadLipFile(initialFile);
        } else {
            ui->initFileName->setText(initialFile);
            on_reloadFile_clicked();
        }
    } else {
        updateXmlSummary();
    }

    qDebug() << "iLand Lightroom initialized. Ideal thread count:" << QThread::idealThreadCount();
}

MainWindow::~MainWindow()
{
    qInstallMessageHandler(nullptr);
    mLogSpace = nullptr;
    delete m_lightroom;
    delete m_viewContainer;
    delete ui;
}

bool MainWindow::parseCurrentXml(QDomDocument &doc, QString *errorMsg)
{
    doc.clear();
    QString xmlText = ui->iniEdit->toPlainText().trimmed();
    if (xmlText.isEmpty()) {
        if (errorMsg)
            *errorMsg = "XML content is empty.";
        return false;
    }

    QDockWidget *dummy = nullptr;
    Q_UNUSED(dummy);
    QDomDocument::ParseResult res = doc.setContent(xmlText);
    if (!res) {
        if (errorMsg) {
            *errorMsg = QString("XML parse error at line %1, column %2: %3")
                            .arg(res.errorLine)
                            .arg(res.errorColumn)
                            .arg(res.errorMessage);
        }
        return false;
    }

    if (doc.documentElement().tagName() != "lightroom") {
        if (errorMsg)
            *errorMsg = "Root XML node must be <lightroom>.";
        return false;
    }
    return true;
}

void MainWindow::onXmlTextChanged()
{
    updateXmlSummary();
}

void MainWindow::updateXmlSummary()
{
    QDomDocument doc;
    QString err;
    if (!parseCurrentXml(doc, &err)) {
        ui->xmlSummaryLabel->setText(QString("<b style='color:#b02020;'>%1</b>").arg(err.toHtmlEscaped()));
        return;
    }

    QDomElement root = doc.documentElement();
    QDomElement sizeEl = root.firstChildElement("size");
    int x = sizeEl.attribute("x").toInt();
    int y = sizeEl.attribute("y").toInt();
    int z = sizeEl.attribute("z").toInt();
    double cellsize = sizeEl.attribute("cellsize").toDouble();

    QDomElement hemiEl = root.firstChildElement("hemigrid");
    int hemisize = hemiEl.attribute("size").toInt();
    double lat = hemiEl.attribute("latitude").toDouble();
    double diffus = hemiEl.attribute("diffus").toDouble();

    QString cutMode = root.firstChildElement("cutoffMode").text().trimmed();
    double cutVal = root.firstChildElement("cutvalue").text().toDouble();
    QString aggMode = root.firstChildElement("aggregationMode").text().trimmed();
    QString readerFile = root.firstChildElement("readerStamp").text().trimmed();
    QString outputFile = root.firstChildElement("outputStamp").text().trimmed();
    QString desc = root.firstChildElement("desc").text().trimmed();

    int treeCount = 0;
    QDomElement tree = root.firstChildElement("trees").firstChildElement("tree");
    while (!tree.isNull()) {
        ++treeCount;
        tree = tree.nextSiblingElement("tree");
    }

    bool readerExists = !readerFile.isEmpty() && QFileInfo::exists(readerFile);
    QString readerStatus = readerExists
        ? "<span style='color:#187538; font-weight:bold;'>[Found]</span>"
        : "<span style='color:#c03020; font-weight:bold;'>[Missing - create or check path]</span>";

    QString summary = QString(
        "<b>Room:</b> %1 x %2 x %3 m (cellsize: %4 m) &nbsp;|&nbsp; "
        "<b>Hemigrid:</b> %5&deg;, lat %6&deg;, diffuse %7 &nbsp;|&nbsp; "
        "<b>Aggregation:</b> %8 &nbsp;|&nbsp; "
        "<b>Cutoff:</b> %9 (%10) &nbsp;|&nbsp; "
        "<b>Trees:</b> %11<br>"
        "<b>Reader Stamp:</b> %12 %13 &nbsp;|&nbsp; "
        "<b>Output LIP:</b> %14 &nbsp;|&nbsp; "
        "<b>Desc:</b> %15"
    ).arg(x).arg(y).arg(z).arg(cellsize)
     .arg(hemisize).arg(lat).arg(diffus)
     .arg(aggMode.isEmpty() ? "<i style='color:#c03020;'>not set</i>" : aggMode.toHtmlEscaped())
     .arg(cutMode.isEmpty() ? "<i style='color:#c03020;'>not set</i>" : cutMode.toHtmlEscaped())
     .arg(cutVal)
     .arg(treeCount)
     .arg(readerFile.isEmpty() ? "<i>none</i>" : readerFile.toHtmlEscaped())
     .arg(readerStatus)
     .arg(outputFile.isEmpty() ? "<i>none</i>" : outputFile.toHtmlEscaped())
     .arg(desc.toHtmlEscaped());

    ui->xmlSummaryLabel->setText(summary);
}

void MainWindow::on_openFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Lightroom XML Configuration File",
        ui->initFileName->text(),
        "XML files (*.xml);;All files (*.*)"
    );
    if (fileName.isEmpty())
        return;

    ui->initFileName->setText(fileName);
    on_reloadFile_clicked();
}

void MainWindow::on_reloadFile_clicked()
{
    QString fileName = ui->initFileName->text().trimmed();
    if (fileName.isEmpty())
        return;

    QString xmlContent = Helper::loadTextFile(fileName);
    if (xmlContent.isEmpty() && !QFileInfo::exists(fileName)) {
        QMessageBox::warning(this, "File Not Found",
                             QString("Could not load XML file:\n%1").arg(fileName));
        return;
    }
    ui->iniEdit->setPlainText(xmlContent);
    updateXmlSummary();
    qDebug() << "Loaded XML file:" << fileName;
}

void MainWindow::on_saveFile_clicked()
{
    QString fileName = ui->initFileName->text().trimmed();
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(
            this, "Save XML Configuration File", QString(), "XML files (*.xml);;All files (*.*)"
        );
        if (fileName.isEmpty())
            return;
        ui->initFileName->setText(fileName);
    }
    QString content = ui->iniEdit->toPlainText();
    Helper::saveToTextFile(fileName, content);
    qDebug() << "Saved XML file to:" << fileName;
    statusBar()->showMessage(QString("Saved %1").arg(fileName), 4000);
}

void MainWindow::setRunningState(bool running)
{
    ui->lrProcess->setEnabled(!running);
    ui->lrReadStamps->setEnabled(!running);
    ui->openFile->setEnabled(!running);
    ui->reloadFile->setEnabled(!running);
    ui->cancelRunBtn->setEnabled(running);
}

void MainWindow::on_cancelRunBtn_clicked()
{
    m_cancelRequested = true;
    qDebug() << "Cancellation requested by user...";
}

void MainWindow::on_lrProcess_clicked()
{
    QDomDocument doc;
    QString errMsg;
    if (!parseCurrentXml(doc, &errMsg)) {
        QMessageBox::warning(this, "Invalid XML", errMsg);
        return;
    }

    QDomElement docElem = doc.documentElement();

    // Parse simulation room parameters
    QDomElement sizeEl = docElem.firstChildElement("size");
    int x = sizeEl.attribute("x").toInt();
    int y = sizeEl.attribute("y").toInt();
    int z = sizeEl.attribute("z").toInt();
    double cellsize = sizeEl.attribute("cellsize").toDouble();
    if (x <= 0 || y <= 0 || z <= 0 || cellsize <= 0.) {
        QMessageBox::warning(this, "Invalid Parameters",
                             "Invalid <size> attributes (x, y, z, cellsize must be > 0).");
        return;
    }

    QDomElement hemiEl = docElem.firstChildElement("hemigrid");
    int hemisize = hemiEl.attribute("size").toInt();
    double lat = hemiEl.attribute("latitude").toFloat();
    double diffus = hemiEl.attribute("diffus").toFloat();
    if (hemisize <= 0) {
        QMessageBox::warning(this, "Invalid Parameters",
                             "Invalid <hemigrid> size attribute.");
        return;
    }

    QString outputFile = docElem.firstChildElement("outputStamp").text().trimmed();
    QString binaryReaderStampFile = docElem.firstChildElement("readerStamp").text().trimmed();
    QString stampDesc = docElem.firstChildElement("desc").text().trimmed();
    double cutThreshold = docElem.firstChildElement("cutvalue").text().toDouble();
    QString cutMode = docElem.firstChildElement("cutoffMode").text().trimmed();
    QString aggMode = docElem.firstChildElement("aggregationMode").text().trimmed();

    int mode = -1;
    if (aggMode == "sum") {
        mode = 1;
    } else if (aggMode == "mean") {
        mode = 0;
    } else {
        QMessageBox::warning(this, "Invalid Aggregation Mode",
                             "Invalid or missing <aggregationMode> in XML (expected 'mean' or 'sum').");
        return;
    }

    if (cutMode != "sum" && cutMode != "north") {
        QMessageBox::warning(this, "Invalid Cutoff Mode",
                             "Invalid or missing <cutoffMode> in XML (expected 'north' or 'sum').");
        return;
    }

    if (outputFile.isEmpty()) {
        QMessageBox::warning(this, "Missing Output File",
                             "No <outputStamp> file path specified in XML.");
        return;
    }

    // Validate reader stamp file before running
    if (binaryReaderStampFile.isEmpty() || !QFileInfo::exists(binaryReaderStampFile)) {
        QMessageBox::warning(
            this,
            "Reader Stamp File Required",
            QString("A valid reader stamp file is required before generating LIP files.\n\n"
                    "Configured path: %1\n\n"
                    "Please click 'Create Reader Stamps...' first or update <readerStamp> in the XML.")
                .arg(binaryReaderStampFile.isEmpty() ? "(empty)" : binaryReaderStampFile)
        );
        return;
    }

    StampContainer readers;
    try {
        QFile infile(binaryReaderStampFile);
        if (!infile.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Reader Stamp Error",
                                 QString("Cannot open reader stamp file:\n%1\n%2")
                                     .arg(binaryReaderStampFile, infile.errorString()));
            return;
        }
        QDataStream in(&infile);
        readers.load(in);
        infile.close();
    } catch (const IException &e) {
        QMessageBox::warning(this, "Invalid Reader Stamp File",
                             QString("Failed to load reader stamp file '%1':\n%2")
                                 .arg(binaryReaderStampFile, e.message()));
        return;
    }

    // Count trees
    QList<QDomElement> treeElements;
    QDomElement treeEl = docElem.firstChildElement("trees").firstChildElement("tree");
    while (!treeEl.isNull()) {
        treeElements.append(treeEl);
        treeEl = treeEl.nextSiblingElement("tree");
    }
    if (treeElements.isEmpty()) {
        QMessageBox::warning(this, "No Trees Defined",
                             "No <tree> elements found under <trees> in the XML configuration.");
        return;
    }

    m_cancelRequested = false;
    setRunningState(true);
    ui->progressBar->setRange(0, treeElements.count());
    ui->progressBar->setValue(0);
    statusBar()->showMessage("Setting up Lightroom...");
    QCoreApplication::processEvents();

    try {
        if (!m_lightroom)
            m_lightroom = new LightRoom();

        m_lightroom->setup(x, y, z, cellsize, hemisize, lat, diffus);

        LightRoomObject *lro = new LightRoomObject();
        m_lightroom->setLightRoomObject(lro);
        m_lightroom->setAggregationMode(mode);

        qDebug() << "Aggregation mode:" << aggMode << "| Cutoff mode:" << cutMode << "| Cut threshold:" << cutThreshold;
        qDebug() << "Loaded reader stamps from:" << binaryReaderStampFile << "(" << readers.count() << "stamps)";

        StampContainer container;
        container.useLookup(false); // disable lookup table during creation

        DebugTimer creationTime("total creation of stamps");

        for (int idx = 0; idx < treeElements.count(); ++idx) {
            if (m_cancelRequested) {
                qDebug() << "LIP creation canceled by user.";
                statusBar()->showMessage("LIP creation canceled.", 5000);
                setRunningState(false);
                return;
            }

            const QDomElement &tree = treeElements[idx];
            QString name = tree.attribute("name");
            float height = tree.attribute("h").toFloat();
            float crown = tree.attribute("crown").toFloat();
            float bhd = tree.attribute("bhd").toFloat();
            QString formula = tree.attribute("formula");

            if (bhd <= 0.f || height <= 0.f || crown < 0.f || crown >= height) {
                throw IException(QString("Invalid parameters in tree #%1 ('%2'): bhd=%3, h=%4, crown=%5.\n"
                                         "Required: bhd > 0, h > 0, and 0 <= crown < h.")
                                     .arg(idx + 1).arg(name).arg(bhd).arg(height).arg(crown));
            }

            statusBar()->showMessage(QString("Computing tree %1 / %2: %3 (DBH=%4 cm, H=%5 m)...")
                                         .arg(idx + 1).arg(treeElements.count()).arg(name).arg(bhd).arg(height));
            lro->setuptree(height, crown, formula);
            qDebug() << QString("[%1/%2] Start tree %3 (DBH=%4, H=%5, crown=%6)")
                            .arg(idx + 1).arg(treeElements.count()).arg(name).arg(bhd).arg(height).arg(crown);

            m_lightroom->calculateFullGrid();

            const FloatGrid &gr = m_lightroom->result();
            QVector<double> sums;
            QVector<double> rel_sum;
            QVector<double> max_vals;
            double total_sum = 0.;

            for (int o = 0; o < gr.sizeX() / 2; o++) {
                double sum = 0;
                int ring_count = 0;
                // top and bottom
                for (int i = o; i < gr.sizeX() - o; i++) {
                    sum += gr(i, o);
                    sum += gr(i, gr.sizeX() - 1 - o);
                    ring_count += 2;
                }
                // left, right (do not count corners twice)
                for (int i = o + 1; i < gr.sizeX() - o - 1; i++) {
                    sum += gr(o, i);
                    sum += gr(gr.sizeX() - 1 - o, i);
                    ring_count += 2;
                }
                total_sum += sum;
                sums.push_back(sum);
                rel_sum.push_back(sum / double(ring_count));
                max_vals.push_back(gr(gr.sizeX() / 2, gr.sizeX() - 1 - o));
            }
            if (gr.sizeX() % 2) {
                float centerVal = gr(gr.sizeX() / 2, gr.sizeX() / 2);
                total_sum += centerVal;
                sums.push_back(centerVal);
                rel_sum.push_back(centerVal);
                max_vals.push_back(centerVal);
            }

            int end_ring = 0;
            if (cutMode == "sum") {
                double rsum = 0;
                for (end_ring = 0; end_ring < sums.count(); end_ring++) {
                    rsum += sums[end_ring];
                    if (rsum > cutThreshold * total_sum)
                        break;
                }
            } else if (cutMode == "north") {
                for (end_ring = 0; end_ring < sums.count(); end_ring++) {
                    if (max_vals[end_ring] > cutThreshold)
                        break;
                }
            }

            end_ring = sums.count() - end_ring;
            if (end_ring < 2)
                end_ring = 2; // minimum ring-count=2 (i.e. 3x3 = 9 pixels)

            int target_grid_size = 2 * end_ring - 1;
            qDebug() << "  break at ring" << end_ring << "-> stamp size" << target_grid_size << "x" << target_grid_size;

            Stamp *stamp = stampFromGrid(gr, target_grid_size);
            if (!stamp) {
                QMessageBox::critical(this, "Stamp Creation Failed", "stampFromGrid returned NULL.");
                setRunningState(false);
                return;
            }

            // Check against reader stamp
            double maxradius = lro->maxRadius();
            const Stamp *readerstamp = readers.readerStamp(maxradius);
            if (readerstamp) {
                int offset = stamp->offset() - readerstamp->offset();
                double rsum = 0;
                if (offset >= 0) {
                    for (int rx = 0; rx < readerstamp->size(); rx++)
                        for (int ry = 0; ry < readerstamp->size(); ry++)
                            rsum += *(readerstamp->data(rx, ry)) * *(stamp->data(rx + offset, ry + offset));
                }
                qDebug() << "  reader check: crown radius =" << maxradius
                         << "m, stamp offset =" << stamp->offset()
                         << ", reader offset =" << readerstamp->offset()
                         << ", read-sum =" << rsum;
            } else {
                qWarning() << "  No reader stamp found for crown radius" << maxradius;
            }

            double hd = qRound(height * 100. / bhd);
            container.addStamp(stamp, bhd, hd, lro->maxRadius());

            ui->progressBar->setValue(idx + 1);
            QCoreApplication::processEvents();
        }

        // Ensure target directory exists and write binary LIP file
        QFileInfo outInfo(outputFile);
        if (outInfo.dir().exists() == false) {
            outInfo.dir().mkpath(".");
        }

        QFile file(outputFile);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Write Error",
                                  QString("Could not open output file for writing:\n%1\n\nError: %2")
                                      .arg(outputFile, file.errorString()));
            setRunningState(false);
            return;
        }

        container.setDescription(stampDesc);
        QDataStream out(&file);
        container.save(out);
        file.close();

        qDebug() << "Successfully wrote" << container.count() << "stamps to" << outputFile;
        statusBar()->showMessage(QString("Created LIP file: %1 (%2 stamps)").arg(outputFile).arg(container.count()), 8000);

        // Automatically load the newly created LIP file in the viewer tab
        loadLipFile(outputFile);

        QMessageBox::information(this, "LIP Creation Complete",
                                 QString("Successfully generated %1 LIP stamps and saved to:\n%2\n\n"
                                         "The file has been loaded into the 'View LIP Files' tab.")
                                     .arg(container.count())
                                     .arg(outputFile));

    } catch (const IException &e) {
        QMessageBox::critical(this, "Error During Calculation", e.message());
    } catch (const QString &s) {
        QMessageBox::critical(this, "Error During Calculation", s);
    }

    setRunningState(false);
}

void MainWindow::on_lrReadStamps_clicked()
{
    QDomDocument doc;
    parseCurrentXml(doc);
    QString defaultTarget = doc.documentElement().firstChildElement("readerStamp").text().trimmed();

    QString targetFile = QFileDialog::getSaveFileName(
        this,
        "Save Binary Reader Stamps File",
        defaultTarget.isEmpty() ? "readerstamp.bin" : defaultTarget,
        "Binary Stamp files (*.bin);;All files (*.*)"
    );
    if (targetFile.isEmpty())
        return;

    m_cancelRequested = false;
    setRunningState(true);

    // Radii from 0.5 to 15.0 with step 0.1 -> 146 steps
    const int totalSteps = 146;
    ui->progressBar->setRange(0, totalSteps);
    ui->progressBar->setValue(0);

    FloatGrid grid;
    grid.setup(QRectF(-21., -21., 42., 42.), 2.);
    StampContainer container;
    int totcount = 0;
    DebugTimer t("reader stamps creation");

    int stepIdx = 0;
    for (double radius = 0.5; radius <= 15.001; radius += 0.1) {
        if (m_cancelRequested) {
            qDebug() << "Reader stamp creation canceled.";
            setRunningState(false);
            return;
        }
        grid.initialize(0.);
        int tested = 0;
        for (float x = -radius; x <= radius; x += 0.01f) {
            for (float y = -radius; y <= radius; y += 0.01f) {
                tested++;
                if (x * x + y * y < radius * radius) {
                    grid.valueAt(x, y)++;
                }
            }
        }
        totcount += tested;
        FloatGrid ngrid = grid.normalized(1.);
        int width = (((int)radius + 1) / 2) * 2 + 1;
        Stamp *stamp = stampFromGrid(ngrid, width);
        container.addReaderStamp(stamp, radius);

        ++stepIdx;
        ui->progressBar->setValue(stepIdx);
        statusBar()->showMessage(QString("Creating reader stamp for radius %1 m...").arg(radius, 0, 'f', 1));
        QCoreApplication::processEvents();
    }

    qDebug() << "Reader stamps calculated. Total points tested:" << totcount;

    QFileInfo outInfo(targetFile);
    if (!outInfo.dir().exists())
        outInfo.dir().mkpath(".");

    QFile file(targetFile);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error Saving Reader Stamps",
                              QString("Cannot open file '%1' for writing:\n%2")
                                  .arg(targetFile, file.errorString()));
        setRunningState(false);
        return;
    }

    QDataStream out(&file);
    container.setDescription("Reader-stamps for crown-radii ranging from 0.5m to 15m, stepwidth is 0.1m.");
    container.save(out);
    file.close();

    qDebug() << "Saved" << container.count() << "reader stamps to" << targetFile;
    statusBar()->showMessage(QString("Saved reader stamps to %1").arg(targetFile), 6000);
    setRunningState(false);
    updateXmlSummary();
}

///////////////////////////////////////////////////////////////////////////////
// Part B: View LIP Files
///////////////////////////////////////////////////////////////////////////////

void MainWindow::on_openLipBtn_clicked()
{
    QString startPath = ui->lipFileEdit->text().trimmed();
    if (startPath.isEmpty()) {
        QDomDocument doc;
        if (parseCurrentXml(doc))
            startPath = doc.documentElement().firstChildElement("outputStamp").text().trimmed();
    }

    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open Binary LIP / Stamp File",
        startPath,
        "Binary LIP files (*.bin);;All files (*.*)"
    );
    if (fileName.isEmpty())
        return;

    loadLipFile(fileName);
}

void MainWindow::loadLipFile(const QString &fileName)
{
    QFile infile(fileName);
    if (!infile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Cannot Open File",
                             QString("Failed to open file:\n%1\n\nError: %2")
                                 .arg(fileName, infile.errorString()));
        return;
    }

    StampContainer *newContainer = new StampContainer();
    newContainer->useLookup(false);
    try {
        QDataStream in(&infile);
        newContainer->load(in);
        infile.close();
    } catch (const IException &e) {
        delete newContainer;
        QMessageBox::critical(this, "Error Loading LIP File",
                              QString("Failed to read '%1':\n%2").arg(fileName, e.message()));
        return;
    }

    delete m_viewContainer;
    m_viewContainer = newContainer;
    ui->lipFileEdit->setText(fileName);

    populateOverviewGrid();
    statusBar()->showMessage(QString("Loaded %1 (%2 stamps)").arg(fileName).arg(m_viewContainer->count()), 5000);
}

void MainWindow::on_dumpLipBtn_clicked()
{
    if (!m_viewContainer) {
        QMessageBox::information(this, "No File Loaded", "Please open a LIP file first.");
        return;
    }
    qDebug() << "Dumping content of StampContainer:" << ui->lipFileEdit->text();
    qDebug() << m_viewContainer->dump();
}

void MainWindow::populateOverviewGrid()
{
    ui->stampGridTable->clear();
    ui->stampGridTable->setRowCount(0);
    ui->stampGridTable->setColumnCount(0);
    ui->PaintWidget->clearStamp();

    if (!m_viewContainer || m_viewContainer->stamps().isEmpty()) {
        ui->lipSummaryLabel->setText("No stamps in container.");
        return;
    }

    const QList<StampContainer::StampItem> &stamps = m_viewContainer->stamps();

    // Compute global statistics across all stamps in this file
    int maxLogicalSize = 1;
    int minLogicalSize = 9999;
    float globalMaxVal = 0.f;
    float maxSumVal = 0.f;
    bool isReaderFile = true;

    QVector<float> stampSums(stamps.size(), 0.f);
    QVector<float> stampMaxs(stamps.size(), 0.f);

    for (int i = 0; i < stamps.size(); ++i) {
        const StampContainer::StampItem &si = stamps[i];
        if (si.dbh > 0.f)
            isReaderFile = false;

        int s = si.stamp->size();
        maxLogicalSize = qMax(maxLogicalSize, s);
        minLogicalSize = qMin(minLogicalSize, s);

        float sum = 0.f;
        float smax = 0.f;
        for (int y = 0; y < s; ++y) {
            for (int x = 0; x < s; ++x) {
                float v = (*si.stamp)(x, y);
                sum += v;
                smax = qMax(smax, v);
            }
        }
        stampSums[i] = sum;
        stampMaxs[i] = smax;
        globalMaxVal = qMax(globalMaxVal, smax);
        maxSumVal = qMax(maxSumVal, sum);
    }

    ui->PaintWidget->setReferenceContext(maxLogicalSize, globalMaxVal);

    if (!isReaderFile) {
        // Species LIP file: 2D grid of unique DBH (rows) x unique HD (columns)
        ui->overviewGridTitle->setText("Overview Grid (Rows: DBH [cm], Columns: H/D Ratio)");

        QList<int> dbhKeys; // rounded to 0.1 cm (int = qRound(dbh * 10))
        QList<int> hdKeys;  // rounded to 0.1 (int = qRound(hd * 10))
        for (const auto &si : stamps) {
            int dk = qRound(si.dbh * 10.f);
            int hk = qRound(si.hd * 10.f);
            if (!dbhKeys.contains(dk))
                dbhKeys.append(dk);
            if (!hdKeys.contains(hk))
                hdKeys.append(hk);
        }
        std::sort(dbhKeys.begin(), dbhKeys.end());
        std::sort(hdKeys.begin(), hdKeys.end());

        ui->stampGridTable->setRowCount(dbhKeys.size());
        ui->stampGridTable->setColumnCount(hdKeys.size());

        QStringList rowLabels;
        for (int dk : dbhKeys)
            rowLabels << QString("%1 cm").arg(dk / 10.0, 0, 'f', (dk % 10 == 0) ? 0 : 1);
        ui->stampGridTable->setVerticalHeaderLabels(rowLabels);

        QStringList colLabels;
        for (int hk : hdKeys)
            colLabels << QString("HD %1").arg(hk / 10.0, 0, 'f', (hk % 10 == 0) ? 0 : 1);
        ui->stampGridTable->setHorizontalHeaderLabels(colLabels);

        // Initialize all cells as unrealized (empty / disabled)
        for (int r = 0; r < dbhKeys.size(); ++r) {
            for (int c = 0; c < hdKeys.size(); ++c) {
                QTableWidgetItem *emptyItem = new QTableWidgetItem("-");
                emptyItem->setTextAlignment(Qt::AlignCenter);
                emptyItem->setFlags(Qt::NoItemFlags);
                emptyItem->setBackground(QColor(238, 240, 244));
                emptyItem->setForeground(QColor(175, 180, 190));
                emptyItem->setData(Qt::UserRole, -1);
                ui->stampGridTable->setItem(r, c, emptyItem);
            }
        }

        QTableWidgetItem *firstValidItem = nullptr;
        for (int i = 0; i < stamps.size(); ++i) {
            const auto &si = stamps[i];
            int r = dbhKeys.indexOf(qRound(si.dbh * 10.f));
            int c = hdKeys.indexOf(qRound(si.hd * 10.f));
            if (r < 0 || c < 0)
                continue;

            int s = si.stamp->size();
            float heightM = si.dbh * si.hd / 100.f;

            QTableWidgetItem *item = new QTableWidgetItem(
                QString("%1x%1\n(%2m)").arg(s).arg(heightM, 0, 'f', 1)
            );
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(Qt::UserRole, i);

            // Soft color tint proportional to stamp sum
            float frac = (maxSumVal > 0.f) ? qBound(0.f, stampSums[i] / maxSumVal, 1.f) : 0.f;
            QColor bg = QColor::fromRgb(
                255,
                qRound(248 - frac * 75),
                qRound(225 - frac * 115)
            );
            item->setBackground(bg);
            item->setForeground(QColor(25, 30, 40));
            item->setToolTip(QString("DBH: %1 cm\nHeight: %2 m (H/D: %3)\nCrown radius: %4 m\nMatrix: %5x%5 (%6x%6 m)\nMax: %7 | Sum: %8")
                                 .arg(si.dbh, 0, 'f', 1)
                                 .arg(heightM, 0, 'f', 2)
                                 .arg(si.hd, 0, 'f', 0)
                                 .arg(si.crown_radius, 0, 'f', 2)
                                 .arg(s)
                                 .arg(s * cPxSize)
                                 .arg(stampMaxs[i], 0, 'f', 3)
                                 .arg(stampSums[i], 0, 'f', 2));

            ui->stampGridTable->setItem(r, c, item);
            if (!firstValidItem)
                firstValidItem = item;
        }

        ui->lipSummaryLabel->setText(QString(
            "<b>Stamps:</b> %1 ( across %2 DBH classes &times; %3 H/D classes ) &nbsp;|&nbsp; "
            "<b>DBH Range:</b> %4 &ndash; %5 cm &nbsp;|&nbsp; "
            "<b>Matrix Size Range:</b> %6&times;%6 to %7&times;%7 (%8&times;%8 m) &nbsp;|&nbsp; "
            "<b>Description:</b> %9"
        ).arg(stamps.size())
         .arg(dbhKeys.size())
         .arg(hdKeys.size())
         .arg(dbhKeys.first() / 10.0, 0, 'f', 1)
         .arg(dbhKeys.last() / 10.0, 0, 'f', 1)
         .arg(minLogicalSize)
         .arg(maxLogicalSize)
         .arg(maxLogicalSize * cPxSize)
         .arg(m_viewContainer->description().toHtmlEscaped()));

        ui->stampGridTable->resizeColumnsToContents();
        ui->stampGridTable->resizeRowsToContents();

        if (firstValidItem)
            ui->stampGridTable->setCurrentItem(firstValidItem);

    } else {
        // Reader stamp file: 2D grid of integer meter (rows) x 0.1m decimal (columns)
        ui->overviewGridTitle->setText("Reader Stamps Grid (Rows: Crown Radius [m], Columns: +0.0 .. +0.9 m)");

        int maxMeter = 0;
        for (const auto &si : stamps)
            maxMeter = qMax(maxMeter, int(si.crown_radius + 0.001f));

        ui->stampGridTable->setRowCount(maxMeter + 1);
        ui->stampGridTable->setColumnCount(10);

        QStringList rowLabels;
        for (int r = 0; r <= maxMeter; ++r)
            rowLabels << QString("%1 m").arg(r);
        ui->stampGridTable->setVerticalHeaderLabels(rowLabels);

        QStringList colLabels;
        for (int c = 0; c < 10; ++c)
            colLabels << QString("+0.%1 m").arg(c);
        ui->stampGridTable->setHorizontalHeaderLabels(colLabels);

        for (int r = 0; r <= maxMeter; ++r) {
            for (int c = 0; c < 10; ++c) {
                QTableWidgetItem *emptyItem = new QTableWidgetItem("-");
                emptyItem->setTextAlignment(Qt::AlignCenter);
                emptyItem->setFlags(Qt::NoItemFlags);
                emptyItem->setBackground(QColor(238, 240, 244));
                emptyItem->setForeground(QColor(175, 180, 190));
                emptyItem->setData(Qt::UserRole, -1);
                ui->stampGridTable->setItem(r, c, emptyItem);
            }
        }

        QTableWidgetItem *firstValidItem = nullptr;
        for (int i = 0; i < stamps.size(); ++i) {
            const auto &si = stamps[i];
            int r = int(si.crown_radius + 0.001f);
            int c = int(( std::fmod(si.crown_radius, 1.f) + 0.001f ) * 10.f);
            c = qBound(0, c, 9);

            int s = si.stamp->size();
            QTableWidgetItem *item = new QTableWidgetItem(
                QString("r=%1m\n%2x%2").arg(si.crown_radius, 0, 'f', 1).arg(s)
            );
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(Qt::UserRole, i);
            item->setBackground(QColor(232, 245, 238));
            item->setForeground(QColor(25, 30, 40));

            ui->stampGridTable->setItem(r, c, item);
            if (!firstValidItem)
                firstValidItem = item;
        }

        ui->lipSummaryLabel->setText(QString(
            "<b>Reader Stamp File</b> &nbsp;|&nbsp; <b>Stamps:</b> %1 &nbsp;|&nbsp; "
            "<b>Max Matrix Size:</b> %2&times;%2 (%3&times;%3 m) &nbsp;|&nbsp; "
            "<b>Description:</b> %4"
        ).arg(stamps.size())
         .arg(maxLogicalSize)
         .arg(maxLogicalSize * cPxSize)
         .arg(m_viewContainer->description().toHtmlEscaped()));

        ui->stampGridTable->resizeColumnsToContents();
        ui->stampGridTable->resizeRowsToContents();

        if (firstValidItem)
            ui->stampGridTable->setCurrentItem(firstValidItem);
    }
}

void MainWindow::on_stampGridTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *)
{
    if (!current)
        return;
    int idx = current->data(Qt::UserRole).toInt();
    if (idx >= 0)
        selectStampByIndex(idx);
}

void MainWindow::selectStampByIndex(int index)
{
    if (!m_viewContainer || index < 0 || index >= m_viewContainer->stamps().size())
        return;

    const StampContainer::StampItem &si = m_viewContainer->stamps()[index];
    const Stamp *s = si.stamp;
    int sz = s->size();
    int off = s->offset();

    float sum = 0.f;
    float maxVal = 0.f;
    float minNonZero = 999999.f;
    for (int y = 0; y < sz; ++y) {
        for (int x = 0; x < sz; ++x) {
            float v = (*s)(x, y);
            sum += v;
            maxVal = qMax(maxVal, v);
            if (v > 0.f)
                minNonZero = qMin(minNonZero, v);
        }
    }
    if (minNonZero > maxVal)
        minNonZero = 0.f;
    float centerVal = (*s)(off, off);
    float heightM = si.dbh * si.hd / 100.f;

    QString details;
    if (si.dbh > 0.f) {
        details = QString(
            "<b>DBH:</b> %1 cm &nbsp;|&nbsp; <b>Height:</b> %2 m &nbsp;|&nbsp; <b>H/D Ratio:</b> %3<br>"
            "<b>Crown Radius:</b> %4 m (Area: %5 m&sup2;)<br>"
            "<b>Matrix Size:</b> %6 &times; %6 cells (%7 &times; %7 m, offset: %8, block: %9&times;%9)<br>"
            "<b>Values:</b> Center = %10 &nbsp;|&nbsp; Max = %11 &nbsp;|&nbsp; Min(&gt;0) = %12 &nbsp;|&nbsp; Sum = %13"
        ).arg(si.dbh, 0, 'f', 1)
         .arg(heightM, 0, 'f', 2)
         .arg(si.hd, 0, 'f', 0)
         .arg(si.crown_radius, 0, 'f', 2)
         .arg(si.crown_radius * si.crown_radius * M_PI, 0, 'f', 1)
         .arg(sz)
         .arg(sz * cPxSize)
         .arg(off)
         .arg(s->dataSize())
         .arg(centerVal, 0, 'f', 4)
         .arg(maxVal, 0, 'f', 4)
         .arg(minNonZero, 0, 'f', 4)
         .arg(sum, 0, 'f', 2);
    } else {
        details = QString(
            "<b>Reader Stamp</b> &nbsp;|&nbsp; <b>Crown Radius:</b> %1 m (Area: %2 m&sup2;)<br>"
            "<b>Matrix Size:</b> %3 &times; %3 cells (%4 &times; %4 m, offset: %5, block: %6&times;%6)<br>"
            "<b>Values:</b> Center = %7 &nbsp;|&nbsp; Max = %8 &nbsp;|&nbsp; Sum = %9"
        ).arg(si.crown_radius, 0, 'f', 2)
         .arg(si.crown_radius * si.crown_radius * M_PI, 0, 'f', 1)
         .arg(sz)
         .arg(sz * cPxSize)
         .arg(off)
         .arg(s->dataSize())
         .arg(centerVal, 0, 'f', 4)
         .arg(maxVal, 0, 'f', 4)
         .arg(sum, 0, 'f', 4);
    }

    ui->stampDetailsLabel->setText(details);
    ui->PaintWidget->setStamp(s, si.dbh, si.hd, si.crown_radius);
}

void MainWindow::on_rb2DView_toggled(bool checked)
{
    if (checked) {
        ui->PaintWidget->set3DMode(false);
        ui->cbShowValues->setEnabled(true);
        ui->cbShowCrown->setEnabled(true);
    }
}

void MainWindow::on_rb3DView_toggled(bool checked)
{
    if (checked) {
        ui->PaintWidget->set3DMode(true);
        ui->cbShowValues->setEnabled(false);
        ui->cbShowCrown->setEnabled(false);
    }
}

void MainWindow::on_cbFixedScale_toggled(bool checked)
{
    ui->PaintWidget->setFixedScale(checked);
}

void MainWindow::on_cbShowValues_toggled(bool checked)
{
    ui->PaintWidget->setShowValues(checked);
}

void MainWindow::on_cbShowCrown_toggled(bool checked)
{
    ui->PaintWidget->setShowCrownCircle(checked);
}

void MainWindow::on_resetViewBtn_clicked()
{
    ui->PaintWidget->resetView();
}

void MainWindow::on_clearLogBtn_clicked()
{
    ui->logOutput->clear();
}
