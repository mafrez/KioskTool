/*
 *   profilePropsDialog.h
 *
 *   Copyright (C) 2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
 *   Copyright (C) 2010 Petr Mrázek <peterix@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.
 */
#ifndef _PROFILEPROPSDIALOG_H_
#define _PROFILEPROPSDIALOG_H_

#include <KDE/KDialog>
#include "ui_profilePropsDialog.h"

class ProfilePropsUI : public QWidget, public Ui::ProfilePropsDialog
{
public:
    ProfilePropsUI( QWidget *parent ) : QWidget( parent )
    {
        setupUi( this );
    }
};


class ProfilePropsDialog : public KDialog
{
    Q_OBJECT
public:
    ProfilePropsDialog(QWidget* parent, const QString& profile, QStringList allProfiles);
    ~ProfilePropsDialog();

    bool save(QString& out);
protected slots:
    void slotProfileNameChanged();

protected:
    void init();

private:
    ProfilePropsUI *w;
    QString m_profile;
    QStringList m_allProfiles;
    bool m_fixedProfileDir;
    QString m_origProfile;
    QString m_origInstallDir;
};

#endif
