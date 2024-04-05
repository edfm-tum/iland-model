#include "genericinputwidget.h"
#include "qboxlayout.h"
#include "qcheckbox.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qtoolbutton.h"
#include "helper.h"
#include "ui/dialogcomment.h"
#include "qtablewidget.h"

GenericInputWidget::GenericInputWidget( LinkXmlQt* Linkxqt,
                                       const QString& inputDataType,
                                       const QString& inputDefaultValue,
                                       QStringList inputXmlPath,
                                       const QString& inputLabelName,
                                       const QString& inputToolTip,
                                       QWidget *parent,
                                       bool connected)
    : QWidget{parent}, // pointer holding parent, to which widget is added (e. g. tab: QTabWidget->widget(i))
    mDataType{inputDataType}, // inputType: "boolean", "string", "numeric", "path"
    mDefaultValue(inputDefaultValue), // Default value defined in metadata file
    mXmlPath{inputXmlPath}, // Name of the variable in xml-file
    mLabelName{inputLabelName}, // Name on label to show up
    mToolTip{inputToolTip}, // Tool tip to show up when hovering over label or input element
    mConnected{connected}, // defautl "false", when true gui element is a copy of another element
    mLinkxqt{Linkxqt}
{
    // Each genericInputWidget consists of a label, a comment button, and input field
    // Path elements also include a button for a file dialog
    // Widgets are aligned in horizontal box layout, aligned left
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(11,3,11,3);

    // Set label
    QLabel *label1 = new QLabel(mLabelName);
    richToolTip = QString("<FONT COLOR=black>") + mToolTip + QString("</FONT>");
    label1->setToolTip(richToolTip);
    // Label name is later used for formatting purposes in settingsdialog.cpp
    QString labelNa = mLabelName + "_label";
    label1->setObjectName(labelNa);

    // Define button to open comment dialog, connect() below
    QToolButton *buttonComment = new QToolButton();
    buttonComment->setObjectName(mXmlPath.join(".") + ".comment");

    connect(buttonComment, &QToolButton::clicked, this, [=]() {openCommentDialog(mXmlPath);});

    // Add widgets to layout
    layout->addWidget(label1);
    layout->addWidget(buttonComment);


    // Depending on input type, define the corresponding input widget
    // Defined as generic class QWidget, in case cast inputField with dynamic_cast<T *>(inputField)

    // Name of inputField, which is used for referencing in settingsdialog.cpp
    QString objName;
    if (!mConnected) {
        objName = mXmlPath.join(".");
    } else {
        objName = mXmlPath.join(".") + ".connected";
    }


    if (mDataType == "string" ||
        mDataType == "path" ||
        mDataType == "numeric" ||
        mDataType == "file" ||
        mDataType == "directory") {
        QLineEdit *inputField = new QLineEdit();
        // default value given in metadata shown as grey placeholder text
        inputField->setPlaceholderText(mDefaultValue);
        if (mDataType == "file" ||
            mDataType == "directory") {
            QToolButton *fileDialog = new QToolButton();
            fileDialog->setText("...");
            layout->addWidget(fileDialog);
            if ( mDataType == "file") {
                connect(fileDialog, &QToolButton::clicked, this, [=]{connectFileDialog(mXmlPath.join("_"), inputField, "file");});
            }
            else if ( mDataType == "directory") {
                connect(fileDialog, &QToolButton::clicked, this, [=]{connectFileDialog(mXmlPath.join("_"), inputField, "directory");});
            }
        }
        else if (mDataType == "numeric") {
            QDoubleValidator* numericValidator = new QDoubleValidator(inputField);
            numericValidator->setNotation(QDoubleValidator::StandardNotation);
            numericValidator->setLocale(QLocale::English);
            inputField->setValidator(numericValidator);
        }
        inputField->setToolTip(richToolTip);
        inputField->setObjectName(objName);
        layout->addWidget(inputField);
    }
    else if (mDataType == "boolean") {
        QCheckBox* inputField = new QCheckBox();
        inputField->setToolTip(richToolTip);
        inputField->setObjectName(objName);
        layout->addWidget(inputField);
    }
    else if (mDataType == "combo") {
        QComboBox* inputField = new QComboBox();
        inputField->addItems(mDefaultValue.split(";"));
        inputField->setToolTip(richToolTip);
        inputField->setObjectName(objName);
        layout->addWidget(inputField);
    }
    else if (mDataType == "table") {
        QTableWidget* table = new QTableWidget(this);
        QStringList tableItems = mDefaultValue.split(";");
        table->setRowCount(tableItems.length());
        for (int i = 0; i < tableItems.length(); i ++) {
            QTableWidgetItem *newItem = new QTableWidgetItem();
            newItem->setWhatsThis(tableItems[i]);
            table->setVerticalHeaderItem(i, newItem);
        }
    }


}

GenericInputWidget::GenericInputWidget(LinkXmlQt *link, SettingsItem *item, bool connected):
    mInputCheckBox(nullptr), mInputField(nullptr), mInputComboBox(nullptr), mLabel(nullptr)
{
    mLinkxqt = link;
//    mConnected = connected;
    mConnected = false;
    mSetting = item;
    if ( !connected ) {
        item->widget = this;
    }
    else {
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
    QString label = (item->altLabel == "") ? item->label : item->altLabel;
    mLabel = new QLabel(label);
    richToolTip = QString("<FONT COLOR=black>") + item->tooltip + QString("</FONT>");
    mLabel->setToolTip(richToolTip);
    // Label name is later used for formatting purposes in settingsdialog.cpp
    QString labelName = item->key + "_label" + suffix;
    mLabel->setObjectName(labelName);

    // Define button to open comment dialog, connect() below
    mButtonComment = new QToolButton();
    mButtonComment->setObjectName(item->key + suffix);

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
        item->type == SettingsItem::DataPathDirectory ||
        item->type == SettingsItem::DataPathFile ||
        item->type == SettingsItem::DataFunction) {
        mInputField = new QLineEdit();
        // default value given in metadata shown as grey placeholder text
        mInputField->setPlaceholderText(item->defaultValue);
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
        connect(mInputComboBox, &QComboBox::currentTextChanged, this, [=]{emit widgetValueChanged(item, mInputComboBox->currentText());});
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
    /*
    // According to type of widget, the value of the widget is set
    if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(curWidget)) {
        lineEdit->setText(curValue);
    }
    else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(curWidget)) {
        curValue = curValue.toLower();

        QStringList comboBoxValidValues;
        for (int i = 0; i < comboBox->count(); ++i) {
            comboBoxValidValues.append(comboBox->itemText(i));
        }
        if (comboBoxValidValues.contains(curValue)) {
            comboBox->setCurrentText(curValue);
        }

    }
    else if (QCheckBox* checkBox = dynamic_cast<QCheckBox*>(curWidget)) {
        if (curValue == "true" || curValue == "True" || curValue == "1") {
            checkBox->setChecked(TRUE);
        }
        else if (curValue == "false" || curValue == "False" || curValue == "0") {
            checkBox->setChecked(FALSE);
        }
    }
    // To be included: table widget,...
}
*/
}


QString GenericInputWidget::comment()
{
    return mSetting->comment;
}

void GenericInputWidget::setComment(QString comment)
{
    mSetting->comment = comment;
    //mLinkxqt->writeCommentXml(comment, mSetting->key);
    checkCommentButton();
}


void GenericInputWidget::connectFileDialog(const QString& variableName, QLineEdit *lineEdit, const QString& type)
{
    QString xmlPath;
    QDir homePathAbsolute;
   //QFileInfo xmlFileInfo(mLinkxqt->getXmlFile());
    QDir xmlHomePathAbsolute = mLinkxqt->getTempHomePath();

//    if ( xmlHomePathAbsolute.exists() ) {
//        homePathAbsolute = xmlHomePathAbsolute;
//        qDebug() << "Home path set to user defined path: ";
//    }
//    else {
//        xmlPath = mLinkxqt->getXmlFile();

//        QFileInfo fileInfo(xmlPath);
//        homePathAbsolute = fileInfo.absolutePath();

//        qDebug() << "Home path set to directory containing project file (xml file): ";
//    }
    homePathAbsolute = mLinkxqt->getXmlFile();
    qDebug() << "Project Folder:";
    qDebug() << homePathAbsolute.path();

    QString selectedFile = Helper::fileDialog("Select " + variableName, homePathAbsolute.absolutePath(), "", type, nullptr);
    if (selectedFile.isEmpty())
        return;
    QString relativeFilePath = homePathAbsolute.relativeFilePath(selectedFile);
    QString fileName;
    if (relativeFilePath.startsWith("..") ||
        variableName == "system.path.home" ) {
        qDebug() << "Set absolute path.";
        fileName = selectedFile;
    } else {
        qDebug() << "Set relative path.";
        fileName = relativeFilePath;
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
        connect(mirroredWidget, &GenericInputWidget::commentChanged, this, [this]{checkCommentButton();});
    }

    ui_comment->show();

}

void GenericInputWidget::checkCommentButton()
{
    if (!mSetting->comment.isEmpty()) {
        //mButtonComment->setIcon(QIcon(":/go_next.png"));
        mButtonComment->setText("!");


    } else {
        //mButtonComment->setIcon(QIcon());
        mButtonComment->setText(" ");

    }
    mButtonComment->setToolTip(mSetting->comment);
}

QString GenericInputWidget::getWidgetName()
{
    return mSetting->key;
}

LinkXmlQt *GenericInputWidget::getLinkXmlQt()
{
    return this->mLinkxqt;
}

void GenericInputWidget::openFunctionPlotter(SettingsItem *item, const QString& curExpr)
{
    ui_functionPlotter = new DialogFunctionPlotter(curExpr, item->label, this);
    connect(ui_functionPlotter, &DialogFunctionPlotter::acceptFunction,
            [=](const QString& funcExpr){ item->strValue = funcExpr;
                                          mInputField->setText(funcExpr);});
    ui_functionPlotter->show();
}
