
#ifndef MICROCLIMATE_H
#define MICROCLIMATE_H

#include "grid.h"

class ResourceUnit; // forward

// data structure for a single cell with 10m size
// the MicroclimateCell stores vegetation information
struct MicroclimateCell {
public:
    MicroclimateCell() { clear(); }
    MicroclimateCell(double evergreen_share, double lai, double shade_tol, double tpi, double northness);
    void clear() { mEvergreenShare = 0; mLAI = 0; mTPI=0; mNorthness=0; }
    bool valid() { return mNorthness > std::numeric_limits<short int>().min(); }
    void setInvalid() {mNorthness = std::numeric_limits<short int>().min(); }

    /// set conifer share on the cell (0..1)
    void setEvergreenShare(double share) { mEvergreenShare = static_cast<short unsigned int>(share * 1000.); /* save as short int */ }
    /// conifer share from 0 (=0%) to 1 (=100%). Empty cells have a share of 0.
    double evergreenShare() const { return static_cast<double>(mEvergreenShare) / 1000.; }

    /// set conifer share on the cell (0..1)
    void setLAI(double lai) { mLAI = static_cast<short unsigned int>(lai * 1000.); /* save as short int */ }
    /// conifer share from 0 (=0%) to 1 (=100%). Empty cells have a share of 0.
    double LAI() const { return static_cast<double>(mLAI) / 1000.; }

    void setShadeToleranceMean(double stol) {mShadeTol = static_cast<short unsigned int>(stol*10000.); /*stol: 1-5*/}
    /// basal area weighted shade tolerance class (iLand species parameter)
    double shadeToleranceMean() const { return static_cast<double>(mShadeTol) / 10000.; }

    /// northness (= cos(aspect) ) [-1 .. 1]
    double northness() const {  return mNorthness > std::numeric_limits<short int>().min() ? static_cast<double>(mNorthness) / 10000. : 0.; }
    void setNorthness(double value)  { mNorthness = static_cast<short int>(value * 10000); }

    /// topographic Position Index (~ differece between elevation and average elevation in with a radius)
    double topographicPositionIndex() const { return static_cast<double>(mTPI) / 10.; }
    void setTopographicPositionIndex(double value)  { mTPI = static_cast<short int>(value * 10); }

    double growingSeasonIndex(const ResourceUnit *ru, int dayofyear) const;

    // microclimate buffering for a single day

    /// minimum microclimate buffering
    double minimumMicroclimateBuffering(const ResourceUnit *ru, int dayofyear) const;
    double maximumMicroclimateBuffering(const ResourceUnit *ru, int dayofyear) const;

    /// faster calculation minimum microclimate buffering, when growingseasonindex is known
    double minimumMicroclimateBuffering(double gsi) const;
    double maximumMicroclimateBuffering(double gsi) const;

private:
    // use 16 bit per value
    short unsigned int mEvergreenShare;
    short unsigned int mLAI;
    short unsigned int mShadeTol;
    short int mTPI;
    short int mNorthness;
};

class Microclimate
{
public:
    Microclimate(const ResourceUnit *ru);
    ~Microclimate();

    /// analyze vegetation on resource unit and calculate indices
    void calculateVegetation();

    // get resource unit aggregates
    /// average minimum buffering, i.e. actual min temperature = min_temp + buffering
    double minimumMicroclimateBufferingRU(int dayofyear) const;


    /// average maximum buffering, i.e. actual max temperature = max_temp + buffering
    double maximumMicroclimateBufferingRU(int dayofyear) const;

    double meanMicroclimateBufferingRU(int dayofyear) const;


    MicroclimateCell &cell(int index) { Q_ASSERT(index>=0 && index < 100); return mCells[index]; }
    const MicroclimateCell &constCell(int index) const { Q_ASSERT(index>=0 && index < 100); return mCells[index]; }
    /// get the cell located at a given metric location
    int cellIndex(const QPointF &coord);
    QPointF cellCoord(int index);
private:
    void calculateFixedFactors();
    void calculateRUMeanValues();
    const ResourceUnit *mRU;
    MicroclimateCell *mCells;
    QVector< QPair<float, float> > mRUvalues;
    bool mIsSetup;
};

class MicroclimateVisualizer: public QObject {
    Q_OBJECT
public:
    MicroclimateVisualizer(QObject *parent = nullptr);
    ~MicroclimateVisualizer();
    static void setupVisualization();
public slots:
    //QJSValue grid(); ///< return a copy of the underlying grid
    Grid<double> *paintGrid(QString what, QStringList &names, QStringList &colors); ///< function called from iLand visualization

    static Grid<double> *grid(QString what, int dayofyear);
private:
    Grid<double> mGrid;
    static MicroclimateVisualizer *mVisualizer;

};

#endif // MICROCLIMATE_H
