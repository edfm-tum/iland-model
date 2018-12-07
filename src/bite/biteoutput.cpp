#include "biteoutput.h"
#include "biteagent.h"
#include "biteengine.h"
namespace BITE {

BiteOutput::BiteOutput()
{
    setName("Annual summary for each Bite agent", "bite");
    setDescription("The output provides annual statistics for each simulated biotic agent. " );
    columns() << OutputColumn::year()
              << OutputColumn("agent", "name of the biotic agent", OutString)
              << OutputColumn("NColonized", "Number of cells that were (at the end of the year, without mortality) colonized by the agent", OutInteger)
              << OutputColumn("NDispersing", "Number of cells that were actively spreading the agent in this year", OutInteger)
              << OutputColumn("NNewlyColonized", "Number of cells that were newly colonized in this year", OutInteger)
              << OutputColumn("agentBiomass", "total biomass of the agent (on all active cells, if applicable)", OutDouble)
              << OutputColumn("treesKilled", "number of host trees killed in the current year", OutInteger)
              << OutputColumn("volumeKilled", "total volume (m3) of trees killed by the agent in the current year", OutDouble)
              << OutputColumn("totalImpact", "total impact (e.g. for defoliatores foliage mass consumed)", OutDouble);

}

void BiteOutput::exec()
{
    BiteEngine *e = BiteEngine::instance();
    for (int i=0;i<e->mAgents.count();++i) {
        BiteAgent *a = e->mAgents[i];
        *this << currentYear();
        *this << a->name();
        *this << a->stats().nActive << a->stats().nDispersal << a->stats().nNewlyColonized;
        *this << a->stats().agentBiomass << a->stats().treesKilled << a->stats().m3Killed;
        *this << a->stats().totalImpact;
        writeRow();
    }
}

void BiteOutput::setup()
{

}


} // namespace
