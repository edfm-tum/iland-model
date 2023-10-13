#include "moduledialog.h"
#include "ui_moduledialog.h"

#include <QtXml>
#include <QDomDocument>


//ModuleDialog::ModuleDialog(const QString& xmlFile, QWidget *parent)
ModuleDialog::ModuleDialog(LinkXmlQt* Linkxqt, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ModuleDialog),
    mLinkxqt(Linkxqt)
{
    //XmlHelper helper;

    ui->setupUi(this);

    //connect(ui->buttonBox_moduleDialog, SIGNAL(accepted()), this, SLOT(acceptChanges()));
    connect(ui->buttonBox_moduleDialog, &QDialogButtonBox::accepted, this, [=]() {acceptChanges();});
    //connect(ui->buttonBox_moduleDialog, SIGNAL(accepted()), this, &ModuleDialog::printLineEdits);
    connect(ui->buttonBox_moduleDialog, SIGNAL(rejected()), this, SLOT(closeModuleDialog()));

    //readValuesXml();
    //LinkXmlQt linkxqt(mXmlFile);
    mModuleTabs = ui->moduleTabs;
    QString element = "modules";
    mLinkxqt->readValuesXml(mModuleTabs);

}

ModuleDialog::~ModuleDialog()
{
    delete ui;
}

void ModuleDialog::closeModuleDialog()
{
    close();
}

void ModuleDialog::acceptChanges() {
    mLinkxqt->writeValuesXml(mModuleTabs);
    mLinkxqt->writeToFile();
}


