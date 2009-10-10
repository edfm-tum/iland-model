/** @class Expression
  An expression engine for mathematical expressions provided as strings.
  @ingroup tools
  @ingroup script
  The main purpose is fast execution speed.
  notes regarding the syntax:
  +,-,*,/ as expected, additionally "^" for power.
  mod(x,y): modulo division, gets remainder of x/y
  functions:
    - sin cos tan
    - exp ln sqrt
    - min max: variable number of arguments, e.g: min(x,y,z)
    - if: if(condition, true, false): if condition=true, return true-case, else false-case. note: both (true, false) are evaluated anyway!
    - incsum: ?? incremental sum - currently not supported.
    - polygon: special function for polygons. polygon(value, x1,y1, x2,y2, x3,y3, ..., xn,yn): return is: y1 if value<x1, yn if value>xn, or the lineraly interpolated numeric y-value.
    - sigmoid: returns a sigmoid function. sigmoid(value, type, param1, param2). see udfSigmoid() for details.
    - rnd rndg: random functions; rnd(from, to): uniform random number, rndg(mean, stddev): gaussian randomnumber (Note: gaussian currently not supported)
    The Expression class also supports some logical operations:
    (logical) True equals to "1", "False" to zero. The precedence rules for parentheses...
    - and
    - or
    - not
  @par Using Model Variables
  With the help of descendants of ExpressionWrapper values of model objects can be accessed. Example Usage:
  @code
  TreeWrapper wrapper;
  Expression basalArea("dbh*dbh*3.1415/4", &wrapper); // expression for basal area, add wrapper (see also setModelObject())
  AllTreeIterator at(GlobalSettings::instance()->model()); // iterator to iterate over all tree in the model
  double sum;
  while (Tree *tree = at.next()) {
      at.setTree(tree); // set actual tree
      sum += basalArea.execute(); // execute calculation
  }
  @endcode

*/
#include <QtCore>
#include "expression.h"

#include "exception.h"
#include "expressionwrapper.h"

#include "helper.h"

#define opEqual 1
#define opGreaterThen 2
#define opLowerThen 3
#define opNotEqual 4
#define opLowerOrEqual 5
#define opGreaterOrEqual 6
#define opAnd 7
#define opOr  8

QString mathFuncList="sin cos tan exp ln sqrt min max if incsum polygon mod sigmoid rnd rndg";
const int  MaxArgCount[15]={1,1,1,1,  1, 1,   -1, -1, 3, 1, -1, 2, 4, 2, 2};
#define    AGGFUNCCOUNT 6
QString AggFuncList[AGGFUNCCOUNT]={"sum", "avg", "max", "min", "stddev", "variance"};

Expression::Expression()
{
    mModelObject = 0;
    m_externVarSpace=0;
}


Expression::ETokType  Expression::next_token()
{
    m_tokCount++;
    m_lastState=m_state;
    // nächsten m_token aus String lesen...
    // whitespaces eliminieren...
    while (strchr(" \t\n\r", *m_pos) && *m_pos)
        m_pos++;

    if (*m_pos==0) {
        m_state=etStop;
        m_token="";
        return etStop; // Ende der Vorstellung
    }
    // whitespaces eliminieren...
    while (strchr(" \t\n\r", *m_pos))
        m_pos++;
    if (*m_pos==',')
    {

        m_token=*m_pos++;
        m_state=etDelimeter;
        return etDelimeter;
    }
    if (strchr("+-*/(){}^", *m_pos)) {
        m_token=*m_pos++;
        m_state=etOperator;
        return etOperator;
    }
    if (strchr("=<>", *m_pos)) {
        m_token=*m_pos++;
        if (*m_pos=='>' || *m_pos=='=')
            m_token+=*m_pos++;
        m_state=etCompare;
        return etCompare;
    }
    if (*m_pos>='0' && *m_pos<='9') {
        // Zahl
        m_token.setNum(atof(m_pos));
        while (strchr("0123456789.",*m_pos) && *m_pos!=0)
            m_pos++;  // nächstes Zeichen suchen...

        m_state=etNumber;
        return etNumber;
    }

    if ((*m_pos>='a' && *m_pos<='z') || (*m_pos>='A' && *m_pos<='Z')) {
        // function ... find brace
        m_token="";
        while (( (*m_pos>='a' && *m_pos<='z') || (*m_pos>='A' && *m_pos<='Z')
                || (*m_pos>='0' && *m_pos<='9') || (*m_pos=='_' || *m_pos=='.') )
            && *m_pos!='(' && m_pos!=0 )
            m_token+=*m_pos++;
        // wenn am Ende Klammer, dann Funktion, sonst Variable.
        if (*m_pos=='(' || *m_pos=='{') {
            m_pos++; // skip brace
            m_state=etFunction;
            return etFunction;
        } else {
            if (m_token.toLower()=="and" || m_token.toLower()=="or") {
                m_state=etLogical;
                return etLogical;
            } else {
                m_state=etVariable;
                return etVariable;
            }
        }
    }
    m_state=etUnknown;
    return etUnknown; // in case no match was found

}

Expression::~Expression()
{
    if (m_expr)
        delete[] m_expr;
    delete[] m_execList;
}


/** sets expression @p expr and checks the syntax (parse).
    Expressions are setup with strict = false, i.e. no fixed binding of variable names.
  */
void Expression::setAndParse(const QString &expr)
{
    setExpression(expr);
    m_strict = false; 
    parse();
}

/// set the current expression.
/// do some preprocessing (e.g. handle the different use of ",", ".", ";")
void Expression::setExpression(const QString& aExpression)
{
    m_expression=aExpression.simplified();

    QByteArray ba = m_expression.toLocal8Bit(); // convert from unicode to 8bit
    m_expr=new char[ba.length()+1]; // reserve memory...
    strcpy(m_expr, ba.constData());

    m_pos=m_expr;  // set starting point...

    for (int i=0; i<10; i++)
        m_varSpace[i]=0.;
    m_parsed=false;
    m_catchExceptions = false;

    mModelObject = 0;
    m_externVarSpace=0;

    m_strict=true; // default....
    m_incSumEnabled=false;
    m_empty=aExpression.trimmed().isEmpty();
    // Buffer:
    m_execListSize = 5; // inital value...
    m_execList = new ExtExecListItem[m_execListSize]; // init

}


// mutex used to serialize expression parsing.
QMutex mutex;

void  Expression::parse()
{
    QMutexLocker locker(&mutex);
    try {
        m_tokString="";
        m_state=etUnknown;
        m_lastState=etUnknown;
        m_constExpression=true;
        m_execIndex=0;
        m_tokCount=0;
        int AktTok;
        next_token();
        while (m_state!=etStop) {
            m_tokString+="\n"+m_token;
            AktTok=m_tokCount;
            parse_levelL0();  // start with logical level 0
            if (AktTok==m_tokCount)
                throw IException("Expression::parse(): Unbalanced Braces.");
            if (m_state==etUnknown){
                m_tokString+="\n***Error***";
                throw IException("Expression::parse(): Syntax error, token: " + m_token);
            }
        }
        m_empty = (m_execIndex == 0);
        m_execList[m_execIndex].Type=etStop;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=0;
        checkBuffer(m_execIndex);
        m_parsed=true;

    } catch (const IException& e) {
        QString msg =QString("Expression::parse: Error in: %1 : %2").arg(m_expr, e.toString());
        if (m_catchExceptions)
            Helper::msg(msg);
        throw IException(msg);
    }
}

void  Expression::parse_levelL0()
{
    // logical operations  (and, or, not)
    QString op;
    parse_levelL1();

    if (m_state==etLogical)  {
        op=m_token.toLower();
        next_token();
        parse_levelL1();
        int logicaltok=0;
        if (op=="and") logicaltok=opAnd;
        if (op=="or") logicaltok=opOr;


        m_execList[m_execIndex].Type=etLogical;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=logicaltok;
        checkBuffer(m_execIndex);
    }
    /*       if (op=='+')
          FResult=temp+FResult;
        if (op=='-')
          FResult=temp-FResult;     */
    //if (m_token=="+" || m_token=="-") {
    //   parse_level0();
    //}
}
void  Expression::parse_levelL1()
{
    // logische operationen (<,>,=,...)
    QString op;
    parse_level0();
    //double temp=FResult;
    if (m_state==etCompare)  {
        op=m_token;
        next_token();
        parse_level0();
        int logicaltok=0;
        if (op=="<") logicaltok=opLowerThen;
        if (op==">") logicaltok=opGreaterThen;
        if (op=="<>") logicaltok=opNotEqual;
        if (op=="<=") logicaltok=opLowerOrEqual;
        if (op==">=") logicaltok=opGreaterOrEqual;
        if (op=="=")  logicaltok=opEqual;

        m_execList[m_execIndex].Type=etCompare;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=logicaltok;
        checkBuffer(m_execIndex);
    }
    /*       if (op=='+')
          FResult=temp+FResult;
        if (op=='-')
          FResult=temp-FResult;     */
    //if (m_token=="+" || m_token=="-") {
    //   parse_level0();
    //}
}

void  Expression::parse_level0()
{
    // plus und minus
    QByteArray op;
    parse_level1();

    while (m_token=="+" || m_token=="-")  {
        op=m_token.toAscii();
        next_token();
        parse_level1();
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=op.at(0);///op.constData()[0];
        checkBuffer(m_execIndex);
    }
    /*       if (op=='+')
          FResult=temp+FResult;
        if (op=='-')
          FResult=temp-FResult;     */
    //if (m_token=="+" || m_token=="-") {
    //   parse_level0();
    //}
}

void  Expression::parse_level1()
{
    // mal und division
    QByteArray op;
    parse_level2();
    //double temp=FResult;
    // alt:        if (m_token=="*" || m_token=="/") {
    while (m_token=="*" || m_token=="/") {
        op=m_token.toAscii();
        next_token();
        parse_level2();
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index=op.at(0);
        checkBuffer(m_execIndex);
    }
    /*       if (op=="*")
           FResult=temp*FResult;
        if (op=="/")
           FResult=temp/FResult; */

}

void  Expression::atom()
{
    if (m_state==etVariable || m_state==etNumber) {
        if (m_state==etNumber) {
            m_result=m_token.toDouble();
            m_execList[m_execIndex].Type=etNumber;
            m_execList[m_execIndex].Value=m_result;
            m_execList[m_execIndex++].Index=-1;
            checkBuffer(m_execIndex);
        }
        if (m_state==etVariable) {
            addVar(m_token);
            m_execList[m_execIndex].Type=etVariable;
            m_execList[m_execIndex].Value=0;
            m_execList[m_execIndex++].Index=getVarIndex(m_token);
            checkBuffer(m_execIndex);
            m_constExpression=false;
        }
        next_token();
    } else if (m_state==etStop || m_state==etUnknown)
        throw IException("Unexpected end of m_expression.");
}


void  Expression::parse_level2()
{
    // x^y
    parse_level3();
    //double temp=FResult;
    while (m_token=="^") {
        next_token();
        parse_level3();
        //FResult=pow(temp,FResult);
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index='^';
        checkBuffer(m_execIndex);
    }
}
void  Expression::parse_level3()
{
    // unary operator (- bzw. +)
    QString op;
    op=m_token;
    bool Unary=false;
    if (op=="-" && (m_lastState==etOperator || m_lastState==etUnknown || m_lastState==etCompare || m_lastState==etLogical || m_lastState==etFunction)) {
        next_token();
        Unary=true;
    }
    parse_level4();
    if (Unary && op=="-") {
        //FResult=-FResult;
        m_execList[m_execIndex].Type=etOperator;
        m_execList[m_execIndex].Value=0;
        m_execList[m_execIndex++].Index='_';
        checkBuffer(m_execIndex);
    }

}

void  Expression::parse_level4()
{
    // Klammer und Funktionen
    QString func;
    atom();
    //double temp=FResult;
    if (m_token=="(" || m_state==etFunction) {
        func=m_token;
        if (func=="(")   // klammerausdruck
        {
            next_token();
            parse_levelL0();
        }
        else        // funktion...
        {
            int argcount=0;
            int idx=getFuncIndex(func);
            next_token();
            //m_token="{";
            // bei funktionen mit mehreren Parametern
            while (m_token!=")") {
                argcount++;
                parse_levelL0();
                if (m_state==etDelimeter)
                    next_token();
            }
            if (MaxArgCount[idx]>0 && MaxArgCount[idx]!=argcount)
                throw IException( QString("Function %1 assumes %2 arguments!").arg(func).arg(MaxArgCount[idx]));
            //throw std::logic_error("Funktion " + func + " erwartet " + std::string(MaxArgCount[idx]) + " Parameter!");
            m_execList[m_execIndex].Type=etFunction;
            m_execList[m_execIndex].Value=argcount;
            m_execList[m_execIndex++].Index=idx;
            checkBuffer(m_execIndex);
        }
        if (m_token!="}" && m_token!=")") // Fehler
            throw IException(QString("Expression::unbalanced number of parentheses in [%1].").arg(m_expression));
        next_token();
    }
}

void Expression::setVar(const QString& Var, double Value)
{
    if (!m_parsed)
        parse();
    int idx=getVarIndex(Var);
    if (idx>=0 && idx<10)
        m_varSpace[idx]=Value;
    else
        throw IException("Invalid variable " + Var);
}

double Expression::calculate(double Val1, double Val2)
{
    m_varSpace[0]=Val1;
    m_varSpace[1]=Val2;
    m_strict=false;
    return execute();
}

int Expression::getFuncIndex(const QString& functionName)
{
    int pos=mathFuncList.indexOf(functionName);
    if (pos<0)
        throw IException("Function " + functionName + " not defined!");
    int idx=0;
    for (int i=1;i<pos;i++)
        if (mathFuncList[i]==' ') ++idx;
    return idx;
}

double Expression::execute()
{
    if (!m_parsed)
        parse();
    ExtExecListItem *exec=m_execList;
    int i;
    m_result=0.;
    double Stack[20];
    bool   LogicStack[20];
    bool   *lp=LogicStack;
    double *p=Stack;  // p=head pointer
    *lp++=true; // zumindest eins am anfang...
    if (isEmpty()) {
        // leere expr.
        //m_logicResult=false;
        m_result=0.;
        return 0.;
    }
    while (exec->Type!=etStop) {
        switch (exec->Type) {
        case etOperator:
            p--;
            switch (exec->Index) {
                  case '+': *(p-1)=*(p-1) + *p;  break;
                  case '-': *(p-1)=*(p-1)-*p;  break;
                  case '*': *(p-1)=*(p-1) * *p;  break;
                  case '/': *(p-1)=*(p-1) / *p;  break;
                  case '^': *(p-1)=pow(*(p-1), *p);  break;
                  case '_': *p=-*p; p++; break;  // unary operator -
                  }
            break;
        case etVariable:
            if (exec->Index<100)
                *p++=m_varSpace[exec->Index];
            else if (exec->Index<1000)
                *p++=getModelVar(exec->Index);
            else
                *p++=getExternVar(exec->Index);
            break;
        case etNumber:
            *p++=exec->Value;
            break;
        case etFunction:
            p--;
            switch (exec->Index) {
                 case 0: *p=sin(*p); break;
                 case 1: *p=cos(*p); break;
                 case 2: *p=tan(*p); break;
                 case 3: *p=exp(*p); break;
                 case 4: *p=log(*p); break;
                 case 5: *p=sqrt(*p); break;
                     // min, max, if:  variable zahl von argumenten
                 case 6:      // min
                     for (i=0;i<exec->Value-1;i++,p--)
                         *(p-1)=(*p<*(p-1))?*p:*(p-1);
                     break;
                 case 7:  //max
                     for (i=0;i<exec->Value-1;i++,p--)
                         *(p-1)=(*p>*(p-1))?*p:*(p-1);
                     break;
                 case 8: // if
                     if (*(p-2)==1) // true
                         *(p-2)=*(p-1);
                     else
                         *(p-2)=*p; // false
                     p-= 2; // die beiden argumente wegwerfen...
                     break;
                 case 9: // incrementelle summe
                     m_incSumVar+=*p;
                     *p=m_incSumVar;
                     break;
                 case 10: // Polygon-Funktion
                     *(p-(int)(exec->Value-1))=udfPolygon(*(p-(int)(exec->Value-1)), p, (int)exec->Value);
                     p-=(int) (exec->Value-1);
                     break;
                 case 11: // Modulo-Division: erg=rest von arg1/arg2
                     p--; // p zeigt auf ergebnis...
                     *p=fmod(*p, *(p+1));
                     break;
                 case 12: // hilfsfunktion für sigmoidie sachen.....
                     *(p-3)=udfSigmoid(*(p-3), *(p-2), *(p-1), *p);
                     p-=3; // drei argumente (4-1) wegwerfen...
                     break;
                 case 13: case 14: // rnd(from, to) bzw. rndg(mean, stddev)
                             p--;
                     // index-13: 1 bei rnd, 0 bei rndg
                     *p=udfRandom(exec->Index-13, *p, *(p+1));
                     break;
                 }
            p++;
            break;
        case etLogical:
            p--;
            lp--;
            switch (exec->Index) {
                case opAnd: *(lp-1)=(*(lp-1) && *lp);  break;
                case opOr:  *(lp-1)=(*(lp-1) || *lp);  break;
            }
            if (*(lp-1))
                *(p-1)=1;
            else
                *(p-1)=0;
            break;
        case etCompare: {
            p--;
            bool LogicResult=false;
            switch (exec->Index) {
                 case opEqual: LogicResult=(*(p-1)==*p); break;
                 case opNotEqual: LogicResult=(*(p-1)!=*p); break;
                 case opLowerThen: LogicResult=(*(p-1)<*p); break;
                 case opGreaterThen: LogicResult=(*(p-1)>*p); break;
                 case opGreaterOrEqual: LogicResult=(*(p-1)>=*p); break;
                 case opLowerOrEqual: LogicResult=(*(p-1)<=*p); break;
                 }
            if (LogicResult) {
                *(p-1)=1;   // 1 means true...
            } else {
                *(p-1)=0;
            }

            *lp++=LogicResult;
            break; }
        case etStop: case etUnknown: case etDelimeter: throw IException("invalid token during execution.");
        } // switch()

        exec++;
    }
    if (p-Stack!=1)
        throw IException("Expression::execute: stack unbalanced!");
    m_result=*(p-1);
    //m_logicResult=*(lp-1);
    return m_result;
}

double * Expression::addVar(const QString& VarName)
{
    // add var
    int idx=m_varList.indexOf(VarName);
    if (idx==-1) {
        m_varList+=VarName;
        idx=m_varList.size()-1;
    }
    return &m_varSpace[getVarIndex(VarName)];
}

double *  Expression::getVarAdress(const QString& VarName)
{
    if (!m_parsed)
        parse();
    int idx=getVarIndex(VarName);
    if (idx>=0 && idx<10)
        return &m_varSpace[idx];
    else
        throw IException(QString("Expression::getVarAdress: Invalid variable <%1> ").arg(VarName));
}

int  Expression::getVarIndex(const QString& variableName)
{
    int idx;
    if (mModelObject) {
        idx = mModelObject->variableIndex(variableName);
        if (idx>-1)
            return 100 + idx;
    }

    /*if (Script)
        {
           int dummy;
           EDatatype aType;
           idx=Script->GetName(VarName, aType, dummy);
           if (idx>-1)
              return 1000+idx;
        }*/

    // externe variablen
    if (!m_externVarNames.isEmpty())
    {
        idx=m_externVarNames.indexOf(variableName);
        if (idx>-1)
            return 1000 + idx;
    }
    idx = m_varList.indexOf(variableName);
    if (idx>-1)
        return idx;
    // if in strict mode, all variables must be already available at this stage.
    if (m_strict)
       throw IException(QString("Variable %1 in (strict) expression %2 not available!").arg(variableName, m_expression));
    return -1;
}

inline double Expression::getModelVar(const int varIdx)
{
    // der weg nach draussen....
    int idx=varIdx - 100; // intern als 100+x gespeichert...
    if (mModelObject)
        return mModelObject->value(idx);
    // hier evtl. verschiedene objekte unterscheiden (Zahlenraum???)
    throw IException("Expression::getModelVar: invalid modell variable!");
}

void Expression::setExternalVarSpace(const QStringList& ExternSpaceNames, double* ExternSpace)
{
    // externe variablen (zB von Scripting-Engine) bekannt machen...
    m_externVarSpace=ExternSpace;
    m_externVarNames=ExternSpaceNames;
}

double Expression::getExternVar(int Index)
{
    //if (Script)
    //   return Script->GetNumVar(Index-1000);
    //else   // überhaupt noch notwendig???
    return m_externVarSpace[Index-1000];
}

void Expression::enableIncSum()
{
    // Funktion "inkrementelle summe" einschalten.
    // dabei wird der zähler zurückgesetzt und ein flag gesetzt.
    m_incSumEnabled=true;
    m_incSumVar=0.;
}

// "Userdefined Function" Polygon
double  Expression::udfPolygon(double Value, double* Stack, int ArgCount)
{
    // Polygon-Funktion: auf dem Stack liegen (x/y) Paare, aus denen ein "Polygon"
    // aus Linien zusammengesetzt ist. return ist der y-Wert zu x (Value).
    // Achtung: *Stack zeigt auf das letzte Argument! (ist das letzte y).
    // Stack bereinigen tut der Aufrufer.
    if (ArgCount%2!=1)
        throw IException("Expression::polygon: falsche zahl parameter. polygon(<val>; x0; y0; x1; y1; ....)");
    int PointCnt = (ArgCount-1) / 2;
    if (PointCnt<2)
        throw IException("Expression::polygon: falsche zahl parameter. polygon(<val>; x0; y0; x1; y1; ....)");
    double x,y, xold, yold;
    y=*Stack--;   // 1. Argument: ganz rechts.
    x=*Stack--;
    if (Value>x)   // rechts draußen: annahme gerade.
        return y;
    for (int i=0; i<PointCnt-1; i++)
    {
        xold=x;
        yold=y;
        y=*Stack--;   // x,y-Paar vom Stack....
        x=*Stack--;
        if (Value>x)
        {
            // es geht los: Gerade zwischen (x,y) und (xold,yold)
            // es geht vielleicht eleganter, aber auf die schnelle:
            return (yold-y)/(xold-x) * (Value-x) + y;
        }

    }
    // falls nichts gefunden: value < als linkester x-wert
    return y;
}

// userdefined func sigmoid....
double Expression::udfSigmoid(double Value, double sType, double p1, double p2)
{
    // sType: typ der Funktion:
    // 0: logistische f
    // 1: Hill-funktion
    // 2: 1 - logistisch (geht von 1 bis 0)
    // 3: 1- hill
    double Result;

    double x=qMax(qMin(Value, 1.), 0.);  // limit auf [0..1]
    int typ=(int) sType;
    switch (typ) {
         case 0: case 2: // logistisch: f(x)=1 / (1 + p1 e^(-p2 * x))
                     Result=1. / (1. + p1 * exp(-p2 * x));
             break;
         case 1: case 3:     // Hill-Funktion: f(x)=(x^p1)/(p2^p1+x^p1)
                     Result=pow(x, p1) / ( pow(p2,p1) + pow(x,p1));
             break;
         default:
             throw IException("sigmoid-funktion: ungültiger kurventyp. erlaubt: 0..3");
         }
    if (typ==2 || typ==3)
        Result=1. - Result;

    return Result;
}


void Expression::checkBuffer(int Index)
{
    // um den Buffer für Befehle kümmern.
    // wenn der Buffer zu klein wird, neuen Platz reservieren.
    if (Index<m_execListSize)
        return; // nix zu tun.
    int NewSize=m_execListSize * 2; // immer verdoppeln: 5->10->20->40->80->160
    // (1) neuen Buffer anlegen....
    ExtExecListItem *NewBuf=new ExtExecListItem[NewSize];
    // (2) bisherige Werte umkopieren....
    for (int i=0;i<m_execListSize;i++)
        NewBuf[i]=m_execList[i];
    // (3) alten buffer löschen und pointer umsetzen...
    delete[] m_execList;
    m_execList = NewBuf;
    m_execListSize=NewSize;
}


double Expression::udfRandom(int type, double p1, double p2)
{
    // random / gleichverteilt - normalverteilt

    if (type == 0)
        return nrandom(p1, p2);
    else    // gaussverteilt
        throw IException("std-deviated random numbers not supported.");
    //return RandG(p1, p2);
}


