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
#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <QString>

class Snapshot
{
public:
    Snapshot();
    bool createSnapshot(const QString &file_name);
    bool loadSnapshot(const QString &file_name);
private:
    bool openDatabase(const QString &file_name, const bool read);
    void saveTrees();
    void saveSoil();
    void saveSnags();
    void loadTrees();
    void loadSoil();
    void loadSnags();
};

#endif // SNAPSHOT_H
