#ifndef GENERICINPUTWIDGET_H
#define GENERICINPUTWIDGET_H

#include "ui/dialogcomment.h"
#include "ui/linkxmlqt.h"
#include <QWidget>

//struct metadata;

class genericInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit genericInputWidget(LinkXmlQt* Linkxqt,
                                const QString& inputDataType = "string",
                                const QString& inputDefaultValue = "",
                                QStringList list = QStringList() << "default" << "path",
                                const QString& inputLabelName = "default label",
                                const QString& inputToolTip = "default tool tip",
                                QWidget *parent = nullptr,
                                bool connected = false);
    //~genericInputWidget();

private:
    QString mDataType;
    QString mDefaultValue;
    QStringList mXmlPath;
    QString mLabelName;
    QString mToolTip;
    QString richToolTip;
    bool mConnected;

    void connectFileDialog(const QString& variableName, QLineEdit *lineEdit);
    void openCommentDialog(QStringList xmlPath);

    DialogComment* ui_comment;
    LinkXmlQt* mLinkxqt;

signals:

};

#endif // GENERICINPUTWIDGET_H
