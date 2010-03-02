#include "dynamicstandout.h"

#include "helper.h"
#include "model.h"
#include "resourceunit.h"
#include "species.h"
#include "expressionwrapper.h"

DynamicStandOut::DynamicStandOut()
{
    setName("dynamic stand output by species/RU", "dynamicstand");
    setDescription("Userdefined outputs for tree aggregates for each stand.\n"\
                   "Technically, each field is calculated 'live', i.e. it is looped over all trees, and eventually the statistics (percentiles) "\
                   "are calculated.\n" \
                   "Each field is defined as: ''field.aggregatio''n (separated by a dot). A ''field''' is a valid [Expression]. ''Aggregation'' is one of the following:  " \
                   "mean, sum, min, max, p25, p50, p75, p5, 10, p90, p95 (pXX=XXth percentile).");
    columns() << OutputColumn::year() << OutputColumn::ru() << OutputColumn::species();
    // other colums are added during setup...
}

const QStringList aggList = QStringList() << "mean" << "sum" << "min" << "max" << "p25" << "p50" << "p75" << "p5"<< "p10" << "p90" << "p95";

void DynamicStandOut::setup()
{
    QString filter = settings().value(".rufilter","");
    QString fieldList = settings().value(".columns", "");
    if (fieldList.isEmpty())
        return;
    mRUFilter.setExpression(filter);
    // clear columns
    columns().erase(columns().begin()+3, columns().end());
    mFieldList.clear();

    // setup fields
   if (!fieldList.isEmpty()) {
       QRegExp rx("([^\\.]+).(\\w+)[,\\s]*"); // two parts: before dot and after dot, and , + whitespace at the end
        int pos=0;
        QString field, aggregation;
        TreeWrapper tw;
        while ((pos = rx.indexIn(fieldList, pos)) != -1) {
            pos += rx.matchedLength();

            field = rx.cap(1); // field / expresssion
            aggregation = rx.cap(2);
            mFieldList.append(SDynamicField());
            // parse field
            if (field.count()>0 && !field.contains('(')) {
                // simple expression
                mFieldList.back().var_index = tw.variableIndex(field);
            } else {
                // complex expression
                mFieldList.back().var_index=-1;
                mFieldList.back().expression = field;
            }

            mFieldList.back().agg_index = aggList.indexOf(aggregation);
            if (mFieldList.back().agg_index==-1)
                 throw IException(QString("Invalid aggregate expression for dynamic output: %1\nallowed:%2")
                                          .arg(aggregation).arg(aggList.join(" ")));

             QString stripped_field=QString("%1_%2").arg(field, aggregation);
             stripped_field.replace(QRegExp("[\\[\\]\\,\\(\\)<>=!\\s]"), "_");
             stripped_field.replace("__", "_");
             columns() << OutputColumn(stripped_field, field, OutDouble);
        }
    }
}


void DynamicStandOut::exec()
{

    if (mFieldList.count()==0)
        return;

    DebugTimer t("dynamic stand output");
    Model *m = GlobalSettings::instance()->model();
    QVector<double> data; //statistics data
    StatData stat; // statistcs helper class
    TreeWrapper tw;
    RUWrapper ruwrapper;
    mRUFilter.setModelObject(&ruwrapper);

    Expression custom_expr;

    foreach(ResourceUnit *ru, m->ruList()) {
        // test filter
        if (!mRUFilter.isEmpty()) {
            ruwrapper.setResourceUnit(ru);
            if (!mRUFilter.execute())
                continue;
        }
        foreach(const ResourceUnitSpecies &rus, ru->ruSpecies()) {
            if (rus.constStatistics().count()==0)
                continue;

            *this << currentYear() << ru->index() << rus.species()->id(); // keys
            // dynamic calculations
            foreach (const SDynamicField &field, mFieldList) {

                if (!field.expression.isEmpty()) {
                    // setup dynamic dynamic expression if present
                    custom_expr.setExpression(field.expression);
                    custom_expr.setModelObject(&tw);
                }
                data.clear();
                foreach(const Tree &tree, ru->trees()) {
                    if (tree.species()->index()!=rus.species()->index())
                        continue;
                    tw.setTree(&tree);
                    if (field.var_index>=0)
                        data.push_back(tw.value(field.var_index));
                    else
                        data.push_back(custom_expr.execute());
                }
                // calculate statistics
                stat.setData(data);
                // aggregate
                double value;
                switch (field.agg_index) {
                case 0: value = stat.mean(); break;
                case 1: value = stat.sum(); break;
                case 2: value = stat.min(); break;
                case 3: value = stat.max(); break;
                case 4: value = stat.percentile25(); break;
                case 5: value = stat.median(); break;
                case 6: value = stat.percentile75(); break;
                case 7: value = stat.percentile(5); break;
                case 8: value = stat.percentile(10); break;
                case 9: value = stat.percentile(90); break;
                case 10: value = stat.percentile(95); break;

                default: value = 0.; break;
                }
                // add current value to output
                *this << value;

            } // foreach (field)

            writeRow();
        } //foreach species
    } // foreach ressource unit

}
