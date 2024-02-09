#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H
#include <QDialog>
#include <QToolBar>

#include "ui/linkxmlqt.h"
#include "qstackedwidget.h"
#include "qtreewidget.h"

namespace Ui {
class SettingsDialog;
}

//struct metadata;
//{
//    QStringList elements;
//    QStringList inputType;
//    QStringList defaultValue;
//    QStringList labelName;
//    QStringList toolTip;
//} ;



class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(LinkXmlQt* Linkxqt,
                            QStringList& inputSettings,
                            QList<QStringList>& inputSettingsList,
                            QStringList inputMetaKeys,
                            QStringList inputMetaValues,
                            //metadata& inputMetaData,
                            QWidget *parent = nullptr);
    //~SettingsDialog();
public slots:
    void updateData(); ///< fetch data from data structure and fill ui element
    void setFilterMode(int mode);


private:
    // The settings list holds the first level of the navigation hierachy
    QStringList& mSettingsList;

    // Second level of settings. The first list corresponds
    // to the first element in mSettingsList, second to second...
    QList<QStringList>& mTabsOfSettingsList;

    //mMetaKeys and mMetaValues hold the entries of project_file_metadata.txt
    // The lists are used to preserve the order of the elements in the file
    QStringList mMetaKeys;
    QStringList mMetaValues;

    QMap<QString, SettingsItem*> mKeys;

    // Class to read/write files from/to xml document/gui
    LinkXmlQt* mLinkxqt;

    // Navigation element in gui
    QTreeWidget* treeWidget;

    // Container for gui elements
    QStackedWidget* stackedWidget;

    // Based on keys and values and their respective order as defined in project_file_metadata.txt
    // setDialogLayout builds the layout of the dialog and defines all the elements.
    void setDialogLayout(QTreeWidget* treeWidget, QStackedWidget* stackedWidget);


    // void setTabCaptions(QStackedWidget* stackedWidget);

    void readXMLValues();

    QToolBar *createToolbar();


};

#endif // SETTINGSDIALOG_H
