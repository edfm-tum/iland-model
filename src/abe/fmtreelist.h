#ifndef FMTREELIST_H
#define FMTREELIST_H

#include <QObject>
#include "scriptglobal.h"
#include "grid.h"
class Tree; // forward
class Expression;

namespace ABE {
class FMStand;

class FMTreeList : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int stand READ standId) ///< return stand, -1 if not set
    Q_PROPERTY(bool simulate READ simulate WRITE setSimulate) ///< if 'simulate' is true, trees are only marked for removal
public:

    explicit FMTreeList(QObject *parent = 0);
    explicit FMTreeList(FMStand *stand, QObject *parent = 0);
    ~FMTreeList();
    int standId() const { return mStandId; }
    void setStand(FMStand *stand);
    bool simulate() const {return mOnlySimulate; }
    void setSimulate(bool do_simulate) {mOnlySimulate = do_simulate; }

    /// access the list of trees
    const QVector<QPair<Tree*, double> > trees() const { return mTrees; }

    /// access to local grid (setup if necessary)
    Grid<float> &localGrid() { prepareGrids(); return mLocalGrid; }

signals:

public slots:
    // loading of trees
    /// load all trees of the stand, return number of trees (living trees)
    int loadAll() { return load(QString()); }
    /// load all trees passing the filter, return number of trees (load only living trees)
    int load(const QString &filter);

    /// load all trees of the stand and either kill or harvest trees that are marked for that operation.
    int removeMarkedTrees();

    /** kill "number" of stems
     *  in the percentile interval "from" - "to".
     *  remove all if "number" is higher than the count.
     *  return the number of removed trees. */
//    int killPct(int pctfrom, int pctto, int number);
//    int killAll(); ///< kill all trees in the list
    /** kill 'fraction' of all trees with 'filter'=true */
//    int kill(QString filter, double fraction);
    // management
    /** kill "number" of stems
     *  in the percentile interval "from" - "to".
     *  remove all if "number" is higher than the count.
     * Use the removal fractions set by the removeStem, removeBranch and removeFoliage properties.
     *  return the number of removed trees. */
//    int managePct(int pctfrom, int pctto, int number);
//    int manageAll(); ///< manage all trees in the list
    /** manage 'fraction' of all trees [0..1] with 'filter'. Return number of removed trees. */
    int harvest(QString filter=QString(), double fraction=1.);

    double percentile(int pct); ///< get value for the pct th percentile (1..100)
//    void killSaplings(MapGridWrapper *wrap, int key); ///< kill all saplings that are on the area denoted by 'key' of the given grid (script access)

    /** hacky access function to resource units covered by a polygon.
     the parameters are "remove-fractions": i.e. value=0: no change, value=1: set to zero. */
//    void removeSoilCarbon(MapGridWrapper *wrap, int key, double SWDfrac, double DWDfrac, double litterFrac, double soilFrac);
    /** slash snags (SWD and otherWood-Pools) of polygon 'key' on the map 'wrap'.
      @param slash_fraction 0: no change, 1: 100%
       */
//    void slashSnags(MapGridWrapper *wrap, int key, double slash_fraction);
    /**  sort the list according to 'statement'. Note that sorting is in ascending order. To
     *   have e.g. tallest trees first in the list, use '-height'.
    */
    void sort(QString statement); ///< sort trees in the list according to a criterion
//    int filter(QString filter); ///< apply a filter on the list of trees (expression), return number of remaining trees.
//    int filterIdList(QVariantList idList); ///< apply filter in form of a list of ids, return number of remaining trees
    void randomize(); ///< random shuffle of all trees in the list

    /// calculate the mean value for all trees in the internal list for 'expression' (filtered by the filter criterion)
    double mean(QString expression, QString filter=QString()) { return aggregate_function( expression, filter, "mean"); }
    /// calculate the sum for all trees in the internal list for the 'expression' (filtered by the filter criterion)
    double sum(QString expression, QString filter=QString()) { return aggregate_function( expression, filter, "sum"); }

    /// set up internally a map (10m grid cells) of the stand
    /// with a given grid type or using a custom expression.
    void prepareStandGrid(QString type, QString custom_expression=QString());
    void exportStandGrid(QString file_name);
    FloatGrid &standGrid() {return mStandGrid; }


private:
    bool trace() const;
    ///
    int remove_percentiles(int pctfrom, int pctto, int number, bool management);
    int remove_trees(QString expression, double fraction, bool management);
    double aggregate_function(QString expression, QString filter, QString type);
    bool remove_single_tree(int index, bool harvest=true);
    void check_locks();

    // grid functions
    void prepareGrids();
    /// run function 'func' for all trees in the current tree list of the stand.
    /// signature: function(&ref_to_float, &ref_to_int, *tree);
    /// after all trees are processed, func is called again (aggregations, ...) with tree=0.
    void runGrid(void (*func)(float &, int &, const Tree *, const FMTreeList *) );

    QVector<QPair<Tree*, double> > mTrees; ///< store a Tree-pointer and a value (e.g. for sorting)
    bool mResourceUnitsLocked;
    int mRemoved;
    FMStand *mStand; /// the stand the list is currently connected
    int mStandId; ///< link to active stand
    int mNumberOfStems; ///< estimate for the number of trees in the stand
    bool mOnlySimulate; ///< mode
    QRectF mStandRect;
    FloatGrid mStandGrid; ///< local stand grid (10m pixel)
    Grid<int> mTreeCountGrid; ///< tree counts on local stand grid (10m)
    Grid<float> mLocalGrid; ///< 2m grid of the stand
    Expression *mRunGridCustom;
    double *mRunGridCustomCell;
    friend void rungrid_custom(float &cell, int &n, const Tree *tree, const FMTreeList *list);

    friend class ActThinning;
};

} // namespace
#endif // FMTREELIST_H
