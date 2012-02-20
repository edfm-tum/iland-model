#ifndef XMLHELPER_H
#define XMLHELPER_H

#include <QtXml>

class XmlHelper
{
public:
   XmlHelper();
   ~XmlHelper();
   XmlHelper(const QString &fileName) {loadFromFile(fileName);}
   XmlHelper(QDomElement topNode);
   void loadFromFile(const QString &fileName);
   // relative top nodes
   QDomElement top() const { return mTopNode;}
   void setCurrentNode(const QString &path) { mCurrentTop = node(path); } ///< sets @p path as the current (relative) node.
   void setCurrentNode(const QDomElement &node) { mCurrentTop = node; } ///< sets node as the current (relative) top node.
   /// returns true if the current (relative!) node is valid (i.e. not null).
   bool isValid() const { return !mCurrentTop.isNull(); }
   bool hasNode(const QString &path) const; ///< returns true if @p path exists.
    // read access
   QDomElement node(const QString &path) const; ///< retrieve node defined by path (see class description)
   QString value(const QString &path, const QString &defaultValue="") const; ///< retrieve value (as string) from node @p path.
   bool valueBool(const QString &path, const bool defaultValue=false) const; ///< retrieve value (as bool) from node @p path.
   double valueDouble(const QString &path, const double defaultValue=0.) const; ///< retrieve value (as double) from node @p path.
   // write access
   bool setNodeValue(QDomElement &node, const QString &value); ///< set value of 'node'. return true on success.
   bool setNodeValue(const QString &path, const QString &value); ///< set value of node indicated by 'path'. return true on success.
   // special parameters
   double paramValue(const QString &paramName, const double defaultValue=0.) const; ///< get value of special "parameter" space
   QString paramValueString(const QString &paramName, const QString &defaultValue="") const; ///< get value of special "parameter" space
   bool paramValueBool(const QString &paramName, const bool &defaultValue=true) const; ///< get value of special "parameter" space
    // helpers
   QString dump(const QString &path, int levels=-1);
private:
   void dump_rec(QDomElement c, QStringList &stack, QStringList &out, int idx=-1);
   QDomDocument mDoc;
   QDomElement mCurrentTop;
   QDomElement mTopNode;
   QHash<const QString, QString> mParamCache;
};

#endif // XMLHELPER_H
