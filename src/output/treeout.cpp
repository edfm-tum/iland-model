#include "treeout.h"
#include "helper.h"
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
 }

void TreeOut::setup()
{
    qDebug() << "treeout::setup() called";
    if (!settings().isValid())
        throw IException("TreeOut::setup(): no parameter section in init file!");
    QString filter = settings().value(".filter","");
    if (filter!="") {
        mFilter = QSharedPointer<Expression>(new Expression(filter));
    }
}

void TreeOut::exec()
{
    AllTreeIterator at(GlobalSettings::instance()->model());
    DebugTimer t("TreeOut::exec()");

    while (Tree *t=at.next()) {
        if (mFilter && !mFilter->execute()) // skip fields
            continue;
        *this << t->id() << t->species()->id() << t->dbh();
        writeRow();
    }

}

