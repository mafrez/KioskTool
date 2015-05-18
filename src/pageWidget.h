/*
 *   pageWidget.h
 *
 *   Copyright (C) 2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
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
#ifndef _PAGEWIDGET_H_
#define _PAGEWIDGET_H_

#include <KDE/KListWidget>

class ComponentAction;
class ComponentData;

class ComponentActionItem : public QListWidgetItem
{
public:
   ComponentActionItem( QListWidget * parent, ComponentAction *action, int index);

   ComponentAction *action() const { return m_action; }

   virtual int compare ( QListWidgetItem * i, int col, bool ascending ) const;
private:

   ComponentAction *m_action;
   int m_index;
};

class PageWidget
{
public:
   PageWidget(const QString &profileName, QWidget *me);
   virtual ~PageWidget();

   QWidget *widget() const { return m_widget; }

   void fillActionList(KListWidget *listView, ComponentData *componentData);
   bool saveActionListChanges(KListWidget *listView);

   virtual void load() = 0;
   virtual bool save() = 0;

   virtual void setFocus() = 0;

   virtual QString subCaption() = 0;

protected:
   void saveActionListItem(ComponentAction *action, bool b);

private:

   QWidget *m_widget;
   QString m_profileName;
};

#endif
