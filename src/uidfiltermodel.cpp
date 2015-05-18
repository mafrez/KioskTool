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


#include "uidfiltermodel.h"
#include <QtGui>
#include "ProfileManager.h"

UIDFilterModel::UIDFilterModel(QObject* parent, ProfileManager * _prof_man, K_UID _UID):
    QSortFilterProxyModel(parent)
{
    minUID = _UID;
    prof_man = _prof_man;
    invalidateFilter();
}

bool UIDFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    // pass through users with UID >= minUID and the root user
    QModelIndex indexUID = sourceModel()->index(sourceRow, 0, sourceParent);
    K_UID UID = sourceModel()->data(indexUID).toUInt();
    if(UID == 0 || UID >= minUID)
        return true;

    // always show users who have an user profile assigned
    QModelIndex indexName = sourceModel()->index(sourceRow, 1, sourceParent);
    QString Name = sourceModel()->data(indexName).toString();
    if(prof_man->hasUserProfile(Name))
        return true;

    // hide the rest
    return false;
}

void UIDFilterModel::setFilterMinimumUID(const K_UID& uid)
{
    minUID = uid;
    invalidateFilter();
}
