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


#ifndef UIDFILTERMODEL_H
#define UIDFILTERMODEL_H
#include <QSortFilterProxyModel>
#include <KDE/KUser>

class ProfileManager;

class UIDFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    UIDFilterModel(QObject* parent, ProfileManager * _prof_man, K_UID _UID = 0);

    K_UID filterMinimumUID() const { return minUID; }
    void setFilterMinimumUID(const K_UID &uid);
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    K_UID minUID;
    ProfileManager* prof_man;
};

#endif // UIDFILTERMODEL_H
