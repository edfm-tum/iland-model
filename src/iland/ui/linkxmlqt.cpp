#include "linkxmlqt.h"
#include "qtablewidget.h"
//#include "qlineedit.h"

#include <QtXml>
#include <QDomDocument>
#include <QPlainTextEdit>


LinkXmlQt::LinkXmlQt()

{

}

LinkXmlQt::~LinkXmlQt(){

}

void LinkXmlQt::setXmlPath(const QString xmlPath) {
    mXmlFile = xmlPath;
}


void LinkXmlQt::readCommentXml(QPlainTextEdit* commentEdit,
                               const QStringList& xmlPath)
{
    QDomDocument curXml;
    QFile file(mXmlFile);

    QString errorMsg;
    int errorLine, errorColumn;

    if (!curXml.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        qDebug() << "Error loading file content. Abort.";
        file.close();
    }

    else {
        QDomElement rootElement = curXml.documentElement();
        QDomElement curNode = rootElement;
        foreach (QString node, xmlPath) {
            curNode = curNode.firstChildElement(node);
        }

        QDomNode prevSibl = curNode.previousSibling();
            if (prevSibl.isComment()) {
                QString commentText = prevSibl.toComment().nodeValue();
                commentEdit->setPlainText(commentText);
                mSiblingIsComment = true;
                }
            else {
                //commentEdit->setPlainText("Default");
                mSiblingIsComment = false;
            }
    file.close();
    }

}

void LinkXmlQt::writeCommentXml(QPlainTextEdit* commentEdit,
                                const QStringList& xmlPath)

{
    QDomDocument curXml;

    QFile file(mXmlFile);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error with File";
    }

    QString errorMsg;
    int errorLine, errorColumn;

    if (!curXml.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
            qDebug() << "Error loading file content. Abort.";
            file.close();
    }

    else {
            QDomElement rootElement = curXml.documentElement();
            QDomElement curNode = rootElement;
            foreach (QString node, xmlPath) {
                curNode = curNode.firstChildElement(node);
            }

            QDomNode prevSibl = curNode.previousSibling();
            if (prevSibl.isComment()) {
                QString commentText = commentEdit->toPlainText();
                prevSibl.setNodeValue(commentText);
                QDomNode testComment = curXml.createComment(commentText);
                curXml.insertBefore(testComment, curNode);
                //commentEdit->setPlainText(commentText);
                mSiblingIsComment = true;
            }
            else {
                //commentEdit->setPlainText("Default");
                mSiblingIsComment = false;
            }
        file.close();
        //QFile testFile("C:/Users/gu47yiy/Documents/iLand/iLand/example/tst.xml");
        QFile testFile(mXmlFile);
        testFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream outStream(&testFile);
        curXml.save(outStream, 4);
        testFile.close();
    }
}


void LinkXmlQt::readValuesXml(QTabWidget* tabWidget, QString xmlElement) {
    QDomDocument curXml;

    QFile file(mXmlFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error with File";
    }

    QString errorMsg;
    int errorLine, errorColumn;

        if (!curXml.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        qDebug() << "Error loading file content. Abort.";
        file.close();
    }

    else {
        //QTabWidget* moduleTabs = tabWidget;
        QDomElement rootElement = curXml.documentElement();
        QDomElement moduleBranch = rootElement.firstChildElement(xmlElement);

        for (int i = 0; i < tabWidget->count(); i++) {
            QString currentTab = tabWidget->widget(i)->objectName();
            QDomElement curBranch = moduleBranch.firstChildElement(currentTab);
            traverseTreeSetElements(curBranch.firstChild(), i, tabWidget);
        }

        file.close();
    }
}


void LinkXmlQt::traverseTreeSetElements(const QDomNode& curNode,
                                        int tabIndex,
                                        QTabWidget* widgetElement)
{
    QDomNode node = curNode;
    QString nameModule = widgetElement->widget(tabIndex)->objectName();

    while (!node.isNull()) {
        if (node.isElement()) {
            QDomElement element = node.toElement();
            //qDebug() << QString("Element: %1").arg(element.tagName());

            // Recurse into child nodes
            traverseTreeSetElements(node.firstChild(), tabIndex, widgetElement);
        } else if (node.isText()) {
            QString curValue = node.toText().data();
            QString curTag = node.parentNode().toElement().tagName();
            QString widgetName = nameModule + '_' + curTag;
            QWidget* w = widgetElement->widget(tabIndex)->findChild<QWidget *>(widgetName);

            if (w != nullptr) {
                //qDebug() << widgetName << ", " << w->metaObject()->className();
                if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(w)) {
                    lineEdit->setText(curValue);
                }
                else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(w)) {
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
                    else {
                        qDebug() << curTag << ": " << curValue << " is not a valid value. Please Check notation.";
                    }
                }
                else if (QCheckBox* checkBox = dynamic_cast<QCheckBox*>(w)) {
                    if (curValue == "true" || curValue == "True" || curValue == "1") {
                        checkBox->setChecked(TRUE);
                    }
                    else if (curValue == "false" || curValue == "False" || curValue == "0") {
                        checkBox->setChecked(FALSE);
                    }
                else if (QTableWidget* table = dynamic_cast<QTableWidget*>(w)) {
                        QTableWidgetItem* item;
                        int tableRows = table->rowCount();
                        int tableColumns = table->columnCount();
                        for (int i = 0; i < tableRows; i++) {
                            for (int n = 0; n < tableColumns; n++) {
                                item = table->item(i, n);
                                QString curElem = item->text();
                                qDebug() << "row: " << i << ", column: " << n << ", item.text: " << curElem;
                            }
                        }
                    }
                }
            }
            else {
                qDebug() << "Could not find GUI element for variable " << widgetName;
            }
        }


        // Move to the next sibling
        node = node.nextSibling();
    }
}


void LinkXmlQt::writeToXml(QDomDocument& curXml, QFile& xmlFile)
{



}
