#ifndef DIALOGCHANGEDVALUES_H
#define DIALOGCHANGEDVALUES_H

#include "qtablewidget.h"
#include <QDialog>

struct SettingsItem; //forward

namespace Ui {
class DialogChangedValues;
}

class DialogChangedValues : public QDialog
{
    Q_OBJECT

public:
    explicit DialogChangedValues(QWidget *parent = nullptr);
    ~DialogChangedValues();
    QTableWidget* mValueTable;
    QStringList mKeys;


private:
    Ui::DialogChangedValues *ui;
    int mNumCols;

public slots:
    void updateTable(SettingsItem* item, QVariant newValue);

signals:
    void noChanges();
};

#endif // DIALOGCHANGEDVALUES_H
