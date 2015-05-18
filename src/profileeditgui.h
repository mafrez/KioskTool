/*
 *   profileeditgui.h
 *
 *   Copyright (C) 2003,2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
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
#ifndef _PROFILEEDITGUI_H_
#define _PROFILEEDITGUI_H_

#include <QListWidget>

#include <KDE/KConfig>
#include <KDE/KXmlGuiWindow>
#include "ui_editorDialog.h"

class ComponentSelectionPage;
class ProfilePropsPage;
class PageWidget;
class UserManagementPage;
class KioskData;
class KioskRun;
class ComponentData;
class KAction;
class KToggleAction;


class MainView : public QWidget, public Ui::MainView
{
public:
  MainView( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class ProfileEditGui : public KDialog
{
   Q_OBJECT
public:
   enum Page {
          PAGE_ENTRY = 0,
          PAGE_COMPONENT_SELECTION = 1,
          PAGE_COMPONENT = 2,
          PAGE_LAST = PAGE_COMPONENT };

   ProfileEditGui(QWidget* parent, const QString& profile, KioskRun* _krun);
   ~ProfileEditGui();

   void selectPage(Page page);
   void loadPage(Page page);
   bool savePage(Page page);

   void loadProfiles();

public slots:
   void updateButtons();
   void finishedPage(bool save=true);
   void discardPage();
   void slotComponentSelection();
   void help();

protected:
   void updateActions();
   void setSubCaption(const QString &caption);

private:
   MainView *m_view;
   ComponentSelectionPage *m_componentSelectionPage;
   PageWidget *m_componentPage;

   KioskData *m_data;
   KioskRun *m_run;

   QString m_profile;
   QString m_component;
   ComponentData * m_componentData;
   KToggleAction *m_backgroundAction;
   QMap<Page,int> m_pageMapping;
   Page m_page;
};

#endif
