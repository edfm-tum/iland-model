#include "genericinputwidget.h"
#include "qboxlayout.h"
#include "qcheckbox.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qtoolbutton.h"
#include "helper.h"

genericInputWidget::genericInputWidget(QWidget *parent, QString inputType, QString inputName)
    : QWidget{parent},
    inputWidget{inputType},
    variableName{inputName}
{
    // Widgets are aligned in horizontal box layout, aligned left
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignLeft);

    // Set label
    QLabel *label1 = new QLabel(variableName);

    // Define button to open comment dialog, connect() below
    QToolButton *buttonComment = new QToolButton();
    buttonComment->setObjectName(variableName + "_comment");


    // Depending on input type, define the corresponding input widget
    // Defined as generic class QWidget, in case cast inputField with dynamic_cast<T *>(inputField)
    QWidget *inputField;

    if (inputType == "string" || inputType == "path") {
        inputField = new QLineEdit();
    }
    else if (inputType == "bool") {
        inputField = new QCheckBox();
    }

    // Name of inputField, which is used for internal functionality
    inputField->setObjectName(variableName);

    // Add widgets to layout
    layout->addWidget(label1);
    layout->addWidget(buttonComment);
    layout->addWidget(inputField);

    // If input type is path, an additional button is added and connected to open a file dialog
    if (inputType == "path") {
        QToolButton *fileDialog = new QToolButton();
        fileDialog->setText("...");
        layout->addWidget(fileDialog);
        connect(fileDialog, &QToolButton::clicked, this, [=]{connectFileDialog(variableName, dynamic_cast<QLineEdit*>(inputField));});
    }

}


void genericInputWidget::connectFileDialog(QString variableName, QLineEdit *lineEdit) {
    QString fileName = Helper::fileDialog("Select " + variableName, "", "");
    if (fileName.isEmpty())
        return;
    lineEdit->setText(fileName);
}
