#include "patch.h"
#include "fmtreelist.h"

namespace ABE {

Patch::Patch(Patches* patches, int id, QObject *parent)
    : QObject{parent}, mPatches{patches}, mPatchId{id}
{
    mArea = 0;
}


} // namespace ABE
