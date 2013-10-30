#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H
#include <QList>
class Activity; // forward
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
    QList<Activity*> mActivities;

};

#endif // KNOWLEDGEBASE_H
