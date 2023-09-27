#ifndef DIALOGCOMMENT_H
#define DIALOGCOMMENT_H

#include <QDialog>

#include "ui/linkxmlqt.h"

namespace Ui {
class DialogComment;
}

class DialogComment : public QDialog
{
    Q_OBJECT

public:
    explicit DialogComment(LinkXmlQt* Linkxqt, const QStringList& xmlPath,QWidget *parent = nullptr);
    ~DialogComment();

private:
    Ui::DialogComment *ui;
    QPlainTextEdit* mCommentEdit;
    void acceptComment();
    const QStringList mXmlPath;
    LinkXmlQt* mLinkxqt;
    //QString& mXmlComment;
};

#endif // DIALOGCOMMENT_H
