#include "treeout.h"
#include "tree.h"
#include "model.h"
#include "ressourceunit.h"
#include "species.h"

TreeOut::TreeOut()
{
    setName("Tree Output", "tree");
    setDescription("Output of indivdual trees.");
    columns() << OutputColumn("id", "id of the tree", OutInteger)
             << OutputColumn("name", "tree species name", OutString)
             << OutputColumn("v1", "a double value", OutDouble);
    // filtering
    mFilter=0;
 }

void TreeOut::setup()
{
    if (!settings().isValid())
        throw IException("TreeOut::setup(): no parameter section in init file!");
    QString filter = settings().value(".filter","");
    if (filter!="") {
        mFilter = new Expression(filter);
    }
}

void TreeOut::exec()
{
    AllTreeIterator at(GlobalSettings::instance()->model());
    while (Tree *t=at.next()) {
        if (mFilter && !mFilter->execute()) // skip fields
            continue;
        *this << t->id() << t->species()->name() << t->dbh();
        save();
    }
}

