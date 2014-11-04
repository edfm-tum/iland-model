#ifndef BARKBEETLEMODULE_H
#define BARKBEETLEMODULE_H

class ResourceUnit; // forward

class BarkBeetleModule
{
public:
    BarkBeetleModule();

    void setup(); ///< general setup
    void setup(const ResourceUnit *ru); ///< setup for a specific resource unit

    /// main function to execute the bark beetle module
    void run();

    void yearBegin();

};

#endif // BARKBEETLEMODULE_H
