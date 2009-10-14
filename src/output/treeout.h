#ifndef TREEOUT_H
#define TREEOUT_H
#include "expression.h"
#include "output.h"

class Expression;
class TreeOut: public Output
{
public:
    TreeOut();
    virtual void exec();
    virtual void setup();
private:
    Expression mFilter;
};

#endif // TREEOUT_H
