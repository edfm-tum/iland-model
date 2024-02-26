#include "linkxmlqt.h"
#include "qtablewidget.h"
//#include "qlineedit.h"

#include <QtXml>
#include <QDomDocument>
#include <QPlainTextEdit>

/*
 * Module used to read/write data from/to xml file.
 * In the gui, data or a new xml file is written
 * whenever the OK button of the Settings Dialog is pressed.
 * Script relies on consistency concerning the names of the nodes in xml tree
 * and the names of the objects.
 * Basis for this is the file project_file_metadata.txt.
 * In the metafile, the dot separated keys refer to the path in the xml doc.
 * The path is created by splitting the keys at the ".". This splitting
 * creates a list (xmlPath) which holds the node names in the xml tree. With the node names
 * the tree is traversed via a simple loop which eventually leads to the value.
 *     foreach (QString node, xmlPath) {
 *       curNode = curNode.firstChildElement(node);
 *   }
 *
 * mXmlFile holds the path on disk to the xml file.
 *
 * */


LinkXmlQt::LinkXmlQt(const QString& xmlFile) :
    mXmlFile(xmlFile),
    mTempHomePath("")
{
    xmlFileLoaded = loadXmlFile();

}

LinkXmlQt::~LinkXmlQt() {

}

// set the path to the xml document on disc
void LinkXmlQt::setXmlPath(const QString& xmlPath)
{
    mXmlFile = xmlPath;
}


bool LinkXmlQt::loadXmlFile()
{
    QDomDocument curXml;
    //setXmlPath(xmlPath);
    //QString filetest = mXmlFile;
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

// Read a comment from xml file.
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

        while (prevSibl.isComment()) {
            commentListed.prepend(prevSibl.toComment().nodeValue());
            prevSibl = prevSibl.previousSibling();
        }

        //file.close();
        return commentListed.join("\n");

    }
    else {
        return "Problem reading comment (see LinkXmlQt::readCommentXml";
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
    // with curNode as the root "looping upwards" through xmltree and delete comments
    // until next not comment node is reached
    while (prevSibl.isComment()) {
        curParentNode.removeChild(prevSibl);
        prevSibl = curNode.previousSibling();
    }
}

void LinkXmlQt::setComment(QDomNode& curNode, QStringList& commentSplittedLines) {
    QDomDocument curXml = curNode.ownerDocument();
    // loop through lines in comments window and successively add comments
    // to xml document
    foreach(QString comment, commentSplittedLines) {
        QDomNode newComment = curXml.createComment(comment);
        QDomNode curParentNode = curNode.parentNode();
        curParentNode.insertBefore(newComment, curNode);
    }
    mLoadedXml = curXml;
}

QString LinkXmlQt::readXmlValue(QString key)
{
    if (!xmlFileLoaded) {
        throw IException("Error with loading data. Check xml file! Abort.");
    }
    QDomDocument curXml = mLoadedXml;
    QDomElement rootElement = curXml.documentElement();
    QStringList xmlPath = key.split(".");
    QDomElement curNode = rootElement;
    foreach (QString node, xmlPath) {
        curNode = curNode.firstChildElement(node);
        //qDebug() << "Current node: " << curNode.nodeName();
    }
    if (!curNode.isNull()) {
        // Get value of current node
        return curNode.firstChild().toText().data();
    } else {
        return QString();
    }
}

QString LinkXmlQt::readXmlComment(QString key)
{
    QStringList xmlPath = key.split(".");
    return readCommentXml(xmlPath);
}

// Function reads the values from the xml file and sets the values in the gui
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
        // Tables are also widgets. However, tables must be treated separately,
        // therefore a dedicated tableList is defined
        QList<QWidget *> widgetElements = stackedWidget->findChildren<QWidget *>();
        QList<QTableWidget *> tableList = stackedWidget->findChildren<QTableWidget *>();

        // Loop through widgets, which aren't tables
        foreach (QWidget* curWidget, widgetElements) {
            //qDebug() << "Aktuelles Widget: " << curWidget->objectName();
            // Objectname of widget corresponds to path in xml tree
            QStringList xmlPath = curWidget->objectName().split(".");

            // Traverse tree along node names
            curNode = rootElement;
            foreach (QString node, xmlPath) {
                curNode = curNode.firstChildElement(node);
                //qDebug() << "Current node: " << curNode.nodeName();
            }

            if (!curNode.isNull()) {
                // Get value of current node
                QString curValue = curNode.firstChild().toText().data();
                //qDebug() << "Aktueller Wert: " << curValue;
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

        }

    }
}


// Values from the gui are read and written into the xml tree
// In general, procedure follows the approach described in loadValuesXml
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
            QString widgetName = curWidget->objectName();
            QStringList xmlPath = widgetName.split(".");

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
                if ( widgetName == "system.path.database" ) {
                    qDebug() << "system.path.database = " << elementValue;
                }
                //QDomText curText = curXml.createTextNode(elementValue);
                curNode.firstChild().setNodeValue(elementValue);
                //curNode.setNodeValue(elementValue);
            }

        }

    }
}

// Save open document to file
void LinkXmlQt::writeToFile(const QString& xmlFilePath)
{
    if (xmlFilePath != "") {
        this->setXmlPath(xmlFilePath);
    }

    QFile file(mXmlFile);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "File couldn't be opened for writing. Abort.";
        return;
    } else {
        qDebug() << "file error output: " << file.error();
        qDebug() << "Write current data to file.";
        QTextStream outStream(&file);
        mLoadedXml.save(outStream, 4);

        file.close();
    }



}

void LinkXmlQt::createXML(const QStringList& metaKeys, const QString &pathXmlFile)
{
    QStringList initCommentList;

    initCommentList << "More details on iland-model.org/project+file";
    initCommentList << "***********************************************************************";
    initCommentList << "(c) 2016, Rupert Seidl & Werner Rammer";
    initCommentList << "***********************************************************************";

    QFile xmlFile(pathXmlFile);

    if (!xmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "File couldn't be opened for writing. Abort.";
        return;
    }

    QTextStream outStream(&xmlFile);

    QDomDocument newXml;

    QDomProcessingInstruction header = newXml.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"");
    newXml.appendChild(header);

    foreach (QString comment, initCommentList) {
        newXml.appendChild(newXml.createComment(comment));
    }

    QDomElement root = newXml.createElement("project");

    QStringList xmlPath;
    QDomNode childBranch, curNode;

    foreach (QString element, metaKeys) {
        if (element != "gui.layout") {
            xmlPath = element.split(".");
            curNode = root;

            for (int i = 0; i < xmlPath.length(); i++) {
                childBranch = curNode.firstChildElement(xmlPath[i]);
                if ( childBranch.isNull() ) {
                    curNode = curNode.appendChild(newXml.createElement(xmlPath[i]));
                }
                else {
                    curNode = childBranch;
                }
            }
            curNode.appendChild(newXml.createTextNode(""));
        }
    }

    newXml.appendChild(root);
    newXml.save(outStream, 4);

    xmlFile.close();
}

void LinkXmlQt::setTempHomePath(QString homePath)
{
    if (homePath == "") {
        QDomElement rootElement = mLoadedXml.documentElement();
        QDomElement curNode = rootElement;

        QStringList homePathXml = {"system", "path", "home"};

        foreach (QString node, homePathXml) {
            curNode = curNode.firstChildElement(node);
        }
        homePath = curNode.firstChild().toText().data();
        //mTempHomePath = curNode.firstChild().toText().data();
    }
    mTempHomePath = homePath;


}

QString LinkXmlQt::getTempHomePath()
{
    return mTempHomePath;
}

QString LinkXmlQt::getXmlFile()
{
    return mXmlFile;
}
