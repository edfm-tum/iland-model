#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "ui/linkxmlqt.h"
#include <QDialog>
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


private:
    QStringList& mSettingsList;
    QList<QStringList>& mTabsOfSettingsList;
    //metadata& mMeta;
    QStringList mMetaKeys;
    QStringList mMetaValues;
    LinkXmlQt* mLinkxqt;
    QTreeWidget* treeWidget;
    QStackedWidget* stackedWidget;

    void setDialogLayout(QTreeWidget* treeWidget, QStackedWidget* stackedWidget);
    void setTabCaptions(QStackedWidget* stackedWidget);

//    Ui::SettingsDialog *ui;

//signals:

};

#endif // SETTINGSDIALOG_H
