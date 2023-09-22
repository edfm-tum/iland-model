#include "dialogcomment.h"
#include "ui_dialogcomment.h"

#include "ui/linkxmlqt.h"

DialogComment::DialogComment(LinkXmlQt* Linkxqt, const QStringList& xmlPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogComment),
    mXmlPath(xmlPath),
    mLinkxqt(Linkxqt)
{
    ui->setupUi(this);

    connect(ui->buttonBox_dialogComment, &QDialogButtonBox::accepted, this, [=]() {acceptComment();});

    mCommentEdit = this->findChild<QPlainTextEdit *>();

    mLinkxqt->readCommentXml(mCommentEdit, mXmlPath);
}

DialogComment::~DialogComment()
{
    delete ui;
}


void DialogComment::acceptComment()
{
    mLinkxqt->writeCommentXml(mCommentEdit, mXmlPath);
}
