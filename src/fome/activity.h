#ifndef ACTIVITY_H
#define ACTIVITY_H
#include <QJSValue>
class Expression; // forward

/// Activity encapsulates an individual forest management activity.
/// Activities are stored and organized in the silvicultural KnowledgeBase.
class Activity
{
public:
    Activity();
    // general properties
    QString name() const { return mJS.property("name").toString(); }
    QString description() const { return mJS.property("description").toString(); }
    // properties
    double knowledge() const {return mKnowledge; }
    double economy() const {return mEconomy; }

    // functions
    /// load definition of the Activity from an Javascript Object (value).
    bool setupFromJavascript(QJSValue &value, const QString &variable_name);
    /// if verbose is true, detailed debug information is provided.
    static void setVerbose(bool verbose) {mVerbose = verbose; }
    static bool verbose()  {return mVerbose; } ///< returns true in debug mode
private:
    bool addFilter(QJSValue &js_value); ///< add a filter from the JS
    static bool mVerbose; ///< debug mode
    QJSValue mJS; ///< holds the javascript representation of the activity

    // properties of activities
    double mKnowledge;
    double mEconomy;

    // filter items
    struct filter_item {
        filter_item(): filter_type(ftInvalid), expression(0), value(0) {}
        filter_item(const filter_item &item); // copy constructor
        ~filter_item();
        /// set the filter from javascript
        void set(QJSValue &js_value);
        QString toString(); ///< for debugging

        // properties
        enum { ftInvalid, ftConstant, ftExpression, ftJavascript} filter_type;
        Expression *expression;
        double value;
        QJSValue func;
    };
    QVector<filter_item> mFilters;


};

#endif // ACTIVITY_H
