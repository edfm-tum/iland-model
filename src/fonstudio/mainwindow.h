#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QDomDocument>

namespace Ui
{
    class MainWindowClass;
}

class LightRoom;
class StampContainer;
class QTableWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    static QPlainTextEdit* logSpace();

private slots:
    // Part A: Create LIP Files
    void on_openFile_clicked();
    void on_reloadFile_clicked();
    void on_saveFile_clicked();
    void on_lrProcess_clicked();
    void on_lrReadStamps_clicked();
    void on_cancelRunBtn_clicked();
    void onXmlTextChanged();

    // Part B: View LIP Files
    void on_openLipBtn_clicked();
    void on_dumpLipBtn_clicked();
    void on_stampGridTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous);
    void on_rb2DView_toggled(bool checked);
    void on_rb3DView_toggled(bool checked);
    void on_cbFixedScale_toggled(bool checked);
    void on_cbShowValues_toggled(bool checked);
    void on_cbShowCrown_toggled(bool checked);
    void on_resetViewBtn_clicked();

    // Common
    void on_clearLogBtn_clicked();

private:
    bool parseCurrentXml(QDomDocument &doc, QString *errorMsg = nullptr);
    void updateXmlSummary();
    void loadLipFile(const QString &fileName);
    void populateOverviewGrid();
    void selectStampByIndex(int index);
    void setRunningState(bool running);

    Ui::MainWindowClass *ui;
    LightRoom *m_lightroom;
    StampContainer *m_viewContainer;
    bool m_cancelRequested;

    static QPlainTextEdit *mLogSpace;
};

#endif // MAINWINDOW_H
