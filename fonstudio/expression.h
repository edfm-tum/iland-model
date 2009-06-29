#ifndef LOGICEXPRESSION_H
#define LOGICEXPRESSION_H

#include <QTCore>

enum ETokType {etNumber, etOperator, etVariable, etFunction, etLogical, etCompare, etStop, etUnknown, etDelimeter};
enum EValueClasses {evcBHD, evcHoehe, evcAlter};
struct ExtExecListItem {
    ETokType Type;
    double  Value;
    int     Index;
};
enum EDatatype {edtInfo, edtNumber, edtString, edtObject, edtVoid, edtObjVar, edtReference, edtObjectReference};


/** Expression engine for mathematical expressions.
  The main purpose is fast execution speed.
  notes regarding the syntax:
  +,-,*,/ as expected, additionally "^" for power.
  mod(x,y): modulo division, gets remainder of x/y
  functions:
    - sin cos tan
    - exp ln sqrt
    - min max: variable number of arguments, e.g: min(x,y,z)
    - if: if(condition, true, false): if condition=true, return true-case, else false-case. note: both (true, false) are evaluated anyway.
    - incsum: ?? incremental sum - currently not supported.
    - polygon: special function for polygons. polygon(value, x1,y1, x2,y2, x3,y3, ..., xn,yn): return is: y1 if value<x1, yn if value>xn, or the lineraly interpolated numeric y-value.
    - sigmoid: returns a sigmoid function. sigmoid(value, type, param1, param2). see udfSigmoid() for details.
    - rnd rndg: random functions; rnd(from, to): uniform random number, rndg(mean, stddev): gaussian randomnumber (Note: gaussian currently not supported)
    The Expression class also supports some logical operations:
    (logical) True equals to "1", "False" to zero. The precedence rules for parentheses...
    - and
    - or
    - not

*/
class Expression
{
public:
        ~Expression();
        Expression() {} // empty constructor
        Expression(const QString &aExpression);
        void setExpression(const QString &aExpression);
        /// calculate formula and return result. variable values need to be set using "setVar()"
        double execute();
        /** calculate formula. the first two variables are assigned the values Val1 and Val2. This function is for convenience.
           the return is the result of the calculation.
           e.g.: x+3*y --> Val1->x, Val2->y*/
        double calculate(double Val1=0., double Val2=0.);

        /// set the value of the variable named "Var". Note: using addVar to obtain a pointer may be more efficient for multiple executions.
        void  setVar(const QString& Var, double Value);
        /// adds variable "VarName" and returns a double pointer to the variable. Use *ptr to set the value (before calling execute())
        double *addVar(const QString& VarName);
        /// retrieve again the value pointer of a variable.
        double *  getVarAdress(const QString& VarName);

        void  parse(); ///< force a parsing of the expression

        const double result(){ return m_result; } ///< get the result of the last calcuation.
        const bool logicResult() { return m_logicResult; } ///< get the logical result (true/false) of the last calculation.
        const bool isConstExpression() { return m_constExpression; } /// returns true if current expression is a constant.
        /** strict property: if true, variables must be named before execution.
          When strict=true, all variables in the expression must be added by setVar or addVar.
          if false, variable values are assigned depending on occurence. strict is false is the default for "calculate()".
        */
        const bool isStrict() { return m_strict;}
        void setStrict(bool str) { m_strict=str; }

        void   setExternalVarSpace(const QStringList& ExternSpaceNames, double* ExternSpace);
        void enableIncSum();
        // scripting...
        //TPScript *Script;
        double udfRandom(int type, double p1, double p2);
private:
        bool m_parsed;
        double m_result;
        double m_logicResult;
        double m_strict;
        bool m_constExpression;
        QString m_tokString;
        QString m_expression;
        ExtExecListItem *m_execList;
        int m_execListSize; // größe des buffers
        int m_execIndex;
        double m_varSpace[10];
        QStringList m_varList;
        QStringList m_externVarNames;
        double *m_externVarSpace;
        ETokType m_state;
        ETokType m_lastState;
        char *m_pos;
        char *m_expr;
        QString m_token;
        QString m_prepStr;
        int   m_tokCount;
        ETokType  next_token();
        void  Atom();
        void  parse_levelL0();
        void  parse_levelL1();
        void  parse_level0();
        void  parse_level1();
        void  parse_level2();
        void  parse_level3();
        void  parse_level4();
        double  getVar(const QString& VarName);
        int  getFuncIndex(const QString& FuncName);
        int  getVarIndex(const QString& VarName);
        double getModelVar(int VarIdx);
        QStringList m_modelVarList;
        int         m_modelVarCnt;
        bool        m_modelVarSet;
        //TSimObject  *FSimObject;
        //TBaum       *TestBaum;
        double getExternVar(const int Index);
        // inc-sum
        double m_incSumVar;
        bool   m_incSumEnabled;
        double  udfPolygon(double Value, double* Stack, int ArgCount);
        double udfSigmoid(double Value, double sType, double p1, double p2);
        void checkBuffer(int Index);

};

#endif // LOGICEXPRESSION_H
