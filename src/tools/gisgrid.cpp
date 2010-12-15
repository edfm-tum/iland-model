#include "gisgrid.h"
#include <stdexcept>
#include "helper.h"

#include "globalsettings.h"
#include "model.h"

// global transformation record:
SCoordTrans GISCoordTrans;

// setup of global GIS transformation
// not a good place to put that code here.... please relocate!
void setupGISTransformation(double offsetx, double offsety, double offsetz, double angle_degree)
{
    GISCoordTrans.offsetX = offsetx;
    GISCoordTrans.offsetY = offsety;
    GISCoordTrans.offsetZ = offsetz;
    GISCoordTrans.RotationAngle=angle_degree * M_PI / 180.;
    GISCoordTrans.sinRotate=sin(GISCoordTrans.RotationAngle);
    GISCoordTrans.cosRotate=cos(GISCoordTrans.RotationAngle);
    GISCoordTrans.sinRotateReverse=sin(-GISCoordTrans.RotationAngle);
    GISCoordTrans.cosRotateReverse=cos(-GISCoordTrans.RotationAngle);
}

void worldToModel(const QVector3D &From, QVector3D &To)
{
    double x=From.x() - GISCoordTrans.offsetX;
    double y=From.y() - GISCoordTrans.offsetY;
    To.setZ( From.z() - GISCoordTrans.offsetZ );
    To.setX( x * GISCoordTrans.cosRotate - y*GISCoordTrans.sinRotate);
    To.setY( x * GISCoordTrans.sinRotate + y*GISCoordTrans.cosRotate);
    //To.setY(-To.y()); // spiegeln
}
void modelToWorld(const QVector3D &From, QVector3D &To)
{
    double x=From.x();
    double y=From.y(); // spiegeln
    To.setX( x * GISCoordTrans.cosRotateReverse - y*GISCoordTrans.sinRotateReverse + GISCoordTrans.offsetX);
    To.setY( x * GISCoordTrans.sinRotateReverse + y*GISCoordTrans.cosRotateReverse  + GISCoordTrans.offsetY);
    To.setZ( From.z() + GISCoordTrans.offsetZ );
}


GISGrid::GISGrid()
{
    mData=0;
    mNRows=0; mNCols=0;
    mCellSize = 1; // default value (for line mode)
}

GISGrid::~GISGrid()
{
    if (mData)
        delete[] mData;
}

bool GISGrid::loadFromFile(const QString &fileName)
{
    min_value = 1000000000;
    max_value = -1000000000;

    // loads from a ESRI-Grid [RasterToFile] File.
    QByteArray file_content = Helper::loadTextFile(fileName).toAscii();
    if (file_content.isEmpty()) {
        qDebug() << "GISGrid: file" << fileName << "not present or empty.";
        return false;
    }
    QList<QByteArray> lines = file_content.split('\n');

    // processing of header-data
    bool header=true;
    int pos=0;
    QString line;
    QString key;
    double value;
    do {
        if (pos>lines.count())
            throw std::logic_error("GISGrid: unexpected end of file ");
        line=lines[pos].simplified();
        if (line.length()==0 || line.at(0)=='#') {
            pos++; // skip comments
            continue;
        }
        key=line.left(line.indexOf(' '));
        if (key.length()>0 && (key.at(0).isNumber() || key.at(0)=='-')) {
            header=false;
        } else {
            value = line.mid(line.indexOf(' ')).toDouble();
            if (key=="ncols")
                mNCols=(int)value;
            else if (key=="nrows")
                mNRows=int(value);
            else if (key=="xllcorner")
                mOrigin.setX(value);
            else if (key=="yllcorner")
                mOrigin.setY(value);
            else if (key=="cellsize")
                mCellSize = value;
            else if (key=="NODATA_value")
                mNODATAValue=value;
            else
                throw std::logic_error(std::string("GISGrid: invalid key ") + key.toStdString());
            pos++;
        }
    } while (header);

    // create data
    if (mData)
        delete[] mData;
    mDataSize = mNRows * mNCols;
    mData = new double[mDataSize];


    // loop thru datalines
    int i,j;
    char *p=0;
    char *p2;
    pos--;
    for (i=mNRows-1;i>=0;i--)
        for (j=0;j<mNCols;j++) {
        // copy next value to buffer, change "," to "."
        if (!p || *p==0) {
            pos++;
            if (pos>=lines.count())
                throw std::logic_error("GISGrid: Unexpected End of File!");
            p=lines[pos].data();
            // replace chars
            p2=p;
            while (*p2) {
                if (*p2==',')
                    *p2='.';
                p2++;
            }
        }
        // skip spaces
        while (*p && strchr(" \r\n\t", *p))
            p++;
        if (*p) {
            value = atof(p);
            if (value!=mNODATAValue) {
                min_value=std::min(min_value, value);
                max_value=std::max(max_value, value);
            }
            mData[i*mNCols + j] = value;
            // skip text...
            while (*p && !strchr(" \r\n\t", *p))
                p++;
        } else
            j--;
    }

    return true;
}

QList<double> GISGrid::distinctValues()
{
    if (!mData)
        return QList<double>();
    QMap<double, double> temp_map;
    for (int i=0;i<mDataSize;i++)
        temp_map[mData[i]] = 1.;
    temp_map.remove(mNODATAValue);
    return temp_map.keys();
}

/*
void GISGrid::GetDistinctValues(TStringList *ResultList, double x_m, double y_m)
{
   // alle "distinct" values in einem rechteck (picus-koordinaten)
   // herauslesen. geht nur mit integers.
    double stepsize=CellSize/2; //  default stepsize, die hälfte der Cellsize, damit sollten alle pixel überstrichen werden.
    double x=0, y=0;
    int v;
    TList *List=new TList;
    while (x<=x_m) {
       y=0;
       while (y<=y_m) {
          v=value(x,y);
          if (List->IndexOf((void*)v)==-1)
             List->Add((void*)v);
          y+=stepsize;
       }
       x+=stepsize;
    }
    ResultList->Clear();
    for (int i=0;i<List->Count;i++)
       ResultList->Add(AnsiString((int)List->Items[i]));
    delete List;

}*/

/// get value of grid at index positions
double GISGrid::value(const int indexx, const int indexy) const
{
    if (indexx>=0 && indexx < mNCols && indexy>=0 && indexy<mNRows)
        return mData[indexy*mNCols + indexx];
    return -1.;  // out of scope
}

double GISGrid::value(const double X, const double Y) const
{

    QVector3D model;
    model.setX(X);
    model.setY(Y);
    model.setZ(0.);
    QVector3D world;
    modelToWorld(model, world);

    world.setX(world.x() - mOrigin.x());
    world.setY(world.y() - mOrigin.y());


    // get value out of grid.
    // double rx = Origin.x + X * xAxis.x + Y * yAxis.x;
    // double ry = Origin.y + X * xAxis.y + Y * yAxis.y;
    int ix = world.x() / mCellSize;
    int iy = world.y() / mCellSize;
    if (ix>=0 && ix<mNCols && iy>=0 && iy<mNRows) {
        double value = mData[iy*mNCols + ix];
        if (value!=mNODATAValue)
            return value;
    }
    return -1; // the ultimate NODATA- or ErrorValue
}

QVector3D GISGrid::coord(const int indexx, const int indexy) const
{
    QVector3D world((indexx+0.5)*mCellSize + mOrigin.x(),
                    (indexy+0.5)*mCellSize + mOrigin.y(),
                    0.);
    QVector3D model;
    worldToModel(world, model);
    return model;
}

QRectF GISGrid::rectangle(const int indexx, const int indexy) const
{
    QVector3D world(indexx*mCellSize + mOrigin.x(),
                    indexy*mCellSize + mOrigin.y(),
                    0.);
    QVector3D model;
    worldToModel(world, model);
    QRectF rect(model.x(), // left
                model.y(), // top
                mCellSize, // width
                mCellSize); // height
    return rect;
}

QVector3D GISGrid::coord(const int Index) const
{
    if (Index<0 || Index>=mDataSize)
        throw std::logic_error("gisgrid:coord: invalid index.");
    int ix = Index%mNCols;
    int iy = Index / mNCols;
    return coord(ix,iy);

}


/*

void GISGrid::CountOccurence(int intID, int & Count, int & left, int & upper, int &right, int &lower, QRectF *OuterBox)
{
        // zählt, wie of intID im Grid vorkommt,
        // ausserdem das rectangle, in dem es vorkommt.
        // rectangle ist durch indices [z.b. 0..NRows-1] und nicht längen definiert!
        int ix,iy;
        Count=0;
        left=100000;
        right=-1;
        upper=100000;
        lower=-1;
        QVector3D akoord;
        for (ix=0;ix<mNCols;ix++)
           for (iy=0;iy<mNRows;iy++)
               if (mData[iy*mNCols + ix]==intID) {
                    // gefunden!
                    // innerhalb der Box?
                    if (OuterBox) {
                       akoord = koord(iy*mNCols + ix);
                       if (akoord.x<OuterBox->x1 || akoord.x>OuterBox->x2 || akoord.y<OuterBox->y1 || akoord.y>OuterBox->y2)
                           continue; // nicht zählen, falls punkt ausserhalb rect.
                    }
                    Count++;
                    left=ix<left?ix:left;
                    upper=iy<upper?iy:upper;
                    right=ix>right?ix:right;
                    lower=iy>lower?iy:lower;
              }
        if (Count==0)
           left=upper=right=lower=-1; // if not found.

}
*/
/*
QVector3D GISGrid::GetNthOccurence(int ID, int N, int left, int upper, int right, int lower)
{
        // aus dem (index-)rectangle left/upper..right/lower
        // das N-te vorkommen von "ID" heraussuchen.
        // das ergebnis sind die koordinaten des mittelpunktes der grid-zelle.
        int ix,iy;
        int Counter=0;
        for (ix=left;ix<=right;ix++)
           for (iy=upper;iy<=lower;iy++)
               if (mData[iy*mNCols+ix]==ID) {
                   Counter++;
                   if (Counter==N) {  // N-tes vorkommen gefunden!!!
                       // Picus-Koordinaten zurückgeben.
                       return koord(iy*mNCols + ix);
                   }
               }
        // n-tes vorkommen nicht gefunden!!
        throw Exception("GISGrid:getNthOccurence. ID="+AnsiString(ID)+", N="+AnsiString(N)+" nicht gefunden!");
}
*/
/*
bool GISGrid::GetBoundingBox(int LookFor, QRectF &Result, double x_m, double y_m)
{
     // alle "distinct" values in einem rechteck (picus-koordinaten)
     // herauslesen. geht nur mit integers.
      double stepsize=CellSize/2; //  default stepsize, die hälfte der Cellsize, damit sollten alle pixel überstrichen werden.
      double x=0, y=0;
      int v;
      Result.x1 = 1000000; Result.x2 = -10000000;
      Result.y1 = 1000000; Result.y2 = -10000000;
      bool Found = false;
      while (x<=x_m) {
         y=0;
         while (y<=y_m) {
            v=value(x,y);
            if (v==LookFor) {
               Result.x1 = Min(Result.x1, x);
               Result.x2 = Max(Result.x2, x);
               Result.y1 = Min(Result.y1, y);
               Result.y2 = Max(Result.y2, y);
               Found = true;
            }
            y+=stepsize;
         }
         x+=stepsize;
      }
      return Found;
}
*/

void GISGrid::clip(const QRectF & box)
{
    // auf das angegebene Rechteck zuschneiden, alle
    // werte draußen auf -1 setzen.
    int ix,iy;
    QVector3D akoord;
    for (ix=0;ix<mNCols;ix++)
        for (iy=0;iy<mNRows;iy++) {
        akoord = coord(iy*mNCols + ix);
        if (!box.contains(akoord.x(), akoord.y()))
            mData[iy*mNCols + ix]=-1.;
        }

}

/// create (new!!) and return a grid that covers the full iland project area
/// and holds a list of integer keys
Grid<int> * GISGrid::create10mGrid()
{

    if (!GlobalSettings::instance()->model())
        throw IException("GisGrid::create10mGrid: no valid model to retrieve height grid.");

    HeightGrid *h_grid = GlobalSettings::instance()->model()->heightGrid();
    if (!h_grid || h_grid->isEmpty())
        throw IException("GisGrid::create10mGrid: no valid height grid to copy grid size.");
    // create a grid with the same size as the height grid
    // (height-grid: 10m size, covering the full extent)
    Grid<int> *grid = new Grid<int>(h_grid->metricRect(),h_grid->cellsize());
//    for (int *p=grid->begin();p!=grid->end;++p)
//        grid->valueAtIndex(grid->indexOf(p)) = value(grid->cellCenterPoint(grid->indexOf(p)));
    for (int i=0;i<grid->count();i++)
        grid->valueAtIndex(i) = value(grid->cellCenterPoint(grid->indexOf(i)));

    return grid;
}

/*
void GISGrid::ExportToTable(AnsiString OutFileName)
{
    TStringList *Result = new TStringList();
    AnsiString Line;
    int ix,iy;
    double Value;
    for (ix=0;ix<mNCols;ix++)
        for (iy=0;iy<mNRows;iy++) {
           Value = mData[iy*mNCols + ix];
           if (Value != mNODATAValue) {
             Line.sprintf("%d;%d;%f", ix, iy, Value);
             Result->Add(Line);
           }
        }
    Result->SaveToFile(OutFileName);
    delete Result;
}
*/
