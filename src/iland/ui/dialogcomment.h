#ifndef DIALOGCOMMENT_H
#define DIALOGCOMMENT_H

#include <QDialog>

#include "ui/linkxmlqt.h"
#include "genericinputwidget.h"

namespace Ui {
class DialogComment;
}

class DialogComment : public QDialog
{
    Q_OBJECT

public:
    explicit DialogComment(LinkXmlQt* Linkxqt, const QStringList& xmlPath,QWidget *parent = nullptr);
    explicit DialogComment(GenericInputWidget *widget, QWidget *parent = nullptr);
    ~DialogComment();

private:
    Ui::DialogComment *ui;
    QPlainTextEdit* mCommentEdit;
    void acceptComment();
    const QStringList mXmlPath;
    LinkXmlQt* mLinkxqt;
    GenericInputWidget *mWidget;
    //QString& mXmlComment;

signals:
    void commentBoxStatus();
};

#endif // DIALOGCOMMENT_H
