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
    void writeCommentXml(QPlainTextEdit* commentEdit, const QStringList& xmlPath);

    void setXmlPath(const QString xmlPath);

    void writeToXml(QDomDocument& curXml, QFile& xmlFile);

private:
    QString mXmlFile;
    QWidget* guiWidget;
    bool mSiblingIsComment;



};

#endif // LINKXMLQT_H
