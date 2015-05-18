/*
    This is a modal dialog that can display a model and lets the user pick one row.
    Returns a QString value that's on the intersection
    of the user-selected row and programmer-selected column.

    Copyright (C) 2010 Petr Mrázek <peterix@gmail.com>

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

#ifndef PROFILESELECTORDIALOG_H
#define PROFILESELECTORDIALOG_H

#include <KDE/KDialog>
#include "ui_ProfileSelectorDialog.h"

class QModelIndex;
class QAbstractItemModel;

class ProfileSelectorDialogUI : public QWidget, public Ui::Form
{
    public:
        ProfileSelectorDialogUI( QWidget *parent ) : QWidget( parent ) {
            setupUi( this );
        }
};

class ProfileSelectorDialog : public KDialog
{
Q_OBJECT
public:
    ProfileSelectorDialog(QWidget* parent, const QString& query, QAbstractItemModel* listModel, int pickColumn = 0);
    ~ProfileSelectorDialog();
    QString getSelectedProfile() {return selectedProfile;};
private:
    int myPickColumn;
    QString myQuery;
    QString selectedProfile;
    QAbstractItemModel * myModel;
    ProfileSelectorDialogUI *w;
private slots:
    void updateSelection(QModelIndex current ,QModelIndex previous);
};

#endif // PROFILESELECTORDIALOG_H
