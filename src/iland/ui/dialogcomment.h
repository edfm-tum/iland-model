#ifndef DIALOGCOMMENT_H
#define DIALOGCOMMENT_H

#include <QDialog>

namespace Ui {
class DialogComment;
}

class DialogComment : public QDialog
{
    Q_OBJECT

public:
    explicit DialogComment(QWidget *parent = nullptr);
    ~DialogComment();

private:
    Ui::DialogComment *ui;
};

#endif // DIALOGCOMMENT_H
