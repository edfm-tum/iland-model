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

#include <QtGui>
#include "grid.h"
#include "exception.h"
#include "global.h"

QString gridToString(const FloatGrid &grid)
{
    QString res;
    for (int y=0;y<grid.sizeY();y++) {
         for (int x=0;x<grid.sizeX();x++) {
            res+=QString::number(grid.constValueAtIndex(QPoint(x,y))) + ";";
        }
        res+="\r\n";
    }
    return res;
}

QImage gridToImage(const FloatGrid &grid,
                   bool black_white,
                   double min_value, double max_value,
                   bool reverse)
{
    QImage res(grid.sizeX(), grid.sizeY(), QImage::Format_ARGB32);
    QRgb col;
    QColor qcol;
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
