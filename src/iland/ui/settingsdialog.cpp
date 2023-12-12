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
    QDialogButtonBox *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    // create a tree widget used to navigate
    treeWidget->setHeaderLabel("Settings");

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
        layoutParent->setObjectName("layout" + curModule);

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

    QString curTabName;
//        QLayout *tabLay;
    QWidget *curChildStack;
    QVBoxLayout *tabLay;
    QStringList valueTypes = {"string", "boolean", "numeric", "path", "combo"};

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
//        else if (inputType == "connected") {
//            QStringList connectedValue =
//        }

    }

        int curLabelSize;
        int labelSize;
        QString maxLabel;
        QRegularExpression exp("\\btab[A-Z]\\w*");
        QList<QWidget *> stackList = stackedWidget->findChildren<QWidget *>(exp);
        //qDebug() << "Num Stacks: " << stackList.length();
        foreach (QWidget *curStack, stackList) {
            //qDebug() << "Current Widget: " << curStack->objectName();
            labelSize = 0;
            QList<QLabel *> labelList = curStack->findChildren<QLabel *>();
//            qDebug() << "Num labels: " << labelList.length();
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


    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->addWidget(treeWidget);
    contentLayout->addWidget(stackedWidget);
    contentLayout->setStretch(0, 1);
    contentLayout->setStretch(1, 3);

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

