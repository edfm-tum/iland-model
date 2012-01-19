#ifndef WINDSCRIPT_H
#define WINDSCRIPT_H

#include <QObject>
class WindModule; // forward

class WindScript : public QObject
{
    Q_OBJECT
public:
    explicit WindScript(QObject *parent = 0);
    void setModule(WindModule *module) { mModule = module; }
signals:
    
public slots:
    /** trigger a wind event from javascript.
      @param windspeed average wind speed (m/s)
      @param winddireciton wind direction (0°=N..180°=S..270=W°)
      @param maximum_iterations maximum number of iterations
      @param simulate if true, trees are not really affected
      @param iteration if given a value >=0, then only one iteration is calculated ("interactive mode")
    */
    int windEvent(double windspeed, double winddirection, int max_iteration, bool simulate=false, int iteration=-1);
private:
    WindModule *mModule;
};

#endif // WINDSCRIPT_H
