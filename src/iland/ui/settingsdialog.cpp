#include "settingsdialog.h"
#include "mainwindow.h"
#include "qboxlayout.h"
#include "qlabel.h"
#include "qscrollarea.h"
#include "qstackedwidget.h"
#include "qtreewidget.h"
#include "ui/genericinputwidget.h"

SettingsDialog::SettingsDialog(LinkXmlQt* Linkxqt,
                               QStringList &inputModules,
                               QList<QStringList> &inputTabList,
                               metadata& inputMetaData,
                               QWidget *parent) :
    QDialog(parent),
    mModulesList(inputModules),
    mTabsOfModulesList(inputTabList),
    mMeta(inputMetaData),
    mLinkxqt(Linkxqt)
{

    QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    //ui->setupUi(this);
    // create a tree widget used to navigate
    QTreeWidget *treeWidget = new QTreeWidget(this);

    // create a QStackedWidget
    QStackedWidget *stackedWidget = new QStackedWidget();
//    QScrollArea* scrollArea = new QScrollArea(this);
//    scrollArea->setWidgetResizable(true);
//    scrollArea->setWidget(stackedWidget);
    int treeIndex = 0;

    for (int i = 0; i < mModulesList.length(); i++) {
        const QString& curModule = mModulesList[i];

        QTreeWidgetItem* curItem = new QTreeWidgetItem(treeWidget);
        curItem->setText(0, curModule);
        curItem->setWhatsThis(0, QString::number(treeIndex ++));

        // Create pages with scroll areas
        QWidget* curParentStack = new QWidget();

        QVBoxLayout* layoutParent = new QVBoxLayout();
        curParentStack->setLayout(layoutParent);

//        scrollParent->setWidget(curParentStack);
        stackedWidget->addWidget(curParentStack);

        QString element;
        QString inputType;
        QStringList xmlPath;
        //QString dialogName = mModulesList[i];

        foreach (QString tab, mTabsOfModulesList[i]) {
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
            for (int n = 0; n < mMeta.elements.length(); n++) {
                element = mMeta.elements[n];
                xmlPath = element.split(".");
                if (xmlPath.length() > 2) {
                    if (xmlPath[0] == curModule.toLower() && xmlPath[1] == tab.toLower()) {
                        inputType = mMeta.inputType[n];

                        if (inputType != "noInput") {
                            if (inputType == "group") {
                                QLabel* subheading = new QLabel(mMeta.defaultValue[n]);
                                subheading->setStyleSheet("font-weight: bold");
                                tabLay->addWidget(subheading);
                            }
                            else {
                                genericInputWidget *newInputWidget = new genericInputWidget(mLinkxqt,
                                                                                            inputType,
                                                                                            mMeta.defaultValue[n],
                                                                                            xmlPath,
                                                                                            mMeta.labelName[n],
                                                                                            mMeta.toolTip[n],
                                                                                            curChildStack);
                                //newInputWidget->setContentsMargins(0,0,0,0);
                                tabLay->addWidget(newInputWidget);
                                QString labelName = mMeta.labelName[n] + "_label";
                                QLabel *label = curChildStack->findChild<QLabel *>(labelName);
                                curLabelSize = label->fontMetrics().boundingRect(label->text()).width();

                                if (curLabelSize > labelSize) {
                                    labelSize = curLabelSize;
                                    maxLabel = labelName;
                                }

                            }
                        }
                    }
                }
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

