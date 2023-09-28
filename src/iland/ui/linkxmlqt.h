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
    void readValuesXml(QTabWidget* tabWidget, QString xmlElement);
    void traverseTreeSetElements(const QDomNode& node, int tabIndex, QTabWidget* tabWidget);

    QString readCommentXml(const QStringList& xmlPath);
    void writeCommentXml(const QString& comment, const QStringList& xmlPath);
    void setXmlPath(const QString xmlPath);
    void writeToFile(QString xmlFilePath = "");

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
    bool loadXmlFile();

};

#endif // LINKXMLQT_H
