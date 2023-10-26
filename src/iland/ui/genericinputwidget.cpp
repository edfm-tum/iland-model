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
                                       QStringList inputXmlPath,
                                       const QString& inputLabelName,
                                       const QString& inputToolTip,
                                       QWidget *parent)
    : QWidget{parent}, // pointer holding parent, to which widget is added (e. g. tab: QTabWidget->widget(i))
    dataType{inputDataType}, // inputType: "boolean", "string", "numeric", "path"
    xmlPath{inputXmlPath}, // Name of the variable in xml-file
    labelName{inputLabelName}, // Name on label to show up
    toolTip{inputToolTip}, // Tool tip to show up when hovering over label
    mLinkxqt{Linkxqt}
{
    // Widgets are aligned in horizontal box layout, aligned left
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignLeft);

    // Set label
    QLabel *label1 = new QLabel(labelName);
    label1->setToolTip(toolTip);

    // Define button to open comment dialog, connect() below
    QToolButton *buttonComment = new QToolButton();
    buttonComment->setObjectName(xmlPath.join("_") + "_comment");

    connect(buttonComment, &QToolButton::clicked, this, [=]() {openCommentDialog(xmlPath);});

    // Depending on input type, define the corresponding input widget
    // Defined as generic class QWidget, in case cast inputField with dynamic_cast<T *>(inputField)
    QWidget *inputField;

    if (dataType == "string" || dataType == "path") {
        inputField = new QLineEdit();
    }
    else if (dataType == "boolean") {
        inputField = new QCheckBox();
    }
    else if (dataType == "numeric") {
        inputField = new QLineEdit();
    }

    // Name of inputField, which is used for internal functionality
    const QString& objName = inputXmlPath.join(".");
    inputField->setObjectName(objName);

    // Add widgets to layout
    layout->addWidget(label1);
    layout->addWidget(buttonComment);
    layout->addWidget(inputField);

    // If input type is path, an additional button is added and connected to open a file dialog
    if (dataType == "path") {
        QToolButton *fileDialog = new QToolButton();
        fileDialog->setText("...");
        layout->addWidget(fileDialog);
        connect(fileDialog, &QToolButton::clicked, this, [=]{connectFileDialog(xmlPath.join("_"), dynamic_cast<QLineEdit*>(inputField));});
    }

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
