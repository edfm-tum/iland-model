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
    void readValuesXml();
    void readValuesXml_2();
    void readValuesXml_3();
    QMap<QString, QString> traverseNode(const QDomNode& node, QString& nameModule);
    void traverseTreeSetElements(const QDomNode& node, int tabIndex, QTabWidget* widget);
    void setWidget(const QDomNode& node, QString nameModule);

private slots:
    void getModuleInput();
    void closeModuleDialog();

};

#endif // MODULEDIALOG_H
