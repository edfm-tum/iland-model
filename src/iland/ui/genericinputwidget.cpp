#include "genericinputwidget.h"
#include "qboxlayout.h"
#include "qcheckbox.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qtoolbutton.h"
#include "helper.h"
#include "ui/dialogcomment.h"

genericInputWidget::genericInputWidget( LinkXmlQt* Linkxqt,
                                       const QString& inputDataType,
                                       const QString& inputDefaultValue,
                                       QStringList inputXmlPath,
                                       const QString& inputLabelName,
                                       const QString& inputToolTip,
                                       QWidget *parent)
    : QWidget{parent}, // pointer holding parent, to which widget is added (e. g. tab: QTabWidget->widget(i))
    dataType{inputDataType}, // inputType: "boolean", "string", "numeric", "path"
    defaultValue(inputDefaultValue), // Default value defined in metadata file
    xmlPath{inputXmlPath}, // Name of the variable in xml-file
    mLabelName{inputLabelName}, // Name on label to show up
    mToolTip{inputToolTip}, // Tool tip to show up when hovering over label
    mLinkxqt{Linkxqt}
{
    // Widgets are aligned in horizontal box layout, aligned left
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(11,3,11,3);

    // Set label
    QLabel *label1 = new QLabel(mLabelName);
    label1->setToolTip(mToolTip);
    QString labelNa = mLabelName + "_label";
    label1->setObjectName(labelNa);

    // Define button to open comment dialog, connect() below
    QToolButton *buttonComment = new QToolButton();
    buttonComment->setObjectName(xmlPath.join(".") + ".comment");

    connect(buttonComment, &QToolButton::clicked, this, [=]() {openCommentDialog(xmlPath);});

    // Add widgets to layout
    layout->addWidget(label1);
    layout->addWidget(buttonComment);

    // Depending on input type, define the corresponding input widget
    // Defined as generic class QWidget, in case cast inputField with dynamic_cast<T *>(inputField)
    //QWidget *inputField;
    // Name of inputField, which is used for internal functionality
    const QString& objName = inputXmlPath.join(".");

    if (dataType == "string" ||
        dataType == "path" ||
        dataType == "numeric") {
        QLineEdit *inputField = new QLineEdit();
        // default value given in metadata shown as grey placeholder text
        inputField->setPlaceholderText(defaultValue);
        if (dataType == "path") {
            QToolButton *fileDialog = new QToolButton();
            fileDialog->setText("...");
            layout->addWidget(fileDialog);
            connect(fileDialog, &QToolButton::clicked, this, [=]{connectFileDialog(xmlPath.join("_"), inputField);});
        }
        else if (dataType == "numeric") {
            QDoubleValidator* numericValidator = new QDoubleValidator(inputField);
            numericValidator->setNotation(QDoubleValidator::StandardNotation);
            numericValidator->setLocale(QLocale::English);
            inputField->setValidator(numericValidator);
        }
        inputField->setObjectName(objName);
        layout->addWidget(inputField);
    }
    else if (dataType == "boolean") {
        QCheckBox* inputField = new QCheckBox();
        inputField->setObjectName(objName);
        layout->addWidget(inputField);
    }
//    else if (dataType == "numeric") {
//        inputField = new QLineEdit();
//    }

    // If input type is path, an additional button is added and connected to open a file dialog


}


void genericInputWidget::connectFileDialog(const QString& variableName, QLineEdit *lineEdit) {
    QString fileName = Helper::fileDialog("Select " + variableName, "", "");
    if (fileName.isEmpty())
        return;
    lineEdit->setText(fileName);
}

void genericInputWidget::openCommentDialog(QStringList xmlPath) {

//    QString currentObject = nameObject;
//    QString currentTag = currentObject.remove("_commentDialog");

//    QStringList xmlPath = currentTag.split("_");
//    xmlPath.prepend(submodule);

    ui_comment = new DialogComment(mLinkxqt, xmlPath, this);
    ui_comment->show();

}
