#ifndef FMUNIT_H
#define FMUNIT_H
namespace AMIE {


class Agent; // forward
/** The FMUnit represents a management unit, i.e. a collection of stands.
 *  */
class FMUnit
{
public:
    FMUnit():mAgent(0) {}

    FMUnit(const Agent *agent): mAgent(agent) {}
    void setId(const QString &id);
    const QString &id() const {return mId; }
    int index() const {return mIndex; }
    // actions
    void evaluateActivities() const;

private:
    QString mId;
    int mIndex;
    const Agent *mAgent;
};

} // namespace
#endif // FOMEUNITS_H
