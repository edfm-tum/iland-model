#include "settingsdialog.h"

#include <QToolBar>
#include <QBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QScrollArea>


#include "qpushbutton.h"
#include "ui/genericinputwidget.h"
#include "qdialogbuttonbox.h"

FilterButton::FilterButton(const QString& text, const QString& pathIcon, QWidget *parent)
    :QAbstractButton(parent),
    mText(text),
    mPathIcon(pathIcon)
{
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    this->setCheckable(true);
    this->setAutoExclusive(true);

    this->setText(mText);
}

void FilterButton::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPixmap pixmapIconRaw = QPixmap(mPathIcon);
    int iconWidth = pixmapIconRaw.width();
    int iconHeight = pixmapIconRaw.height();
    QIcon icon = isChecked() ? this->icon() : QIcon(pixmapIconRaw);
    QSize iconSize = QSize(iconWidth, iconHeight);
    QPixmap pixmapIcon = icon.pixmap(iconSize, isChecked() ? QIcon::Normal : QIcon::Disabled);

    painter.drawPixmap(rect().center() - pixmapIcon.rect().center(), pixmapIcon);

}

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
    ui_dialogChangedValues = nullptr;
    setDialogLayout(mTreeWidget, mStackedWidget);

}

void SettingsDialog::updateData()
{
    this->setWindowTitle(mLinkxqt->getXmlFile());
    readXMLValues();
}

void SettingsDialog::setFilterMode(int mode)
{
    qDebug() << "filter mode " << mode;
    // 0: all
    // 1: simplified
    // 2: advanced
    switch (mode) {
    case 0:
        for (auto &v : mKeys) {
            v->widget->setVisible(true);
        }
        break;

    case 1:
        for (auto &v : mKeys) {
            // set visibility based on a flag - this is fake here, should be part of the meta data!
            // actually, it should be three categories:
            // simplified - often used settings
            // advanced - including the more detailed flags
            // all - also include "deprecated" flags
            v->widget->setVisible(v->visibility == "simple");
            }
        break;
    case 2:
        for (auto &v : mKeys) {
            v->widget->setVisible(v->visibility != "all");

        }
        break;
    }

}

void SettingsDialog::registerChangedValue(const QString &itemKey, QVariant newValue)
{
    qDebug() << "Key: " << itemKey;
    qDebug() << "Old Value: " << mKeys[itemKey]->strValue;
    qDebug() << "new Value: " << newValue;
    qDebug() << "Tab/parent :" << mKeys[itemKey]->parentTab;
    emit updateValueChangeTable(mKeys[itemKey], newValue);

    if (!saveButton->isEnabled()) {saveButton->setEnabled(true);};
        //a_changedValuesDialog->setEnabled(true);}
}

void SettingsDialog::setDialogLayout(QTreeWidget* treeWidget, QStackedWidget* stackedWidget)
{
    // First part of function defines the general layout and elements of the gui
    // Second part adds all the edits, labels, input elements to the layout
    // In the third part everything is put together and connections are defined

    // First part
    // create a tree widget used for navigation
    treeWidget->setHeaderHidden(true);

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
            //curChildStack->setWhatsThis(curModule + ":" + tab);

            QVBoxLayout* tabLay = new QVBoxLayout();
            tabLay->setObjectName(localScroll->objectName() + "Layout");
            curChildStack->setLayout(tabLay);
            tabLay->setAlignment(Qt::AlignTop);

            localScroll->setWhatsThis(curModule + ":" + tab);
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
    QStringList valueTypes = {"string", "boolean", "numeric", "path", "file", "directory", "combo"};
    QFont fontHeading("Arial", 15, QFont::Bold);
    // List used to store copied gui elements to connect them to their respective twin
    QList<QStringList> connectedElements;
    QLabel* tabHeading;
    bool activeTab = false;

    for (int n = 0; n < mMetaKeys.length(); n++) {
        element = mMetaKeys[n];
        // xmlPath is used to traverse tree in xml document
        xmlPath = element.split(".");
        QString metaValue = mMetaValues[n];
        values = mMetaValues[n].split("|");
        inputType = values[0];


        if (inputType == "tab") {
            // curTabName holds the object name of the tab were subsequent elements are added
            curTabName = values[1];
            curChildStack = stackedWidget->findChild<QWidget *>(curTabName);


            if (curChildStack) {
                tabLay = curChildStack->findChild<QVBoxLayout *>(curTabName + "Layout");
                activeTab = true;

                if (values.length() > 2) {
                    tabHeading = new QLabel(values[2]);
                    tabHeading->setFont(fontHeading);
                    tabLay->addWidget(tabHeading);
                    if (values.length() > 3) {
                        QLabel* tabDescription = new QLabel(values[3]);
                        tabDescription->setWordWrap(true);
                        tabLay->addWidget(tabDescription);
                    }
                }

//                tabHeading->setStyleSheet("font-size: 15");
            }
            else {
                qDebug() << curTabName << ": Tab not found";
                activeTab = false;
            }
        }
        if (activeTab) {
            if (inputType == "layout") {
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
                // a description of the subgroup can be included
                if (values.length() > 2) {
                    QLabel* descriptionSubgroup = new QLabel(values[2]);
                    descriptionSubgroup->setWordWrap(true);
                    tabLay->addWidget(descriptionSubgroup);
                }
            }
            else if (valueTypes.contains(inputType)) {
                // adds an actual element
                // size_t index, QString akey, QString atype, QString alabel, QString atooltip, QString adefault
                QString tabName = curChildStack->whatsThis();
                SettingsItem *item = new SettingsItem(n,
                                  element,
                                  inputType,
                                  values[2], // label
                                  values[3], // tooltip
                                  values[1], // default
                                  values[4], // visibility
                                  tabName); // name of parent tab
                mKeys[element] = item;

                defaultValue = values[1];
                labelName = values[2];
                toolTip = values[3];
    //            GenericInputWidget *newInputWidget = new GenericInputWidget(mLinkxqt,
    //                                                                        inputType,
    //                                                                        defaultValue,
    //                                                                        xmlPath,
    //                                                                        labelName,
    //                                                                        toolTip,
    //                                                                        curChildStack);
                GenericInputWidget *newInputWidget = new GenericInputWidget(mLinkxqt,
                                                                            item);

                tabLay->addWidget(newInputWidget);

            }
            else if (inputType == "connected") {
                // adds a copy of a gui element
                // By now (21.12.2023) only one copy is allowed
                int k;
                QStringList connectedValues;
                if (mMetaKeys.mid(0, n).contains(element)) {
                    k = mMetaKeys.indexOf(element);
                }  else if (mMetaKeys.mid(n+1).contains(element)) {
                    k = mMetaKeys.lastIndexOf(element);
                } else {
                    k=0; // todo: check?
                }
                connectedValues = mMetaValues[k].split("|");
                inputType = connectedValues[0];
                defaultValue = connectedValues[1];
                labelName = values[1];
                if (labelName == "string")
                    labelName = xmlPath.last();
                toolTip = connectedValues[3];
                GenericInputWidget *newInputWidget = new GenericInputWidget(mLinkxqt,
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
        // That way only labels which name input widgets are considered.
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
    saveButton = dialogButtons->button(QDialogButtonBox::Ok);
    cancelButton = dialogButtons->button(QDialogButtonBox::Cancel);

    saveButton->setText("Save Changes");
    cancelButton->setText("Cancel");

    QVBoxLayout* overallLayout = new QVBoxLayout(this);
    overallLayout->addWidget(  createToolbar(), 1 );
    overallLayout->addLayout(contentLayout);
    overallLayout->addWidget(dialogButtons);

    // Connect the tree widget with the stacked widget for navigation
    connect(treeWidget, &QTreeWidget::itemSelectionChanged, this, [=]() {
        int itemIndex = treeWidget->currentItem()->whatsThis(0).toInt();
        stackedWidget->setCurrentIndex(itemIndex);
    });

    setLayout(overallLayout);

    // Set values in gui defined in xml file
    //mLinkxqt->readValuesXml(stackedWidget);


    connect(dialogButtons, &QDialogButtonBox::accepted, this, [=]() {mLinkxqt->writeValuesXml(stackedWidget);
                                                                     mLinkxqt->writeToFile(mLinkxqt->getXmlFile());
                                                                     this->close();});
    connect(dialogButtons, &QDialogButtonBox::rejected, this, [=]() {this->close();});
    connect(this, &QDialog::rejected, this, [=]() {this->close();});

    // Home path should be used as relative file path.
    // To use it after it was changed without saving the changes and opening the dialog again, a temporary global variable is used.
    QLineEdit* homePathEdit = this->findChild<QLineEdit *>("system.path.home");
    connect(homePathEdit, &QLineEdit::editingFinished, this, [=]{updateFilePaths(homePathEdit->text());});

    // connect to register changed values
    for (auto &item : mKeys) {
        connect(item, &SettingsItem::itemChanged, this, &SettingsDialog::registerChangedValue);
    }

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

void SettingsDialog::showChangedValuesDialog()
{
    if (!ui_dialogChangedValues) {
        ui_dialogChangedValues = new DialogChangedValues(this);
       connect(this, &SettingsDialog::updateValueChangeTable, ui_dialogChangedValues, &DialogChangedValues::updateTable);
    }
    ui_dialogChangedValues->show();
}

void SettingsDialog::updateFilePaths(const QString &homePath)
{
    QString fileName, relativeFilePath, lineEditContent;
    QDir pathOldDir;
    QDir homePathAbsolute = QDir(homePath);
    QDir formerHomePath = QDir(mLinkxqt->getTempHomePath());
    mLinkxqt->setTempHomePath(homePath);

    for (auto &item : mKeys) {

        if ((item->type == SettingsItem::DataPathDirectory ||
            item->type == SettingsItem::DataPathFile) &&
            item->key != "system.path.home") {
            QLineEdit *pathField = item->widget->findChild<QLineEdit *>();
            lineEditContent = pathField->text();

            if ( lineEditContent != "" ) {

                pathOldDir = QDir(lineEditContent);
                if ( pathOldDir.isRelative() ) {pathOldDir = QDir(formerHomePath.path() + "/" + lineEditContent);};

                relativeFilePath = homePathAbsolute.relativeFilePath(pathOldDir.path());
                if (relativeFilePath.startsWith(".."))
                {
                    qDebug() << "Set absolute path.";
                    fileName = pathOldDir.path();
                } else {
                    qDebug() << "Set relative path.";
                    fileName = relativeFilePath;
                }
                pathField->setText(QDir::cleanPath(fileName));
            }
        }
    }
}


void SettingsDialog::readXMLValues()
{
    for (auto &item : mKeys) {
        if (item->widget) {
            QString value = mLinkxqt->readXmlValue(item->key);
            QString comment = mLinkxqt->readXmlComment(item->key);
            item->strValue = value;
            item->widget->setValue(value);
            item->widget->setComment(comment);
        }
    }
}

QToolBar *SettingsDialog::createToolbar()
{
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QActionGroup *groupFilter = new QActionGroup(toolbar);
    groupFilter->setExclusive(true);

    QAction *a_all = new QAction("Show &all", groupFilter);
    QAction *a_simple = new QAction("Sim&plified view", groupFilter);
    QAction *a_advanced = new QAction("Ad&vanced view", groupFilter);

    QAction *a_changedValuesDialog = new QAction("Show Changes");
    a_changedValuesDialog->setText("Show changes");
    a_changedValuesDialog->setDisabled(true);

    groupFilter->addAction(a_all);
    groupFilter->addAction(a_simple);
    groupFilter->addAction(a_advanced);

    a_all->setCheckable(true);
    a_simple->setCheckable(true);
    a_advanced->setCheckable(true);

    QIcon iconAll = QIcon(":/iconFilterAll.png");
    QIcon iconAdvanced = QIcon(":/iconFilterAdvanced.png");
    QIcon iconSimple = QIcon(":/iconFilterSimple.png");

    a_all->setIcon(iconAll);
    a_all->setText("Show &all");
    a_all->setToolTip("Advanced settings and also deprecated variables.");
    a_advanced->setIcon(iconAdvanced);
    a_advanced->setText("Ad&vanced view");
    a_advanced->setToolTip("Shows also more advanced options, besides the elements of 'Simple view'.");
    a_simple->setIcon(iconSimple);
    a_simple->setText("Sim&ple view");
    a_simple->setToolTip("Only basic options are shown.");

    connect(a_all, &QAction::triggered, this, [this]() { setFilterMode(0);});
    connect(a_simple, &QAction::triggered, this, [this]() { setFilterMode(1);});
    connect(a_advanced, &QAction::triggered, this, [this]() { setFilterMode(2);});

    connect(a_changedValuesDialog, &QAction::triggered, this, [=] {showChangedValuesDialog();});
    QList actions = {a_simple, a_advanced, a_all, a_changedValuesDialog};

    toolbar->addActions(actions);

    a_simple->setChecked(true);

    return toolbar;
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

