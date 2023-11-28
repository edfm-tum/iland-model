#include "settingsdialog.h"
#include "mainwindow.h"
#include "qboxlayout.h"
#include "qlabel.h"
#include "qscrollarea.h"
#include "qstackedwidget.h"
#include "qtreewidget.h"
#include "ui/genericinputwidget.h"

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

    QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    //ui->setupUi(this);
    // create a tree widget used to navigate
    QTreeWidget *treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderLabel("Settings");

    // create a QStackedWidget
    QStackedWidget *stackedWidget = new QStackedWidget();
//    QScrollArea* scrollArea = new QScrollArea(this);
//    scrollArea->setWidgetResizable(true);
//    scrollArea->setWidget(stackedWidget);
    int treeIndex = 0;

    QStringList values;
    QString inputType, defaultValue, labelName, toolTip;
    QString element;
    QStringList xmlPath;


    for (int i = 0; i < mSettingsList.length(); i++) {
        const QString& curModule = mSettingsList[i];

        QTreeWidgetItem* curItem = new QTreeWidgetItem(treeWidget);
        curItem->setText(0, curModule);
        curItem->setWhatsThis(0, QString::number(treeIndex ++));

        // Create pages with scroll areas
        QWidget* curParentStack = new QWidget();

        QScrollArea* ParentScroll = new QScrollArea(this);
        ParentScroll->setWidgetResizable(true);
        ParentScroll->setWidget(curParentStack);

        QVBoxLayout* layoutParent = new QVBoxLayout();
        curParentStack->setLayout(layoutParent);
        layoutParent->setAlignment(Qt::AlignTop);

        QTextBrowser *TextBrowser = new QTextBrowser();
        TextBrowser->setObjectName("setting_" + curModule);
        layoutParent->addWidget(TextBrowser);

        //QUrl htmlFile = QUrl::fromLocalFile("C:/Users/gu47yiy/Documents/iLand_svn/src/iland/res/" + curModule.toLower() + ".html");
        QUrl htmlFile = QUrl::fromLocalFile("F:\\iLand\\book\\book\\_book\\scripting.html");
        //QUrl htmlFile = QUrl("iland-model.org/project+file");
        TextBrowser->setSource(htmlFile);

        //scrollParent->setWidget(curParentStack);
        stackedWidget->addWidget(ParentScroll);

        //QString dialogName = mModulesList[i];

        foreach (QString tab, mTabsOfSettingsList[i]) {
            xmlPath.clear();
            QTreeWidgetItem* curSubItem = new QTreeWidgetItem(curItem);
            curSubItem->setText(0, tab);
            curSubItem->setWhatsThis(0, QString::number(treeIndex ++));

            QWidget* curChildStack = new QWidget();
            curChildStack->setObjectName(curModule + "." + tab.toLower());

            QScrollArea* localScroll = new QScrollArea(this);
            localScroll->setWidgetResizable(true);
            localScroll->setWidget(curChildStack);

            QVBoxLayout* tabLay = new QVBoxLayout();
            curChildStack->setLayout(tabLay);
            tabLay->setAlignment(Qt::AlignTop);

            int labelSize = 0;
            int curLabelSize;

            QString maxLabel;

            for (int n = 0; n < mMetaKeys.length(); n++) {
                element = mMetaKeys[n];
                xmlPath = element.split(".");
                if (xmlPath[0] == curModule.toLower()) {
                    if (xmlPath[1] == tab.toLower()) {
                        values = mMetaValues[n].split(",");
                        inputType = values[0];

                        if (inputType != "noInput" && inputType != "layout") {
                            if (inputType == "group") {
                                QLabel* subheading = new QLabel(values[1]);
                                subheading->setStyleSheet("font-weight: bold");
                                tabLay->addWidget(subheading);
                            }
                            else {
                                if (element.contains("modules.fire") ||
                                    element.contains("modules.wind") ||
                                    element.contains("modules.barkbeetle")) {
                                    defaultValue = values[1];
                                    labelName = values[2];
                                    toolTip = values[3];
                                    }
                                else {
                                    defaultValue = "default";
                                    labelName = "values[2]";
                                    toolTip = "values[3]";
                                }

                                genericInputWidget *newInputWidget = new genericInputWidget(mLinkxqt,
                                                                                            inputType,
                                                                                            defaultValue,
                                                                                            xmlPath,
                                                                                            labelName,
                                                                                            toolTip,
                                                                                            curChildStack);
                                //newInputWidget->setContentsMargins(0,0,0,0);
                                tabLay->addWidget(newInputWidget);
                                QString labelNameGui = labelName + "_label";
                                QLabel *label = curChildStack->findChild<QLabel *>(labelNameGui);
                                curLabelSize = label->fontMetrics().boundingRect(label->text()).width();

                                if (curLabelSize > labelSize) {
                                    labelSize = curLabelSize;
                                    maxLabel = labelNameGui;
                                }

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
                }

                }
                xmlPath.clear();
            }

            foreach (QLabel *label, curChildStack->findChildren<QLabel *>()) {
                label->setMinimumWidth(labelSize + 2);

            }
        stackedWidget->addWidget(localScroll);
        }


    }

    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->addWidget(treeWidget);
    contentLayout->addWidget(stackedWidget);
    contentLayout->setStretch(0, 1);
    contentLayout->setStretch(1, 3);

    QVBoxLayout* overallLayout = new QVBoxLayout(this);
    overallLayout->addLayout(contentLayout);
    overallLayout->addWidget(dialogButtons);

    connect(treeWidget, &QTreeWidget::itemSelectionChanged, this, [stackedWidget, treeWidget]() {
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


}

//SettingsDialog::~SettingsDialog()
//{
//    delete ui;
//}

