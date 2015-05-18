/*
 *   kioskgui.h
 *
 *   Copyright (C) 2010 Petr Mrázek <peterix@gmail.com>
 *
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
#ifndef _PROFILEMANAGER_H_
#define _PROFILEMANAGER_H_

#include <QListWidget>

#include <KDE/KConfig>
#include <KDE/KXmlGuiWindow>
#include "ui_mainWindow.h"
#include <QStandardItemModel>
#include "kioskrun.h"
#include <KDE/KUser>
#include <KDE/KUserGroup>

class ProfileManager : public QObject
{
    Q_OBJECT
public:
    ProfileManager(KioskRun* krun, 
                   QStandardItemModel * _userModel,
                   QStandardItemModel * _groupModel,
                   QStandardItemModel * _profileModel);
    ~ProfileManager(){};
    bool Reload();
    bool Save();
    bool isDirty(){return bDirty;};
    bool needsSetup(){return bNeedsSetup;};

    void UpdateGroupModel( void );
    void UpdateUserModel( void );
    void UpdateProfileModel( void );

    bool PromoteGroup(int index);
    bool DemoteGroup(int index);

    bool DeleteProfile(const QString &name);

    bool ClearUserProfile(const QString & username);
    bool AssignUserProfile(const QString & username, const QString & profile);

    bool ClearGroupProfile(const QString & groupname);
    bool AssignGroupProfile(const QString & groupname, const QString & profile);
    QStringList getAllProfiles () {return allProfiles;};
    bool InitialSetup(bool do_setup);

    // ALERT: this doesn't rename the actual profile, just changes names in the lists!
    bool renameProfile(const QString & originalname, const QString & newname);

    bool hasGroupProfile(const QString &unixgroup);
    bool hasUserProfile(const QString &unixuser);
signals:
    void dirtyChanged(bool newDirty);
private:
    void setDirty(bool dirty);
    void SetupModels( void );

    // TODO: hide these properly to avoid header creep
    KioskRun::ProfileMapping userProfiles;
    KioskRun::ProfileMapping groupProfiles;
    KioskRun::ProfileMapping userGroups;
    QStringList groupOrder;
    QStringList allGroups;
    QStringList allUsers;
    QStringList allProfiles;
    QStringList deleteProfiles;

    QList<KUser> allKUsers;
    QList<KUserGroup> allKGroups;

    QStandardItemModel * userModel;
    QStandardItemModel * groupModel;
    QStandardItemModel * profileModel;
    KioskRun * krun;
    bool bDirty;
    bool bNeedsSetup;
};
#endif //_PROFILEMANAGER_H_

