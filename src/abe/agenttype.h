#ifndef AGENTTYPE_H
#define AGENTTYPE_H
#include <QHash>
#include <QVector>
#include <QJSValue>


namespace ABE {


/** AgentType is the archtype agent including the
 *  agents decision logic. The 'Agent' class describes an indivdual agent.
 *
 **/
class FMSTP; // forward
class FMStand; // forward
class Agent; // forward

class AgentType
{
public:

    AgentType();
    const QString &name() const {return mName; }
    /// setup the definition of STPs for the agent
    void setupSTP(QJSValue agent_code, const QString agent_name);
    /// create an agent of the agent type
    Agent *createAgent(QString agent_name=QString());
    /// get stand treatment program by name; return 0 if the stp is not available.
    FMSTP *stpByName(const QString &name);
    /// access to the javascript object
    QJSValue &jsObject() { return mJSObj; }
    // factory functions to create agents.... (
private:
    QString mName; // agent name
    QJSValue mJSObj; ///< javascript object
    QHash<QString,FMSTP*> mSTP; ///< list of all STP linked to this agent type
};

} // namespace
#endif // AGENTTYPE_H
