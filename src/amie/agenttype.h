#ifndef AGENTTYPE_H
#define AGENTTYPE_H
#include <QHash>
#include <QVector>
#include <QJSValue>
namespace AMIE {


/** AgentType is the archtype agent including the
 *  agents decision logic. The 'Agent' class describes an indivdual agent.
 *
 **/
class FMSTP; // forward
class FMUnit; // forward
class FMStand; // forward

class AgentType
{
public:

    AgentType();
    const QString &name() const {return mName; }
    /// setup the definition of STPs for the agent
    void setupSTP(QJSValue agent_code, const QString agent_name);
    /// add a unit to the list of managed units
    void addUnit(FMUnit *unit) {mUnits.push_back(unit);}
    ///
    void setup();
    // factory functions to create agents.... (
private:
    QString mName; // agent name
    QJSValue mJSObj; ///< javascript object
    QHash<QString,FMSTP*> mSTP; ///< list of all STP linked to this agent type
    QVector<FMUnit*> mUnits; ///< list of units managed by the agent
};

} // namespace
#endif // AGENTTYPE_H
