#ifndef FMUNIT_H
#define FMUNIT_H

class Agent; // forward
/** The FMUnit represents a management unit, i.e. a collection of stands.
 *  */
class FMUnit
{
public:
    FMUnit();
    FMUnit(const Agent *agent): mAgent(agent) {}
    // actions
    void evaluateActivities() const;

private:
    const Agent *mAgent;
};

#endif // FOMEUNITS_H
