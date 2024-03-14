#ifndef GENERICINPUTWIDGET_H
#define GENERICINPUTWIDGET_H

#include "ui/dialogfunctionplotter.h"
#include "ui/linkxmlqt.h"
#include <QWidget>
#include <QLabel>

struct SettingsItem;
class DialogComment; // forward
class GenericInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GenericInputWidget(LinkXmlQt* Linkxqt,
                                const QString& inputDataType = "string",
                                const QString& inputDefaultValue = "",
                                QStringList list = QStringList() << "default" << "path",
                                const QString& inputLabelName = "default label",
                                const QString& inputToolTip = "default tool tip",
                                QWidget *parent = nullptr,
                                bool connected = false);
    //~genericInputWidget();
    explicit GenericInputWidget(LinkXmlQt *link,
                                SettingsItem *item,
                                bool connected = false);

    QString strValue();
    void setValue(QString str_value);

    QString comment();
    void setComment(QString comment);
private:
    SettingsItem *mSetting;
    QCheckBox *mInputCheckBox;
    QLineEdit *mInputField;
    QComboBox *mInputComboBox;
    QLabel *mLabel;
    QToolButton *mButtonComment;

    QString mDataType;
    QString mDefaultValue;
    QStringList mXmlPath;
    QString mLabelName;
    QString mToolTip;
    QString richToolTip;
    bool mConnected;

    //void updateFilePath(const QString& path, QLineEdit *lineEdit);
    void connectFileDialog(const QString& variableName, QLineEdit *lineEdit, const QString &type);
    void openCommentDialog(QStringList xmlPath);
    void checkCommentButton();
    void openFunctionPlotter(SettingsItem *item, const QString& curExpr);
    //void valueChanged(QWidget *inputField);

    DialogComment* ui_comment;
    DialogFunctionPlotter* ui_functionPlotter;
    LinkXmlQt* mLinkxqt;

signals:
    //void homePathUpdate(const QString& homePath);
    void widgetValueChanged(SettingsItem* item, QVariant newValue);
};

#endif // GENERICINPUTWIDGET_H
