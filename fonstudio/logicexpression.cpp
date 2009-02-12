#include <QTCore>
#include <stdio.h>


#include <stdexcept>

#include "logicexpression.h"

//---------------------------------------------------------------------------

//#include "pinclude.h"
//#pragma hdrstop
//#include "modell.h"
//#include "ExtParser.h"
//#include "PicusScript.h"
//#include <strutils.hpp>

//---------------------------------------------------------------------------


#define opEqual 1
#define opGreaterThen 2
#define opLowerThen 3
#define opNotEqual 4
#define opLowerOrEqual 5
#define opGreaterOrEqual 6
#define opAnd 7
#define opOr  8
//Global *Globals=Global::Instance();

QString FuncList="sin cos tan exp ln sqrt min max if incsum polygon mod sigmoid rnd rndg";
const int  MaxArgCount[15]={1,1,1,1,  1, 1,   -1, -1, 3, 1, -1, 2, 4, 2, 2};
#define    AGGFUNCCOUNT 6
QString AggFuncList[AGGFUNCCOUNT]={"sum", "avg", "max", "min", "stddev", "variance"};

// static vars für simquery
//bool TSimQuery::DoFreezeClasses=false;
//bool TSimQuery::ClassesFrozen=false;
//#define ModellVarCnt 4
//AnsiString ModellVarList[ModellVarCnt]={"bhd", "height", "age", "art"};
ETokType  LogicExpression::next_token()
{
    TokCnt++;
    LastState=State;
    // nächsten Token aus String lesen...
    // whitespaces eliminieren...
    while (strchr(" \t", *Pos) && *Pos)
        Pos++;

    if (*Pos==0) {
        State=etStop;
        Token="";
        return etStop; // Ende der Vorstellung
    }
    // whitespaces eliminieren...
    while (strchr(" \t", *Pos))
        Pos++;
    if (*Pos==';')
    {

        Token=*Pos++;
        State=etDelimeter;
        return etDelimeter;
    }
    if (strchr("+-*/(){}^", *Pos)) {
        Token=*Pos++;
        State=etOperator;
        return etOperator;
    }
    if (strchr("=<>", *Pos)) {
        Token=*Pos++;
        if (*Pos=='>' || *Pos=='=')
            Token+=*Pos++;
        State=etCompare;
        return etCompare;
    }
    if (*Pos>='0' && *Pos<='9') {
        // Zahl
        Token.setNum(atof(Pos));
        while (strchr("0123456789.",*Pos) && *Pos!=0)
            Pos++;  // nächstes Zeichen suchen...

        State=etNumber;
        return etNumber;
    }

    if (*Pos>='a' && *Pos<='z'|| *Pos>='A' && *Pos<='Z') {
        // function ... find brace
        Token="";
        while ((*Pos>='a' && *Pos<='z' || *Pos>='A' && *Pos<='Z' || *Pos>='0' && *Pos<='9' || *Pos=='_' || *Pos=='.') && *Pos!='(' && Pos!=0)
            Token+=*Pos++;
        // wenn am Ende Klammer, dann Funktion, sonst Variable.
        if (*Pos=='(' || *Pos=='{') {
            Pos++; // skip brace
            State=etFunction;
            return etFunction;
        } else {
            if (Token.toLower()=="and" || Token.toLower()=="or") {
                State=etLogical;
                return etLogical;
            } else {
                State=etVariable;
                return etVariable;
            }
        }
    }
    State=etUnknown;
    return etUnknown; // in case no match was found

}

LogicExpression::~LogicExpression()
{
    if (Expr)
        delete[] Expr;
    delete[] ExecList;
}

void LogicExpression::setExpression(const QString& aExpression)
{
    expression=aExpression;
    //Expr=StrNew(PrepareExpr(expression).c_str());

    QByteArray ba = expression.toLocal8Bit(); // convert from unicode to 8bit
    Expr=new char[ba.length()+1]; // reserve memory...
    strcpy(Expr, ba.constData());

    Pos=Expr; // set pointer to begin of expression
    while (*Pos) {  // selbergestricktes string-replace (dezimalpunkt)
        if (*Pos==',') *Pos='.';
        Pos++;
    }
    Pos=Expr;  // set starting point...
    FResult=0.;

    for (int i=0; i<10; i++)
        VarSpace[i]=0.;
    parsed=false;

    ExternVarSpace=0;
    ModellVarCnt=0;
    ModellVarSet=false;
    strict=true; // default....
    IncSumEnabled=false;
    // Buffer:
    ExecListSize = 5; // inital value...
    ExecList = new ExtExecListItem[ExecListSize]; // init
}

LogicExpression::LogicExpression(const QString& aExpression)
{
    setExpression(aExpression);
}

void  LogicExpression::parse()
{
    try {
        tokString="";
        State=etUnknown;
        LastState=etUnknown;
        constExpression=true;
        ExecIndex=0;
        TokCnt=0;
        int AktTok;
        next_token();
        while (State!=etStop) {
            tokString+="\n"+Token;
            AktTok=TokCnt;
            parse_levelL0();  // start with logical level 0
            if (AktTok==TokCnt)
                throw std::logic_error("Unbalanced Braces.");
            if (State==etUnknown){
                tokString+="\n***Error***";
                throw std::logic_error("Syntax error, token: " + Token.toStdString());
            }
        }
        ExecList[ExecIndex].Type=etStop;
        ExecList[ExecIndex].Value=0;
        ExecList[ExecIndex++].Index=0;
        CheckBuffer(ExecIndex);
        parsed=true;
        result=FResult;
    } catch (const std::logic_error& e) {
        throw std::logic_error("Fehler in: " + std::string(Expr) + "\n"+e.what());
    }
}

void  LogicExpression::parse_levelL0()
{
    // logical operations  (and, or, not)
    QString op;
    parse_levelL1();

    if (State==etLogical)  {
        op=Token.toLower();
        next_token();
        parse_levelL1();
        int logicaltok=0;
        if (op=="and") logicaltok=opAnd;
        if (op=="or") logicaltok=opOr;


        ExecList[ExecIndex].Type=etLogical;
        ExecList[ExecIndex].Value=0;
        ExecList[ExecIndex++].Index=logicaltok;
        CheckBuffer(ExecIndex);
    }
    /*       if (op=='+')
          FResult=temp+FResult;
        if (op=='-')
          FResult=temp-FResult;     */
    //if (Token=="+" || Token=="-") {
    //   parse_level0();
    //}
}
void  LogicExpression::parse_levelL1()
{
    // logische operationen (<,>,=,...)
    QString op;
    parse_level0();
    //double temp=FResult;
    if (State==etCompare)  {
        op=Token;
        next_token();
        parse_level0();
        int logicaltok=0;
        if (op=="<") logicaltok=opLowerThen;
        if (op==">") logicaltok=opGreaterThen;
        if (op=="<>") logicaltok=opNotEqual;
        if (op=="<=") logicaltok=opLowerOrEqual;
        if (op==">=") logicaltok=opGreaterOrEqual;
        if (op=="=")  logicaltok=opEqual;

        ExecList[ExecIndex].Type=etCompare;
        ExecList[ExecIndex].Value=0;
        ExecList[ExecIndex++].Index=logicaltok;
        CheckBuffer(ExecIndex);
    }
    /*       if (op=='+')
          FResult=temp+FResult;
        if (op=='-')
          FResult=temp-FResult;     */
    //if (Token=="+" || Token=="-") {
    //   parse_level0();
    //}
}

void  LogicExpression::parse_level0()
{
    // plus und minus
    QByteArray op;
    parse_level1();

    while (Token=="+" || Token=="-")  {
        op=Token.toAscii();
        next_token();
        parse_level1();
        ExecList[ExecIndex].Type=etOperator;
        ExecList[ExecIndex].Value=0;
        ExecList[ExecIndex++].Index=op.at(0);///op.constData()[0];
        CheckBuffer(ExecIndex);
    }
    /*       if (op=='+')
          FResult=temp+FResult;
        if (op=='-')
          FResult=temp-FResult;     */
    //if (Token=="+" || Token=="-") {
    //   parse_level0();
    //}
}

void  LogicExpression::parse_level1()
{
    // mal und division
    QByteArray op;
    parse_level2();
    //double temp=FResult;
    // alt:        if (Token=="*" || Token=="/") {
    while (Token=="*" || Token=="/") {
        op=Token.toAscii();
        next_token();
        parse_level2();
        ExecList[ExecIndex].Type=etOperator;
        ExecList[ExecIndex].Value=0;
        ExecList[ExecIndex++].Index=op.at(0);
        CheckBuffer(ExecIndex);
    }
    /*       if (op=="*")
           FResult=temp*FResult;
        if (op=="/")
           FResult=temp/FResult; */

}

void  LogicExpression::Atom()
{
    if (State==etVariable || State==etNumber) {
        if (State==etNumber) {
            FResult=Token.toDouble();
            ExecList[ExecIndex].Type=etNumber;
            ExecList[ExecIndex].Value=FResult;
            ExecList[ExecIndex++].Index=-1;
            CheckBuffer(ExecIndex);
        }
        if (State==etVariable) {
            FResult=getVar(Token);
            ExecList[ExecIndex].Type=etVariable;
            ExecList[ExecIndex].Value=0;
            ExecList[ExecIndex++].Index=GetVarIdx(Token);
            CheckBuffer(ExecIndex);
            constExpression=false;
        }
        next_token();
    } else if (State==etStop)
        throw std::logic_error("Unexpected end of expression.");
}


void  LogicExpression::parse_level2()
{
    // x^y
    parse_level3();
    //double temp=FResult;
    while (Token=="^") {
        next_token();
        parse_level3();
        //FResult=pow(temp,FResult);
        ExecList[ExecIndex].Type=etOperator;
        ExecList[ExecIndex].Value=0;
        ExecList[ExecIndex++].Index='^';
        CheckBuffer(ExecIndex);
    }
}
void  LogicExpression::parse_level3()
{
    // unary operator (- bzw. +)
    QString op;
    op=Token;
    bool Unary=false;
    if (op=="-" && (LastState==etOperator || LastState==etUnknown || LastState==etCompare || LastState==etLogical || LastState==etFunction)) {
        next_token();
        Unary=true;
    }
    parse_level4();
    if (Unary && op=="-") {
        //FResult=-FResult;
        ExecList[ExecIndex].Type=etOperator;
        ExecList[ExecIndex].Value=0;
        ExecList[ExecIndex++].Index='_';
        CheckBuffer(ExecIndex);
    }

}

void  LogicExpression::parse_level4()
{
    // Klammer und Funktionen
    QString func;
    Atom();
    //double temp=FResult;
    if (Token=="(" || State==etFunction) {
        func=Token;
        if (func=="(")   // klammerausdruck
        {
            next_token();
            parse_levelL0();
        }
        else        // funktion...
        {
            int argcount=0;
            int idx=GetFuncIndex(func);
            next_token();
            //Token="{";
            // bei funktionen mit mehreren Parametern
            while (Token!=")") {
                argcount++;
                parse_levelL0();
                if (State==etDelimeter)
                    next_token();
            }
            if (MaxArgCount[idx]>0 && MaxArgCount[idx]!=argcount)
                throw std::logic_error( QString("Function %1 assumes %2 arguments!").arg(func).arg(MaxArgCount[idx]).toStdString());
            //throw std::logic_error("Funktion " + func + " erwartet " + std::string(MaxArgCount[idx]) + " Parameter!");
            ExecList[ExecIndex].Type=etFunction;
            ExecList[ExecIndex].Value=argcount;
            ExecList[ExecIndex++].Index=idx;
            CheckBuffer(ExecIndex);
        }
        if (Token!="}" && Token!=")") // Fehler
            throw std::logic_error("Falsche Zahl von Klammern.");
        next_token();
    }
}

double  LogicExpression::getVar(const QString& VarName)
{
    // zuerst schauen, ob in system-liste....
    int idx;
    if (!ModellVarList.isEmpty())
    {
        ModellVarList.indexOf(VarName.toLower());
        //idx=AnsiIndexStr(VarName.toLower(), ModellVarList, ModellVarCnt-1);
        if (idx>-1) {
            tokString+="\nModellvar: " + VarName;
            return 0; // no need to add...
        }
    }
    /*
        if (Script)
        {
            EDatatype aType;
            int ref;
            idx=Script->GetName(VarName, aType, ref);
            if (aType==edtNumber)
              return 0;  // nur numerische
        }*/
    if (!ExternVarNames.isEmpty())
    {
        idx=ExternVarNames.indexOf(VarName);
        if (idx>-1) {
            tokString+="\nExternvar: " + VarName;
            return 0; // no need to add...
        }
    }
    idx=VarList.indexOf(VarName);
    if (idx==-1) {
        if (strict)
            throw std::logic_error("Undefined symbol: " + VarName.toStdString());
        VarList+=VarName;
        idx = VarList.size()-1;
        tokString+="\nVariable: "+VarName;
    }

    return VarSpace[idx];
}

void  LogicExpression::setVar(const QString& Var, double Value)
{
    if (!parsed)
        parse();
    int idx=GetVarIdx(Var);
    if (idx>=0 && idx<10)
        VarSpace[idx]=Value;
    else
        throw std::logic_error("Ungültige Variable " + Var.toStdString());
}

double  LogicExpression::calculate(double Val1, double Val2)
{
    VarSpace[0]=Val1;
    VarSpace[1]=Val2;
    strict=false;
    return execute();
}

int  LogicExpression::GetFuncIndex(const QString& FuncName)
{
    int pos=FuncList.indexOf(FuncName);
    if (pos<0)
        throw std::logic_error("Function " + FuncName.toStdString() + " not defined!");
    int idx=0;
    for (int i=1;i<pos;i++)
        if (FuncList[i]==' ') ++idx;
    return idx;
}

double  LogicExpression::execute()
{
    if (!parsed)
        parse();
    ExtExecListItem *Exec=ExecList;
    int i;
    FResult=0.;
    double Stack[20];
    bool   LogicStack[20];
    bool   *lp=LogicStack;
    double *p=Stack;  // Kopf
    *lp++=true; // zumindest eins am anfang...
    if (Exec->Type==etStop) {
        // leere expr.
        logicResult=false;
        result=0.;
        return 0.;
    }
    //tokString="Start\n";
    while (Exec->Type!=etStop) {
        /* switch (Exec->Type) {
         case etOperator: tokString+="Operator " + AnsiString((char)Exec->Index)+"\n"; break;
         case etNumber: tokString+=AnsiString(Exec->Value)+"\n";  break;
         case etVariable: tokString+="Variable"+AnsiString(Exec->Index)+"\n"; break;
         case etFunction: tokString+="Function"+AnsiString(Exec->Index)+" Args: "+AnsiString(Exec->Value) +"\n"; break;
      }  */
        switch (Exec->Type) {
        case etOperator:
            p--;
            switch (Exec->Index) {
                  case '+': *(p-1)=*(p-1) + *p;  break;
                  case '-': *(p-1)=*(p-1)-*p;  break;
                  case '*': *(p-1)=*(p-1) * *p;  break;
                  case '/': *(p-1)=*(p-1) / *p;  break;
                  case '^': *(p-1)=pow(*(p-1), *p);  break;
                  case '_': *p=-*p; p++; break;  // unary operator -
                  }
            break;
        case etVariable:
            if (Exec->Index<100)
                *p++=VarSpace[Exec->Index];
            else if (Exec->Index<1000)
                *p++=GetModellVar(Exec->Index);
            else
                *p++=GetExternVar(Exec->Index);
            break;
        case etNumber:
            *p++=Exec->Value;
            break;
        case etFunction:
            p--;
            switch (Exec->Index) {
                 case 0: *p=sin(*p); break;
                 case 1: *p=cos(*p); break;
                 case 2: *p=tan(*p); break;
                 case 3: *p=exp(*p); break;
                 case 4: *p=log(*p); break;
                 case 5: *p=sqrt(*p); break;
                     // min, max, if:  variable zahl von argumenten
                 case 6:      // min
                     for (i=0;i<Exec->Value-1;i++,p--)
                         *(p-1)=(*p<*(p-1))?*p:*(p-1);
                     break;
                 case 7:  //max
                     for (i=0;i<Exec->Value-1;i++,p--)
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
                     IncSumVar+=*p;
                     *p=IncSumVar;
                     break;
                 case 10: // Polygon-Funktion
                     *(p-(int)(Exec->Value-1))=udfPolygon(*(p-(int)(Exec->Value-1)), p, (int)Exec->Value);
                     p-=(int) (Exec->Value-1);
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
                     *p=udfRandom(Exec->Index-13, *p, *(p+1));
                     break;
                 }
            p++;
            break;
        case etLogical:
            p--;
            lp--;
            switch (Exec->Index) {
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
            switch (Exec->Index) {
                 case opEqual: LogicResult=(*(p-1)==*p); break;
                 case opNotEqual: LogicResult=(*(p-1)!=*p); break;
                 case opLowerThen: LogicResult=(*(p-1)<*p); break;
                 case opGreaterThen: LogicResult=(*(p-1)>*p); break;
                 case opGreaterOrEqual: LogicResult=(*(p-1)>=*p); break;
                 case opLowerOrEqual: LogicResult=(*(p-1)<=*p); break;
                 }
            if (LogicResult) {
                *(p-1)=1;   // 1 means true...
                //tokString+="TRUE\n";
            } else {
                //tokString+="FALSE\n";
                *(p-1)=0;
            }

            *lp++=LogicResult;
            break; }
        case etStop: case etUnknown: case etDelimeter: throw std::logic_error("invalid token during execution.");
        } // switch()

        Exec++;
        //tokString+="Pos: " + AnsiString(p-Stack) + "; Value: " + AnsiString(*(p-1))+"\n";
    }
    if (p-Stack!=1)
        throw std::logic_error("...unbalanced");
    result=*(p-1);
    logicResult=*(lp-1);
    return result;
}

double * LogicExpression::addVar(const QString& VarName)
{
    // add var
    int idx=VarList.indexOf(VarName);
    if (idx==-1) {
        VarList+=VarName;
        idx=VarList.size()-1;
    }
    return &VarSpace[GetVarIdx(VarName)];
}

double *  LogicExpression::getVarAdress(const QString& VarName)
{
    if (!parsed)
        parse();
    int idx=GetVarIdx(VarName);
    if (idx>=0 && idx<10)
        return &VarSpace[idx];
    else
        throw std::logic_error("Ungültige Variable " + VarName.toStdString());
}

int  LogicExpression::GetVarIdx(const QString& VarName)
{
    int idx;

    /*if (Script)
        {
           int dummy;
           EDatatype aType;
           idx=Script->GetName(VarName, aType, dummy);
           if (idx>-1)
              return 1000+idx;
        }*/
    if (!ModellVarList.isEmpty()) {
        idx=ModellVarList.indexOf(VarName.toLower());
        if (idx>-1) {
            return 100 + idx; //
        }
    }
    // externe variablen
    if (!ExternVarNames.isEmpty())
    {
        ExternVarNames.indexOf(VarName);
        if (idx>-1)
            return 1000 + idx;
    }
    return VarList.indexOf(VarName);
}

double LogicExpression::GetModellVar(int VarIdx)
{
    // der weg nach draussen....
    //int idx=VarIdx - 100; // intern als 100+x gespeichert...
    // hier evtl. verschiedene objekte unterscheiden (Zahlenraum???)
    throw std::logic_error("invalid modell var!");
    //return TestBaum->getVar(idx);
    //return FSimObject->getVar(idx);
}

/*void   LogicExpression::SetModellObject(TSimObject *Obj)
{
   if (!ModellVarSet) {
     ModellVarList=Obj->GetVarList(ModellVarCnt);
     ModellVarSet=true;
   }
   FSimObject=Obj;
}
void   LogicExpression::SetModellBaum(TBaum *tree)
{
  if (!ModellVarSet) {
     ModellVarList=tree->GetVarList(ModellVarCnt);
     ModellVarSet=true;
  }
  TestBaum=tree;
}*/


void LogicExpression::setExternalVarSpace(const QStringList& ExternSpaceNames, double* ExternSpace)
{
    // externe variablen (zB von Scripting-Engine) bekannt machen...
    ExternVarSpace=ExternSpace;
    ExternVarNames=ExternSpaceNames;
}

double LogicExpression::GetExternVar(int Index)
{
    //if (Script)
    //   return Script->GetNumVar(Index-1000);
    //else   // überhaupt noch notwendig???
    return ExternVarSpace[Index-1000];
}

void LogicExpression::enableIncSum()
{
    // Funktion "inkrementelle summe" einschalten.
    // dabei wird der zähler zurückgesetzt und ein flag gesetzt.
    IncSumEnabled=true;
    IncSumVar=0.;
}

// "Userdefined Function" Polygon
double  LogicExpression::udfPolygon(double Value, double* Stack, int ArgCount)
{
    // Polygon-Funktion: auf dem Stack liegen (x/y) Paare, aus denen ein "Polygon"
    // aus Linien zusammengesetzt ist. return ist der y-Wert zu x (Value).
    // Achtung: *Stack zeigt auf das letzte Argument! (ist das letzte y).
    // Stack bereinigen tut der Aufrufer.
    if (ArgCount%2!=1)
        throw std::logic_error("polygon: falsche zahl parameter. polygon(<val>; x0; y0; x1; y1; ....)");
    int PointCnt = (ArgCount-1) / 2;
    if (PointCnt<2)
        throw std::logic_error("polygon: falsche zahl parameter. polygon(<val>; x0; y0; x1; y1; ....)");
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
double LogicExpression::udfSigmoid(double Value, double sType, double p1, double p2)
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
             throw std::logic_error("sigmoid-funktion: ungültiger kurventyp. erlaubt: 0..3");
         }
    if (typ==2 || typ==3)
        Result=1. - Result;

    return Result;
}


void LogicExpression::CheckBuffer(int Index)
{
    // um den Buffer für Befehle kümmern.
    // wenn der Buffer zu klein wird, neuen Platz reservieren.
    if (Index<ExecListSize)
        return; // nix zu tun.
    int NewSize=ExecListSize * 2; // immer verdoppeln: 5->10->20->40->80->160
    // (1) neuen Buffer anlegen....
    ExtExecListItem *NewBuf=new ExtExecListItem[NewSize];
    // (2) bisherige Werte umkopieren....
    for (int i=0;i<ExecListSize;i++)
        NewBuf[i]=ExecList[i];
    // (3) alten buffer löschen und pointer umsetzen...
    delete[] ExecList;
    ExecList = NewBuf;
    ExecListSize=NewSize;
}

double nrandom(const double& p1, const double& p2)
{
    return p1 + (p2-p1)*(rand()/double(RAND_MAX));

}
double LogicExpression::udfRandom(int type, double p1, double p2)
{
    // random / gleichverteilt - normalverteilt

    if (type == 0)
        return nrandom(p1, p2);
    else    // gaussverteilt
        throw std::logic_error("std-deviated random numbers not supported.");
    //return RandG(p1, p2);
}
