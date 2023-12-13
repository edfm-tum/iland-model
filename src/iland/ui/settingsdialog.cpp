#include "settingsdialog.h"
#include "qboxlayout.h"
#include "qlabel.h"
#include "qscrollarea.h"
#include "qstackedwidget.h"
#include "qtreewidget.h"
#include "ui/genericinputwidget.h"
#include "qdialogbuttonbox.h"
#include "qtextbrowser.h"

SettingsDialog::SettingsDialog(LinkXmlQt* Linkxqt,
                               QStringList &inputSettings,
                               QList<QStringList> &inputSettingsList,
                               QStringList inputMetaKeys,
                               QStringList inputMetaValues,
                               //metadata& inputMetaData,
                               QWidget *parent) :
    QDialog(parent),
    mSettingsList(inputSettings),
    mTabsOfSettingsList(inputSettingsList),
    mMetaKeys(inputMetaKeys),
    mMetaValues(inputMetaValues),
    //mMeta(inputMetaData),
    mLinkxqt(Linkxqt)
{

QTreeWidget* mTreeWidget = new QTreeWidget();
QStackedWidget* mStackedWidget = new QStackedWidget();

setDialogLayout(mTreeWidget, mStackedWidget);


}

void SettingsDialog::setDialogLayout(QTreeWidget* treeWidget, QStackedWidget* stackedWidget)
{
    // First part of function defines the general layout and elements of the gui
    // Second part adds all the edits, labels, input elements to the layout
    // In the third part everything is put together and connections are defined
    // First part
    // create a tree widget used to navigate
    treeWidget->setHeaderLabel("Settings");

    int treeIndex = 0;

    QStringList values;
    QString inputType, defaultValue, labelName, toolTip;
    QString element;
    QStringList xmlPath;

    for (int i = 0; i < mSettingsList.length(); i++) {
        QString curModule = mSettingsList[i];

        QTreeWidgetItem* curItem = new QTreeWidgetItem(treeWidget);
        curItem->setText(0, curModule);
        curItem->setWhatsThis(0, QString::number(treeIndex ++));

        // Create pages with scroll areas
        QWidget* curParentStack = new QWidget();

        QScrollArea* ParentScroll = new QScrollArea(this);
        ParentScroll->setWidgetResizable(true);
        ParentScroll->setWidget(curParentStack);
        ParentScroll->setObjectName("tab" + curModule.replace(" ", ""));

        QVBoxLayout* layoutParent = new QVBoxLayout();
        curParentStack->setLayout(layoutParent);
        layoutParent->setAlignment(Qt::AlignTop);
        layoutParent->setObjectName(ParentScroll->objectName() + "Layout");

//        QTextBrowser *TextBrowser = new QTextBrowser();
//        TextBrowser->setObjectName("setting_" + curModule);
//        layoutParent->addWidget(TextBrowser);

//        //QUrl htmlFile = QUrl::fromLocalFile("C:/Users/gu47yiy/Documents/iLand_svn/src/iland/res/" + curModule.toLower() + ".html");
//        QUrl htmlFile = QUrl::fromLocalFile("F:\\iLand\\book\\book\\_book\\scripting.html");
//        //QUrl htmlFile = QUrl("iland-model.org/project+file");
//        TextBrowser->setSource(htmlFile);

        //scrollParent->setWidget(curParentStack);
        stackedWidget->addWidget(ParentScroll);

        //QString dialogName = mModulesList[i];

        foreach (QString tab, mTabsOfSettingsList[i]) {

            QTreeWidgetItem* curSubItem = new QTreeWidgetItem(curItem);
            curSubItem->setText(0, tab);
            curSubItem->setWhatsThis(0, QString::number(treeIndex ++));

            QWidget* curChildStack = new QWidget();

            QScrollArea* localScroll = new QScrollArea(this);
            localScroll->setWidgetResizable(true);
            localScroll->setWidget(curChildStack);
            localScroll->setObjectName("tab" + tab.replace(" ", ""));

            QVBoxLayout* tabLay = new QVBoxLayout();
            tabLay->setObjectName(localScroll->objectName() + "Layout");
            curChildStack->setLayout(tabLay);
            tabLay->setAlignment(Qt::AlignTop);

            stackedWidget->addWidget(localScroll);
        }
    }

    // Second part

    QString curTabName;
//        QLayout *tabLay;
    QWidget *curChildStack;
    QVBoxLayout *tabLay;
    QStringList valueTypes = {"string", "boolean", "numeric", "path", "combo"};
    QFont fontHeading("Arial", 15, QFont::Bold);
    // List used to store copied gui elements to connect them to their respective twin
    QList<QStringList> connectedElements;

    for (int n = 0; n < mMetaKeys.length(); n++) {
        element = mMetaKeys[n];
        xmlPath = element.split(".");
        values = mMetaValues[n].split("|");
        inputType = values[0];

        if (inputType == "tab") {
            curTabName = values[1];
            curChildStack = stackedWidget->findChild<QWidget *>(curTabName);

            if (curChildStack) {
                tabLay = curChildStack->findChild<QVBoxLayout *>(curTabName + "Layout");
                if (values.length() > 2) {
                    QLabel* tabHeading = new QLabel(values[2]);
                    tabHeading->setFont(fontHeading);
                    QLabel* tabDescription = new QLabel(values[3]);
                    tabDescription->setWordWrap(true);

                    tabLay->addWidget(tabHeading);
                    tabLay->addWidget(tabDescription);
                }

//                tabHeading->setStyleSheet("font-size: 15");
            }
            else {
                qDebug() << curTabName << ": Tab not found";
            }
        }
        else if (inputType == "layout") {
            if (values[1] == "hl") {
                QFrame *line = new QFrame();
                line->setObjectName(QString::fromUtf8("line"));
                line->setGeometry(QRect(320, 150, 118, 3));
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                tabLay->addWidget(line);
            }
        }
        else if (inputType == "group") {
            QLabel* subheading = new QLabel(values[1]);
            subheading->setStyleSheet("font-weight: bold");
            tabLay->addWidget(subheading);
        }
        else if (valueTypes.contains(inputType)) {
            defaultValue = values[1];
            labelName = values[2];
            toolTip = values[3];
            genericInputWidget *newInputWidget = new genericInputWidget(mLinkxqt,
                                                                        inputType,
                                                                        defaultValue,
                                                                        xmlPath,
                                                                        labelName,
                                                                        toolTip,
                                                                        curChildStack);

            tabLay->addWidget(newInputWidget);

        }
        else if (inputType == "connected") {
            int k;
            QStringList connectedValues;
            if (mMetaKeys.mid(0, n).contains(element)) {
                k = mMetaKeys.indexOf(element);
            }
            else if (mMetaKeys.mid(n+1).contains(element)) {
                k = mMetaKeys.lastIndexOf(element);
            }
            connectedValues = mMetaValues[k].split("|");
            inputType = connectedValues[0];
            defaultValue = connectedValues[1];
            labelName = values[1];
            toolTip = connectedValues[3];
            genericInputWidget *newInputWidget = new genericInputWidget(mLinkxqt,
                                                                        inputType,
                                                                        defaultValue,
                                                                        xmlPath,
                                                                        labelName,
                                                                        toolTip,
                                                                        curChildStack,
                                                                        true);

            tabLay->addWidget(newInputWidget);
            QStringList curPair = {element, inputType};
            connectedElements.append(curPair);
        }

    }

        int curLabelSize;
        int labelSize;
        QString maxLabel;
        QRegularExpression reTab("\\btab[A-Z]\\w*");
        QList<QWidget *> stackList = stackedWidget->findChildren<QWidget *>(reTab);
        //qDebug() << "Num Stacks: " << stackList.length();
        foreach (QWidget *curStack, stackList) {
            //qDebug() << "Current Widget: " << curStack->objectName();
            labelSize = 0;
            QRegularExpression reLabel("\\w*_label");
            QList<QLabel *> labelList = curStack->findChildren<QLabel *>(reLabel);

            foreach (QLabel *label, labelList) {
                curLabelSize = label->fontMetrics().boundingRect(label->text()).width();
//                labelNameGui = label->objectName();

                if (curLabelSize > labelSize) {
                    labelSize = curLabelSize;
                    maxLabel = label->objectName();
                }
            }
            foreach (QLabel *label, labelList) {
                label->setMinimumWidth(labelSize + 2);
            }
        }

    // third part
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->addWidget(treeWidget);
    contentLayout->addWidget(stackedWidget);
    contentLayout->setStretch(0, 1);
    contentLayout->setStretch(1, 3);

    QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QVBoxLayout* overallLayout = new QVBoxLayout(this);
    overallLayout->addLayout(contentLayout);
    overallLayout->addWidget(dialogButtons);

    connect(treeWidget, &QTreeWidget::itemSelectionChanged, this, [=]() {
        int itemIndex = treeWidget->currentItem()->whatsThis(0).toInt();
        stackedWidget->setCurrentIndex(itemIndex);
    });

    setLayout(overallLayout);

    mLinkxqt->readValuesXml(stackedWidget);

    connect(dialogButtons, &QDialogButtonBox::accepted, this, [=]() {mLinkxqt->writeValuesXml(stackedWidget);
                                                                     mLinkxqt->writeToFile();
                                                                     this->close();});
    connect(dialogButtons, &QDialogButtonBox::rejected, this, [=]() {this->close();});
    connect(this, &QDialog::rejected, this, [=]() {this->close();});

    QString sibling;
    foreach (QStringList elementType, connectedElements) {
        element = elementType[0];
        sibling = element + ".connected";
        inputType = elementType[1];

        if (inputType == "boolean") {
            QCheckBox *origEl = stackedWidget->findChild<QCheckBox *>(element);
            QCheckBox *siblEl = stackedWidget->findChild<QCheckBox *>(sibling);
            siblEl->setCheckState(origEl->checkState());
            connect(origEl, &QCheckBox::stateChanged, this, [siblEl](int state) {
                siblEl->setChecked(state == Qt::Checked); // Set state of checkBox2 accordingly
            });

            connect(siblEl, &QCheckBox::stateChanged, this, [origEl](int state) {
                origEl->setChecked(state == Qt::Checked); // Set state of checkBox1 accordingly
            });
        }

    }

}

//void SettingsDialog::setTabCaptions(QStackedWidget* stackWidget)
//{
//    for (int i = 0; i < mSettingsList.length(); i ++) {

//    }
//}

//SettingsDialog::~SettingsDialog()
//{
//    delete ui;
//}

