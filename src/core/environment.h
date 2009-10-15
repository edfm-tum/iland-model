#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H
class Climate;

class Environment
{
public:
    Environment();
private:
    QList<Climate*> mClimate;
};

#endif // ENVIRONMENT_H
