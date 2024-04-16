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
    QString element = item->key;
    int index = mKeys.indexOf(element.remove(".connected"));

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
        QString oldValue = item->strValue;
        if (oldValue == newValue.toString() ) {
            mValueTable->removeRow(index);
            mKeys.removeAt(index);
            mKeys.squeeze();
            if ( mKeys.isEmpty() ) {
                emit noChanges();
            }
        }
        else {
            QStringList curItem = {item->label, newValue.toString(), oldValue, item->parentTab};
            for (int column = 0; column < mNumCols; ++ column) {
                QTableWidgetItem *tableItem = new QTableWidgetItem(curItem[column]);
                mValueTable->setItem(index, column, tableItem);
            }
        }
    }
}
