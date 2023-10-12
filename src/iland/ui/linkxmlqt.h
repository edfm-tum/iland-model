#ifndef LINKXMLQT_H
#define LINKXMLQT_H


#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>

class LinkXmlQt
{
public:
    //LinkXmlQt();
    explicit LinkXmlQt(QString xmlFileIn);
    ~LinkXmlQt();
    void readValuesXml(QTabWidget* tabWidget);
    void traverseTreeSetElementsGui(const QDomNode& node, int tabIndex, QTabWidget* tabWidget);
    void traverseTreeSetElementsXml(const QDomNode& node, int tabIndex, QTabWidget* tabWidget);

    QString readCommentXml(const QStringList& xmlPath);
    void writeCommentXml(const QString& comment, const QStringList& xmlPath);
    void setXmlPath(const QString xmlPath);
    void writeToFile(QString xmlFilePath = "");
    bool loadXmlFile(const QString filePath);
    void writeValuesXml(QTabWidget* tabWidget);

private:
    // Viariables
    QString xmlFile;
    QWidget* guiWidget;
    bool mSiblingIsComment;

    QFile mFile(QString& xmlFile);
    bool xmlFileLoaded;
    QDomDocument loadedXml;

    // Functions
    void clearCommentXml(QDomNode& curNode);
    void setComment(QDomNode& curNode, QStringList& commentSplittedLines);


};

#endif // LINKXMLQT_H
