#ifndef LOGICEXPRESSION_H
#define LOGICEXPRESSION_H

class ExpressionWrapper;
class Expression
{
public:
        ~Expression();
        Expression();
        Expression(const QString &aExpression) { setExpression(aExpression); }
        Expression(const QString &expression, ExpressionWrapper *wrapper) { setExpression(expression); mModelObject = wrapper;  }
        void setExpression(const QString &aExpression); ///< set expression
        void setAndParse(const QString &expr); ///< set expression and parse instantly
        void setModelObject(ExpressionWrapper *wrapper) { mModelObject = wrapper; }
        const QString &expression() const { return m_expression; }
        /// calculate formula and return result. variable values need to be set using "setVar()"
        double execute();
        double executeLocked() { QMutexLocker m(&m_execMutex); return execute();  } ///< thread safe version
        /** calculate formula. the first two variables are assigned the values Val1 and Val2. This function is for convenience.
           the return is the result of the calculation.
           e.g.: x+3*y --> Val1->x, Val2->y*/
        double calculate(double Val1=0., double Val2=0.);
        double calculateLocked(double Val1=0., double Val2=0.) { QMutexLocker m(&m_execMutex); return calculate(Val1, Val2); } ///< threadsafe version

        /// set the value of the variable named "Var". Note: using addVar to obtain a pointer may be more efficient for multiple executions.
        void  setVar(const QString& Var, double Value);
        /// adds variable "VarName" and returns a double pointer to the variable. Use *ptr to set the value (before calling execute())
        double *addVar(const QString& VarName);
        /// retrieve again the value pointer of a variable.
        double *  getVarAdress(const QString& VarName);

        void  parse(); ///< force a parsing of the expression

        bool isConstExpression() const { return m_constExpression; } ///< returns true if current expression is a constant.
        bool isEmpty() const { return m_empty; } ///< returns true if expression is empty
        /** strict property: if true, variables must be named before execution.
          When strict=true, all variables in the expression must be added by setVar or addVar.
          if false, variable values are assigned depending on occurence. strict is false is the default for "calculate()".
        */
        bool isStrict() { return m_strict;}
        void setStrict(bool str) { m_strict=str; }
        void setCatchExceptions(bool docatch=true) { m_catchExceptions = docatch; }
        void   setExternalVarSpace(const QStringList& ExternSpaceNames, double* ExternSpace);
        void enableIncSum();
        double udfRandom(int type, double p1, double p2); ///< user defined function rnd() (normal distribution does not work now!)
private:
        enum ETokType {etNumber, etOperator, etVariable, etFunction, etLogical, etCompare, etStop, etUnknown, etDelimeter};
        enum EValueClasses {evcBHD, evcHoehe, evcAlter};
        struct ExtExecListItem {
            ETokType Type;
            double  Value;
            int     Index;
        };
        enum EDatatype {edtInfo, edtNumber, edtString, edtObject, edtVoid, edtObjVar, edtReference, edtObjectReference};
        double m_result;
        bool m_catchExceptions;

        bool m_parsed;
        double m_strict;
        bool m_empty; // empty expression
        bool m_constExpression;
        QString m_tokString;
        QString m_expression;
        Expression::ExtExecListItem *m_execList;
        int m_execListSize; // size of buffer
        int m_execIndex;
        double m_varSpace[10];
        QStringList m_varList;
        QStringList m_externVarNames;
        double *m_externVarSpace;
        Expression::ETokType m_state;
        Expression::ETokType m_lastState;
        char *m_pos;
        char *m_expr;
        QString m_token;
        QString m_prepStr;
        int   m_tokCount;
        Expression::ETokType  next_token();
        void  atom();
        void  parse_levelL0();
        void  parse_levelL1();
        void  parse_level0();
        void  parse_level1();
        void  parse_level2();
        void  parse_level3();
        void  parse_level4();
        int  getFuncIndex(const QString& functionName);
        int  getVarIndex(const QString& variableName);
        inline double getModelVar(const int varIdx);

        // link to external model variable
        ExpressionWrapper *mModelObject;

        double getExternVar(const int Index);
        // inc-sum
        double m_incSumVar;
        bool   m_incSumEnabled;
        double  udfPolygon(double Value, double* Stack, int ArgCount); ///< special function polygon()
        double udfSigmoid(double Value, double sType, double p1, double p2); ///< special function sigmoid()
        void checkBuffer(int Index);
        QMutex m_execMutex;
};

#endif // LOGICEXPRESSION_H
