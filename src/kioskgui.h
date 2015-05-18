/*
 *   kioskgui.h
 *
 *   Copyright (C) 2003,2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
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
#ifndef _KIOSKGUI_H_
#define _KIOSKGUI_H_

#include <QListWidget>

#include <KDE/KConfig>
#include <KDE/KXmlGuiWindow>
#include "ui_mainWindow.h"
#include <QStandardItemModel>
#include "uidfiltermodel.h"
#include "gidfiltermodel.h"

class ComponentSelectionPage;
class ProfilePropsPage;
class PageWidget;
class UserManagementPage;
class KioskData;
class KioskRun;
class ComponentData;
class KAction;
class KToggleAction;
class ProfileManager;


class MainWidget : public QWidget, public Ui::MainWidget
{
public:
    MainWidget( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class KioskGui : public KXmlGuiWindow
{
   Q_OBJECT
public:
   KioskGui();
   ~KioskGui();

   void setupActions();
   void saveProperties(KConfigGroup &config);
   void readProperties(const KConfigGroup &config);
   void loadProfiles();

public slots:
   void slotDeleteProfile();
   void slotCreateProfile();
   void slotEditProfile();
   void slotProfileProperties();

   void slotAssignGroupProfile();
   void slotClearGroupProfile();
   void slotPromoteGroup();
   void slotDemoteGroup();
   void slotCopyGroupProfile();
   void slotPasteGroupProfile();

   void slotAssignUserProfile();
   void slotClearUserProfile();
   void slotCopyUserProfile();
   void slotPasteUserProfile();

   void slotCommit();
   void slotDiscard();

   void slotDirtyChanged(bool dirty);

   void slotPKLAConvert();

   void slotConfig();

   void uploadAllProfiles();
   void uploadCurrentProfile();

   void slotCheckEtcSkel();
   void help();
   void toggle_gid_filter(bool);
   void toggle_uid_filter(bool);

protected:
   void updateActions();
   void updateFilters();
   virtual bool queryClose();

private:
   MainWidget *m_widget;
   KioskData *m_data;
   KioskRun *m_run;

   ProfileManager * m_profmanager;
   QStandardItemModel * userModel;
   UIDFilterModel * userFilter;
   GIDFilterModel * groupFilter;
   QStandardItemModel * groupModel;
   QStandardItemModel * profileModel;

   QString m_profile;
   QString m_component;
   ComponentData * m_componentData;
   KAction *m_uploadAction;
   KAction *m_convertAction;
   KAction* uidFilterAction;
   KAction* gidFilterAction;
   bool m_emergencyExit;
};

#endif
