#include "bitewrapper.h"

#include "bitecell.h"

#include <QStringList>

namespace BITE {

static QStringList bitecell_vars=QStringList() << "test" << "test2";

const QStringList BiteWrapper::getVariablesList()
{
    return bitecell_vars;
}

double BiteWrapper::value(const int variableIndex)
{
    return 0.;
}



} // end namespace
