#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>

/** Exception IException is the iLand model exception class.
    The class uses a string list to store exception messages.
  */
class IException : public std::runtime_error {
 public:
    ~IException () throw () {}
   IException() : std::runtime_error("iLand model exception.") { }
   IException(const QString& msg) : std::runtime_error("iLand model exception.") {mMsg += msg;}
   QString asString() const { return mMsg.join("\n"); }
   QStringList asList() const { return mMsg; }
private:
    QStringList mMsg;
};


#endif // EXCEPTION_H
