#ifndef BITECELLSCRIPT_H
#define BITECELLSCRIPT_H

#include <QObject>
#include <QJSValue>
#include <QMap>
#include "expression.h"
#include "bitecell.h"
namespace ABE {
class FMTreeList;
}

namespace BITE {

class BiteCellScript : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ABE::FMTreeList *trees READ trees)
public:
    explicit BiteCellScript(QObject *parent = nullptr);
    void setCell(BiteCell *c) { mCell = c; }

    ABE::FMTreeList *trees();

signals:

public slots:
    void reloadTrees();
private:
    BiteCell *mCell;
};


class Events {
public:
    Events() {}
    /// clear the list of events
    void clear();
    /// setup events from the javascript object
    void setup(QJSValue &js_value, QStringList event_names);
    /// execute javascript event /if registered) in the context of the forest stand 'stand'.
    QJSValue run(const QString event, BiteCell *cell, QJSValueList *params=nullptr);
    /// returns true, if the event 'event' is available.
    bool hasEvent(const QString &event) const;
    QString dump(); ///< prints some debug info
private:
    QJSValue mInstance; ///< object holding the events
    QMap<QString, QJSValue> mEvents; ///< list of event names and javascript functions
};

/** DynamicExpression encapsulates an "expression" that can be either a iLand expression, a constant or a javascript function.
*/
struct DynamicExpression {
    enum EWrapperType { CellWrap, TreeWrap } ;
    DynamicExpression(): wrapper_type(CellWrap), filter_type(ftInvalid), expr(nullptr) {}
    ~DynamicExpression();
    void setup(const QJSValue &js_value, EWrapperType type);
    bool evaluate(BiteCell *cell) const;
    bool evaluate(ABE::FMTreeList* treelist) const;
    bool isValid() const { return filter_type!=ftInvalid;}
    QString dump() const;
private:
    EWrapperType wrapper_type;
    enum { ftInvalid, ftExpression, ftJavascript} filter_type;
    Expression *expr;
    QJSValue func;
};

class Constraints {
public:
    Constraints() {}
    void setup(QJSValue &js_value, DynamicExpression::EWrapperType wrap); ///< setup from javascript
    double evaluate(BiteCell *cell); ///< run the constraints
    double evaluate(ABE::FMTreeList *treelist); ///< run for trees
    QStringList dump(); ///< prints some debug info
private:
    QList<DynamicExpression> mConstraints;
};


} // end namespace
#endif // BITECELLSCRIPT_H
