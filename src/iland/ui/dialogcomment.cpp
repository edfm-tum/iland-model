#include "dialogcomment.h"
#include "ui_dialogcomment.h"

#include "ui/linkxmlqt.h"

DialogComment::DialogComment(LinkXmlQt* Linkxqt, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogComment),
    mLinkxqt(Linkxqt)
{

    mLinkxqt->readXmlProjectDescription();

    ui->setupUi(this);
    //mLinkxqt = mWidget->getLinkXmlQt();
    connect(ui->buttonBox_dialogComment, &QDialogButtonBox::accepted, this, [=]() {acceptEditedDescription();});

    ui->commentField->setPlainText(mLinkxqt->xmlProjectDescription);
}

DialogComment::DialogComment(LinkXmlQt* Linkxqt, const QStringList& xmlPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogComment),
    mXmlPath(xmlPath),
    mLinkxqt(Linkxqt)
{
    ui->setupUi(this);

    connect(ui->buttonBox_dialogComment, &QDialogButtonBox::accepted, this, [=]() {acceptEditedDescription();});

    mCommentEdit = this->findChild<QPlainTextEdit *>();

    mCommentEdit->setPlainText(mLinkxqt->readCommentXml(mXmlPath));

}

DialogComment::DialogComment(GenericInputWidget *widget, QWidget *parent):
    QDialog(parent),
    ui(new Ui::DialogComment),
    mWidget(widget)
{
    ui->setupUi(this);
    mLinkxqt = mWidget->getLinkXmlQt();
    connect(ui->buttonBox_dialogComment, &QDialogButtonBox::accepted, this, [=]() {acceptComment();});


    //mCommentEdit = this->findChild<QPlainTextEdit *>();

    //mXmlComment = mLinkxqt->readCommentXml(mXmlPath);
    ui->commentField->setPlainText(widget->comment());

}

DialogComment::~DialogComment()
{
    delete ui;
}


void DialogComment::acceptComment()
{
    QString commentText = ui->commentField->toPlainText();
    mWidget->setComment(commentText);
    emit commentBoxStatus();
    emit mWidget->commentChanged();
    QStringList xmlPath = mWidget->getWidgetName().split(".");
    mLinkxqt->writeCommentXml(commentText, xmlPath);
}

void DialogComment::acceptEditedDescription()
{
    QString descriptionText = ui->commentField->toPlainText();
    mLinkxqt->writeProjectDescriptionXml(descriptionText);
    mLinkxqt->xmlProjectDescription = descriptionText;
    emit projectDescriptionEdited();

}
