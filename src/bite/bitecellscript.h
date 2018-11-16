#ifndef BITECELLSCRIPT_H
#define BITECELLSCRIPT_H

#include <QObject>
#include <QJSValue>
#include <QMap>
#include "expression.h"
#include "bitecell.h"
#include "scripttree.h"

namespace ABE {
class FMTreeList;
}

namespace BITE {

class BiteCellScript : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(bool spreading READ spreading WRITE setSpreading)
    Q_PROPERTY(int yearsLiving READ yearsLiving)
    Q_PROPERTY(ABE::FMTreeList* trees READ trees)
    Q_PROPERTY(BiteAgent* agent READ agent)
public:
    explicit BiteCellScript(QObject *parent = nullptr);
    ~BiteCellScript() {}
    void setCell(BiteCell *c) { mCell = c; }
    BiteCell *cell() const {return mCell; }
    void setAgent(BiteAgent *a) { mAgent = a; }
    BiteAgent *agent() const { return mAgent; }

    bool active() const { return mCell->isActive(); }
    void setActive(bool a) { mCell->setActive(a); }

    bool spreading() const { return mCell->isSpreading(); }
    void setSpreading(bool a) { mCell->setSpreading(a); }

    int yearsLiving() const { return mCell->yearsLiving(); }

    ABE::FMTreeList *trees();

signals:

public slots:
    // access to variables of the cell
    bool hasValue(QString variable_name);
    double value(QString variable_name);
    void setValue(QString var_name, double value);

    void die() { mCell->die(); }

    void reloadTrees();
private:
    BiteCell *mCell;
    BiteAgent *mAgent;
};


class Events {
public:
    Events() {}
    /// clear the list of events
    void clear();
    /// setup events from the javascript object
    void setup(QJSValue &js_value, QStringList event_names, BiteAgent *agent);
    /// execute javascript event /if registered) in the context of the forest stand 'stand'.
    QString run(const QString event, BiteCell *cell=nullptr, QJSValueList *params=nullptr);
    /// returns true, if the event 'event' is available.
    bool hasEvent(const QString &event) const;
    QString dump(); ///< prints some debug info
private:
    QJSValue mInstance; ///< object holding the events
    QMap<QString, QJSValue> mEvents; ///< list of event names and javascript functions
    BiteCellScript mCell;
    QJSValue mScriptCell;
    BiteAgent *mAgent;
};

/** DynamicExpression encapsulates an "expression" that can be either a iLand expression, a constant or a javascript function.
*/
struct DynamicExpression {
    enum EWrapperType { CellWrap, TreeWrap } ;
    DynamicExpression(): wrapper_type(CellWrap), filter_type(ftInvalid), expr(nullptr), mAgent(nullptr), mTree(nullptr) {}
    DynamicExpression(const DynamicExpression &src);
    ~DynamicExpression();
    void setup(const QJSValue &js_value, EWrapperType type, BiteAgent *agent);
    double evaluate(BiteCell *cell) const;
    double evaluate(Tree* tree) const;

    bool evaluateBool(BiteCell *cell) const { return evaluate(cell) > 0.; }
    bool evaluateBool(Tree *tree) const { return evaluate(tree) > 0.; }

    bool isValid() const { return filter_type!=ftInvalid;}
    QString dump() const;
private:
    EWrapperType wrapper_type;
    enum { ftInvalid, ftExpression, ftJavascript, ftConstant} filter_type;
    Expression *expr;
    QJSValue func;
    BiteAgent *mAgent;
    double mConstValue;

    // value for trees
    QJSValue mTreeValue;
    ScriptTree *mTree;

    // value for cells
    BiteCellScript mCell;
    QJSValue mScriptCell;


};

class Constraints {
public:
    Constraints() {}
    void setup(QJSValue &js_value, DynamicExpression::EWrapperType wrap, BiteAgent *agent); ///< setup from javascript
    double evaluate(BiteCell *cell); ///< run the constraints
    double evaluate(ABE::FMTreeList *treelist); ///< run for trees
    QStringList dump(); ///< prints some debug info
private:
    QList<DynamicExpression> mConstraints;
    BiteAgent *mAgent;
};


} // end namespace
#endif // BITECELLSCRIPT_H
