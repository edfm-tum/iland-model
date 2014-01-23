#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H
#include <QList>
class ActivityOld; // forward
class FMStand; // forward

/// KnowledgeBase is the "container" for the silvicultural knowledge base.
class KnowledgeBase
{
public:
    // life cycle
    KnowledgeBase();
    ~KnowledgeBase();
    bool setup(const QString &directory);
    // actions
    bool evaluate(const FMStand *stand);

private:
    void clear(); ///< delete all activities
    QList<ActivityOld*> mActivities;

};

#endif // KNOWLEDGEBASE_H
