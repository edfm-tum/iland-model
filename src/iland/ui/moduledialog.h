#ifndef MODULEDIALOG_H
#define MODULEDIALOG_H

#include <QDialog>

#include <ui/linkxmlqt.h>

namespace Ui {
class ModuleDialog;
}

class ModuleDialog : public QDialog
{
    Q_OBJECT


public:
    explicit ModuleDialog(LinkXmlQt* Linkxqt, QWidget *parent = nullptr);
    ~ModuleDialog();

private:
    Ui::ModuleDialog *ui;
    //const QString& mXmlFile = mXmlFile;
    LinkXmlQt* mLinkxqt;
    QMap<QString, QString> traverseNode(const QDomNode& node, QString& nameModule);
    void traverseTreeSetElements(const QDomNode& node, int tabIndex, QTabWidget* widget);
    void setWidget(const QDomNode& node, QString nameModule);
    QTabWidget* mModuleTabs;
    void acceptChanges();


private slots:
    //void getModuleInput();
    void closeModuleDialog();

    //void on_button_addColumn_clicked();
};

#endif // MODULEDIALOG_H
