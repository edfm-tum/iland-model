#ifndef TIMEEVENTS_H
#define TIMEEVENTS_H
#include <QtCore>
/** */
class TimeEvents
{
public:
    TimeEvents();
    // setup
    void clear() { mData.clear(); }
    bool loadFromString(const QString &source);
    bool loadFromFile(const QString &fileName);
    // excecute
    void run(); ///< execute all settings

private:
    QMultiMap<int, QPair<QString, QVariant> > mData;
};

#endif // TIMEEVENTS_H
