#include "dialogsystemsettings.h"
#include "qfiledialog.h"
#include "qplaintextedit.h"
#include "ui/linkxmlqt.h"
#include "ui_dialogsystemsettings.h"
#include "ui/dialogcomment.h"
#include "ui/genericinputwidget.h"

#include "helper.h"



//DialogSystemSettings::DialogSystemSettings(const QString& xmlFile, QWidget *parent) :
DialogSystemSettings::DialogSystemSettings(LinkXmlQt* Linkxqt, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSystemSettings),
    mLinkxqt(Linkxqt)


{
    ui->setupUi(this);

    //connect(ui->buttonBox_systemSettingsDialog, &QDialogButtonBox::accepted, this, [=]() {mLinkxqt->writeToFile();});
    connect(ui->buttonBox_systemSettingsDialog, &QDialogButtonBox::accepted, this, [=]() {acceptChanges();});
    connect(ui->path_fileDialog_home, SIGNAL(clicked()), this, SLOT(setPath_home()));
    connect(ui->path_fileDialog_database, SIGNAL(clicked()), this, SLOT(setPath_database()));
    connect(ui->path_fileDialog_lip, SIGNAL(clicked()), this, SLOT(setPath_lip()));
    connect(ui->path_fileDialog_temp, SIGNAL(clicked()), this, SLOT(setPath_temp()));
    connect(ui->path_fileDialog_script, SIGNAL(clicked()), this, SLOT(setPath_script()));
    connect(ui->path_fileDialog_init, SIGNAL(clicked()), this, SLOT(setPath_init()));
    connect(ui->path_fileDialog_output, SIGNAL(clicked()), this, SLOT(setPath_output()));
    connect(ui->database_fileDialog_in, SIGNAL(clicked()), this, SLOT(setDatabase_in()));
    connect(ui->database_fileDialog_out, SIGNAL(clicked()), this, SLOT(setDatabase_out()));
    connect(ui->database_fileDialog_climate, SIGNAL(clicked()), this, SLOT(setDatabase_climate()));
    connect(ui->javascript_fileDialog_fileName, SIGNAL(clicked()), this, SLOT(setJavascript_fileName()));

    connect(ui->path_database_commentDialog, &QToolButton::clicked, this, [=]() {openCommentDialog(ui->path_database_commentDialog->objectName(), "system");});
    connect(ui->path_home_commentDialog, &QToolButton::clicked, this, [=]() {openCommentDialog(ui->path_home_commentDialog->objectName(), "system");});

    mSystemTab = ui->systemTab;
    QString element = "system";
    mLinkxqt->readValuesXml(mSystemTab);

    /*QWidget *testTab = new QWidget(mSystemTab);

    mSystemTab->addTab(testTab, "test");

    QVBoxLayout *testLay = new QVBoxLayout(testTab);
    testLay->setAlignment(Qt::AlignTop);


    //QLayout *gridLay = mSystemTab->widget(3)->layout();

    genericInputWidget *genericInput1 = new genericInputWidget(mSystemTab->widget(4), "string", "testSTring");
    genericInputWidget *genericInput2 = new genericInputWidget(mSystemTab->widget(4), "bool", "testBool");
    genericInputWidget *genericInput3 = new genericInputWidget(mSystemTab->widget(4), "path", "testPath");


    testLay->addWidget(genericInput1);
    testLay->addWidget(genericInput2);
    testLay->addWidget(genericInput3);
    testLay->
*/
}


DialogSystemSettings::~DialogSystemSettings()
{
    delete ui;
    //delete mLinkxqt;
}

void DialogSystemSettings::acceptChanges() {
    mLinkxqt->writeValuesXml(mSystemTab);
    mLinkxqt->writeToFile();

}

void DialogSystemSettings::openCommentDialog(const QString& nameObject, const QString submodule) {

    QString currentObject = nameObject;
    QString currentTag = currentObject.remove("_commentDialog");

    QStringList xmlPath = currentTag.split("_");
    xmlPath.prepend(submodule);

    ui_comment = new DialogComment(mLinkxqt, xmlPath, this);
    ui_comment->show();

}


void DialogSystemSettings::setPath_home()
{
    QString fileName = QFileDialog::getExistingDirectory(this, "Select home directory", ui->path_home->text());

    if (fileName.isEmpty())
        return;
    ui->path_home->setText(fileName);
}

void DialogSystemSettings::setPath_database()
{
    QString fileName = Helper::fileDialog("Select database", ui->path_database->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->path_database->setText(fileName);
}

void DialogSystemSettings::setPath_lip()
{
    QString fileName = Helper::fileDialog("Select lip", ui->path_lip->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->path_lip->setText(fileName);
}

void DialogSystemSettings::setPath_temp()
{
    QString fileName = Helper::fileDialog("Select temp", ui->path_temp->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->path_temp->setText(fileName);
}

void DialogSystemSettings::setPath_script()
{
    QString fileName = Helper::fileDialog("Select script", ui->path_script->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->path_script->setText(fileName);
}

void DialogSystemSettings::setPath_init()
{
    QString fileName = Helper::fileDialog("Select init", ui->path_init->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->path_init->setText(fileName);
}

void DialogSystemSettings::setPath_output()
{
    QString fileName = Helper::fileDialog("Select output", ui->path_output->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->path_output->setText(fileName);
}

void DialogSystemSettings::setDatabase_in()
{
    QString fileName = Helper::fileDialog("Select input database", ui->database_in->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->database_in->setText(fileName);
}

void DialogSystemSettings::setDatabase_out()
{
    QString fileName = Helper::fileDialog("Select output database", ui->database_out->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->database_out->setText(fileName);
 }

void DialogSystemSettings::setDatabase_climate()
{
    QString fileName = Helper::fileDialog("Select climate database", ui->database_climate->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->database_climate->setText(fileName);
}

void DialogSystemSettings::setJavascript_fileName()
{
    QString fileName = Helper::fileDialog("Select javascript file", ui->javascript_fileName->text(), "",this);
    if (fileName.isEmpty())
        return;
    ui->javascript_fileName->setText(fileName);
}
