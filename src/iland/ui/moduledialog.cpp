#include "moduledialog.h"
#include "ui_moduledialog.h"

#include <QtXml>
#include <QDomDocument>


//ModuleDialog::ModuleDialog(const QString& xmlFile, QWidget *parent)
ModuleDialog::ModuleDialog(LinkXmlQt* Linkxqt, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ModuleDialog),
    mLinkxqt(Linkxqt)
{
    //XmlHelper helper;

    ui->setupUi(this);

    connect(ui->buttonBox_moduleDialog, SIGNAL(accepted()), this, SLOT(getModuleInput()));
    //connect(ui->buttonBox_moduleDialog, SIGNAL(accepted()), this, &ModuleDialog::printLineEdits);
    connect(ui->buttonBox_moduleDialog, SIGNAL(rejected()), this, SLOT(closeModuleDialog()));

    //readValuesXml();
    //LinkXmlQt linkxqt(mXmlFile);
    QTabWidget* moduleTabs = ui->moduleTabs;
    QString element = "modules";
    mLinkxqt->readValuesXml(moduleTabs);

}

ModuleDialog::~ModuleDialog()
{
    delete ui;
}

void ModuleDialog::closeModuleDialog()
{
    close();
}

void ModuleDialog::getModuleInput()
{

    QDomDocument xmlDoc;
    QDomElement rootElement = xmlDoc.createElement("project");
    QDomElement moduleNode = xmlDoc.createElement("modules");
    rootElement.appendChild(moduleNode);

    QTabWidget* moduleTabs = ui->moduleTabs;

    //QStringList childrenModules = {"fire", "wind"}

    for (int i = 0; i < moduleTabs->count(); i++) {
        QString currentTab = moduleTabs->widget(i)->objectName();
        QDomElement parentElement = xmlDoc.createElement(currentTab);
        moduleNode.appendChild(parentElement);

        QList<QLineEdit*> list = moduleTabs->widget(i)->findChildren<QLineEdit*>();

        foreach (QLineEdit *w, list) {
            //QString element = "modules." + currentTab + "." + w->objectName() + "=" + w->text();
            //out <<
            //qDebug() << element << "\n";
            QString curElement = w->objectName();
            QDomText curValue = xmlDoc.createTextNode(w->text());

            QDomElement curChild = xmlDoc.createElement(curElement);
            parentElement.appendChild(curChild);
            curChild.appendChild(curValue);
            //curChild.firstChild().setNodeValue(w->text());
            //moduleElement.firstChildElement("fire").firstChild();

    }
    }
    xmlDoc.appendChild(rootElement);
    QFile file("C:\\Users\\gu47yiy\\Documents\\iLand\\tst.xml");
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    xmlDoc.save(out, 4);

    file.close();

 }


//void ModuleDialog::readValuesXml() {
//QDomDocument curXml;
//QFile file(xmlFile);

//QString errorMsg;
//int errorLine, errorColumn;

//if (!curXml.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
//    qDebug() << "Error loading file content. Abort.";
//    file.close();
//}

//else {
//    QTabWidget* moduleTabs = ui->moduleTabs;
//    QDomElement rootElement = curXml.documentElement();
//    QDomElement moduleBranch = rootElement.firstChildElement("modules");

//    for (int i = 0; i < moduleTabs->count(); i++) {
//            QString currentTab = moduleTabs->widget(i)->objectName();
//            QDomElement curBranch = moduleBranch.firstChildElement(currentTab);
//            traverseTreeSetElements(curBranch.firstChild(), i, moduleTabs);
//    }

//}
//}

//void ModuleDialog::traverseTreeSetElements(const QDomNode& node, int tabIndex, QTabWidget* widgetElement) {
//QDomNode curNode = node;
//QString nameModule = widgetElement->widget(tabIndex)->objectName();

//while (!curNode.isNull()) {
//    if (curNode.isElement()) {
//        QDomElement element = curNode.toElement();
//        //qDebug() << QString("Element: %1").arg(element.tagName());

//        // Recurse into child nodes
//        traverseTreeSetElements(curNode.firstChild(), tabIndex, widgetElement);
//    } else if (curNode.isText()) {
//        QString curValue = curNode.toText().data();
//        QString widgetName = nameModule + '_' + curNode.parentNode().toElement().tagName();
//        QWidget* w = widgetElement->widget(tabIndex)->findChild<QWidget *>(widgetName);
//        if (w != nullptr) {
//            if (QLineEdit* lineEdit = dynamic_cast<QLineEdit*>(w)) {
//                lineEdit->setText(curValue);
//                }
//            else if (QComboBox* comboBox = dynamic_cast<QComboBox*>(w)) {
//                curValue = curValue.toLower();

//                if (curValue == "1") {
//                    curValue = "true";
//                    }
//                else if (curValue == "0") {
//                    curValue = "false";
//                    }

//                QStringList comboBoxValidValues;
//                for (int i = 0; i < comboBox->count(); ++i) {
//                        comboBoxValidValues.append(comboBox->itemText(i));
//                }
//                if (comboBoxValidValues.contains(curValue)) {
//                    comboBox->setCurrentText(curValue);
//                    }
//                else {
//                    qDebug() << curValue << " is not valid value. Please Check notation.";
//                    }
//                }
//            else if (QCheckBox* checkBox = dynamic_cast<QCheckBox*>(w)) {
//                if (curValue == "true" || curValue == "True" || curValue == "1") {
//                    checkBox->setChecked(TRUE);
//                    }
//                else if (curValue == "false" || curValue == "False" || curValue == "0") {
//                    checkBox->setChecked(FALSE);
//                }
//                }
//            }
//        else {
//            qDebug() << "Could not find GUI element for variable " << widgetName;
//            }
//        }


//    // Move to the next sibling
//    curNode = curNode.nextSibling();
//}
//}




