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

#include "test_expression.h"

#include <QDir>
#include <QJSEngine>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <cmath>

#include "deadtree.h"
#include "exception.h"
#include "expression.h"
#include "expressionwrapper.h"
#include "fmstand.h"
#include "fmunit.h"
#include "fomewrapper.h"
#include "globalsettings.h"
#include "saplings.h"
#include "scriptglobal.h"
#include "tree.h"

void TestExpression::arithmeticAndPrecedence_data()
{
    QTest::addColumn<QString>("formula");
    QTest::addColumn<double>("expected");

    QTest::newRow("addition_mul_precedence") << "2 + 3 * 4" << 14.0;
    QTest::newRow("parentheses")             << "(2 + 3) * 4" << 20.0;
    QTest::newRow("power_and_division")      << "2^3 + 10 / 4" << 10.5;
    QTest::newRow("modulo")                  << "mod(17, 5)" << 2.0;
    QTest::newRow("min_variadic")            << "min(9, 4, 7, 2, 8)" << 2.0;
    QTest::newRow("max_variadic")            << "max(1, 12, 5, 3)" << 12.0;
    QTest::newRow("round_positive")          << "round(3.6)" << 4.0;
    QTest::newRow("round_negative")          << "round(-2.6)" << -3.0;
    QTest::newRow("exp_ln_identity")         << "ln(exp(2.5))" << 2.5;
}

void TestExpression::arithmeticAndPrecedence()
{
    QFETCH(QString, formula);
    QFETCH(double, expected);

    Expression expr(formula);
    QVERIFY(std::abs(expr.execute() - expected) < 1e-9);
}

void TestExpression::unaryOperators()
{
    // Leading unary minus and parenthesized negation
    QCOMPARE(Expression("-5 + 3").execute(), -2.0);
    QCOMPARE(Expression("-(2 + 3) * 4").execute(), -20.0);

    // Unary minus after comma delimiter inside functions: if(x<0, -x, x)
    Expression absExpr("if(x < 0, -x, x)");
    QCOMPARE(absExpr.calculate(-7.5), 7.5);
    QCOMPARE(absExpr.calculate(4.25), 4.25);

    // Unary minus AND unary plus inside function arguments: if(x<0, -x, +x)
    Expression absPlusExpr("if(x < 0, -x, +x)");
    QCOMPARE(absPlusExpr.calculate(-3.0), 3.0);
    QCOMPARE(absPlusExpr.calculate(3.0), 3.0);

    // Multiple unary arguments in variadic functions
    Expression minNeg("min(-x, -y)");
    QCOMPARE(minNeg.calculate(2.0, 5.0), -5.0);
}

void TestExpression::comparisonsAndLogicalOps()
{
    // Standard comparisons (=, <>, <, >, <=, >=)
    Expression cmp("x >= 10 and y <> 0");
    QVERIFY(cmp.calculateBool(10.0, 1.0));
    QVERIFY(!cmp.calculateBool(9.9, 1.0));
    QVERIFY(!cmp.calculateBool(12.0, 0.0));

    // Boolean literals combined with comparisons
    Expression boolLit("true and x > 5");
    QVERIFY(boolLit.calculateBool(6.0));
    QVERIFY(!boolLit.calculateBool(4.0));

    Expression boolFalse("false or x = 42");
    QVERIFY(boolFalse.calculateBool(42.0));
    QVERIFY(!boolFalse.calculateBool(41.0));

    // in(...) combined with 'and' / 'or' in both operand orders
    Expression inFirst("in(x, 1, 2, 3) and y > 10");
    QVERIFY(inFirst.calculateBool(2.0, 15.0));
    QVERIFY(!inFirst.calculateBool(4.0, 15.0));
    QVERIFY(!inFirst.calculateBool(2.0, 5.0));

    Expression inSecond("y > 10 and in(x, 1, 2, 3)");
    QVERIFY(inSecond.calculateBool(15.0, 3.0));
    QVERIFY(!inSecond.calculateBool(15.0, 9.0));
    QVERIFY(!inSecond.calculateBool(5.0, 3.0));

    // C-style '==' is not valid iLand syntax ('=' is equality) and must throw a parse error
    QVERIFY_THROWS_EXCEPTION(IException, {
        Expression badEq("x == 5");
        badEq.parse();
    });
}

void TestExpression::builtInFunctions()
{
    // Piecewise linear polygon(val, x0, y0, x1, y1, x2, y2)
    Expression poly("polygon(x, 0, 10, 10, 30, 20, 20)");
    QCOMPARE(poly.calculate(-5.0), 10.0); // clamped below x0
    QCOMPARE(poly.calculate(0.0),  10.0);
    QCOMPARE(poly.calculate(5.0),  20.0); // midpoint [0..10] -> 20
    QCOMPARE(poly.calculate(15.0), 25.0); // midpoint [10..20] -> 25
    QCOMPARE(poly.calculate(30.0), 20.0); // clamped above x2

    // Sigmoid type 0 (logistic) and type 2 (1 - logistic) sum to 1.0
    Expression sig0("sigmoid(x, 0, 10, 5)");
    Expression sig2("sigmoid(x, 2, 10, 5)");
    QVERIFY(std::abs((sig0.calculate(0.4) + sig2.calculate(0.4)) - 1.0) < 1e-12);
}

void TestExpression::variablesAndTreeWrapper()
{
    // Pointer-bound local variable execution
    Expression linComb;
    double *px = linComb.addVar("x");
    double *py = linComb.addVar("y");
    linComb.setExpression("3 * x + 0.5 * y");
    linComb.parse();

    *px = 4.0;
    *py = 10.0;
    QCOMPARE(linComb.execute(), 17.0);

    // Model wrapper binding on a standalone Tree instance
    Tree tree;
    tree.setId(7);
    tree.setDbh(30.0f);
    tree.setHeight(24.0f);

    TreeWrapper tw(&tree);
    Expression hdRatio("height / dbh * 100", &tw);
    QVERIFY(std::abs(hdRatio.execute() - 80.0) < 1e-5);

    Expression filter("dbh >= 25 and height > 20", &tw);
    QVERIFY(filter.executeBool());
}

void TestExpression::linearization1DAnd2D()
{
    Expression::setLinearizationEnabled(true);

    // 1D linearization of a smooth function
    Expression e1("exp(-0.2 * x)");
    e1.linearize(0.0, 10.0, 500);
    QVERIFY(std::abs(e1.calculate(3.7) - std::exp(-0.2 * 3.7)) < 1e-4);
    // Out-of-bounds falls back to exact evaluation
    QVERIFY(std::abs(e1.calculate(15.0) - std::exp(-0.2 * 15.0)) < 1e-12);

    // 2D linearization on a plane f(x, y) = 2*x + 5*y with distinct X and Y step sizes!
    // step_x = (100 - 0)/10 = 10.0, step_y = (10 - 0)/20 = 0.5
    Expression e2("2 * x + 5 * y");
    e2.linearize2d(0.0, 100.0, 0.0, 10.0, 10, 20);
    const double approx = e2.calculate(25.0, 3.25);
    const double exact  = 2.0 * 25.0 + 5.0 * 3.25; // 66.25
    QVERIFY2(std::abs(approx - exact) < 1e-6,
             qPrintable(QString("Expected %1, got %2").arg(exact).arg(approx)));

    Expression::setLinearizationEnabled(false);
}

void TestExpression::errorHandlingOutsideJS()
{
    // Outside a JS execution scope, invalid C++ expressions must throw IException
    QVERIFY_THROWS_EXCEPTION(IException, {
        Expression broken("1 + * 2");
        broken.parse();
    });

    // Unknown variable in strict mode must also throw IException
    QVERIFY_THROWS_EXCEPTION(IException, {
        Expression unknownVar("non_existent_var + 1");
        unknownVar.setStrict(true);
        unknownVar.parse();
    });
}

void TestExpression::constantsExternalVarsAndIncSum()
{
    Expression emptyExpr("");
    emptyExpr.parse();
    QVERIFY(emptyExpr.isEmpty());

    Expression constExpr("42 * 2");
    constExpr.parse();
    QVERIFY(!constExpr.isEmpty());
    QVERIFY(constExpr.isConstExpression());
    QCOMPARE(constExpr.execute(), 84.0);

    Expression nonConst;
    nonConst.setAndParse("x + 1");
    QVERIFY(!nonConst.isConstExpression());

    Expression::addConstant("my_pi_test", 3.14159265);
    Expression constUse("2 * my_pi_test");
    QVERIFY(std::abs(constUse.execute() - 6.28318530) < 1e-8);

    double extVals[2] = {3.5, 4.0};
    Expression extExpr("alpha * beta + 1");
    extExpr.setExternalVarSpace(QStringList() << "alpha" << "beta", extVals);
    extExpr.parse();
    QCOMPARE(extExpr.execute(), 15.0);

    extVals[0] = 10.0;
    QCOMPARE(extExpr.execute(), 41.0);

    Expression incExpr("incsum(x)");
    incExpr.enableIncSum();
    QCOMPARE(incExpr.calculate(5.0), 5.0);
    QCOMPARE(incExpr.calculate(3.0), 8.0);
    QCOMPARE(incExpr.calculate(2.0), 10.0);
}

void TestExpression::wrappersSaplingDeadTreeAndFome()
{
    SaplingTree sap;
    sap.setSapling(1.25f, 6, 2);
    SaplingWrapper sw(&sap, nullptr);
    QCOMPARE(Expression("height * 2 + age", &sw).execute(), 8.5);
    QCOMPARE(Expression("species", &sw).execute(), 2.0);

    DeadTree dt;
    DeadTreeWrapper dtw(&dt, nullptr);
    QCOMPARE(Expression("snag", &dtw).execute(), 1.0);
    QCOMPARE(Expression("dbh", &dtw).execute(), 0.0);
    QCOMPARE(Expression("species", &dtw).execute(), -1.0);
    QCOMPARE(Expression("ruindex", &dtw).execute(), -1.0);

    ABE::FMUnit unit(nullptr);
    unit.setAnnualIncrement(9.5);
    ABE::FMStand stand(&unit, 101);
    stand.setArea(2.5);
    stand.setU(120.0);
    ABE::FOMEWrapper fw(&stand);

    QCOMPARE(Expression("stand__area", &fw).execute(), 2.5);
    QCOMPARE(Expression("site__U", &fw).execute(), 120.0);
    QCOMPARE(Expression("site__annualIncrement", &fw).execute(), 9.5);
}

void TestExpression::scriptErrorRouting()
{
    GlobalSettings::instance()->resetScriptEngine();
    ScriptGlobal::setupGlobalScripting();

    {
        ExprExceptionAsScriptError guard;
        Expression bad("1 + * 2");
        bad.parse();
        QVERIFY(!bad.lastError().isEmpty());
        QVERIFY(GlobalSettings::instance()->scriptEngine()->hasError());
        QJSValue err = GlobalSettings::instance()->scriptEngine()->catchError();
        QVERIFY(err.isError());
    }

    QVERIFY_THROWS_EXCEPTION(IException, {
        Expression badOutside("1 + * 2");
        badOutside.parse();
    });
}
