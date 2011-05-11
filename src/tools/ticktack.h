#ifndef TICKTACK_H
#define TICKTACK_H

class TTickTack; // forward of class containing windows specifics

class TickTack
{
public:
    TickTack();
    ~TickTack();
    void start();
    double elapsed(); // seconds since start

private:
    TTickTack *d;
};

#endif // TICKTACK_H
