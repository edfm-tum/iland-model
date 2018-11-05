#include "bitecell.h"
#include "globalsettings.h"
#include "model.h"
#include "grid.h"
#include "resourceunit.h"
#include "biteagent.h"
#include "fmtreelist.h"

namespace BITE {

void BiteCell::setup(int cellidx, QPointF pos, BiteAgent *agent)
{
    mIndex = cellidx;
    mRU = GlobalSettings::instance()->model()->RUgrid().constValueAt(pos);
    mAgent = agent;
}

int BiteCell::loadTrees(ABE::FMTreeList *treelist)
{
    Q_ASSERT(mRU != nullptr && mAgent != nullptr);

    QRectF rect = agent()->grid().cellRect( agent()->grid().indexOf(index()) );

    int added = treelist->loadFromRect(mRU, rect);
    return added;
}



} // end namespace
