#include "helper.h"
#include <QtCore>

Helper::Helper()
{
}

QString Helper::loadTextFile(const QString& fileName)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }
    QTextStream s(&file);
    //s.setCodec("UTF-8");
    QString result=s.readAll();
    return result;
}

void Helper::saveToTextFile(const QString& fileName, const QString& text)
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    QTextStream s(&file);
    s << text;
}

void Helper::msg(const QString &message, QWidget *parent)
{
   QMessageBox::information(parent, "Eforwood MCA", message);
}

bool Helper::question(const QString &message, QWidget *parent)
{
   return QMessageBox::question(parent, "Eforwood MCA", message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}


void Helper::openHelp(const QString& topic)
{
    QUrl url;
    qDebug() << "current path" << QDir::currentPath();
    url.setUrl(QString("file:///%1/help/%2.html").arg(QDir::currentPath(),topic) , QUrl::TolerantMode);
    qDebug() << url;
    if (url.isValid())
        qDebug() << "url is valid";
    QDesktopServices::openUrl(url);
}

QString Helper::stripHtml(const QString &source)
{
    QString str = source.simplified();
    return str.replace(QRegExp("<[^>]+>"),"");
}


// debugging
bool Helper::m_quiet = true;
bool Helper::m_NoDebug = false;



// colors
QColor Helper::colorFromValue(const float value, const float min_value, const float max_value, const bool reverse)
{
    float rval = value;
    rval = std::max(min_value, rval);
    rval = std::min(max_value, rval);
    if (reverse)
        rval = max_value - rval;
    float rel_value;
    QColor col;
    if (min_value < max_value) {
        // default: high values -> red (h=0), low values to blue (h=high)
        rel_value = 1 - (rval - min_value) / (max_value - min_value);
        col=  QColor::fromHsvF(0.66666666666*rel_value, 0.95, 0.95).rgb();
    } else {
        col = Qt::white;
    }
    return col;
}

// StatData
StatData::StatData(QVector<double> &data)
{
    mData=data;
    calculate();
}

void StatData::calculate()
{
   if (mData.isEmpty()) {
       mMedian=mP25=mP75=mMean=mMin=mMax=0.;
       return;
   }
   mP25 = percentile(25);
   mP75 = percentile(75);
   mMedian = percentile(50);
   mMin = std::numeric_limits<double>::max();
   mMax = - std::numeric_limits<double>::max();
   QVector<double>::const_iterator end = mData.constEnd();
   QVector<double>::const_iterator i = mData.constBegin();
   double sum = 0.;
   while (i!=end) {
       sum += *i;
       mMin = qMin(*i, mMin);
       mMax = qMax(*i, mMax);
       ++i;
   }
   mMean = sum / double(mData.count());
   //qDebug() << QString("p25: %1 Median: %2 p75: %3 min: %4 max: %5").arg(mP25).arg(mMedian).arg(mP75).arg(mMin).arg(mMax);
}


const double StatData::percentile(const int perc)
{
// double *Values, int ValueCount,
    // code von: Fast median search: an ANSI C implementation, Nicolas Devillard, http://ndevilla.free.fr/median/median/index.html
        // algo. kommt von Wirth, hier nur an c++ angepasst.

    int ValueCount = mData.count();
    int i,j,l,m, n, k ;
    double x, temp ;
    if (ValueCount==0)
      return 0;
    n = ValueCount;
    // k ist der "Index" des gesuchten wertes
    if (perc!=50) {
        // irgendwelche perzentillen
        int d = 100 / ( (perc>50?(100-perc):perc) );
        k = ValueCount / d;
        if (perc>50)
          k=ValueCount - k - 1;
    } else {
        // median
        if (ValueCount & 1)  // gerade/ungerade?
          k = ValueCount / 2 ;  // mittlerer wert
        else
          k= ValueCount / 2 -1; // wert unter der mitte
    }
    l=0 ; m=n-1 ;
    while (l<m) {
        x=mData[k] ;
        i=l ;
        j=m ;
        do {
            while (mData[i]<x) i++ ;
            while (x<mData[j]) j-- ;
            if (i<=j) {
                //ELEM_SWAP(a[i],a[j]) ; swap elements:
                temp = mData[i]; mData[i]=mData[j]; mData[j]=temp;
                i++ ; j-- ;
            }
        } while (i<=j) ;
        if (j<k) l=i ;
        if (k<i) m=j ;
    }
    return mData[k] ;

}

/** calculate Ranks.
  @param data values for N items,
  @param descending true: better ranks for lower values
  @return a vector that contains for the Nth data item the resulting rank.
  Example: in: {5, 2, 7, 5}
           out: {2, 1, 4, 2}
  */
QVector<int> StatData::calculateRanks(const QVector<double> &data, bool descending)
{
   // simple ranking algorithm.
   // we have "N" data-values.
   // rank := N - (N smaller or equal)
   int i, j;
   int smaller;
   QVector<int> ranks;
   ranks.resize(data.count());
   int n=data.count();
   for (i=0;i<n;i++) {
       smaller = 0;
       for (j=0;j<n;j++) {
          if (i==j)
             continue;
          if (data[j]<=data[i])
             smaller++;
       }
       if (descending) // better rank if lower value...
          ranks[i] = smaller + 1;
       else
          ranks[i] = n - smaller;  // better rank if value is higher...
   }
   return ranks;
}

/** scale the data in such a way that the sum of all data items is "targetSum"
  */
void StatData::normalize(QVector<double> &data, double targetSum)
{
    QVector<double>::iterator i, end=data.end();
    double sum=0.;
    for (i=data.begin(); i!=end; ++i)
        sum+=*i;

    if (sum!=0) {
        double m = targetSum / sum;
        for (i=data.begin(); i!=end; ++i)
        *i *= m;
    }
}


/** UpdateState.

*/
void UpdateState::invalidate(bool self)
{
    if (self)
        mVal++;
    foreach (UpdateState *s, mChilds)
        s->invalidate(true);
}

void UpdateState::saveState(UpdateState *state)
{
    mSavedStates[state]=state->mVal;
}
bool UpdateState::hasChanged(UpdateState *state)
{
    if (!mSavedStates.contains(state))
        return true;
    qDebug() << "UpdateState::hasChanged: saved: " << mSavedStates[state] << "current: " << state->mVal;
    return mSavedStates[state] != state->mVal;
}
// set internal state to the current state
void UpdateState::update()
{
    mCurrentVal = mVal;
}
// check if state needs update
bool UpdateState::needsUpdate()
{
    return mVal > mCurrentVal;
}


void DebugTimer::showElapsed()
{
    if (!m_shown)
        qDebug() << "Timer" << m_caption << ": " << elapsed() << "msec";
    m_shown=true;
}
int DebugTimer::elapsed()
{
    return t.elapsed();
}

/** @class Viewport
  Handles coordinaive transforation between grids (based on real-world metric coordinates).
  The visible part of the grid is defined by the "viewport" (defaults to 100% of the grid).
  The result coordinates are mapped into a "ScreenRect", which is a pixel-based viewing window.
*/

/// toWorld() converts the pixel-information (e.g. by an mouse event) to the corresponding real world coordinates (defined by viewport).
const QPointF Viewport::toWorld(const QPoint pixel)
{
    QPointF p;
    p.setX( m_viewport.left() + (pixel.x()/double(m_screen.width())) * m_viewport.width() );
    p.setY( m_viewport.top() + (m_screen.height() - pixel.y())/double(m_screen.height()) * m_viewport.height());
    return p;
}

/// toScreen() converts world coordinates in screen coordinates using the defined viewport.
const QPoint Viewport::toScreen(const QPointF p)
{
    double x = (p.x()-m_viewport.left()) / m_viewport.width(); // scale to 0..1
    double y = (p.y()-m_viewport.top()) / m_viewport.height(); // scale to 0..1
    QPoint pixel( int( x * m_screen.width()),
                  int( (1. - y) * m_screen.height()));
    return pixel;
}

/// sets the screen rect; this also modifies the viewport.
void Viewport::setScreenRect(const QRect &viewrect)
{
    m_screen = viewrect;
    m_viewport = viewrect;
    if (viewrect.isNull() || m_screen.isNull())
        return;
    double aspectratio = viewrect.width() / double(viewrect.height());
    double px_per_meter_x = m_world.width() / double(m_screen.width());
    double px_per_meter_y = m_world.height() / double(m_screen.height());
    m_viewport = m_world;
    if (px_per_meter_x > px_per_meter_y) {
        // width is too high... center horizontally
        m_viewport.setWidth(m_world.width() / aspectratio);
    } else {
        // height is too high .. center vertically
        m_viewport.setHeight(m_world.height() * aspectratio );
    }
}
