/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    https://iland-model.org
**    Copyright (C) 2009-  Werner Rammer, Rupert Seidl
**
**    This program is free software: you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation, either version 3 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************************************/

#ifndef LOGICEXPRESSION_H
#define LOGICEXPRESSION_H
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMutexLocker>
#include <QtCore/QVector>
#define EXPRNLOCALVARS 10
class ExpressionWrapper;
class Expression
{
public:
    ~Expression();
    Expression();
    Expression(const QString &aExpression) { m_expr=nullptr; m_execList=nullptr; setExpression(aExpression); }
    Expression(const QString &expression, ExpressionWrapper *wrapper) { m_expr=nullptr; m_execList=nullptr; setExpression(expression); mModelObject = wrapper;  }
    // intialization
    void setExpression(const QString &aExpression); ///< set expression
    void setAndParse(const QString &expr); ///< set expression and parse instantly
    void setModelObject(ExpressionWrapper *wrapper) { mModelObject = wrapper; }
    const QString &expression() const { return m_expression; }
    void  parse(ExpressionWrapper *wrapper=nullptr); ///< force a parsing of the expression

    /// call linearize() to 'linarize' an expression, i.e. approximate the function by linear interpolation.
    void linearize(const double low_value, const double high_value, const int steps=1000);
    /// lineraize2d works with two variables
    void linearize2d(const double low_x, const double high_x, const double low_y, const double high_y, const int stepsx=50, const int stepsy=50);

    /// global switch for linerization. If set to false, subsequent calls to linearize are ignored.
    static void setLinearizationEnabled(const bool enable) {mLinearizationAllowed = enable; }
    // calculations
    /// calculate formula and return result. variable values need to be set using "setVar()"
    double execute(double *varlist=nullptr, ExpressionWrapper *object=nullptr) const;
    /// execute and return as boolean
    bool executeBool(double *varlist=nullptr, ExpressionWrapper *object=nullptr) const { return execute(varlist, object) != 0.; }

    double executeLocked() { QMutexLocker m(&m_execMutex); return execute();  } ///< thread safe version

    // execute the non-optimized version
    double execute_unopt(double *varlist=nullptr, ExpressionWrapper *object=nullptr) const;

    /** calculate formula. the first two variables are assigned the values Val1 and Val2. This function is for convenience.
           the return is the result of the calculation.
           e.g.: x+3*y --> Val1->x, Val2->y
           forceExecution: do not apply linearization */
    inline double calculate(const double Val1=0., const double Val2=0., const bool forceExecution=false) const
    {
        if (mLinearizeMode>0 && !forceExecution) {
            if (mLinearizeMode==1)
                return linearizedValue(Val1);
            return linearizedValue2d(Val1, Val2); // matrix case
        }
        double var_space[2]; // use only two local variables - good for cache
        var_space[0]=Val1;
        var_space[1]=Val2;
        if (m_strict) const_cast<Expression*>(this)->m_strict = false;
        return execute(var_space); // execute with local variables on stack
    }

    /// evaluate expression with local variables 1 and 2 set to to Val1 and Val2.
    /// Set forceExecution=true to turn off linearization
    bool calculateBool(const double Val1=0., const double Val2=0., const bool forceExecution=false) const
    { return calculate(Val1, Val2, forceExecution) != 0.; }

    /// calculate formula with object (based on ExpressionWrapper)
    /// provide two extra local variables
    /// returns a double (result of expression)
    double calculate(ExpressionWrapper &object, const double variable_value1=0., const double variable_value2=0.) const
    {
        double var_space[2]; // use only two local variables (good for cache)
        var_space[0] = variable_value1;
        var_space[1]=variable_value2;
        if (m_strict)
            const_cast<Expression*>(this)->m_strict=false;

        return execute(var_space,&object); // execute with local variables on stack
    }

    /// calculate formula with object (based on ExpressionWrapper)
    /// provide two extra local variables
    /// returns a bool (result of expression)
    double calculateBool(ExpressionWrapper &object, const double variable_value1=0., const double variable_value2=0.) const { return calculate(object,variable_value1, variable_value2)!=0.; }

    //variables
    /// set the value of the variable named "Var". Note: using addVar to obtain a pointer may be more efficient for multiple executions.
    void  setVar(const QString& Var, double Value);
    /// adds variable "VarName" and returns a double pointer to the variable. Use *ptr to set the value (before calling execute())
    double *addVar(const QString& VarName);
    /// retrieve again the value pointer of a variable.
    double *  getVarAdress(const QString& VarName);
    const QStringList &variables() {return m_varList; }


    bool isConstExpression() const { return m_constExpression; } ///< returns true if current expression is a constant.
    bool isEmpty() const { return m_empty; } ///< returns true if expression is empty
    const QString &lastError() const { return m_errorMsg; }
    /** strict property: if true, variables must be named before execution.
          When strict=true, all variables in the expression must be added by setVar or addVar.
          if false, variable values are assigned depending on occurence. strict is false by default for calls to "calculate()".
        */
    bool isStrict() { return m_strict;}
    void setStrict(bool str) { m_strict=str; }
    void setCatchExceptions(bool docatch=true) { m_catchExceptions = docatch; }
    void   setExternalVarSpace(const QStringList& ExternSpaceNames, double* ExternSpace);
    void enableIncSum();
    // other maintenance
    static void addConstant(const QString const_name, const double const_value);
private:
    enum ETokType {etNumber, etOperator, etVariable, etFunction, etLogical, etCompare, etStop, etUnknown, etDelimeter};

    enum OpCode : uint8_t {
        OP_STOP = 0,
        OP_PUSH_IMM,    // Push Immediate Value
        OP_LOAD_LOCAL,  // Load from varlist (Stack variables 0..99)
        OP_LOAD_MODEL_VAR,  // Calls getModelVar(index, object)
        OP_LOAD_EXTERN_VAR, // Calls getExternVar(index)


        // Arithmetic
        OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_POW, OP_NEG,

        // Functions
        OP_SIN, OP_COS, OP_TAN, OP_EXP, OP_LOG, OP_SQRT,
        OP_MIN, OP_MAX, OP_AVG, // Variadic

        // Logic (Using Double as Bool: 1.0=True, 0.0=False)
        OP_AND, OP_OR,
        OP_GT, OP_LT, OP_GE, OP_LE, OP_EQ, OP_NE,
        OP_IF,
        // custom functions
        OP_INCSUM, OP_POLYGON, OP_MODULO, OP_SIGMOID, OP_RND, OP_RNDG, OP_IN, OP_ROUND,
    };

    struct Instruction {
        OpCode code;
        // Padding to ensure union alignment (usually 8 bytes total for the top half)
        uint8_t _pad[7];

        union {
            double  val;        // For literals
            double* ptr;        // For global variables (resolved address)
            int     index;      // For local variables (index in varlist)
            int     count;      // For variadic functions (min/max)
        } data;
    };
    // optimized data structure for fast execution (cache aware, avoid pointer ops)
    std::vector<Instruction> m_program;
    void compile(); // The translator from the old execution list to the fast program

    struct ExtExecListItem {
        ETokType Type;
        double  Value;
        int     Index;
    };
    enum EDatatype {edtInfo, edtNumber, edtString, edtObject, edtVoid, edtObjVar, edtReference, edtObjectReference};
    bool m_catchExceptions;
    QString m_errorMsg;

    bool m_parsed;
    mutable bool m_strict;
    bool m_empty; // empty expression
    bool m_constExpression;
    QString m_tokString;
    QString m_expression;
    Expression::ExtExecListItem *m_execList;
    int m_execListSize; // size of buffer
    int m_execIndex;
    double m_varSpace[EXPRNLOCALVARS];
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
    inline double getModelVar(const int varIdx, ExpressionWrapper *object=nullptr) const ;

    // link to external model variable
    ExpressionWrapper *mModelObject;

    double getExternVar(const int Index) const;
    double *getExternVarPtr(const int Index) const;
    // inc-sum
    mutable double m_incSumVar;
    bool   m_incSumEnabled;
    double  udfPolygon(double Value, double* Stack, int ArgCount) const; ///< special function polygon()
    double  udfInList(double value, double* stack, int argCount) const; ///< special function in()
    double udfSigmoid(double Value, double sType, double p1, double p2) const; ///< special function sigmoid()
    double udfRandom(int type, double p1, double p2) const; ///< user defined function rnd() (normal distribution does not work now!)

    void checkBuffer(int Index);
    QMutex m_execMutex;
    // Used very fast linearized version of the function,
    // 1D case
    inline double linearizedValue(const double x) const
    {
        if (x<mLinearLow || x>mLinearHigh)
            return calculate(x,0.,true); // standard calculation without linear optimization- but force calculation to avoid infinite loop

        double pos = (x - mLinearLow) * mLinearInvStep;

        int lower = (int)pos;
        double delta = pos - lower; // fractional part (0.0 to 1.0)

        Q_ASSERT(lower+1<mLinearized.count());

        // 4. Access data
        const double* entry = &mLinearized[lower];

        // 5. Lerp (Linear Interpolation)
        // Formula: y0 + (y1 - y0) * delta
        double result = *entry + (*(entry + 1) - *entry) * delta;
        return result;
    }
    // use very fast linearized version of the function
    // 2D case (matrix)
    inline double linearizedValue2d(const double x, const double y) const
    {
        // 1. Bounds Check
        if (x < mLinearLow || x > mLinearHigh || y < mLinearLowY || y > mLinearHighY)
            return calculate(x, y, true);

        // 2. Calculate normalized position (remove expensive divisions)
        // "pos" is the index with a fractional part (e.g., 5.42)
        double posX = (x - mLinearLow) * mLinearInvStep;
        double posY = (y - mLinearLowY) * mLinearInvStepY;

        // 3. Split into Integer Index and Fractional Weight
        // Fast cast to int (truncation)
        int xi = (int)posX;
        int yi = (int)posY;

        // The weights (0.0 to 1.0)
        double tx = posX - xi;
        double ty = posY - yi;

        // 4. Calculate Memory Index
        // Your layout has Y varying fast, so we jump 'mLinearStepCountY' to get to next X
        int idx = xi * mLinearStepCountY + yi;

        // 5. Fetch the 4 corners
        // Pointers are faster than repeated vector[] lookups
        const double* pBase = &mLinearized[idx];

        // Layout:
        // [idx]   = x0, y0
        // [idx+1] = x0, y1 (Next Y)
        // [idx+S] = x1, y0 (Next X)
        double v00 = *pBase;
        double v01 = *(pBase + 1);
        double v10 = *(pBase + mLinearStepCountY);
        double v11 = *(pBase + mLinearStepCountY + 1);

        // 6. Bilinear Interpolation (Lerp)
        // Interpolate along Y axis first
        double col0 = v00 + ty * (v01 - v00);
        double col1 = v10 + ty * (v11 - v10);

        // Interpolate along X axis
        return col0 + tx * (col1 - col0);
    }
    int mLinearizeMode;
    QVector<double> mLinearized;
    double mLinearLow, mLinearHigh;
    double mLinearStep, mLinearInvStep;
    double mLinearLowY, mLinearHighY;
    double mLinearStepY, mLinearInvStepY;
    int mLinearStepCountY;
    static bool mLinearizationAllowed;
    static bool mThrowExceptionsInJS;
    friend class ExprExceptionAsScriptError;
};

/// use this class to force exceptions from expression to be routed to the global
/// JS environment (this avoids crashes as no exceptions can happen during JS execution)
class ExprExceptionAsScriptError {
public:
    ExprExceptionAsScriptError() { Expression::mThrowExceptionsInJS = true; }
    ~ExprExceptionAsScriptError() { Expression::mThrowExceptionsInJS = false; }
};

#endif // LOGICEXPRESSION_H
