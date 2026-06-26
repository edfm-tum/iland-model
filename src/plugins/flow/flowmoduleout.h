#ifndef FLOWMODULEOUT_H
#define FLOWMODULEOUT_H

#include <output.h>
class FlowModel; // forward

class FlowModuleOut : public Output
{
public:
    FlowModuleOut();
    void setFlowModel(const FlowModel *model) { mModel = model; }
    virtual void exec();
    virtual void setup();
private:
    const FlowModel *mModel { nullptr };
};

#endif // FLOWMODULEOUT_H
