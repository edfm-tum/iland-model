#ifndef SCRIPTGLOBAL_H
#define SCRIPTGLOBAL_H

#include <QObject>
#include <QtScript>
class Model;
class ScriptGlobal : public QObject
{
    Q_OBJECT
    // read only properties
    Q_PROPERTY(int year READ year);
    Q_PROPERTY(int resourceUnitCount READ resourceUnitCount);
    Q_PROPERTY(QString currentDir WRITE setCurrentDir READ currentDir);


public:
    ScriptGlobal(QObject *parent=0);
    static void addToScriptEngine(QScriptEngine &engine); ///< add this class to scripting engine
    // properties accesible by scripts
    int year() const; ///< current year in the model
    int resourceUnitCount() const; ///< get number of resource uinit
    QString currentDir() const { return mCurrentDir; } ///< current execution directory (default is the Script execution directory)
    void setCurrentDir(QString newDir) { mCurrentDir = newDir; } ///< set current working dir
public slots:
    // system stuff
    QVariant setting(QString key); ///< get a value from the global xml-settings (returns undefined if not present)
    void set(QString key, QString value); ///< set the value of a setting
    // file stuff
    QString defaultDirectory(QString dir);
    QString loadTextFile(QString fileName); ///< load content from a text file in a String (@sa CSVFile)
    void saveTextFile(QString fileName, QString content); ///< save string (@p content) to a text file.
    bool fileExists(QString fileName); ///< return true if the given file exists.
    // add trees
    void addSingleTrees(const int resourceIndex, QString content); ///< add single trees
    void addTrees(const int resourceIndex, QString content); ///< add tree distribution
private:
    QString mCurrentDir;
    Model *mModel;
};

#endif // SCRIPTGLOBAL_H
