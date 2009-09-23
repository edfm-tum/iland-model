#ifndef STANDLOADER_H
#define STANDLOADER_H

class Model;
class ResourceUnit;

class StandLoader
{
public:
    StandLoader(Model *model): mModel(model) {}
    void processInit();
    void loadFromPicus(const QString &fileName, QPointF offset=QPointF(), ResourceUnit *ru=NULL);
private:
    Model *mModel;
};

#endif // STANDLOADER_H
