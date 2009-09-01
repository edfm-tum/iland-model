#ifndef EXCEPTION_H
#define EXCEPTION_H


/** Exception IException is the iLand model exception class.
    The class uses a string to store exception messages.
  */
class IException  {
 public:
   // ~IException () throw() {  }
   IException()  { }
   //IException(QString msg)  { GlobalSettings::instance()->addErrorMessage(msg); }
   //QString toString() const { return GlobalSettings::instance()->errorMessage(); }
   IException(QString msg)  { add(msg); }
   const QString &toString() const { return mMsg; }
   void add(const QString &msg) { if(!mMsg.isEmpty()) mMsg+="\n"; mMsg += msg; }
private:
   QString mMsg;
};


#endif // EXCEPTION_H
