/*
* ProfileManager.cpp
*
*   Copyright (C) 2010 Petr Mrázek <peterix@gmail.com>
*   Copyright (C) 2015 Jiří Holomek <jiri.holomek@gmail.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License versin 2 as
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

#include <config.h>

#include "kioskrun.h"
#include <QStandardItemModel>
#include <KDE/KDebug>
#include <QDir>
#include "ProfileManager.h"


ProfileManager::ProfileManager(KioskRun* _krun,
                               QStandardItemModel * _userModel,
                               QStandardItemModel * _groupModel,
                               QStandardItemModel * _profileModel)
                               :userModel(_userModel),
                                groupModel(_groupModel),
                                profileModel(_profileModel),
                                krun(_krun),
                                bNeedsSetup(false)
{
    SetupModels();
    if(!Reload())
    {
        bNeedsSetup = true;
    }
    else
    {
        UpdateUserModel();
        UpdateGroupModel();
        UpdateProfileModel();
    }
}

void ProfileManager::setDirty(bool dirty)
{
    if(dirty != this->bDirty)
    {
        this->bDirty = dirty;
        emit dirtyChanged(dirty);
    }
}

bool ProfileManager::InitialSetup(bool do_setup)
{
    if(do_setup)
    {
        if(!krun->setUserProfileMappings(groupProfiles,userProfiles,groupOrder))
            return false;
        bNeedsSetup = false;
    }
    Reload();
    UpdateUserModel();
    UpdateGroupModel();
    UpdateProfileModel();
    return true;
}

bool ProfileManager::Reload()
{
    userGroups.clear();
    allUsers.clear();
    deleteProfiles.clear();
    allKGroups = KUserGroup::allGroups();
    allGroups = KUserGroup::allGroupNames();
    allKUsers = KUser::allUsers();
    allProfiles = krun->allProfiles();
    foreach(KUser user, allKUsers)
    {
        QString username = user.loginName();
        allUsers.push_back(username);
        QStringList groups = user.groupNames();

        // add main group if needed
        KUserGroup maingr(user.gid());
        if(!groups.contains(maingr.name()))
            groups.append(maingr.name());

        userGroups[username] = groups;
    }
    setDirty(false);
    if(!krun->getUserProfileMappings(groupProfiles,userProfiles,groupOrder))
    {
        return false;
    }
    return true;
}

bool ProfileManager::hasGroupProfile(const QString& unixgroup)
{
    if(groupProfiles.contains(unixgroup))
    {
        if(!groupProfiles[unixgroup].isEmpty())
        {
            return true;
        }
    }
    return false;
}

bool ProfileManager::hasUserProfile(const QString& unixuser)
{
    if(userProfiles.contains(unixuser))
    {
        if(!userProfiles[unixuser].isEmpty())
        {
            return true;
        }
    }
    return false;
}


bool ProfileManager::Save()
{
    // nothing to save
    if(!bDirty)
        return true;

    foreach(QString profile, deleteProfiles)
    {
        KioskRun::self()->deleteProfile(profile);
    }
    deleteProfiles.clear();
    bool done = krun->setUserProfileMappings(groupProfiles,userProfiles,groupOrder);
    setDirty(!done);
    return done;
}

void ProfileManager::SetupModels(void )
{
    // set up headers and columns
    userModel->setColumnCount(5);
    userModel->setHeaderData(0, Qt::Horizontal, i18n("UID"));
    userModel->setHeaderData(1, Qt::Horizontal, i18n("User login"));
    userModel->setHeaderData(2, Qt::Horizontal, i18n("User profile"));
    userModel->setHeaderData(3, Qt::Horizontal, i18n("Group profile"));
    userModel->setHeaderData(4, Qt::Horizontal, i18n("Effective profile"));

    groupModel->setColumnCount(4);
    groupModel->setHeaderData(0, Qt::Horizontal, i18n("Order"));
    groupModel->setHeaderData(1, Qt::Horizontal, i18n("GID"));
    groupModel->setHeaderData(2, Qt::Horizontal, i18n("Group"));
    groupModel->setHeaderData(3, Qt::Horizontal, i18n("Group profiles"));

    profileModel->setColumnCount(4);
    profileModel->setHeaderData(0, Qt::Horizontal, i18n("Profile name"));
    profileModel->setHeaderData(1, Qt::Horizontal, i18n("Owner"));
    profileModel->setHeaderData(2, Qt::Horizontal, i18n("Path"));
    profileModel->setHeaderData(3, Qt::Horizontal, i18n("Description"));
}


void ProfileManager::UpdateUserModel( void )
{
    userModel->removeRows(0, userModel->rowCount(QModelIndex()));
    int row = 0;

    foreach(KUser user, allKUsers)
    {
        // get the data
        QString username = user.loginName();
        QString profile;
        if(userProfiles.contains(username))
        {
            profile = userProfiles[username].join(", ");
        }
        QStringList & uGroups = userGroups[username];
        QStringList filtered;
        foreach(QString mappedGroup, groupOrder)
        {
            if(uGroups.contains(mappedGroup))
            {
                if(groupProfiles.contains(mappedGroup))
                {
                    QStringList profNames = groupProfiles[mappedGroup];
                    filtered.append(profNames.join(", "));
                }
            }
        }
        QString joinedGrProfs = filtered.join(", ");

        // feed it to the model
        userModel->insertRows(row, 1, QModelIndex());
        userModel->setData(userModel->index(row, 0), user.uid(), Qt::DisplayRole);
        userModel->setData(userModel->index(row, 1), username, Qt::DisplayRole);
        userModel->setData(userModel->index(row, 2), profile, Qt::DisplayRole);
        userModel->setData(userModel->index(row, 3), joinedGrProfs, Qt::DisplayRole);

        // effective profiles column
        if(!profile.isEmpty())
            userModel->setData(userModel->index(row, 4), profile, Qt::DisplayRole);
        else if(!joinedGrProfs.isEmpty())
            userModel->setData(userModel->index(row, 4), joinedGrProfs, Qt::DisplayRole);
        else
            userModel->setData(userModel->index(row, 4), "default", Qt::DisplayRole);
        row++;
    }
}

struct tempStorage
{
    tempStorage(const QString & _groupname,const QString & _profiles,K_GID _gid):
        groupname(_groupname), profiles(_profiles), gid(_gid){};
    tempStorage(){};
    QString groupname;
    QString profiles;
    K_GID gid;
};

void ProfileManager::UpdateGroupModel( void )
{
    groupModel->removeRows(0, groupModel->rowCount(QModelIndex()));
    QVector < tempStorage > groupAssign(allKGroups.size());

    // we put ordered groups first and unordered groups after them
    int unassignedStart = groupOrder.size();
    int unassignedIdx = unassignedStart;

    // construct a vector of all entries first
    foreach(KUserGroup group, allKGroups)
    {
        // get the data
        QString groupname = group.name();
        QString profiles;
        if(groupOrder.contains(groupname))
        {
            int index = groupOrder.indexOf(groupname);
            if(groupProfiles.contains(groupname))
            {
                profiles = groupProfiles[groupname].join(", ");
            }
            groupAssign[index] = tempStorage(groupname, profiles, group.gid());
        }
        else
        {
            groupAssign[unassignedIdx] = tempStorage(groupname, QString(), group.gid() );
            unassignedIdx ++;
        }
    }

    // feed it to the model
    int row = 0;
    foreach (tempStorage temp , groupAssign)
    {
        groupModel->insertRows(row, 1, QModelIndex());
        if (row < unassignedStart)
        {
            groupModel->setData(groupModel->index(row, 0), row + 1, Qt::DisplayRole);
        }
        groupModel->setData(groupModel->index(row, 1), temp.gid, Qt::DisplayRole);
        groupModel->setData(groupModel->index(row, 2), temp.groupname, Qt::DisplayRole);
        groupModel->setData(groupModel->index(row, 3), temp.profiles, Qt::DisplayRole);
        row++;
    }
}

void ProfileManager::UpdateProfileModel( void )
{
    profileModel->removeRows(0, profileModel->rowCount(QModelIndex()));
    int row = 0;
    // just feed the model all the profiles
    foreach(QString profile,allProfiles)
    {
        QString path;
        QString owner;
        QString description;
        krun->getProfileInfo(profile,description,path,owner);
        profileModel->insertRows(row, 1, QModelIndex());
        profileModel->setData(profileModel->index(row, 0), profile, Qt::DisplayRole);
        profileModel->setData(profileModel->index(row, 1), owner, Qt::DisplayRole);
        profileModel->setData(profileModel->index(row, 2), path, Qt::DisplayRole);
        profileModel->setData(profileModel->index(row, 3), description, Qt::DisplayRole);
        row++;
    }
}

bool ProfileManager::PromoteGroup(int index)
{
    if(index > 0)
    {
        groupOrder.swap(index, index - 1);
        UpdateGroupModel();
        UpdateUserModel();
        setDirty(true);
        return true;
    }
    return false;
}

bool ProfileManager::DemoteGroup(int index)
{
    if(index < groupOrder.size() - 1)
    {
        groupOrder.swap(index, index + 1);
        UpdateGroupModel();
        UpdateUserModel();
        setDirty(true);
        return true;
    }
    return false;
}

bool ProfileManager::DeleteProfile(const QString& name)
{
    // mark the profile to be deleted on save()
    deleteProfiles.append(name);

    // remove the profile from the user -> profiles mapping
    KioskRun::ProfileMapping::iterator i = userProfiles.begin();
    while (i != userProfiles.end())
    {
        QString username = i.key();
        QStringList &List =userProfiles[i.key()];
        List.removeAll(name);
        ++i;
    }

    // remove the profile from the group -> profiles mapping
    i = groupProfiles.begin();
    while (i != groupProfiles.end())
    {
        QString groupname = i.key();
        QStringList &List = groupProfiles[i.key()];
        List.removeAll(name);
        if(List.isEmpty())
        {
            groupOrder.removeAll(groupname);
        }
        ++i;
    }
    // remove the profile from the list of all profile name
    allProfiles.removeAll(name);

    // update everything
    UpdateGroupModel();
    UpdateUserModel();
    UpdateProfileModel();
    setDirty(true);
    return true;
}

bool ProfileManager::ClearGroupProfile(const QString& groupname)
{
    if(groupProfiles.contains(groupname))
    {
        groupProfiles.remove(groupname);
    }
    groupOrder.removeAll(groupname);
    UpdateGroupModel();
    UpdateUserModel();
    setDirty(true);
    return true;
}

bool ProfileManager::AssignGroupProfile(const QString& groupname, const QString& profile)
{
    if(profile.isEmpty())
        return ClearGroupProfile(groupname);
    groupProfiles[groupname] = QStringList(profile);
    if(!groupOrder.contains(groupname))
    {
        groupOrder.append(groupname);
    }
    UpdateGroupModel();
    UpdateUserModel();
    setDirty(true);
    return true;
}

bool ProfileManager::ClearUserProfile(const QString& username)
{
    if(userProfiles.contains(username))
    {
        userProfiles.remove(username);
    }
    UpdateUserModel();
    setDirty(true);
    return true;
}

bool ProfileManager::AssignUserProfile(const QString& username, const QString& profile)
{
    // already assigned
    if(userProfiles.contains(username))
    {
        if(userProfiles[username] == QStringList(profile))
            return false;
    }
    userProfiles[username] = QStringList(profile);
    UpdateUserModel();
    setDirty(true);
    return true;
}

bool ProfileManager::renameProfile(const QString& originalname, const QString& newname)
{
    //qDebug() << "renaming" << originalname << "to" << newname;
    // replace originalname with newname
    KioskRun::ProfileMapping::iterator i = userProfiles.begin();
    while (i != userProfiles.end())
    {
        QString username = i.key();
        QStringList &List =userProfiles[i.key()];
        for (int j = 0; j < List.size(); j++)
        {
            if(List.at(j) == originalname)
            {
                List[j] = newname;
                // qDebug() << "replaced";
            }
        }
        ++i;
    }

    // replace originalname with newname
    i = groupProfiles.begin();
    while (i != groupProfiles.end())
    {
        QString groupname = i.key();
        QStringList &List = groupProfiles[i.key()];
        for (int j = 0; j < List.size(); j++)
        {
            if(List.at(j) == originalname)
            {
                List[j] = newname;
                qDebug() << "replaced";
            }
        }
        ++i;
    }
    bDirty = true;
    return Save();
}
