#ifndef EXCEPTION_H
#define EXCEPTION_H
#include <stdexcept>
#include <QtCore/QString>
/** Exception IException is the iLand model exception class.
    The class uses a string to store exception messages.
  */
class IException : public std::runtime_error {
 public:
   ~IException () throw() {  }
   IException() : std::runtime_error("iLand model exception.") { }
   //IException(QString msg)  { GlobalSettings::instance()->addErrorMessage(msg); }
   //QString toString() const { return GlobalSettings::instance()->errorMessage(); }
   IException(QString msg) : std::runtime_error("iLand model exception.") { add(msg); }
   const QString &message() const { return mMsg; }
   void add(const QString &msg) { if(!mMsg.isEmpty()) mMsg+="\n"; mMsg += msg; }
private:
   QString mMsg;
};


#endif // EXCEPTION_H
