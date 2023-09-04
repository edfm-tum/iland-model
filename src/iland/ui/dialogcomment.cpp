#include "dialogcomment.h"
#include "ui_dialogcomment.h"

DialogComment::DialogComment(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogComment)
{
    ui->setupUi(this);
}

DialogComment::~DialogComment()
{
    delete ui;
}
