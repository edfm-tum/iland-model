#ifndef AGENTTYPE_H
#define AGENTTYPE_H
#include <QHash>
#include <QVector>
#include <QJSValue>


namespace ABE {


class FMSTP; // forward
class FMUnit; // forward
class FMStand; // forward
class Agent; // forward

class AgentUpdate
{
public:
    AgentUpdate(): mWhat(UpdateInvalid), mAge(-1), mYear(-1) {}
    enum UpdateType { UpdateInvalid, UpdateThinning, UpdateU, UpdateSpecies };
    static UpdateType label(const QString &name);
    // getters
    UpdateType type() const { return mWhat; }
    const QString &value() const { return mValue;}
    const QString &afterActivity() const { return mAfterActivity; }
    int age() const {return mAge; }
    // setters
    void setType(UpdateType type) { mWhat = type; }
    void setValue(QString new_value) { mValue = new_value; }
    void setTimeAge(int age) { mAge = age; }
    void setTimeYear(int year) { mYear = year; }
    void setTimeActivity(QString act) { mAfterActivity = act; }
private:
    UpdateType mWhat;
    QString mValue; ///< new value of the given type
    int mAge; ///< update should happen in that age
    int mYear; ///< update should happen in the given year
    QString mAfterActivity; ///< update should happen after given activity is executed

    friend class AgentType;
};

/** AgentType is the archtype agent including the
 *  agents decision logic. The 'Agent' class describes an indivdual agent.
 *
 **/
class AgentType
{
public:

    AgentType();
    const QString &name() const {return mName; }
    /// setup the definition of STPs for the agent
    void setupSTP(QJSValue agent_code, const QString agent_name);
    /// create an agent of the agent type
    Agent *createAgent(QString agent_name=QString());

    void addAgentUpdate(const AgentUpdate &update, const FMUnit *unit);
    bool agentUpdateForStand(FMStand *stand, QString after_activity, int age);


    /// get stand treatment program by name; return 0 if the stp is not available.
    FMSTP *stpByName(const QString &name);
    /// access to the javascript object
    QJSValue &jsObject() { return mJSObj; }

    /// return the index (0-based) of the species composition given by 'key'. Returns -1 if not found.
    int speciesCompositionIndex(const QString &key);
    QString speciesCompositionName(const int index);

private:
    QString mName; // agent name
    QJSValue mJSObj; ///< javascript object
    QHash<QString,FMSTP*> mSTP; ///< list of all STP linked to this agent type
    QVector<QString> mSpeciesCompositions; ///< list of available target species composition (objects)
    QMultiHash<const FMUnit*, AgentUpdate> mAgentChanges;
};

} // namespace
#endif // AGENTTYPE_H
