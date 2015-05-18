/*
 *   This is a modal dialog that can display a model and lets the user pick one row.
 *   Returns a QString value that's on the intersection
 *   of the user-selected row and programmer-selected column.
 *
 *   Copyright (C) 2010 Petr Mrázek <peterix@gmail.com>
 *   Copyright (C) 2015 Jiří Holomek <jiri.holomek@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "ProfileSelectorDialog.h"

ProfileSelectorDialog::ProfileSelectorDialog(QWidget* parent, const QString& query, QAbstractItemModel * listModel, int pickColumn)
: KDialog( parent ), myPickColumn(pickColumn), myQuery(query), myModel(listModel)
{
    setCaption( query );
    setButtons( KDialog::Ok|KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    enableButton(KDialog::Ok, false);
    setModal( true );
    w = new ProfileSelectorDialogUI(this);
    setMainWidget(w);
    w->profileList->setModel(listModel);
    w->label->setText(query);
    connect(w->profileList->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),this,SLOT(updateSelection(QModelIndex ,QModelIndex)));
}

ProfileSelectorDialog::~ProfileSelectorDialog()
{
    // QUACK!
}

void ProfileSelectorDialog::updateSelection(QModelIndex current ,QModelIndex previous)
{
    Q_UNUSED(previous);
    if(current.isValid())
    {
        selectedProfile = myModel->data(myModel->index(current.row(), myPickColumn)).toString();
        enableButton(KDialog::Ok, true);
    }
    else
    {
        selectedProfile = QString();
        enableButton(KDialog::Ok, false);
    }
}

#include "ProfileSelectorDialog.moc"