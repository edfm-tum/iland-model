//#include <QtGui>
#include "grid.h"
#include "exception.h"
#include "global.h"

QString gridToString(const FloatGrid &grid, const QChar sep, const int newline_after)
{
    QString res;
    int newl_counter = newline_after;
    for (int y=grid.sizeY()-1;y>=0;--y) {
         for (int x=0;x<grid.sizeX();x++) {
            res+=QString::number(grid.constValueAtIndex(QPoint(x,y))) + sep;
            if (--newl_counter==0) {
                res += "\r\n";
                newl_counter = newline_after;
            }
        }
        res+="\r\n";
    }
    return res;
}
#ifdef ILAND_GUI
#incldue <QImage>

QImage gridToImage(const FloatGrid &grid,
                   bool black_white,
                   double min_value, double max_value,
                   bool reverse)
{
    QImage res(grid.sizeX(), grid.sizeY(), QImage::Format_ARGB32);
    QRgb col;
    int grey;
    double rval;
    for (int x=0;x<grid.sizeX();x++){
        for (int y=0;y<grid.sizeY();y++) {
            rval = grid.constValueAtIndex(QPoint(x,y));
            rval = std::max(min_value, rval);
            rval = std::min(max_value, rval);
            if (reverse) rval = max_value - rval;
            if (black_white) {
                grey = int(255 * ( (rval-min_value) / (max_value-min_value)));
                col = QColor(grey,grey,grey).rgb();
            } else {
                col = QColor::fromHsvF(0.66666666666*rval, 0.95, 0.95).rgb();
            }
            res.setPixel(x,y,col);
            //res+=QString::number(grid.constValueAtIndex(QPoint(x,y))) + ";";
        }
        //res+="\r\n";
    }
    return res;
}



bool loadGridFromImage(const QString &fileName, FloatGrid &rGrid)
{
    QImage image;
    if (!image.load(fileName))
        throw IException(QString("Grid::loadFromImage: could not load image file %1.").arg(fileName));
    if (rGrid.isEmpty())
        rGrid.setup(1., image.size().width(), image.size().height() );
    double value;
    for (int x=0;x<image.width(); x++)
        for (int y=0;y<image.height(); y++) {
            value = qGray(image.pixel(x,y))/255.;
            if (rGrid.isIndexValid(QPoint(x,y)))
                rGrid.valueAtIndex(x,y) = value;
        }
    return true;
}
#else
//QImage gridToImage(const FloatGrid &grid,
//                   bool black_white,
//                   double min_value, double max_value,
//                   bool reverse)
//{
//}

bool loadGridFromImage(const QString &fileName, FloatGrid &rGrid) {
    return false;
}
#endif

