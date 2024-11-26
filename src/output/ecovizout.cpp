#include "ecovizout.h"
#include "model.h"
#include "resourceunit.h"
#include "tree.h"
#include "stamp.h"
#include "species.h"

EcoVizOut::EcoVizOut()
{
    setName("tree output for visualization software", "ecoviz");
    setDescription("This is a special output for linking with the visualization tool 'Ecoviz'.\n "\
                   "The output is a small standard iLand outpunt and a special textfile ('PDB') " \
                   "which contains *all* trees and *all* saplings of a year (i.e. it tends to get big!). " \
                   "Provide a file pattern in 'fileName', a $-sign is replaced with the current year. For example " \
                   "output/ecoviz_$.pdb is saved as output/ecoviz_0.pdb (initial state), output/ecoviz_1.pdb (after 1 year of simulation), ....). \n" \
                   "You can use the 'condition' to control if the output should be created for the current year(see also dynamic stand output)");
    columns() << OutputColumn::year()
              << OutputColumn("count_trees", "total number of trees count saved to file", OutInteger)
              << OutputColumn("count_saplings", "total number saplings saved to file", OutInteger)
              << OutputColumn("filename", "filename of the created output PDB file", OutString);

}

void EcoVizOut::exec()
{
    Model *m = GlobalSettings::instance()->model();
    if (!mCondition.isEmpty())
        if (!mCondition.calculateBool(GlobalSettings::instance()->currentYear()))
            return;
    int total_tree_count = 0;
    int total_cohort_count = 0;
    foreach (const ResourceUnit *ru,  m->ruList()) {
        total_tree_count += static_cast<int>(ru->statistics().count());
        total_cohort_count += ru->statistics().cohortCount();
    }

    QString stryear = QString::number(currentYear());
    QString file = mFilePattern;
    file.replace("$", stryear);

    *this << currentYear()
            << total_tree_count
            << total_cohort_count
            << file;
    writeRow();

    // write the actual file
    file = GlobalSettings::instance()->path(file);
    writePDBFile(file, total_tree_count, total_cohort_count, currentYear());
    qDebug() << "Saved PDB file" << file;

}

void EcoVizOut::setup()
{
    // use a condition for to control execuation for the current year
    QString condition = settings().value(".condition", "");
    mCondition.setExpression(condition);

    mFilePattern = settings().value(".fileName", "output/pdb_$.pdb");

}

bool EcoVizOut::writePDBFile(QString fileName, int n_trees, int n_cohorts, int year)
{
    QFile file(fileName);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
        // open a stream
        QTextStream stream(&file);
        QChar ws = ' ';

        //stream << "Center Point: " << iter_result[0] << "  " << iter_result[1]
        //       << "  " << iter_result[2] << " Rotation: " << iter_result[3] <<'\n';
        stream << "3.0" << Qt::endl; // PDB_file_version
        // version 3.0: include local origin in metric coordinates
        stream << GlobalSettings::instance()->settings().value("model.world.location.x") << " "
               << GlobalSettings::instance()->settings().value("model.world.location.y") << Qt::endl;
        stream << year << Qt::endl; // Time_step_number
        qint64 ntree_pos = stream.pos();
        stream << "          " << Qt::endl; // space for total number of trees

        // now loop over all trees and write a line for each tree
        AllTreeIterator at(GlobalSettings::instance()->model());
        n_trees = 0;
        while (Tree *tree = at.next()) {
            // Tree_ID<int> species_ID<cahr> pos_x<float> pos_y<float> height<float> canopy_radius<float> diameter_at_breast_height<float> status<int>
            stream << tree->id() << ws << tree->species()->id() << ws <<
                      tree->position().x() << ws << tree->position().y() << ws << tree->height() << ws;
            stream << tree->stamp()->reader()->crownRadius() << ws;
            stream << tree->dbh() << ws << (tree->isDead() ? 1 : 0);
            stream << Qt::endl;
            ++n_trees;
        }

        // now loop over all the saplings
        qint64 nsap_pos = stream.pos();
        stream << "          " << Qt::endl;
        n_cohorts = 0;
        Saplings *saplings = GlobalSettings::instance()->model()->saplings();
        foreach(ResourceUnit *ru, GlobalSettings::instance()->model()->ruList()) {
            if (ru->id()==-1)
                continue; // do not include if out of project area


            SaplingCell *s = ru->saplingCellArray();
            for (int px=0;px<cPxPerHectare;++px, ++s) {
                int n_on_px = s->n_occupied();
                if (n_on_px>0) {

                    QPointF coord = saplings->coordOfCell(ru, px);

                    for (int i=0;i<SaplingCell::NSapCells;++i) {
                        if (s->saplings[i].is_occupied()) {
                            ResourceUnitSpecies *rus = s->saplings[i].resourceUnitSpecies(ru);
                            const Species *species = rus->species();
                            double dbh = s->saplings[i].height / species->saplingGrowthParameters().hdSapling  * 100.;
                            // check minimum dbh
                            if (dbh < 0.1)
                                continue;

                            double n_repr = species->saplingGrowthParameters().representedStemNumberH(s->saplings[i].height) / static_cast<double>(n_on_px);


                            stream << coord.x() << ws << coord.y() << ws <<
                                      rus->species()->id() << ws <<
                                      dbh << ws <<
                                      s->saplings[i].height << ws <<
                                      n_repr << Qt::endl;
                            ++n_cohorts;

                        }
                    }
                }
            }
        }


        // write back the number of trees / saplings in the file
        file.seek(ntree_pos);
        QString nstr = QString::number(n_trees);
        file.write(nstr.toUtf8());

        file.seek(nsap_pos);
        nstr = QString::number(n_cohorts);
        file.write(nstr.toUtf8());

        file.close();

        return true;

    } else {
        qDebug() << "writePDBFile: Error opening file" << fileName;
        return false;
    }
}
