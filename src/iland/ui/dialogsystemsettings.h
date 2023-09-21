#ifndef DIALOGSYSTEMSETTINGS_H
#define DIALOGSYSTEMSETTINGS_H

#include <QDialog>
#include "ui/dialogcomment.h"
#include "ui/linkxmlqt.h"


class LinkXmlQt; // forward

namespace Ui {
class DialogSystemSettings;
}

class DialogSystemSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSystemSettings(const QString& xmlFile, QWidget *parent = nullptr);
    //explicit DialogSystemSettings(LinkXmlQt& Linkxqt, QWidget *parent = nullptr);
    ~DialogSystemSettings();

private:
    Ui::DialogSystemSettings *ui;
    DialogComment* ui_comment;
    QString mXmlFile;
    void editComment(const QString& nameObject, const QString submodule);
    LinkXmlQt *mLinkxqt;

private slots:
    void setPath_home();
    void setPath_database();
    void setPath_lip();
    void setPath_temp();
    void setPath_script();
    void setPath_init();
    void setPath_output();
    void setDatabase_in();
    void setDatabase_out();
    void setDatabase_climate();
    void setJavascript_fileName();

};

#endif // DIALOGSYSTEMSETTINGS_H
