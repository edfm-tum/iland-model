#ifndef STANDLOADER_H
#define STANDLOADER_H

class Model;
class RessourceUnit;

class StandLoader
{
public:
    StandLoader(Model *model): mModel(model) {}
    void processInit();
    void loadFromPicus(const QString &fileName, QPointF offset=QPointF(), RessourceUnit *ru=NULL);
private:
    Model *mModel;
};

#endif // STANDLOADER_H
