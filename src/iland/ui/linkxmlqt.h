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
struct SettingsItem : public QObject {
    Q_OBJECT
public:
    SettingsItem(size_t index, QString akey, QString atype, QString alabel, QString atooltip, QString adefault, QString avisibility, QWidget* parent = nullptr):
        QObject(parent), metakeyIndex(index), key(akey), label(alabel), tooltip(atooltip), defaultValue(adefault), visibility(avisibility){
        auto ti =  mInputTypes.indexOf(atype);
        if (ti < 0)
            throw IException("SettingsItem: invalid input type");
        type = (EInputType) ti;
    };
public:
    enum EInputType {  DataString, DataBoolean, DataNumeric, DataPath, DataPathFile, DataPathDirectory, DataCombo, DataTable };
    GenericInputWidget *widget;
    size_t metakeyIndex;
    QString key;
    EInputType type;
    QString label;
    QString tooltip;
    QString defaultValue;
    QString visibility;
    // link to xml or whatever
    QString strValue; // value as given in XML
    QString comment; // the dynamic comment

public slots:
    void valueChanged(QVariant newValue) {  emit itemChanged(key, newValue);};
    //void valueChanged(const SettingsItem& item, QVariant newValue) {};

signals:
    void itemChanged(const QString& changedKey, QVariant newValue);

private:
    inline const static QStringList mInputTypes { "string", "boolean", "numeric", "path", "file", "directory", "combo", "table" };
};


class LinkXmlQt
{
public:
    //LinkXmlQt();
    explicit LinkXmlQt(const QString& xmlFileIn);
    ~LinkXmlQt();

    QString readCommentXml(const QStringList& xmlPath);
    void writeCommentXml(const QString& comment, const QString& xmlPath);
    void setXmlPath(const QString& xmlPath);
    void writeToFile(const QString &xmlFilePath);
    QString getXmlFile();
    bool loadXmlFile();

    void writeValuesXml(QStackedWidget* stackedWidget);
    void readValuesXml(QStackedWidget* stackedWidget);


    QString readXmlValue(QString key);
    QString readXmlComment(QString key);

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
