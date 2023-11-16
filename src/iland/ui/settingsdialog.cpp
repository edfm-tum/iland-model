#include "settingsdialog.h"
#include "qboxlayout.h"
#include "qscrollarea.h"
#include "qstackedwidget.h"
#include "qtreewidget.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent)

{

    //ui->setupUi(this);
    QStringList modules = QStringList() << "Fire" << "Wind" << "Barkbeetle";
    QTreeWidget *treeWidget = new QTreeWidget(this);

    QTreeWidgetItem* item1 = new QTreeWidgetItem(treeWidget);
    item1->setText(0, "Modules");

    foreach (QString module, modules) {
        QTreeWidgetItem *treeItem = new QTreeWidgetItem(item1);
        treeItem->setText(0, module);
        //item1->Child(treeItem);
    }


    QTreeWidgetItem* item2 = new QTreeWidgetItem(treeWidget);
    item2->setText(0, "System");

    // Create a QStackedWidget
    QStackedWidget* stackedWidget = new QStackedWidget(this);

    // Create pages with scroll areas
    QWidget* page1 = new QWidget(this);
    QScrollArea* scrollArea1 = new QScrollArea;
    scrollArea1->setWidgetResizable(true); // Make the scroll area resizable

    // Create contents for Page 1 (scrollable content)
    QWidget* content1 = new QWidget(scrollArea1);
    // Add content to content1

    scrollArea1->setWidget(content1); // Set the content widget to the scroll area
    QVBoxLayout* layout1 = new QVBoxLayout(page1);
    layout1->addWidget(scrollArea1);

    QWidget* page2 = new QWidget(this);
    QScrollArea* scrollArea2 = new QScrollArea;
    scrollArea2->setWidgetResizable(true);

    // Create contents for Page 2 (scrollable content)
    QWidget* content2 = new QWidget(scrollArea2);
    // Add content to content2

    scrollArea2->setWidget(content2);
    QVBoxLayout* layout2 = new QVBoxLayout(page2);
    layout2->addWidget(scrollArea2);

    // Add pages with scroll areas to the QStackedWidget
    stackedWidget->addWidget(page1);
    stackedWidget->addWidget(page2);

    // Connect signals to switch pages as before...

    // Add the QStackedWidget to the layout of your dialog
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(treeWidget);
    layout->addWidget(stackedWidget);
    setLayout(layout);

}

//SettingsDialog::~SettingsDialog()
//{
//    delete ui;
//}
