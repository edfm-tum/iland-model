#ifndef BITESCRIPT_H
#define BITESCRIPT_H
#include <QJSValue>
namespace BITE {


class BiteScript
{
public:
    BiteScript();
    void setup();

    // static members
    static QString JStoString(QJSValue value);
};


} // end name space
#endif // BITESCRIPT_H
