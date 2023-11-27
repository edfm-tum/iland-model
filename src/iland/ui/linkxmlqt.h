#ifndef LINKXMLQT_H
#define LINKXMLQT_H


#include "qstackedwidget.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>

class LinkXmlQt
{
public:
    //LinkXmlQt();
    explicit LinkXmlQt(const QString& xmlFileIn);
    ~LinkXmlQt();

    void traverseTreeSetElementsGui(const QDomNode& node, QTabWidget* tabWidget);
    void traverseTreeSetElementsXml(const QDomNode& node, QTabWidget* tabWidget);
    void traverseTreeSetElementsGui(const QDomNode& node, QStackedWidget* stackedWidget);
    void traverseTreeSetElementsXml(const QDomNode& node, QStackedWidget* stackedWidget);

    QString readCommentXml(const QStringList& xmlPath);
    void writeCommentXml(const QString& comment, const QStringList& xmlPath);
    void setXmlPath(const QString& xmlPath);
    void writeToFile(const QString& xmlFilePath = "");
    bool loadXmlFile();

    void writeValuesXml(QTabWidget* tabWidget);
    void writeValuesXml(QStackedWidget* stackedWidget);

    void readValuesXml(QTabWidget* tabWidget);
    void readValuesXml(QStackedWidget* stackedWidget);

private:
    // Viariables
    QString mXmlFile;
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
