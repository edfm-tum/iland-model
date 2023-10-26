#ifndef GENERICINPUTWIDGET_H
#define GENERICINPUTWIDGET_H

#include "ui/dialogcomment.h"
#include "ui/linkxmlqt.h"
#include <QWidget>

class genericInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit genericInputWidget(LinkXmlQt* Linkxqt,
                                const QString& inputDataType = "string",
                                QStringList list = QStringList() << "default" << "path",
                                const QString& inputLabelName = "default label",
                                const QString& inputToolTip = "default tool tip",
                                QWidget *parent = nullptr);
    //~genericInputWidget();

private:
    QString dataType;
    QStringList xmlPath;
    QString labelName;
    QString toolTip;

    void connectFileDialog(const QString& variableName, QLineEdit *lineEdit);
    void openCommentDialog(QStringList xmlPath);

    DialogComment* ui_comment;
    LinkXmlQt* mLinkxqt;

signals:

};

#endif // GENERICINPUTWIDGET_H
