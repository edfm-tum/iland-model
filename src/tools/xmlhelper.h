#ifndef XMLHELPER_H
#define XMLHELPER_H

#include <QtXml>

class XmlHelper
{
public:
   XmlHelper();
   XmlHelper(const QString &fileName) {loadFromFile(fileName);}
   XmlHelper(QDomElement topNode);
   void loadFromFile(const QString &fileName);
   QDomElement node(const QString &path) const;
   bool hasNode(const QString &path) const; ///< returns true if @p path exists.
   void setCurrentNode(const QString &path) { mCurrentTop = node(path); } ///< sets @p path as the current (relative) node.
   QString value(const QString &path, const QString &defaultValue="") const; ///< retrieve value (as string) from node @p path.
   QString dump(const QString &path, int levels=-1);
   QDomElement top() const { return mTopNode;}
private:
   void dump_rec(QDomElement c, QStringList &stack, QStringList &out, int idx=-1);
   QDomDocument mDoc;
   QDomElement mCurrentTop;
   QDomElement mTopNode;
};

#endif // XMLHELPER_H
