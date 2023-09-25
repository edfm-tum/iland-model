#ifndef LINKXMLQT_H
#define LINKXMLQT_H


#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>

class LinkXmlQt
{
public:
    //LinkXmlQt();
    explicit LinkXmlQt();
    ~LinkXmlQt();
    void readValuesXml(QTabWidget* tabWidget, QString xmlElement);
    void traverseTreeSetElements(const QDomNode& node, int tabIndex, QTabWidget* tabWidget);

    void readCommentXml(QPlainTextEdit* commentEdit, const QStringList& xmlPath);
    void writeCommentXml(const QString& comment, const QStringList& xmlPath);
    void setXmlPath(const QString xmlPath);
    void writeToXml(QDomDocument& curXml, QFile& xmlFile);
    void clearCommentXml(QDomNode& curNode);
    void setComment(QDomNode& curNode, QStringList& commentSplittedLines);

private:
    QString mXmlFile;
    QWidget* guiWidget;
    bool mSiblingIsComment;
    QDomDocument mLoadedXml;
    QFile file(QString& xmlFile);



};

#endif // LINKXMLQT_H
