#include "dialogchangedvalues.h"
#include "ui/linkxmlqt.h"
#include "ui_dialogchangedvalues.h"

DialogChangedValues::DialogChangedValues(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogChangedValues)
{
    ui->setupUi(this);
    mValueTable = this->findChild<QTableWidget *>();
    mNumCols = mValueTable->columnCount();

}

DialogChangedValues::~DialogChangedValues()
{
    delete ui;
}

void DialogChangedValues::updateTable(SettingsItem *item, QVariant newValue)
{

    int index = mKeys.indexOf(item->key);

    if (index == -1) {
        int numRows = mValueTable->rowCount();
        mValueTable->insertRow(numRows);
        QStringList curItem = {item->label, newValue.toString(), item->strValue, item->parentTab};
        for (int column = 0; column < mNumCols; ++ column) {
            QTableWidgetItem *tableItem = new QTableWidgetItem(curItem[column]);
            mValueTable->setItem(numRows, column, tableItem);
        }
        mKeys << item->key;
    } else {
        QStringList curItem = {item->label, newValue.toString(), item->strValue, item->parentTab};
        for (int column = 0; column < mNumCols; ++ column) {
            QTableWidgetItem *tableItem = new QTableWidgetItem(curItem[column]);
            mValueTable->setItem(index, column, tableItem);
        }
    }
}
