#include "genericinputwidget.h"
#include "qboxlayout.h"
#include "qcheckbox.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qtoolbutton.h"
#include "helper.h"
#include "ui/dialogcomment.h"
#include "qtablewidget.h"


GenericInputWidget::GenericInputWidget(LinkXmlQt *link, SettingsItem *item, bool connected):
    mInputCheckBox(nullptr), mInputField(nullptr), mInputComboBox(nullptr), mLabel(nullptr)
{
    mLinkxqt = link;
//    mConnected = connected;
    mConnected = false;
    mSetting = item;
    item->widget = nullptr;
    if ( !connected ) {
        item->widget = this;
    } else {
        item->connectedWidgets.append(this);
    }

    QString suffix = "";

    if ( connected ) {
        suffix = ".connected";
    }

    // Each genericInputWidget consists of a label, a comment button, and input field
    // Path elements also include a button for a file dialog
    // Widgets are aligned in horizontal box layout, aligned left
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(11,3,11,3);

    // Set label
    QString label = ( connected ) ? item->altLabel : item->label;
    mLabel = new QLabel(label);
    richToolTip = QString("<FONT COLOR=black>%1<br/>%2</FONT>").arg(item->key, item->tooltip);
    mLabel->setToolTip(richToolTip);
    mLabel->setAttribute(Qt::WA_NoSystemBackground); // no background color
    // Label name is later used for formatting purposes in settingsdialog.cpp
    QString labelName = item->key + "_label" + suffix;
    mLabel->setObjectName(labelName);

    // Define button to open comment dialog, connect() below
    mButtonComment = new QToolButton();
    mButtonComment->setObjectName(item->key + suffix);
    mButtonComment->setIcon(QIcon(":/note_empty.png"));
    mButtonComment->setAutoRaise(true);
    //mButtonComment->setStyleSheet("QToolButton { border: none; background: transparent; } "); //

    connect(mButtonComment, &QToolButton::clicked, this, [=]() {openCommentDialog(mXmlPath);});

    // Add widgets to layout
    layout->addWidget(mButtonComment);
    layout->addWidget(mLabel);

    // Depending on input type, define the corresponding input widget
    // Defined as generic class QWidget, in case cast inputField with dynamic_cast<T *>(inputField)

    // Name of inputField, which is used for referencing in settingsdialog.cpp
    QString objName = item->key + suffix;
//    if (mConnected) {
//        objName = objName + ".connected";
//    }


    if (item->type == SettingsItem::DataString ||
        item->type == SettingsItem::DataPath ||
        item->type == SettingsItem::DataNumeric ||
        item->type == SettingsItem::DataInteger ||
        item->type == SettingsItem::DataPathDirectory ||
        item->type == SettingsItem::DataPathFile ||
        item->type == SettingsItem::DataFunction) {
        mInputField = new QLineEdit();
        // default value given in metadata shown as grey placeholder text
        mInputField->setPlaceholderText(item->defaultValue);
        mInputField->setStyleSheet("QLineEdit:hover {border: 1px solid gray;}");
        if (item->type == SettingsItem::DataPathFile ||
            item->type == SettingsItem::DataPathDirectory) {
            QToolButton *fileDialog = new QToolButton();
            fileDialog->setText("...");
            layout->addWidget(fileDialog);
            if (item->type == SettingsItem::DataPathFile) {
                connect(fileDialog, &QToolButton::clicked, this, [=]{connectFileDialog(objName, mInputField, "file");});
            }
            else if (item->type == SettingsItem::DataPathDirectory) {
                connect(fileDialog, &QToolButton::clicked, this, [=]{connectFileDialog(objName, mInputField, "directory");});
            }
        }
        else if (item->type == SettingsItem::DataNumeric) {
            QDoubleValidator* numericValidator = new QDoubleValidator(mInputField);
            numericValidator->setNotation(QDoubleValidator::StandardNotation);
            numericValidator->setLocale(QLocale::English);
            mInputField->setValidator(numericValidator);
        }
        else if (item->type == SettingsItem::DataInteger) {
            QIntValidator* integerValidator = new QIntValidator(mInputField);
            integerValidator->setLocale(QLocale::English);
            mInputField->setValidator(integerValidator);
        }
        else if (item->type == SettingsItem::DataFunction) {
            QToolButton *functionDialog = new QToolButton();
            functionDialog->setText("f(x)");
            functionDialog->setStyleSheet("font: italic;");
            layout->addWidget(functionDialog);
            connect(functionDialog, &QToolButton::clicked, this, [=]{ openFunctionPlotter(item, mInputField->text());});
        }
//        else if (item->type == SettingsItem::DataConnected) {
//            QList<QWidget *> origElementList = item->widget->findChildren<QWidget *>();
//            foreach (QWidget* widget, origElementList) {
//                layout->addWidget(widget);
//            }
//        }
        mInputField->setToolTip(richToolTip);
        mInputField->setObjectName(objName);
        layout->addWidget(mInputField);

        // connect to check for changes
//        connect(mInputField, &QLineEdit::textEdited, mSetting, &SettingsItem::valueChanged);

        connect(mInputField, &QLineEdit::textEdited, this, [=]{emit widgetValueChanged(item, mInputField->text());});


    }
    else if (item->type == SettingsItem::DataBoolean) {
        mInputCheckBox = new QCheckBox();
        mInputCheckBox->setToolTip(richToolTip);
        mInputCheckBox->setObjectName(objName);
        layout->addWidget(mInputCheckBox);

        // connect to check for changes
        connect(mInputCheckBox, &QAbstractButton::clicked, this, [=]{emit widgetValueChanged(item, mInputCheckBox->isChecked());});
    }


    else if (item->type == SettingsItem::DataCombo) {
        mInputComboBox = new QComboBox();
        mInputComboBox->addItems(item->defaultValue.split(";"));
        mInputComboBox->setToolTip(richToolTip);
        mInputComboBox->setObjectName(objName);
        layout->addWidget(mInputComboBox);

        // connect to check for changes
        // signals/changes of combo boxes behave differently, necessary to avoid emitting the signal twice
        if ( !connected ) {
            connect(mInputComboBox, &QComboBox::currentTextChanged, this, [=]{emit widgetValueChanged(item, mInputComboBox->currentText());});
        }
    }

    else if (item->type == SettingsItem::DataTable) {
        QTableWidget* table = new QTableWidget(this);
        QStringList tableItems = item->defaultValue.split(";");
        table->setRowCount(tableItems.length());
        for (int i = 0; i < tableItems.length(); i ++) {
            QTableWidgetItem *newItem = new QTableWidgetItem();
            newItem->setWhatsThis(tableItems[i]);
            table->setVerticalHeaderItem(i, newItem);
        }
    }


}

QString GenericInputWidget::strValue()
{
    switch (mSetting->type) {
    case SettingsItem::DataNumeric:
    case SettingsItem::DataPath:
    case SettingsItem::DataString:
    case SettingsItem::DataPathDirectory:
    case SettingsItem::DataPathFile:
    case SettingsItem::DataFunction:
        return mInputField->text();
    case SettingsItem::DataBoolean:
        return mInputCheckBox->isChecked() ? "true" : "false";
    case SettingsItem::DataCombo:
        return mInputComboBox->currentText();
    default:
        return "invalid";
    }

}

void GenericInputWidget::setValue(QString str_value)
{
    switch (mSetting->type) {
    case SettingsItem::DataNumeric:
    case SettingsItem::DataString:
    case SettingsItem::DataPath:
    case SettingsItem::DataPathDirectory:
    case SettingsItem::DataPathFile:
    case SettingsItem::DataFunction:
        mInputField->setText(str_value);
        break;

    case SettingsItem::DataBoolean: {
            bool value = false;
            if (str_value == "true" || str_value == "True" || str_value == "1") {
                value = true;
            }
            mInputCheckBox->setChecked(value);
        }
        break;

    case SettingsItem::DataCombo: {
            mInputComboBox->setCurrentText(str_value);
            break;
        }



    default: // do nothing
        break;

    }

}


QString GenericInputWidget::comment()
{
    return mSetting->comment;
}

void GenericInputWidget::setComment(QString comment)
{
    mSetting->comment = comment;
    //mLinkxqt->writeCommentXml(comment, mSetting->key);
    emit commentChanged();
    checkCommentButton();
}


void GenericInputWidget::connectFileDialog(const QString& variableName, QLineEdit *lineEdit, const QString& type)
{
    //format dialog title
    QStringList parts = variableName.split('.', Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i)
        if (parts[i] != "connected") {
            parts[i].replace(0, 1, parts[i][0].toUpper());
        }
        else {
            parts.removeAt(i);
        }

    const QString& title = "Select " + parts.join(":");

    // get base path for the setting (including special cases)
    QString base_path = getBasePath(variableName);

    QString selectedFile = Helper::fileDialog(title, base_path, "", type, nullptr);
    if (selectedFile.isEmpty())
        return;

    QString fileName;
    QFileInfo fileInfo(selectedFile);
    if (fileInfo.absoluteFilePath().startsWith(base_path)) {
        // file is relative to base path
        fileName = fileInfo.absoluteFilePath().remove(0, base_path.length() + 1); // +1 also remove slash
    } else {
        // absolute path
        fileName = fileInfo.absoluteFilePath();
    }

    lineEdit->setText(fileName);
    //mLinkxqt->setTempHomePath(fileName);
    if (variableName == "system.path.home" ||
        variableName == "system.path.home.connected") {
        emit lineEdit->editingFinished();
    }
    // needed to register changes via file dialog (unsaved changes)
    emit lineEdit->textEdited(fileName);

}

void GenericInputWidget::openCommentDialog(QStringList xmlPath)
{
    //ui_comment = new DialogComment(mLinkxqt, xmlPath, this);
    ui_comment = new DialogComment(this, this);
    //Connect Comment Dialog accept button to reflect changes over all items
    connect(ui_comment, &DialogComment::commentBoxStatus, this, [this]{checkCommentButton();});

    foreach (GenericInputWidget* mirroredWidget, mSetting->connectedWidgets) {
        connect(ui_comment, &DialogComment::commentBoxStatus, mirroredWidget, [=]{mirroredWidget->checkCommentButton();});
        //connect(mirroredWidget, &GenericInputWidget::commentChanged, this, [this]{checkCommentButton();});

    }

    ui_comment->show();

}

void GenericInputWidget::checkCommentButton()
{
    if (!mSetting->comment.isEmpty()) {
        //mButtonComment->setIcon(QIcon(":/go_next.png"));
        mButtonComment->setIcon(QIcon(":/note_full.png"));
        mButtonComment->setToolTip(mSetting->comment);

    } else {
        //mButtonComment->setIcon(QIcon());
        mButtonComment->setIcon(QIcon(":/note_empty.png"));
        mButtonComment->setToolTip("Click to edit comment");

    }

}

QString GenericInputWidget::getWidgetName()
{
    return mSetting->key;
}

LinkXmlQt *GenericInputWidget::getLinkXmlQt()
{
    return this->mLinkxqt;
}

QString GenericInputWidget::getBasePath(const QString &variableName)
{
    const QList<QPair<QString, QString> > special_cases = { {"system.database.in", "database"},
                                                           {"system.database.out", "database"},
                                                           {"system.database.climate", "database"}
                                                           };

    // get path to project root / xml folder
    QDir homePathAbsolute = mLinkxqt->getXmlFile();
    QFileInfo base_path_info(homePathAbsolute.absolutePath());
    QString base_path = base_path_info.path();

    // modify for select number of species cases
    for (const auto& p : special_cases) {
        if (p.first == variableName) {
            QString extra_path = mLinkxqt->readXmlValue(QString("system.path.%1").arg(p.second));
            QFileInfo fi(extra_path);
            if (fi.isAbsolute())
                base_path = extra_path;
            else
                base_path = QString("%1/%2").arg(base_path, extra_path);
        }
    }
    return base_path;

}

void GenericInputWidget::openFunctionPlotter(SettingsItem *item, const QString& curExpr)
{
    ui_functionPlotter = new DialogFunctionPlotter(curExpr, item->label, this);
    connect(ui_functionPlotter, &DialogFunctionPlotter::acceptFunction,
            [=](const QString& funcExpr){ item->strValue = funcExpr;
                                          mInputField->setText(funcExpr);});
    ui_functionPlotter->show();
}
