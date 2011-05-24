#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "version.h"
AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    // fetch version information
    const char *bd = __DATE__;
    ui->version->setText( QString("Version: %1").arg(currentVersion()) );
    ui->svnversion->setText( QString("SVN-Revision: %1 - build date: %2").arg(svnRevision()).arg(bd));

}

AboutDialog::~AboutDialog()
{
    delete ui;
}
