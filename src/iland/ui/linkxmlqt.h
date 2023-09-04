#ifndef LINKXMLQT_H
#define LINKXMLQT_H


#include <QLineEdit>
#include <QCheckBox>
#include <QPlainTextEdit>

class LinkXmlQt
{
public:
    //LinkXmlQt();
    explicit LinkXmlQt(const QString& xmlFile);
    ~LinkXmlQt();
    void readValuesXml(QTabWidget* tabWidget, QString xmlElement);
    void traverseTreeSetElements(const QDomNode& node, int tabIndex, QTabWidget* tabWidget);

    void readCommentXml(QPlainTextEdit* commentEdit, const QStringList& xmlPath);
    void editComment(const QString& objectName);


private:
    const QString& xmlFile = xmlFile;
    QWidget* guiWidget;


};

#endif // LINKXMLQT_H
