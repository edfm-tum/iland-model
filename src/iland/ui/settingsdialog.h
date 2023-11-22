#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "ui/linkxmlqt.h"
#include <QDialog>

namespace Ui {
class SettingsDialog;
}

struct metadata;
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
                            QStringList& inputModules,
                            QList<QStringList>& inputTabList,
                            metadata& inputMetaData,
                            QWidget *parent = nullptr);
    //~SettingsDialog();


private:
    QStringList& mModulesList;
    QList<QStringList>& mTabsOfModulesList;
    metadata& mMeta;
    LinkXmlQt* mLinkxqt;

//    Ui::SettingsDialog *ui;

//signals:

};

#endif // SETTINGSDIALOG_H
