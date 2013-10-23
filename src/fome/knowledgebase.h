#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H
#include <QList>
class Activity; // forward

/// KnowledgeBase is the "container" for the silvicultural knowledge base.
class KnowledgeBase
{
public:
    // life cycle
    KnowledgeBase();
    // actions
    bool setup(const QString &directory);

private:
    void clear(); ///< delete all activities
    QList<Activity*> mActivities;

};

#endif // KNOWLEDGEBASE_H
