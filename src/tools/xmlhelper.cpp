/** @class XmlHelper
  XmlHelper wraps a XML file and provides some convenient functions to
  retrieve values. Internally XmlHelper uses a QDomDocument (the full structure is
  kept in memory so the size is restricted).
  Use node() to get a QDomElement or use value() to directly retrieve the node value.
  Nodes could be addressed relative to a node defined by setCurrentNode() using a ".".
  The notation is as follows:
  - a '.' character defines a hierarchy
  - [] the Nth element of the same hierarchical layer can be retrieved by [n-1]
  Use also the convenience functions valueBool() and valueDouble().
  While all the value/node etc. functions parse the DOM tree at every call, the data accessed by paramValue() - type
  functions is parsed only once during startup and stored in a QVariant array. Accessible are all nodes that are children of the
   "<parameter>"-node.

  @code
  QDomElement e,f
  e = xml.node("first.second.third"); // e points to "third"
  xml.setCurrentNode("first");
  f = xml.node(".second.third"); // e and f are equal
  e = xml.node("level1[2].level2[3].level3"); // 3rd element of level 1, ...
  int x = xml.value(".second", "0").toInt(); // node value of "second" or "0" if not found.
  if (xml.valueBool("enabled")) // use of valueBool with default value (false)
     ...

  @endcode

  */
#include "xmlhelper.h"
#include "helper.h"
#include "exception.h"

XmlHelper::XmlHelper()
{
}

XmlHelper::XmlHelper(QDomElement topNode)
{
    mTopNode = topNode;
    mCurrentTop = topNode;
}

void XmlHelper::loadFromFile(const QString &fileName)
{
    mDoc.clear();
    QString xmlFile = Helper::loadTextFile(fileName);

    if (!xmlFile.isEmpty()) {

    QString errMsg;
    int errLine, errCol;
        if (!mDoc.setContent(xmlFile, &errMsg, &errLine, &errCol)) {
            throw IException(QString("Error in xml-file!\nError applying xml line %1, col %2.\nMessage: %3").arg(errLine).arg(errCol).arg(errMsg));
        }
    } else {
         throw IException("xmlfile does not exist or is empty!");
    }
    mCurrentTop = mDoc.documentElement(); // top element
    mTopNode = mCurrentTop;

    // fill parameter cache
    QDomElement e = node("parameter");
    e = e.firstChildElement();
    mParamCache.clear();
    while (!e.isNull()) {
        mParamCache[e.nodeName()] = e.text();
        e = e.nextSiblingElement();
    }
}

/** numeric values of elements in the section <parameter> are stored in a QHash structure for faster access.
    with paramValue() these data can be accessed.
  */
double XmlHelper::paramValue(const QString &paramName, const double defaultValue) const
{
    if (mParamCache.contains(paramName))
        return mParamCache.value(paramName).toDouble();
    return defaultValue;
}
QString XmlHelper::paramValueString(const QString &paramName, const QString &defaultValue) const
{
    if (mParamCache.contains(paramName))
        return mParamCache.value(paramName);
    return defaultValue;
}

bool XmlHelper::paramValueBool(const QString &paramName, const bool &defaultValue) const
{
    if (mParamCache.contains(paramName)) {
        QString v = mParamCache.value(paramName);
       return (v=="1" || v=="true");
    }
    return defaultValue;
}

bool XmlHelper::hasNode(const QString &path) const
{
    return !node(path).isNull();
}

QString XmlHelper::value(const QString &path, const QString &defaultValue) const
{
    QDomElement e = node(path);
    if (e.isNull())
        return defaultValue;
    else
        return e.text();
}
bool XmlHelper::valueBool(const QString &path, const bool defaultValue) const
{
    QDomElement e = node(path);
    if (e.isNull())
        return defaultValue;
    QString v = e.text();
    if (v=="true" || v=="True" || v=="1")
        return true;
    else
        return false;
}
double XmlHelper::valueDouble(const QString &path, const double defaultValue) const
{
    QDomElement e = node(path);
    if (e.isNull())
        return defaultValue;
    else
        return e.text().toDouble();
}

/// retreives node with given @p path and a element where isNull() is true if nothing is found.
QDomElement XmlHelper::node(const QString &path) const
{
    QStringList elem = path.split('.', QString::SkipEmptyParts);
    QDomElement c;
    if (path.count()>0 && path.at(0) == '.')
        c = mCurrentTop;
    else
        c = mTopNode;
    foreach (QString level, elem) {
        if (level.indexOf('[')<0) {
            c = c.firstChildElement(level);
            if (c.isNull())
                break;
        } else {
            int pos = level.indexOf('[');
            level.chop(1); // drop closing bracket
            int ind = level.right( level.length() - pos -1).toInt();
            QString name = level.left(pos);
            c = c.firstChildElement(name);
            while (ind>0 && !c.isNull()) {
                c = c.nextSiblingElement();
                ind--;
            }
            if (c.isNull())
                break;
        }
    }
    return c;
}

// private recursive loop
void XmlHelper::dump_rec(QDomElement c, QStringList &stack, QStringList &out, int idx)
{
    if (c.isNull())
        return;
    QDomElement ch = c.firstChildElement();
    bool hasChildren = !ch.isNull();
    bool nChildren = !ch.isNull() && !ch.nextSiblingElement().isNull();
    int child_index=-1;
    while (!ch.isNull()) {
        if (nChildren) {
            child_index++;
            stack.push_back(QString("%1[%2]").arg(ch.nodeName()).arg(child_index));
        } else
          stack.push_back(ch.nodeName());
        dump_rec(ch, stack, out, child_index);
        stack.pop_back();
        ch = ch.nextSiblingElement();
    }
    QString self;
    if (!hasChildren)
        self = c.text();
    self = QString("%1: %3").arg(stack.join("."), self);
    out.push_back(self);
}

QString XmlHelper::dump(const QString &path, int levels)
{
    QDomElement c = node(path);

    QStringList stack;
    stack.push_back(c.nodeName());
    QStringList result;
    dump_rec(c, stack, result);
    return result.join("\n");
}
