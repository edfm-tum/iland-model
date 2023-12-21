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
// Tree Widget offers navigation
QTreeWidget* mTreeWidget = new QTreeWidget();
// The "pages" of the stacked widget hold the gui elements
QStackedWidget* mStackedWidget = new QStackedWidget();

//
setDialogLayout(mTreeWidget, mStackedWidget);


}

void SettingsDialog::setDialogLayout(QTreeWidget* treeWidget, QStackedWidget* stackedWidget)
{
    // First part of function defines the general layout and elements of the gui
    // Second part adds all the edits, labels, input elements to the layout
    // In the third part everything is put together and connections are defined

    // First part
    // create a tree widget used for navigation
    treeWidget->setHeaderLabel("Settings");

    // By default, a tree widget offers no inherent way for propper indexing
    int treeIndex = 0;

    QStringList values;
    QString inputType, defaultValue, labelName, toolTip;
    QString element;
    QStringList xmlPath;

    // Loop through first level of navigation hierarchy
    for (int i = 0; i < mSettingsList.length(); i++) {
        QString curModule = mSettingsList[i];

        QTreeWidgetItem* curItem = new QTreeWidgetItem(treeWidget);
        curItem->setText(0, curModule);
        // Set index to use for calling the right stack over tree
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

        // In this loop the second level of the hierarchy is created
        // Definition of the stacks is the same as with the parents before
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
    // Layout set
    // Second part: layout is populated with elements and texts and whatsoever
    // The order of the elements is defined by the order of elements in metadata file
    // What is added depends on the first keyword of the value in the metadata file
    // So far included:
    // tab: name of target stack for following elements (until next tab keyword is reached)
    // layout: can be used to add layout elements. So far only "hl" for horizontal line is included
    // group: adds a label in bold. Only for structuring purposes
    // connected: can be used to add one copy of gui element at a different place. Position in gui depends on position in metafile
    // To add an actual element, the keywords listed in valueTypes are used.

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
        // xmlPath is used to traverse tree in xml document
        xmlPath = element.split(".");
        values = mMetaValues[n].split("|");
        inputType = values[0];

        if (inputType == "tab") {
            // curTabName holds the object name of the tab were subsequent elements are added
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
                // adds a horizontal line
                QFrame *line = new QFrame();
                line->setObjectName(QString::fromUtf8("line"));
                line->setGeometry(QRect(320, 150, 118, 3));
                line->setFrameShape(QFrame::HLine);
                line->setFrameShadow(QFrame::Sunken);
                tabLay->addWidget(line);
            }
        }
        else if (inputType == "group") {
            // adds a group label in bold
            QLabel* subheading = new QLabel(values[1]);
            subheading->setStyleSheet("font-weight: bold");
            tabLay->addWidget(subheading);
        }
        else if (valueTypes.contains(inputType)) {
            // adds an actual element
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
            // adds a copy of a gui element
            // By now (21.12.2023) only one copy is allowed
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
            // list is later used to connect the copies to the original element
            connectedElements.append(curPair);
        }

    }
    // Formatting of the labels.
    // On each tab the longest label is determined.
    // The maximum length + buffer is then applied to all labels of the respective tab
    int curLabelSize;
    int labelSize;
    QString maxLabel;
    QRegularExpression reTab("\\btab[A-Z]\\w*");
    QList<QWidget *> stackList = stackedWidget->findChildren<QWidget *>(reTab);
    //qDebug() << "Num Stacks: " << stackList.length();
    foreach (QWidget *curStack, stackList) {
        //qDebug() << "Current Widget: " << curStack->objectName();
        labelSize = 0;
        // The label name is consistently set in genericInputWidget
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

    // third part: Put everything together
    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->addWidget(treeWidget);
    contentLayout->addWidget(stackedWidget);
    contentLayout->setStretch(0, 1);
    contentLayout->setStretch(1, 3);

    QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QVBoxLayout* overallLayout = new QVBoxLayout(this);
    overallLayout->addLayout(contentLayout);
    overallLayout->addWidget(dialogButtons);

    // Connect the tree widget with the stacked widget for navigation
    connect(treeWidget, &QTreeWidget::itemSelectionChanged, this, [=]() {
        int itemIndex = treeWidget->currentItem()->whatsThis(0).toInt();
        stackedWidget->setCurrentIndex(itemIndex);
    });

    setLayout(overallLayout);

    // Set values in gui defined in xml file
    mLinkxqt->readValuesXml(stackedWidget);

    connect(dialogButtons, &QDialogButtonBox::accepted, this, [=]() {mLinkxqt->writeValuesXml(stackedWidget);
                                                                     mLinkxqt->writeToFile();
                                                                     this->close();});
    connect(dialogButtons, &QDialogButtonBox::rejected, this, [=]() {this->close();});
    connect(this, &QDialog::rejected, this, [=]() {this->close();});

    QString sibling;
    // connect the copied and original element, so that they mirror the state of the other
    foreach (QStringList elementType, connectedElements) {
        element = elementType[0];
        sibling = element + ".connected";
        inputType = elementType[1];

        if (inputType == "boolean") {
            QCheckBox *origEl = stackedWidget->findChild<QCheckBox *>(element);
            QCheckBox *siblEl = stackedWidget->findChild<QCheckBox *>(sibling);
            if (origEl && siblEl) {
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

