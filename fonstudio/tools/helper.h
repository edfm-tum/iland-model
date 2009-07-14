#ifndef HELPER_H
#define HELPER_H

#include <QVector>
#include <QtCore>


#define QUIETDEBUG(x) if (!Helper::quiet()) { qDebug() << x; }

/** Helper contains a bunch of (static) helper functions.
  * including simplifed functions to read/write plain text files (loadTextFile(), saveToTextFile()),
  * funcitons to show message dialogs (msg(), question()), and functions to control the amount of
  * debug outputs (quiet(), debugEnabled()).
  */
class Helper
{
public:
    Helper();
    static QString loadTextFile(const QString& fileName);
    static void saveToTextFile(const QString& fileName, const QString& text);
    static void msg(const QString &message, QWidget *parent=0);
    static bool question(const QString &message, QWidget *parent=0);
    /// open a File Dialog and let the user choose a file.
    /// @return the filename selected by the user, an empty string if user cancels.
    static QString fileDialog(const QString &title);
    static bool quiet() { return m_NoDebug || m_quiet; }
    static bool debugEnabled() { return !m_NoDebug; }
    static void setQuiet(bool quiet) { m_quiet = quiet; }
    static void setDebugEnabled(bool enable) { m_NoDebug = !enable; }
    static void openHelp(const QString& topic);
    static QString stripHtml(const QString &source);

    static QColor colorFromValue(const float value, const float min_value=0.f, const float max_value=1.f, const bool reverse=false);
private:
    static bool m_quiet;
    static bool m_NoDebug;
};

/** StatData.
* Helper class for statistics. This class calculates
* from a double-vector relevant information used
* for BoxPlots. */
class StatData
{
public:
    StatData() { calculate(); }
    StatData(QVector<double> &data);
    void setData(QVector<double> &data) { mData=data; calculate(); }
    void calculate();
    // getters
    const double mean() const { return mMean; } ///< arithmetic mean
    const double min() const { return mMin; } ///< minimum value
    const double max() const { return mMax; } ///< maximum value
    const double median() const { return mMedian; } ///< 2nd quartil = median
    const double percentile25() const { return mP25; } ///< 1st quartil
    const double percentile75() const { return mP75; } ///< 3rd quartil
    const double percentile(const int perc); ///< get value of a given percentile (0..100)
    // additional functions
    static QVector<int> calculateRanks(const QVector<double> &data, bool descending=false); ///< rank data.
    static void normalize(QVector<double> &data, double targetSum); ///< normalize, i.e. the sum of all items after processing is targetSum

private:
   QVector<double> mData;
   double mMean;
   double mMin;
   double mMax;
   double mP25;
   double mP75;
   double mMedian;
};

class DebugTimer
{
public:
    DebugTimer() { start(); }
    DebugTimer(const QString &caption) { m_caption = caption; start(); }
    ~DebugTimer() { showElapsed();}
    void showElapsed();
    int elapsed();
    void start() { t.start(); m_shown=false; }
private:
    QTime t;
    bool m_shown;
    QString m_caption;  
};

/** UpdateState details missing.
  */
class UpdateState
{
public:
    // available states
    UpdateState(): mCurrentVal(0), mVal(0) {}
    bool needsUpdate(); // needs local state-object an update?
    void update(); // update with master
    const int value() const { return mVal; } // return current value
    void invalidate(bool self=false); // master object enters a new state
    void addChild(UpdateState* state) { mChilds.push_back(state); }
    void saveState(UpdateState* state);
    bool hasChanged(UpdateState* state);
private:
    int mCurrentVal; // state of last explicit "update"
    int mVal; // current state
    QVector<UpdateState*> mChilds;
    QMap<UpdateState*, int> mSavedStates;
};

class Viewport
{
public:
    Viewport(): m_viewAll(true), m_scale_worldtoscreen(1.) {}
    Viewport(const QRectF worldrect, const QRect screenrect) { setWorldRect(worldrect); setScreenRect(screenrect); zoomToCenter(100.); }
    const QPointF toWorld(const QPoint pixel);
    const QPoint toScreen(const QPointF p);
    const QRect toScreen(const QRectF world) { QRect r( toScreen(world.topLeft()), toScreen(world.bottomRight()) ); return r; }
    // zoom
    void zoomToCenter(const double percent);
    // setters...
    void setViewRect(const QRectF &viewrect) { m_viewport = viewrect; }
    void setWorldRect(const QRectF &worldrect) { m_world = worldrect; }
    void setScreenRect(const QRect &viewrect);
private:
    bool m_viewAll;
    QRect m_screen;
    QRectF m_world;
    QRectF m_viewport;
    QPointF m_delta_worldtoscreen;
    double m_scale_worldtoscreen;
};

#endif // HELPER_H
