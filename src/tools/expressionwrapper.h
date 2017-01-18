/********************************************************************************************
**    iLand - an individual based forest landscape and disturbance model
**    http://iland.boku.ac.at
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

#ifndef EXPRESSIONWRAPPER_H
#define EXPRESSIONWRAPPER_H
#include <QtCore/QString>
/** ExpressionWrapper is the base class for exposing C++ elements
 *  to the built-in Expression engine. See TreeWrapper for an example.
*/
class ExpressionWrapper
{
public:
    ExpressionWrapper();
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);
    virtual double valueByName(const QString &variableName);
    virtual int variableIndex(const QString &variableName);
};

/** TreeWrapper wraps an individual tree in iLand.
 *
 **/
class Tree;
class TreeWrapper: public ExpressionWrapper
{
public:
    TreeWrapper() : mTree(0) {}
    TreeWrapper(const Tree* tree) : mTree(tree) {}
    void setTree(const Tree* tree) { mTree = tree; }
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    const Tree *mTree;
};

/** RUWrapper encapsualates an iLand resource unit (1 ha pixel)
*/
class ResourceUnit;
class RUWrapper: public ExpressionWrapper
{
public:
    RUWrapper() : mRU(0) {}
    RUWrapper(const ResourceUnit* resourceUnit) : mRU(resourceUnit) {}
    void setResourceUnit(const ResourceUnit* resourceUnit) { mRU = resourceUnit; }
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    const ResourceUnit *mRU;
};

/** SaplingWrapper encapsualates a sapling cohort (on a 2x2m pixel)
  */
struct SaplingTree;
class SaplingWrapper: public ExpressionWrapper
{
public:
    SaplingWrapper() : mSapling(0) {}
    SaplingWrapper(const SaplingTree* saplingTree) : mSapling(saplingTree) {}
    void setSaplingTree(const SaplingTree* saplingTree) { mSapling = saplingTree; }
    virtual const QStringList getVariablesList();
    virtual double value(const int variableIndex);

private:
    const SaplingTree *mSapling;
};

#endif // EXPRESSIONWRAPPER_H
