#ifndef FMTREELIST_H
#define FMTREELIST_H

#include <QObject>

namespace AMIE {

class Tree; // forward

class FMTreeList : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int stand READ standId) ///< return stand, -1 if not set
public:
    explicit FMTreeList(QObject *parent = 0);
    int standId() const { return mStandId; }
    void setStandId(int stand) {mStandId = stand; }

signals:

public slots:
    // loading of trees
    /// load all trees of the stand, return number of trees
    int loadAll() { return load(QString()); }
    int load(QString filter); ///< load all trees passing the filter in a list, return number of trees
    void loadFromMap(const MapGrid *map_grid, int key); ///< load all trees that are on the area denoted by 'key' of the given grid
    void loadFromMap(MapGridWrapper *wrap, int key); ///< load all trees that are on the area denoted by 'key' of the given grid (script access)


    /// remove randomly trees until only 'number' of trees remain.
    /// return number of removed trees
    int remain(int number);
    /** kill "number" of stems
     *  in the percentile interval "from" - "to".
     *  remove all if "number" is higher than the count.
     *  return the number of removed trees. */
    int killPct(int pctfrom, int pctto, int number);
    int killAll(); ///< kill all trees in the list
    /** kill 'fraction' of all trees with 'filter'=true */
    int kill(QString filter, double fraction);
    // management
    /** kill "number" of stems
     *  in the percentile interval "from" - "to".
     *  remove all if "number" is higher than the count.
     * Use the removal fractions set by the removeStem, removeBranch and removeFoliage properties.
     *  return the number of removed trees. */
    int managePct(int pctfrom, int pctto, int number);
    int manageAll(); ///< manage all trees in the list
    /** manage 'fraction' of all trees with 'filter'=true */
    int manage(QString filter, double fraction);

    double percentile(int pct); ///< get value for the pct th percentile (1..100)
    void killSaplings(MapGridWrapper *wrap, int key); ///< kill all saplings that are on the area denoted by 'key' of the given grid (script access)

    /** hacky access function to resource units covered by a polygon.
     the parameters are "remove-fractions": i.e. value=0: no change, value=1: set to zero. */
    void removeSoilCarbon(MapGridWrapper *wrap, int key, double SWDfrac, double DWDfrac, double litterFrac, double soilFrac);
    /** slash snags (SWD and otherWood-Pools) of polygon 'key' on the map 'wrap'.
      @param slash_fraction 0: no change, 1: 100%
       */
    void slashSnags(MapGridWrapper *wrap, int key, double slash_fraction);
    void sort(QString statement); ///< sort trees in the list according to a criterion
    int filter(QString filter); ///< apply a filter on the list of trees (expression), return number of remaining trees.
    int filterIdList(QVariantList idList); ///< apply filter in form of a list of ids, return number of remaining trees
    void randomize(); ///< random shuffle of all trees in the list

    /// calculate the mean value for all trees in the internal list for 'expression' (filtered by the filter criterion)
    double mean(QString expression, QString filter=QString()) { return aggregate_function( expression, filter, "mean"); }
    /// calculate the sum for all trees in the internal list for the 'expression' (filtered by the filter criterion)
    double sum(QString expression, QString filter=QString()) { return aggregate_function( expression, filter, "sum"); }

private:
    int remove_percentiles(int pctfrom, int pctto, int number, bool management);
    int remove_trees(QString expression, double fraction, bool management);
    double aggregate_function(QString expression, QString filter, QString type);

    // removal fractions
    double mRemoveFoliage, mRemoveBranch, mRemoveStem;
    QList<QPair<Tree*, double> > mTrees;
    int mRemoved;
    int mStandId; ///< link to active stand
};

} // namespace
#endif // FMTREELIST_H
