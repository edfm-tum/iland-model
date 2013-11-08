#ifndef ACTIVITY_H
#define ACTIVITY_H
#include <QJSValue>
#include <QVector>
class Expression; // forward
class FMStand;

/// Activity encapsulates an individual forest management activity.
/// Activities are stored and organized in the silvicultural KnowledgeBase.
class Activity
{
public:
    Activity();
    ~Activity();
    enum Phase { Invalid, Tending, Thinning, Regeneration };
    // general properties
    QString name() const { return mJS.property("name").toString(); }
    QString description() const { return mJS.property("description").toString(); }
    // properties
    double knowledge() const {return mKnowledge; }
    double economy() const {return mEconomy; }
    double experimentation() const {return mExperimentation; }
    Phase phase() const { return mPhase; }
    // actions
    double evaluate(const FMStand *stand) const;
    // functions
    /// load definition of the Activity from an Javascript Object (value).
    bool setupFromJavascript(QJSValue &value, const QString &variable_name);
    /// if verbose is true, detailed debug information is provided.
    static void setVerbose(bool verbose) {mVerbose = verbose; }
    static bool verbose()  {return mVerbose; } ///< returns true in debug mode
private:
    bool addFilter(QJSValue &js_value, const QString js_name); ///< add a filter from the JS
    static bool mVerbose; ///< debug mode
    QJSValue mJS; ///< holds the javascript representation of the activity

    // properties of activities
    double mKnowledge;
    double mEconomy;
    double mExperimentation;
    Phase mPhase;

    // benchmarking
    int mJSEvaluations;
    int mExprEvaluations;

    // filter items
    struct filter_item {
        filter_item(): filter_type(ftInvalid), expression(0), value(0) {}
        filter_item(const filter_item &item); // copy constructor
        ~filter_item();
        // action
        double evaluate(const Activity *act, const FMStand* stand) const;

        /// set the filter from javascript
        void set(QJSValue &js_value);
        QString toString(); ///< for debugging

        // properties
        enum { ftInvalid, ftConstant, ftExpression, ftJavascript} filter_type;
        Expression *expression;
        double value;
        QJSValue func;
        QString name;
    };
    QVector<filter_item> mFilters;


};

#endif // ACTIVITY_H
