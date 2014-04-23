#ifndef FMUNIT_H
#define FMUNIT_H
namespace AMIE {


class Agent; // forward
class Scheduler;
/** The FMUnit represents a management unit, i.e. a collection of stands.
 *  */
class FMUnit
{
public:
    FMUnit(const Agent *agent);
    ~FMUnit();
    void setId(const QString &id);
    const QString &id() const {return mId; }
    int index() const {return mIndex; }
    Scheduler *scheduler() {return mScheduler; }
    const Scheduler *constScheduler() const { return mScheduler; }
    const Agent* agent() const { return mAgent; }
    // actions
    void aggregate();
    QStringList info() const;

private:
    QString mId;
    int mIndex;
    const Agent *mAgent;
    Scheduler *mScheduler;
};

} // namespace
#endif // FOMEUNITS_H
