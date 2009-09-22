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
   /// returns true if the current (relative!) node is valid (i.e. not null).
   const bool isValid() const { return !mCurrentTop.isNull(); }
   QDomElement node(const QString &path) const;
   bool hasNode(const QString &path) const; ///< returns true if @p path exists.
   void setCurrentNode(const QString &path) { mCurrentTop = node(path); } ///< sets @p path as the current (relative) node.
   QString value(const QString &path, const QString &defaultValue="") const; ///< retrieve value (as string) from node @p path.
   bool valueBool(const QString &path, const bool defaultValue) const; ///< retrieve value (as bool) from node @p path.
   double valueDouble(const QString &path, const double defaultValue) const; ///< retrieve value (as double) from node @p path.
   QString dump(const QString &path, int levels=-1);
   QDomElement top() const { return mTopNode;}
   double paramValue(const QString &paramName, const double defaultValue=0.) const; ///< get value of special "parameter" space
   QString paramValueString(const QString &paramName, const QString &defaultValue="") const; ///< get value of special "parameter" space
   bool paramValueBool(const QString &paramName, const bool &defaultValue=true) const; ///< get value of special "parameter" space
private:
   void dump_rec(QDomElement c, QStringList &stack, QStringList &out, int idx=-1);
   QDomDocument mDoc;
   QDomElement mCurrentTop;
   QDomElement mTopNode;
   QHash<const QString, QString> mParamCache;
};

#endif // XMLHELPER_H
