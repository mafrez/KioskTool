/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "gidfiltermodel.h"
#include <QtGui>
#include "ProfileManager.h"

GIDFilterModel::GIDFilterModel(QObject* parent, ProfileManager * _prof_man, K_GID _UID):
    QSortFilterProxyModel(parent)
{
    minGID = _UID;
    prof_man = _prof_man;
    invalidateFilter();
}

bool GIDFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    // pass through groups with GID >= minGID and the root group
    QModelIndex indexGID = sourceModel()->index(sourceRow, 1, sourceParent);
    K_GID GID = sourceModel()->data(indexGID).toUInt();
    if(GID == 0 || GID >= minGID)
        return true;

    // always show groups that have a group profile assigned
    QModelIndex indexName = sourceModel()->index(sourceRow, 2, sourceParent);
    QString Name = sourceModel()->data(indexName).toString();
    if(prof_man->hasGroupProfile(Name))
        return true;

    // hide the rest
    return false;
}

void GIDFilterModel::setFilterMinimumGID(const K_GID& gid)
{
    minGID = gid;
    invalidateFilter();
}
