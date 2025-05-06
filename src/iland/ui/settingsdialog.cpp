#include "settingsdialog.h"

#include <QToolBar>
#include <QBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QScrollArea>
#include <QMessageBox>
#include <QAction>
#include <QFileDialog>

#include "qactiongroup.h"
#include "qdir.h"
#include "qpushbutton.h"
#include "ui/dialogcomment.h"
#include "ui/genericinputwidget.h"
#include "qdialogbuttonbox.h"

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
    mStackedWidget = new QStackedWidget();

    //
    //ui_dialogChangedValues = nullptr;
    try {
        setDialogLayout(mTreeWidget, mStackedWidget);
        ui_dialogChangedValues = new DialogChangedValues(this);
        connect(this, &SettingsDialog::updateValueChangeTable, ui_dialogChangedValues, &DialogChangedValues::updateTable);
        connect(ui_dialogChangedValues, &DialogChangedValues::noChanges, this, [this]{a_changedValuesDialog->setEnabled(false);});
    } catch (const IException &e) {
        QMessageBox::critical(this,"Error", e.message());
        close();
    }

}

void SettingsDialog::updateData()
{
    this->setWindowTitle(mLinkxqt->getXmlFile());
    readXMLValues();
    setProjectDescription();
}

void SettingsDialog::setFilterMode(int mode)
{

    // 0: all
    // 1: simplified
    // 2: advanced
    switch (mode) {
    case 0:
        for (auto &v : mKeys) {
            if (v->widget)
                v->widget->setVisible(true);
            foreach (GenericInputWidget* mirroredWidget, v->connectedWidgets) {
                mirroredWidget->setVisible(true);
            }
        }
        qDebug() << "Settings Dialog: Show all (including deprecated)";
        break;

    case 1:
        for (auto &v : mKeys) {
            // set visibility based on a flag - this is fake here, should be part of the meta data!
            // actually, it should be three categories:
            // simplified - often used settings
            // advanced - including the more detailed flags
            // all - also include "deprecated" flags
            if (v->widget)
                v->widget->setVisible(v->visibility == "simple");
            foreach (GenericInputWidget* mirroredWidget, v->connectedWidgets) {
                mirroredWidget->setVisible(v->visibility == "simple");
                }
            }
        qDebug() << "Settings Dialog: Simple view";
        break;
    case 2:
        for (auto &v : mKeys) {

            if (v->widget)
                v->widget->setVisible(v->visibility != "all");
            foreach (GenericInputWidget* mirroredWidget, v->connectedWidgets) {
                mirroredWidget->setVisible(v->visibility != "all");
                }

        }
        qDebug() << "Settings Dialog: Advanced view";
        break;
    }

}


void SettingsDialog::registerChangedComment()
{
    if (!saveButton->isEnabled()) { saveButton->setEnabled(true);
                                    saveAsButton->setEnabled(true);    }

}

void SettingsDialog::registerChangedValue(SettingsItem* item, QVariant newValue)
{

    emit updateValueChangeTable(item, newValue);

//    if (!saveButton->isEnabled()) { saveButton->setEnabled(true);
//                                    a_changedValuesDialog->setEnabled(true);}
    saveAsButton->setEnabled(true);
    saveButton->setEnabled(true);
    a_changedValuesDialog->setEnabled(true);
}

QString SettingsDialog::linkify(QString text, bool collapse) {
    // Regular Expression to match URLs
    QRegularExpression urlRegex("((?:https?|ftp)://\\S+)");

    // Find all URL matches in the text
    QRegularExpressionMatchIterator matches = urlRegex.globalMatch(text);

    QString linkifiedText;
    int lastPos = 0; // Keep track of position in the original text

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString matchedUrl = match.captured(1); // Extract the full URL

        // Add text before the link
        linkifiedText += text.mid(lastPos, match.capturedStart() - lastPos);

        // Create the HTML link
        if (collapse)
            linkifiedText += QString("<a href=\"%1\">(more)</a>").arg(matchedUrl);
        else
            linkifiedText += QString("<a href=\"%1\">%1</a>").arg(matchedUrl);

        lastPos = match.capturedEnd();
    }

    // Add any remaining text at the end
    linkifiedText += text.mid(lastPos);

    return linkifiedText;
}


void SettingsDialog::setDialogLayout(QTreeWidget* treeWidget, QStackedWidget* mStackedWidget)
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
        curParentStack->setBackgroundRole(QPalette::Base);

        // backgroud image only for the top category
        if(curModule == "Project")
            curParentStack->setStyleSheet("background-image: url(:/iland_splash3.jpg);");

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
        mStackedWidget->addWidget(ParentScroll);

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
            curChildStack->setBackgroundRole(QPalette::Base);

            QVBoxLayout* tabLay = new QVBoxLayout();
            tabLay->setObjectName(localScroll->objectName() + "Layout");
            curChildStack->setLayout(tabLay);
            tabLay->setAlignment(Qt::AlignTop);

            localScroll->setWhatsThis(curModule + ":" + tab);
            mStackedWidget->addWidget(localScroll);
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
    QStringList valueTypes = {"string", "boolean", "numeric", "integer", "path", "file", "directory", "combo", "function"};
    QStringList connectedValues;
    QFont fontHeading("Arial", 15, QFont::Bold);
    // List used to store copied gui elements to connect them to their respective twin
    QList<QStringList> connectedElements;
    QLabel* tabHeading;
    bool activeTab = false;

    // SettingsItem must be defined first in order to use them with the mirrored items
    SettingsItem *item;
    for (int n = 0; n < mMetaKeys.length(); n++) {
        element = mMetaKeys[n];
        values = mMetaValues[n].split("|");
        inputType = values[0];
        if (valueTypes.contains(inputType) &&
            !element.startsWith("model.species")) {
            item = new SettingsItem(  n,
                                      element,
                                      inputType, //input type
                                      values[2], // label
                                      values[3], // tooltip
                                      values[1], // default
                                      values[4]); // visibility

            mKeys[element] = item;
        }
    }

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
            curChildStack = mStackedWidget->findChild<QWidget *>(curTabName);


            if (curChildStack) {
                tabLay = curChildStack->findChild<QVBoxLayout *>(curTabName + "Layout");
                activeTab = true;

                if (values.length() > 2) {
                    tabHeading = new QLabel(values[2]);
                    tabHeading->setFont(fontHeading);
                    tabLay->addWidget(tabHeading);
                    if (values.length() > 3) {
                        QString txt = linkify( values[3] );
                        QLabel* tabDescription = new QLabel(txt);
                        tabDescription->setTextFormat(Qt::RichText);
                        tabDescription->setOpenExternalLinks(true);
                        tabDescription->setWordWrap(true);
                        tabLay->addWidget(tabDescription);
                    }
                }

//                tabHeading->setStyleSheet("font-size: 15");
            }
            else {
                //qDebug() << curTabName << ": Tab not found";
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
                QString txt = linkify( values[1], true );
                if (!txt.isEmpty()) {
                    // omit label if text is empty
                    QLabel* subheading = new QLabel(txt);
                    subheading->setStyleSheet("font-weight: bold;");
                    subheading->setAttribute(Qt::WA_NoSystemBackground); // no background color
                    tabLay->addWidget(subheading);
                }
                // a description of the subgroup can be included
                if (values.length() > 2) {
                    QString txt = linkify( values[2], true );
                    QLabel* descriptionSubgroup = new QLabel(txt);
                    //descriptionSubgroup->setStyleSheet("background-color: transparent;");
                    descriptionSubgroup->setAttribute(Qt::WA_NoSystemBackground); // no background color
                    descriptionSubgroup->setTextFormat(Qt::RichText);
                    descriptionSubgroup->setOpenExternalLinks(true);
                    descriptionSubgroup->setWordWrap(true);
                    tabLay->addWidget(descriptionSubgroup);
                }
            }
            else if (valueTypes.contains(inputType)) {
                // adds an actual element
                // size_t index, QString akey, QString atype, QString alabel, QString atooltip, QString adefault
                QString tabName = curChildStack->whatsThis();

                item = mKeys[element];
                item->parentTab = tabName;
                GenericInputWidget *newInputWidget = new GenericInputWidget(mLinkxqt,
                                                                            item);
                // to avoid one extra function call, registerChangedValue coud be omitted and its content could be directly called in the lambda function
                connect(newInputWidget, &GenericInputWidget::widgetValueChanged,
                        this, [this](SettingsItem* item, QVariant newValue){registerChangedValue(item, newValue);});

                 // connect to register changed comment
                connect(newInputWidget, &GenericInputWidget::commentChanged, this, [this]{registerChangedComment();});

                tabLay->addWidget(newInputWidget);

            }
            else if (inputType == "connected") {
                // make a list of all connected elements to add the later at once
                item = mKeys[element];
                if (!item)
                    throw IException(QString("Internal error setting up setting dialog: input for connected element not found: %1").arg(element));
                item->altLabel = values[1];
                GenericInputWidget *newMirroredWidget = new GenericInputWidget(mLinkxqt,
                                                                               item,
                                                                               true);

                // connect to register changes
                connect(newMirroredWidget, &GenericInputWidget::widgetValueChanged,
                        this, [this](SettingsItem* item, QVariant newValue){registerChangedValue(item, newValue);});

                connect(newMirroredWidget, &GenericInputWidget::commentChanged, this, [this]{registerChangedComment();});
                connect(newMirroredWidget, &GenericInputWidget::commentChanged, this, [=]{item->widget->checkCommentButton();});

                tabLay->addWidget(newMirroredWidget);
                QStringList curPair = {element, curTabName};
                connectedElements.append(curPair);
            }
//            else if (curTabName == "tabProject") {
//                QPushButton* editProjectDesricption = new QPushButton("Edit Project Description");
//                tabLay->addWidget(editProjectDesricption);
//            }
        }
    }

    QString original, sibling;
    SettingsItem* origItem;

    //Add mirrored items
    foreach (QList mirroredItemProps, connectedElements) {
        original = mirroredItemProps[0];
        sibling = original + ".connected";
        curTabName = mirroredItemProps[1];
        origItem = mKeys[original];
//        mirroredItem->type = SettingsItem::DataConnected;
        curChildStack = mStackedWidget->findChild<QWidget *>(curTabName);
        tabLay = curChildStack->findChild<QVBoxLayout *>(curTabName + "Layout");

        // connect the mirrored element to its original version

        if (origItem->type == SettingsItem::DataBoolean) {
            QCheckBox *origEl = mStackedWidget->findChild<QCheckBox *>(original);
            QCheckBox *siblEl = mStackedWidget->findChild<QCheckBox *>(sibling);
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
        else if (origItem->type == SettingsItem::DataCombo) {
            QComboBox *origEl = mStackedWidget->findChild<QComboBox *>(original);
            QComboBox *siblEl = mStackedWidget->findChild<QComboBox *>(sibling);
            if (origEl && siblEl) {
                siblEl->setCurrentText(origEl->currentText());
                connect(origEl, &QComboBox::currentTextChanged, this, [siblEl](const QString& text) {
                    siblEl->setCurrentText(text);
                });

                connect(siblEl, &QComboBox::currentTextChanged, this, [origEl](const QString& text) {
                    origEl->setCurrentText(text);
                });
            }
        }
        else if (origItem->type == SettingsItem::DataString ||
                 origItem->type == SettingsItem::DataNumeric ||
                 origItem->type == SettingsItem::DataInteger ||
                 origItem->type == SettingsItem::DataPathDirectory ||
                 origItem->type == SettingsItem::DataPathFile ||
                 origItem->type == SettingsItem::DataFunction) {
            QLineEdit *origEl = mStackedWidget->findChild<QLineEdit *>(original);
            QLineEdit *siblEl = mStackedWidget->findChild<QLineEdit *>(sibling);
            if (origEl && siblEl) {
                siblEl->setText(origEl->text());
                connect(origEl, &QLineEdit::textChanged, this, [siblEl](const QString& text) {
                    siblEl->setText(text);
                });

                connect(siblEl, &QLineEdit::textChanged, this, [origEl](const QString& text) {
                    origEl->setText(text);
                });
            }

        // comment button should reflect state of original button and vice versa
            //newMirroredWidget->checkCommentButton();

        }

    }

    // Formatting of the labels.
    // On each tab the longest label is determined.
    // The maximum length + buffer is then applied to all labels of the respective tab
    int curLabelSize;
    int labelSize;
    QString maxLabel;
    QRegularExpression reTab("\\btab[A-Z]\\w*");
    QList<QWidget *> stackList = mStackedWidget->findChildren<QWidget *>(reTab);
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
    //qDebug() << "Add Tree widget to layout";
    contentLayout->addWidget(treeWidget);
    //qDebug() << "Add Stacked widget to layout";
    contentLayout->addWidget(mStackedWidget);
    //qDebug() << "Set Stretch";
    contentLayout->setStretch(0, 1);
    contentLayout->setStretch(1, 3);

    QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                           QDialogButtonBox::Cancel);
    saveAsButton = dialogButtons->addButton("Save as...", QDialogButtonBox::ActionRole);
    saveButton = dialogButtons->button(QDialogButtonBox::Ok);
    cancelButton = dialogButtons->button(QDialogButtonBox::Cancel);

    saveButton->setText("Save Changes");
    saveButton->setToolTip("Saving changes will overwrite current project file");

    saveAsButton->setToolTip("Opens file dialog, changes will be saved to a new project file."
                             "The new file will be set as the current project file");

    cancelButton->setText("Cancel");

    QVBoxLayout* overallLayout = new QVBoxLayout(this);
    //qDebug() << "Add Toolbar to layout";
    overallLayout->addWidget(  createToolbar(), 1 );
    //qDebug() << "Add content QHBoxLayout to overall QVBoxLayout";
    overallLayout->addLayout(contentLayout);
    //qDebug() << "Add button box to overall Layout";
    overallLayout->addWidget(dialogButtons);

    // Connect the tree widget with the stacked widget for navigation
    connect(treeWidget, &QTreeWidget::itemSelectionChanged, this, [=]() {
        int itemIndex = treeWidget->currentItem()->whatsThis(0).toInt();
        mStackedWidget->setCurrentIndex(itemIndex);
    });

    setLayout(overallLayout);

    // Set values in gui defined in xml file
    //mLinkxqt->readValuesXml(stackedWidget);


    connect(saveAsButton, &QPushButton::clicked,
            this, [=]() {QString newProjectFile;
                         newProjectFile = QFileDialog::getSaveFileName( this,
                                                                        "Save new project file as",
                                                                        mLinkxqt->getTempHomePath(),
                                                                        "*.xml");
                        if (!newProjectFile.isEmpty() && !newProjectFile.isNull()) {
                            mLinkxqt->writeValuesXml(mStackedWidget);
                            mLinkxqt->writeToFile(newProjectFile);
                            mLinkxqt->setXmlPath(newProjectFile);
                            this->parentWidget()->findChild<QLineEdit *>("initFileName")->setText(newProjectFile);
                            ui_dialogChangedValues->mValueTable->setRowCount(0);
                            ui_dialogChangedValues->mKeys = {};
                            a_changedValuesDialog->setDisabled(true);
                            this->close();}});


    connect(dialogButtons, &QDialogButtonBox::accepted, this, [=]() {mLinkxqt->writeValuesXml(mStackedWidget);
                                                                     mLinkxqt->writeToFile(mLinkxqt->getXmlFile());
                                                                     ui_dialogChangedValues->mValueTable->setRowCount(0);
                                                                     ui_dialogChangedValues->mKeys = {};
                                                                     a_changedValuesDialog->setDisabled(true);
                                                                     this->close();});
    connect(dialogButtons, &QDialogButtonBox::rejected, this, [=]() {this->close();});
    connect(this, &QDialog::rejected, this, [=]() { ui_dialogChangedValues->mValueTable->setRowCount(0);
                                                    ui_dialogChangedValues->mKeys = {};
                                                    a_changedValuesDialog->setDisabled(true);
                                                    this->close();});

    // Home path should be used as relative file path.
    // To use it after it was changed without saving the changes and opening the dialog again, a temporary global variable is used.
    QRegularExpression homeEdit ("^system\\.path\\.home(?:\\.connected)?$");
    QList homePathEditList = this->findChildren<QLineEdit *>(homeEdit);

    foreach ( QLineEdit* homePathEdit, homePathEditList) {//  = this->findChild<QLineEdit *>("system.path.home");
        connect(homePathEdit, &QLineEdit::editingFinished, this, [=]{updateFilePaths(homePathEdit->text());});
    }

    setFilterMode(1);
    setTabProjectDescription();

}

void SettingsDialog::setTabProjectDescription() {
    const QString& descriptionTab = "tabProject"; //mSettingsList[0];
    QWidget* curChildStack = mStackedWidget->findChild<QWidget *>(descriptionTab);
    QVBoxLayout* tabLay = curChildStack->findChild<QVBoxLayout *>(descriptionTab + "Layout");
    QLabel* projectDescription = new QLabel();
    projectDescription->setWordWrap(true);
    projectDescription->setObjectName("projectDescriptionLabel");
    tabLay->addWidget(projectDescription);
    QPushButton* editProjectDesricption = new QPushButton("Edit Project Description");

    connect(editProjectDesricption, &QPushButton::clicked, this, [=]() {dialogEditProjectDescription();});

    QSpacerItem* vertSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    tabLay->addSpacerItem(vertSpacer);
    tabLay->addWidget(editProjectDesricption);


}

void SettingsDialog::setProjectDescription() {

    QLabel* projectDescription = this->findChild<QLabel *>("projectDescriptionLabel");
    projectDescription->setText(mLinkxqt->xmlProjectDescription);

}

void SettingsDialog::dialogEditProjectDescription() {

    DialogComment *ui_description = new DialogComment(mLinkxqt);

    connect(ui_description, &DialogComment::projectDescriptionEdited, this, [=]{setProjectDescription();});
    connect(ui_description, &DialogComment::accepted, this, &SettingsDialog::registerChangedComment);

    ui_description->show();
}

void SettingsDialog::showChangedValuesDialog()
{
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
            mLinkxqt->checkXmlNodes(item->key);
            QString value = mLinkxqt->readXmlValue(item->key);
            QString comment = mLinkxqt->readXmlComment(item->key);
            item->strValue = value;
            item->widget->setValue(value);
            item->widget->setComment(comment);
            foreach (GenericInputWidget* mirroredWidget, item->connectedWidgets) {
                mirroredWidget->checkCommentButton();
            }
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

    a_changedValuesDialog = new QAction("Show Changes");
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
    QIcon iconValueChanged = QIcon(":/iconMagnifyingGlass.png");

    a_all->setIcon(iconAll);
    a_all->setText("Show &all");
    a_all->setToolTip("Advanced settings and also deprecated variables.");
    a_advanced->setIcon(iconAdvanced);
    a_advanced->setText("Ad&vanced view");
    a_advanced->setToolTip("Shows also more advanced options, besides the elements of 'Simple view'.");
    a_simple->setIcon(iconSimple);
    a_simple->setText("Sim&ple view");
    a_simple->setToolTip("Only basic options are shown.");
    a_changedValuesDialog->setIcon(iconValueChanged);
    a_changedValuesDialog->setToolTip("Show table with modified values.");

    connect(a_all, &QAction::triggered, this, [this]() { setFilterMode(0);});
    connect(a_simple, &QAction::triggered, this, [this]() { setFilterMode(1);});
    connect(a_advanced, &QAction::triggered, this, [this]() { setFilterMode(2);});

    connect(a_changedValuesDialog, &QAction::triggered, this, [=] {showChangedValuesDialog();});
    QList actions = {a_simple, a_advanced, a_all, a_changedValuesDialog};

    toolbar->addActions(actions);

    a_simple->setChecked(true);

    return toolbar;
}
