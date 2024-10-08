#ifndef LINKXMLQT_H
#define LINKXMLQT_H


#include "qstackedwidget.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>

#include "exception.h"

class GenericInputWidget; // forward
/// SettingsItem stores information for a single setting
/// this includes meta data of the setting, as well as the current value
struct SettingsItem  {

    SettingsItem(size_t index, QString akey, QString atype, QString alabel, QString atooltip, QString adefault, QString avisibility, QString aparent = "", QString aAltLabel = ""):
        metakeyIndex(index), widget(nullptr), key(akey), label(alabel), tooltip(atooltip), defaultValue(adefault), visibility(avisibility), parentTab(aparent), altLabel(aAltLabel){
        auto ti =  mInputTypes.indexOf(atype);
        if (ti < 0)
            throw IException("SettingsItem: invalid input type");
        type = (EInputType) ti;
    };

    enum EInputType {  DataString, DataBoolean, DataNumeric, DataInteger, DataPath, DataPathFile, DataPathDirectory, DataCombo, DataFunction, DataConnected, DataTable };
    GenericInputWidget *widget;
    QList<GenericInputWidget *> connectedWidgets;
    size_t metakeyIndex;
    QString key;
    EInputType type;
    QString label;
    QString altLabel;
    QString tooltip;
    QString defaultValue;
    QString visibility;
    QString parentTab;
    // link to xml or whatever
    QString strValue; // value as given in XML
    QString comment; // the dynamic comment

//public slots:
//    void valueChanged(QVariant newValue) {  emit itemChanged(key, newValue);};
//    //void valueChanged(const SettingsItem& item, QVariant newValue) {};

//signals:
//    void itemChanged(const QString& changedKey, QVariant newValue);

private:
    inline const static QStringList mInputTypes { "string", "boolean", "numeric", "integer", "path", "file", "directory", "combo", "function", "connected", "table" };
};


class LinkXmlQt
{
public:
    //LinkXmlQt();
    explicit LinkXmlQt(const QString& xmlFileIn);
    ~LinkXmlQt();

    QString readCommentXml(const QStringList& xmlPath);
    void writeCommentXml(const QString& comment, QStringList xmlPath);
    void setXmlPath(const QString& xmlPath);
    void writeToFile(const QString &xmlFilePath);
    QString getXmlFile();
    bool loadXmlFile();

    void writeValuesXml(QStackedWidget* stackedWidget);
    void readValuesXml(QStackedWidget* stackedWidget);


    QString readXmlValue(QString key);
    QString readXmlComment(QString key);
    void readXmlProjectDescription();
    void writeProjectDescriptionXml(const QString &description);

    QString xmlProjectDescription;

    void createXML(const QStringList &metaKeys, const QString& pathXmlFile);
    void setTempHomePath(QString homePath = "");
    QString getTempHomePath();


private:
    // Variables
    QString mXmlFile;
    QString mTempHomePath;
    QWidget* guiWidget;
    bool mSiblingIsComment;

    QFile mFile(QString& xmlFile);
    bool xmlFileLoaded;
    QDomDocument mLoadedXml;

    // Functions
    void clearCommentXml(QDomNode& curNode);
    void setComment(QDomNode& curNode, QStringList& commentSplittedLines);



};


#endif // LINKXMLQT_H
