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


class LogicExpression
{
public:
        ~LogicExpression();
        LogicExpression() {} // empty constructor
        LogicExpression(const QString &aExpression);
        void setExpression(const QString &aExpression);
        double execute();
        void  parse();
        QString tokString;
        QString expression;
        bool strict; // genau sein mit Variablennamen etc.
        double result;
        bool   logicResult;
        bool   constExpression; // gibt an, ob Expression ein konstanter ausdruck ist...
        void  setVar(const QString& Var, double Value);
        double *addVar(const QString& VarName);
        double  calculate(double Val1=0., double Val2=0.);
        double *  getVarAdress(const QString& VarName);
        //void   SetModellObject(TSimObject *Obj);
        //void   SetModellBaum(TBaum *tree);
        void   setExternalVarSpace(const QStringList& ExternSpaceNames, double* ExternSpace);
        void enableIncSum();
        // scripting...
        //TPScript *Script;
        double udfRandom(int type, double p1, double p2);
private:
        bool parsed;
        ExtExecListItem *ExecList;
        int ExecListSize; // größe des buffers
        int ExecIndex;
        double VarSpace[10];
        QStringList VarList;
        QStringList ExternVarNames;
        double *ExternVarSpace;
        ETokType State;
        ETokType LastState;
        char *Pos;
        char *Expr;
        double FResult;
        QString Token;
        QString PrepStr;
        int   TokCnt;
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
        int  GetFuncIndex(const QString& FuncName);
        int  GetVarIdx(const QString& VarName);
        double GetModellVar(int VarIdx);
        QStringList ModellVarList;
        int         ModellVarCnt;
        bool        ModellVarSet;
        //TSimObject  *FSimObject;
        //TBaum       *TestBaum;
        double GetExternVar(const int Index);
        // inc-sum
        double IncSumVar;
        bool   IncSumEnabled;
        double  udfPolygon(double Value, double* Stack, int ArgCount);
        double udfSigmoid(double Value, double sType, double p1, double p2);
        void CheckBuffer(int Index);

};

#endif // LOGICEXPRESSION_H
