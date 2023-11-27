#include "linkxmlqt.h"
#include "qtablewidget.h"
//#include "qlineedit.h"

#include <QtXml>
#include <QDomDocument>
#include <QPlainTextEdit>


LinkXmlQt::LinkXmlQt(const QString& xmlFile) :
    mXmlFile(xmlFile)
{
    xmlFileLoaded = loadXmlFile();

}

LinkXmlQt::~LinkXmlQt() {

}


void LinkXmlQt::setXmlPath(const QString& xmlPath) {
    mXmlFile = xmlPath;
}


bool LinkXmlQt::loadXmlFile() {
    QDomDocument curXml;
    //setXmlPath(xmlPath);
    QString filetest = mXmlFile;
    QFile file(mXmlFile);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "File couldn't be opened. Abort.";
        return false;
    }

    QString errorMsg;
    int errorLine, errorColumn;

    if (!curXml.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        qDebug() << "Error loading file content. Abort.";
        file.close();
        return false;
    }

    mLoadedXml = curXml;
    file.close();
    return true;
}


QString LinkXmlQt::readCommentXml(const QStringList& xmlPath)
{

    if ( xmlFileLoaded ) {
        QDomDocument curXml = mLoadedXml;

        QDomElement curNode = curXml.documentElement();
        //QDomElement curNode = rootElement;
        foreach (QString node, xmlPath) {
            curNode = curNode.firstChildElement(node);
        }
        QStringList commentListed;
        QDomNode prevSibl = curNode.previousSibling();
//            if (prevSibl.isComment()) {
//                QString commentText = prevSibl.toComment().nodeValue();
//                commentEdit->setPlainText(commentText);
//                mSiblingIsComment = true;
//                }
//            else {
//                //commentEdit->setPlainText("Default");
//                mSiblingIsComment = false;
//            }
        while (prevSibl.isComment()) {
            commentListed.prepend(prevSibl.toComment().nodeValue());
            prevSibl = prevSibl.previousSibling();
        }

        //file.close();
        return commentListed.join("\n");

    }
    else {
        return "Problem";
    }

}

void LinkXmlQt::writeCommentXml(const QString& comment,
                                const QStringList& xmlPath)

{
    QDomDocument curXml = mLoadedXml;

    QDomElement rootElement = curXml.documentElement();
    QDomElement curNode = rootElement;
    foreach (QString node, xmlPath) {
        curNode = curNode.firstChildElement(node);
    }
    //If Qt::SkipEmptyParts is set no empty lines are included as (empty) comments
    //trimmed() delets trailing and leading whitespaces, leaving internal whitespaces alone
    QStringList commentSplittedLines = comment.trimmed().split("\n");//, Qt::SkipEmptyParts);

    //If comments stretch several lines, setting new values and replacing them can be tricky.
    //Because of that, before a new comment is written, old comment(s) is/are deleted and the new comment(s) added.
    clearCommentXml(curNode);
    setComment(curNode, commentSplittedLines);

}


void LinkXmlQt::clearCommentXml(QDomNode& curNode) {
    QDomNode prevSibl = curNode.previousSibling();
    QDomNode curParentNode = curNode.parentNode();
    while (prevSibl.isComment()) {
        curParentNode.removeChild(prevSibl);
        prevSibl = curNode.previousSibling();
    }
}

void LinkXmlQt::setComment(QDomNode& curNode, QStringList& commentSplittedLines) {
    QDomDocument curXml = curNode.ownerDocument();

    foreach(QString comment, commentSplittedLines) {
        QDomNode newComment = curXml.createComment(comment);
        QDomNode curParentNode = curNode.parentNode();
        curParentNode.insertBefore(newComment, curNode);
    }
    mLoadedXml = curXml;
}


void LinkXmlQt::readValuesXml(QTabWidget* tabWidget) {

    QDomDocument curXml = mLoadedXml;
    QString xmlElement = tabWidget->objectName().toLower();

    if (!xmlFileLoaded) {
        qDebug() << "Error with loading data. Check xml file! Abort.";
        return;
    }

    else {
        //QTabWidget* moduleTabs = tabWidget;
        QDomElement rootElement = curXml.documentElement();
        QDomElement moduleBranch = rootElement.firstChildElement(xmlElement);

        for (int i = 0; i < tabWidget->count(); i++) {
            QString currentTab = tabWidget->widget(i)->objectName();
            QDomElement curBranch = moduleBranch.firstChildElement(currentTab);
            traverseTreeSetElementsGui(curBranch.firstChild(), tabWidget);
        }

    }
}

void LinkXmlQt::readValuesXml(QStackedWidget* stackedWidget) {

    QDomDocument curXml = mLoadedXml;

    if (!xmlFileLoaded) {
        qDebug() << "Error with loading data. Check xml file! Abort.";
        return;
    }

    else {
        //QTabWidget* moduleTabs = tabWidget;

        QDomElement rootElement = curXml.documentElement();
        QDomElement curNode;
        QString elementValue;

        QList<QWidget *> widgetElements = stackedWidget->findChildren<QWidget *>();
        QList<QTableWidget *> tableList = stackedWidget->findChildren<QTableWidget *>();

        foreach (QWidget* curWidget, widgetElements) {
            //qDebug() << "Aktuelles Widget: " << curWidget->objectName();
            QStringList xmlPath = curWidget->objectName().split(".");

            curNode = rootElement;
            foreach (QString node, xmlPath) {
                curNode = curNode.firstChildElement(node);
                //qDebug() << "Current node: " << curNode.nodeName();
            }

            if (!curNode.isNull()) {
                QString curValue = curNode.firstChild().toText().data();
                //qDebug() << "Aktueller Wert: " << curValue;
                if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(curWidget)) {
                    lineEdit->setText(curValue);
                }
                else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(curWidget)) {
                    curValue = curValue.toLower();

                    //                    if (curValue == "1") {
                    //                        curValue = "true";
                    //                    }
                    //                    else if (curValue == "0") {
                    //                        curValue = "false";
                    //                    }

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

            }

        }

//        for (int i = 0; i < tabWidget->count(); i++) {
//            QString xmlElement = tabWidget->objectName().toLower();
//            QDomElement moduleBranch = rootElement.firstChildElement(xmlElement);
//            QString currentTab = tabWidget->widget(i)->objectName();
//            QDomElement curBranch = moduleBranch.firstChildElement(currentTab);
//            traverseTreeSetElementsGui(curBranch.firstChild(), tabWidget);
//        }

    }
}

void LinkXmlQt::writeValuesXml(QTabWidget* tabWidget) {

    QDomDocument curXml = mLoadedXml;

    QString xmlElement = tabWidget->objectName();

    //QFile file(xmlFile);
    if (!xmlFileLoaded) {
        qDebug() << "Error with loading data. Check xml file! Abort.";
        return;
    }

    else {
        //QTabWidget* moduleTabs = tabWidget;
        QDomElement rootElement = curXml.documentElement();
        QDomElement moduleBranch = rootElement.firstChildElement(xmlElement);

        for (int i = 0; i < tabWidget->count(); i++) {
            QString currentTab = tabWidget->widget(i)->objectName();
            QDomElement curBranch = moduleBranch.firstChildElement(currentTab);
            traverseTreeSetElementsXml(curBranch.firstChild(), tabWidget);
        }

    }
}

void LinkXmlQt::writeValuesXml(QStackedWidget* stackedWidget) {

    QDomDocument curXml = mLoadedXml;

    //QFile file(xmlFile);
    if (!xmlFileLoaded) {
        qDebug() << "Error with loading data. Check xml file! Abort.";
        return;
    }

    else {
        //QTabWidget* moduleTabs = tabWidget;
        QDomElement rootElement = curXml.documentElement();
        QDomElement curNode;
        QString elementValue;

        QList<QWidget *> widgetElements = stackedWidget->findChildren<QWidget *>();

        foreach (QWidget* curWidget, widgetElements) {
            //qDebug() << "Aktuelles Widget: " << curWidget->objectName();
            QStringList xmlPath = curWidget->objectName().split(".");

            curNode = rootElement;
            foreach (QString node, xmlPath) {
                curNode = curNode.firstChildElement(node);
                //qDebug() << "Current node: " << curNode.nodeName();
            }

            if (!curNode.isNull()) {
                //QString curValue = curNode.firstChild().toText().data();
                //qDebug() << "Aktueller Wert: " << curValue;
                if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(curWidget)) {
                    elementValue = lineEdit->text();
                }
                else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(curWidget)) {
                    elementValue = comboBox->currentText();
                }
                else if (QCheckBox* checkBox = dynamic_cast<QCheckBox*>(curWidget)) {
                    if (checkBox->isChecked()) {
                        elementValue = "true";
                    }
                    else {
                        elementValue = "false";
                    }
                }
                else {
                    //qDebug() << "Widget not specified: " << curWidget->objectName();
                }
                curNode.firstChild().setNodeValue(elementValue);
            }

        }

//        for (int i = 0; i < stackedWidget->count(); i++) {

//            QStringList xmlPath = stackedWidget->objectName().split(".");
//            QDomElement branch = rootElement.firstChildElement(xmlPath[0]);
//            QDomElement curBranch = branch.firstChildElement(xmlPath[1]);
//            traverseTreeSetElementsXml(curBranch.firstChild(), stackedWidget);
//        }

    }
}



//void LinkXmlQt::traverseTreeSetElementsGui(const QDomNode& curNode,
//                                        int tabIndex,
//                                        QTabWidget* widgetElement)
//{
//    QDomNode node = curNode;
//    QString nameModule = widgetElement->objectName() + "." + widgetElement->widget(tabIndex)->objectName();

//    while (!node.isNull()) {
//        if (node.isElement()) {
//            //QDomElement element = node.toElement();
//            //qDebug() << QString("Element: %1").arg(element.tagName());

//            // Recurse into child nodes
//            traverseTreeSetElementsGui(node.firstChild(), tabIndex, widgetElement);
//        } else if (node.isText()) {
//            QString curValue = node.toText().data();
//            //QString curTag = node.parentNode().toElement().tagName();
//            QString curTag = node.parentNode().nodeName();
//            QString widgetName = nameModule + '.' + curTag;
//            QWidget* w = widgetElement->widget(tabIndex)->findChild<QWidget *>(widgetName);
//            QList<QTableWidget *> tableList = widgetElement->widget(tabIndex)->findChildren<QTableWidget *>();

//            if (w != nullptr) {
//                //qDebug() << widgetName << ", " << w->metaObject()->className();
//                if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(w)) {
//                    lineEdit->setText(curValue);
//                }
//                else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(w)) {
//                    curValue = curValue.toLower();

////                    if (curValue == "1") {
////                        curValue = "true";
////                    }
////                    else if (curValue == "0") {
////                        curValue = "false";
////                    }

//                    QStringList comboBoxValidValues;
//                    for (int i = 0; i < comboBox->count(); ++i) {
//                        comboBoxValidValues.append(comboBox->itemText(i));
//                    }
//                    if (comboBoxValidValues.contains(curValue)) {
//                        comboBox->setCurrentText(curValue);
//                    }
//                    else {
//                        qDebug() << curTag << ": " << curValue << " is not a valid value. Please Check notation.";
//                    }
//                }
//                else if (QCheckBox* checkBox = dynamic_cast<QCheckBox*>(w)) {
//                    if (curValue == "true" || curValue == "True" || curValue == "1") {
//                        checkBox->setChecked(TRUE);
//                    }
//                    else if (curValue == "false" || curValue == "False" || curValue == "0") {
//                        checkBox->setChecked(FALSE);
//                    }
//                }
////                else if (QTableWidget* table = dynamic_cast<QTableWidget*>(w)) {
////                        QTableWidgetItem* item;
////                        int tableRows = table->rowCount();
////                        int tableColumns = table->columnCount();
////                        for (int i = 0; i < tableRows; i++) {
////                            for (int n = 0; n < tableColumns; n++) {
////                                item = table->item(i, n);
////                                QString curElem = item->text();
////                                qDebug() << "row: " << i << ", column: " << n << ", item.text: " << curElem;
////                            }
////                        }
////                    }
//            }
//            else if (w == nullptr && !tableList.isEmpty()) {
//                //QTableWidgetItem* item;
//                QStringList columnValues = curValue.split(",");
//                foreach (QTableWidget* table, tableList) {
//                    int numberInputValues = columnValues.length();
//                    int columnNumber = qMax(numberInputValues, table->columnCount());
//                    table->setColumnCount(columnNumber);
//                    int rowNumber = table->rowCount();
//                    for (int i = 0; i < rowNumber; i++) {
//                        QString horHeader = table->verticalHeaderItem(i)->whatsThis();
//                        if (horHeader == widgetName) {
//                            for (int n = 0; n < columnNumber; n++) {
//                                QTableWidgetItem* item = new QTableWidgetItem();
//                                if (n >= numberInputValues) {
//                                    item->setText("");
//                                } else {
//                                    item->setText(columnValues[n].trimmed());
//                                }
//                                table->setItem(i, n, item);
//                            }
//                        }
//                    }
//                }

//            }
//            else {
//                qDebug() << "Could not find GUI element for variable " << widgetName;
//            }
//        }

//        // Move to the next sibling
//        node = node.nextSibling();
//    }
//}

//void LinkXmlQt::traverseTreeSetElementsXml(const QDomNode& curNode,
//                                        int tabIndex,
//                                        QTabWidget* widgetElement)
//{

//    QDomNode node = curNode;
//    QString nameModule = widgetElement->objectName() + "." + widgetElement->widget(tabIndex)->objectName();
//    QString elementValue;

//    while (!node.isNull()) {
//        if (node.isElement()) {

//            // Recurse into child nodes
//            traverseTreeSetElementsXml(node.firstChild(), tabIndex, widgetElement);
//        } else if (node.isText()) {
//            //QString curValue = node.toText().data();
//            QString curTag = node.parentNode().nodeName();
//            QString widgetName = nameModule + '.' + curTag;
//            QWidget* w = widgetElement->widget(tabIndex)->findChild<QWidget *>(widgetName);
//            QList<QTableWidget *> tableList = widgetElement->widget(tabIndex)->findChildren<QTableWidget *>();

//            if (w != nullptr) {
//                //qDebug() << widgetName << ", " << w->metaObject()->className();
//                if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(w)) {
//                    //lineEdit->setText(curValue);
//                    elementValue = lineEdit->text();
//                }
//                else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(w)) {
//                    elementValue = comboBox->currentText();

//                }
//                else if (QCheckBox* checkBox = dynamic_cast<QCheckBox*>(w)) {
//                    if (checkBox->isChecked()) {
//                        elementValue = "true";
//                    }
//                    else {
//                        elementValue = "false";
//                    }
//                }

//                node.setNodeValue(elementValue);
//            }
//            else if (w == nullptr && !tableList.isEmpty()) {
//                //QTableWidgetItem* item;
//                //QStringList columnValues = curValue.split(",");
//                foreach (QTableWidget* table, tableList) {
//                    int columnNumber = table->columnCount();
//                    int rowNumber = table->rowCount();
//                    for (int i = 0; i < rowNumber; i++) {
//                        QString horHeader = table->verticalHeaderItem(i)->whatsThis();
//                        if (horHeader == widgetName) {
//                            QStringList columnValues;
//                            for (int n = 0; n < columnNumber; n++) {
//                                QTableWidgetItem* curItem = table->item(i, n);
//                                if ((curItem != nullptr)) {
//                                    QString itemText = curItem->text();
//                                    if (itemText != "") {columnValues.append(itemText);}
//                                }
//                            }
//                        node.setNodeValue(columnValues.join(","));
//                        }
//                    }
//                }

//            }
//            else {
//                qDebug() << "Could not find GUI element for variable " << widgetName;
//            }
//        }

//        // Move to the next sibling
//        node = node.nextSibling();
//    }
//}


void LinkXmlQt::traverseTreeSetElementsGui(const QDomNode& node,
                                           QTabWidget* tabWidget)
{

    QDomDocument curXml = mLoadedXml;

    QDomElement rootElement = curXml.documentElement();
    QDomElement curNode;
    QString elementValue;

    for (int i = 0; i < tabWidget->count(); i++) {
        QList<QWidget *> widgetElements = tabWidget->widget(i)->findChildren<QWidget *>();
        QList<QTableWidget *> tableList = tabWidget->widget(i)->findChildren<QTableWidget *>();

        foreach (QWidget* curWidget, widgetElements) {
            //qDebug() << "Aktuelles Widget: " << curWidget->objectName();
            QStringList xmlPath = curWidget->objectName().split(".");

            curNode = rootElement;
            foreach (QString node, xmlPath) {
                curNode = curNode.firstChildElement(node);
                //qDebug() << "Current node: " << curNode.nodeName();
            }

            if (!curNode.isNull()) {
                QString curValue = curNode.firstChild().toText().data();
                //qDebug() << "Aktueller Wert: " << curValue;
                if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(curWidget)) {
                    lineEdit->setText(curValue);
                }
                else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(curWidget)) {
                    curValue = curValue.toLower();

                    //                    if (curValue == "1") {
                    //                        curValue = "true";
                    //                    }
                    //                    else if (curValue == "0") {
                    //                        curValue = "false";
                    //                    }

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

            }

        }

    }

}


void LinkXmlQt::traverseTreeSetElementsXml(const QDomNode& node,
                                           QTabWidget* tabWidget)
{

    QDomDocument curXml = mLoadedXml;

    QDomElement rootElement = curXml.documentElement();
    QDomElement curNode;
    QString elementValue;

    for (int i = 0; i < tabWidget->count(); i++) {
        QList<QWidget *> widgetElements = tabWidget->widget(i)->findChildren<QWidget *>();

        foreach (QWidget* curWidget, widgetElements) {
            //qDebug() << "Aktuelles Widget: " << curWidget->objectName();
            QStringList xmlPath = curWidget->objectName().split(".");

            curNode = rootElement;
            foreach (QString node, xmlPath) {
                curNode = curNode.firstChildElement(node);
                //qDebug() << "Current node: " << curNode.nodeName();
                }

            if (!curNode.isNull()) {
                QString curValue = curNode.firstChild().toText().data();
                //qDebug() << "Aktueller Wert: " << curValue;
                if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(curWidget)) {
                    elementValue = lineEdit->text();
                    }
                else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(curWidget)) {
                    elementValue = comboBox->currentText();
                    }
                else if (QCheckBox* checkBox = dynamic_cast<QCheckBox*>(curWidget)) {
                    if (checkBox->isChecked()) {
                        elementValue = "true";
                        }
                    else {
                        elementValue = "false";
                        }
                    }
                else {
                    //qDebug() << "Widget not specified: " << curWidget->objectName();
                    }
                curNode.firstChild().setNodeValue(elementValue);
                }

            }

    }

}

void LinkXmlQt::writeToFile(const QString& xmlFilePath)
{
    if (xmlFilePath != "") {
        this->setXmlPath(xmlFilePath);
    }

    QFile file(mXmlFile);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "File couldn't be opened for writing. Abort.";
        return;
    }

    QTextStream outStream(&file);
    mLoadedXml.save(outStream, 4);

    file.close();

}
