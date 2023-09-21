#include "dialogcomment.h"
#include "ui_dialogcomment.h"

#include "ui/linkxmlqt.h"

DialogComment::DialogComment(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogComment)

{
    ui->setupUi(this);

    connect(ui->buttonBox_dialogComment, &QDialogButtonBox::accepted, this, [=]() {acceptComment();});
}

DialogComment::~DialogComment()
{
    delete ui;
}


void DialogComment::acceptComment()
{

}
