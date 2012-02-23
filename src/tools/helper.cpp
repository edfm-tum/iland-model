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

#include "helper.h"
#include <QtCore>
#include <QtGui>
//#include "cycle.h"
#include "ticktack.h"
#include <limits>

#include "version.h"
// static members
QHash<QString, double> DebugTimer::mTimingList;

Helper::Helper()
{
}

QString Helper::currentRevision()
{
    //QString cur_revision="$Revision: 202 $";
    QString cur_revision = QString(svnRevision());
    return cur_revision; //.section(" ",1,1);

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
QByteArray Helper::loadFile(const QString& fileName)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QTextStream s(&file);
    QByteArray result;
    s >> result;

    return result;
}

void Helper::saveToFile(const QString &fileName, const QByteArray &data)
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    QTextStream s(&file);
    s << data;
}



void Helper::msg(const QString &message, QWidget *parent)
{
    QMessageBox::information(parent, "iLand", message);
}

bool Helper::question(const QString &message, QWidget *parent)
{
   return QMessageBox::question(parent, "iLand", message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

QString Helper::fileDialog(const QString &title)
{
    QString fileName = QFileDialog::getOpenFileName(0,
     title,"", "All files (*.*)");
    return fileName;
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

void StatData::calculatePercentiles() const
{
    mP25 = percentile(25);
    mP75 = percentile(75);
    mMedian = percentile(50);
}

void StatData::calculate()
{
   if (mData.isEmpty()) {
       mSum=mMedian=mP25=mP75=mMean=mMin=mMax=0.;
       return;
   }
   mP25 = std::numeric_limits<double>::max();
   mP75 = std::numeric_limits<double>::max();
   mMedian = std::numeric_limits<double>::max();
   mMin = std::numeric_limits<double>::max();
   mMax = - std::numeric_limits<double>::max();
   QVector<double>::const_iterator end = mData.constEnd();
   QVector<double>::const_iterator i = mData.constBegin();
   mSum = 0.;
   while (i!=end) {
       mSum += *i;
       mMin = qMin(*i, mMin);
       mMax = qMax(*i, mMax);
       ++i;
   }
   mMean = mSum / double(mData.count());
   //qDebug() << QString("p25: %1 Median: %2 p75: %3 min: %4 max: %5").arg(mP25).arg(mMedian).arg(mP75).arg(mMin).arg(mMax);
}


double StatData::percentile(const int perc) const
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


/*
double DebugTimer::m_tick_p_s=0.;

void DebugTimer::sampleClock(int ms)
{
    QTime t;
    t.start();
    ticks now = getticks();

    while (t.elapsed() < ms) {
       int t;
       double x=0.;
       for (t=0; t<100; t++)
           x+=sin(x);
    }
    int el = t.elapsed();
    double tickselapsed = elapsed(getticks(), now);
    m_tick_p_s = tickselapsed / double(el);
    qDebug() << ms << "ms -> ticks/msec" << m_tick_p_s << "ticks elapsed" << tickselapsed;

}*/
DebugTimer::~DebugTimer()
{
    double t = elapsed();
    mTimingList[m_caption]+=t;
    // show message if timer is not set to silent, and if time > 1ms (if timer is set to hideShort (which is the default))
    if (!m_silent && (!m_hideShort || t>1.))
        showElapsed();
}

DebugTimer::DebugTimer(const QString &caption, bool silent)
{
    m_caption = caption;
    m_silent=silent;
    m_hideShort=true;
    if (!mTimingList.contains(caption))
        mTimingList[caption]=0.;
    start();
}

void DebugTimer::clearAllTimers()
{
    QHash<QString, double>::iterator i = mTimingList.begin();
     while (i != mTimingList.end()) {
         i.value() = 0.;
         ++i;
     }
}
void DebugTimer::printAllTimers()
{
    QHash<QString, double>::iterator i = mTimingList.begin();
    qWarning() << "Total timers\n================";
    double total=0.;
    while (i != mTimingList.end()) {
         if (i.value()>0)
            qWarning() << i.key() << ":" << i.value() << "ms";
         total+=i.value();
         ++i;
     }
    qWarning() << "Sum: " << total << "ms";
}

void DebugTimer::interval(const QString &text)
{
    double elapsed_time = elapsed();
    qDebug() << "Timer" << text << elapsed_time << "ms";
    start();
}

void DebugTimer::showElapsed()
{
    if (!m_shown) {
            qDebug() << "Timer" << m_caption << ":" << elapsed() << "ms";
    }
    m_shown=true;
}
double DebugTimer::elapsed()
{
    return t.elapsed()*1000;
}

void DebugTimer::start()
{
    t.start();
    m_shown=false;
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
    p.setX( pixel.x()/m_scale_worldtoscreen +  m_delta_worldtoscreen.x());
    p.setY( (m_screen.height() - pixel.y()  )/m_scale_worldtoscreen + m_delta_worldtoscreen.y() );
    return p;

}

/// toScreen() converts world coordinates in screen coordinates using the defined viewport.
const QPoint Viewport::toScreen(const QPointF p)
{
    QPoint pixel;
    pixel.setX( qRound( (p.x()-m_delta_worldtoscreen.x())* m_scale_worldtoscreen ) );
    pixel.setY( m_screen.height() - 1 -  qRound( (p.y()-m_delta_worldtoscreen.y() ) * m_scale_worldtoscreen ));
    return pixel;
}

/// sets the screen rect; this also modifies the viewport.
void Viewport::setScreenRect(const QRect &viewrect)
{
    if (m_screen!=viewrect) {
        m_screen = viewrect;
        m_viewport = viewrect;
        zoomToAll();
    }
}

/// show the full extent of the world.
void Viewport::zoomToAll()
{
    // calculate move/scale so that world-rect maps entirely onto screen
    double scale_x = m_screen.width() /  m_world.width(); // pixel per meter in x
    double scale_y = m_screen.height() / m_world.height(); // pixel per meter in y
    double scale = qMin(scale_x, scale_y);
    QPointF d;
    if (scale_x < scale_y) {
        // x-axis fills the screen; center in y-axis
        d.setX(m_world.left());
        int py_mid = m_screen.height()/2;
        double world_mid = m_world.center().y();
        d.setY( world_mid - py_mid/scale );
    } else {
        d.setY(m_world.top());
        int px_mid = m_screen.width()/2;
        double world_mid = m_world.center().x();
        d.setX( world_mid - px_mid/scale );
    }
    m_delta_worldtoscreen = d;
    m_scale_worldtoscreen = scale;
    m_viewport.setBottomLeft(toWorld(m_screen.topLeft()));
    m_viewport.setTopRight(toWorld(m_screen.bottomRight()));
}

/// zoom using a factor of @p factor. Values > 1 means zoom out, < 1 zoom in. (factor=1 would have no effect).
/// after zooming, the world-point under the mouse @p screen_point is still under the mouse.
void Viewport::zoomTo(const QPoint &screen_point, const double factor)
{
    QPointF focus_point = toWorld(screen_point); // point under the mouse

    m_viewport.setWidth(m_viewport.width() * factor);
    m_viewport.setHeight(m_viewport.height() * factor);

    m_scale_worldtoscreen /= factor;

    // get scale/delta
    QPointF new_focus = toWorld(screen_point);
    m_delta_worldtoscreen -= (new_focus - focus_point);

    m_viewport.setBottomLeft(toWorld(m_screen.topLeft()));
    m_viewport.setTopRight(toWorld(m_screen.bottomRight()));

    //qDebug() <<"oldf"<< new_focus << "newf" << focus_point << "m_delta" << m_delta_worldtoscreen << "m_scale:" << m_scale_worldtoscreen << "viewport:"<<m_viewport;
}

/// move the viewport. @p screen_from and @p screen_to give mouse positions (in pixel) from dragging the mouse.
void Viewport::moveTo(const QPoint &screen_from, const QPoint &screen_to)
{
    QPointF p1 = toWorld(screen_from);
    QPointF p2 = toWorld(screen_to);
    m_delta_worldtoscreen -= (p2-p1);
    // correct the viewport
    m_viewport.setBottomLeft(toWorld(m_screen.topLeft()));
    m_viewport.setTopRight(toWorld(m_screen.bottomRight()));
}

bool Viewport::isVisible(const QPointF &world_coord) const
{
    return m_viewport.contains(world_coord);
}
bool Viewport::isVisible(const QRectF &world_rect) const
{
    return m_viewport.contains(world_rect)
            || m_viewport.intersects(world_rect);
}

