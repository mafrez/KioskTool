/*
 *   pageWidget.cpp
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

#include "pageWidget.h"

#include <KDE/KConfig>
#include <KDE/KListWidget>

#include "kioskdata.h"
#include "kioskrun.h"
#include "kconfigraweditor.h"

ComponentActionItem::ComponentActionItem( QListWidget * parent, ComponentAction *action, int index)
 : QListWidgetItem(action->caption , parent),
   m_action(action), m_index(index)
{

}

int ComponentActionItem::compare ( QListWidgetItem * i, int, bool ) const
{
   ComponentActionItem *cai = static_cast<ComponentActionItem*>(i);
   if (m_index == cai->m_index)
      return 0;
   if (m_index < cai->m_index)
      return -1;
   return 1;
}

PageWidget::PageWidget(const QString &profileName, QWidget *me): m_widget(me), m_profileName(profileName)
{

}

PageWidget::~PageWidget()
{
}

void
PageWidget::fillActionList(KListWidget *listView, ComponentData *componentData)
{
  int index = 0;
  foreach( ComponentAction *action, componentData->actions )
  {
     //TODO: Check KDE version here vs the component action version
     ComponentActionItem *item = new ComponentActionItem(listView, action, index++);
     if (index == 1)
        item->setSelected(true);
     if (action->type == ComponentAction::ActRestrict)
     {
        QString file = action->file;
        if (file.isEmpty())
           file = "kdeglobals";
        KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
        KConfigRawEditor::KConfigGroupData *grp = cfg->group("KDE Action Restrictions");
        bool restricted = !grp->readEntry(action->key, true).toBool();
        if( restricted )
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
     }
     else if (action->type == ComponentAction::ActResource)
     {
        QString file = action->file;
        if (file.isEmpty())
           file = "kdeglobals";
        KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
        KConfigRawEditor::KConfigGroupData *grp = cfg->group("KDE Resource Restrictions");
        bool restricted = !grp->readEntry(action->key, true).toBool();
        if( restricted )
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
     }
     else if (action->type == ComponentAction::ActModule)
     {
        QString file = "kdeglobals";
        KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
        KConfigRawEditor::KConfigGroupData *grp = cfg->group("KDE Control Module Restrictions");
        bool restricted = !grp->readEntry(action->key, true).toBool();
        if( restricted )
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
     }
     else if (action->type == ComponentAction::ActImmutable)
     {
        QString file = action->file;
        if (file.isEmpty())
           file = "kdeglobals";
        QString group = action->group;
        bool immutable = KioskRun::self()->isConfigImmutable(m_profileName, file, group);
        if( immutable )
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
     }
     else if (action->type == ComponentAction::ActCustom)
     {
        bool checked = KioskRun::self()->lookupCustomAction(m_profileName, action->key);
        if( checked )
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
     }
     else if (action->type == ComponentAction::ActConfig)
     {
        QString file = action->file;
        if (file.isEmpty())
           file = "kdeglobals";
        KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
        KConfigRawEditor::KConfigGroupData *grp = cfg->group(action->group);

        if( !grp->readEntry(action->key, QString() ).toString().isEmpty() )
        {
	    item->setCheckState(Qt::Checked);
	    action->defaultValue = grp->readEntry(action->key, QString() ).toString();
            action->defaultValueProperties = grp->data[action->key].type;
        }
	else
            item->setCheckState(Qt::Unchecked);
     }
  }

}

void
PageWidget::saveActionListItem(ComponentAction *action, bool b)
{
  if (action->type == ComponentAction::ActRestrict)
  {
     QString file = action->file;
     if (file.isEmpty())
        file = "kdeglobals";
     KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
     KConfigRawEditor::KConfigGroupData *grp = cfg->group("KDE Action Restrictions");

     bool allowed = !b; // reverse logic
     if (grp->readEntry(action->key, true).toBool() != allowed)
     {
        grp->writeEntry(action->key, allowed);
        KioskRun::self()->scheduleSycocaUpdate();
     }
  }
  else if (action->type == ComponentAction::ActResource)
  {
     QString file = action->file;
     if (file.isEmpty())
        file = "kdeglobals";
     KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
     KConfigRawEditor::KConfigGroupData *grp = cfg->group("KDE Resource Restrictions");

     bool allowed = !b; // reverse logic
     if (grp->readEntry(action->key, true).toBool() != allowed)
     {
        grp->writeEntry(action->key, allowed);
        KioskRun::self()->scheduleSycocaUpdate();
     }
  }
  else if (action->type == ComponentAction::ActModule)
  {
     QString file = "kdeglobals";
     KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
     KConfigRawEditor::KConfigGroupData *grp = cfg->group("KDE Control Module Restrictions");

     bool allowed = !b; // reverse logic
     if (grp->readEntry(action->key, true).toBool() != allowed)
     {
        grp->writeEntry(action->key, allowed);
        KioskRun::self()->scheduleSycocaUpdate();
     }
  }
  else if (action->type == ComponentAction::ActImmutable)
  {
     QString file = action->file;
     if (file.isEmpty())
        file = "kdeglobals";
     QString group = action->group;
     KioskRun::self()->setConfigImmutable(m_profileName, file, group, b);
  }
  else if (action->type == ComponentAction::ActCustom)
  {
     KioskRun::self()->setCustomAction(m_profileName, action->key, b);
  }
  else if (action->type == ComponentAction::ActConfig)
  {
     QString file = action->file;
     if (file.isEmpty())
        file = "kdeglobals";
     KConfigRawEditor *cfg = KioskRun::self()->config(m_profileName, file);
     KConfigRawEditor::KConfigGroupData *grp = cfg->group(action->group);

     if( action->defaultValue.isEmpty() )
        grp->deleteEntry( action->key, action->defaultValueProperties );
     else
        grp->writeEntry(action->key, action->defaultValue, action->defaultValueProperties );
        KioskRun::self()->scheduleSycocaUpdate();

  }

  foreach( ComponentAction *subAction,  action->subActions )
  {
     saveActionListItem(subAction, b);
  }
}

bool
PageWidget::saveActionListChanges(KListWidget *listView)
{
  for( int idx = 0; idx < listView->count(); ++idx )
  {
     ComponentActionItem *item = static_cast<ComponentActionItem*>(listView->item(idx));
     saveActionListItem(item->action(), item->checkState() == Qt::Checked );
  }
  return true;
}
