#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H
#include <QList>
namespace AMIE {

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

};

} // namespace
#endif // KNOWLEDGEBASE_H
