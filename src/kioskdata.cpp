/*
 *   kioskdata.cpp
 *
 *   Copyright (C) 2003, 2004 Waldo Bastian <bastian@kde.org>
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

#include "kioskdata.h"

#include <QFile>

#include <KDE/KAction>
#include <kdebug.h>
#include <KDE/KLocale>
#include <KDE/KStandardDirs>
#include <KDE/KStandardAction>
#include <KDE/KConfig>
#include <KDE/KConfigGroup>

static QHash<QString,QString> s_stdActionCaptions;

ComponentAction::ComponentAction() : defaultValueProperties(KConfigRawEditor::KConfigEntryData::Normal)
{
}

ComponentAction::~ComponentAction()
{
}

static QHash<QString,QString> readStdActionCaptions()
{
  QHash<QString,QString> captions;
  for(int i = KStandardAction::ActionNone; true;)
  {
     i++;
     KAction *action = KStandardAction::create((KStandardAction::StandardAction) i, 0, 0, 0);
     if (!action)
       break;

     QString caption = action->text();
     caption.replace("&","");

     captions[action->objectName()] = caption;
  }
  return captions;
}

QString
ComponentAction::expand(const QString &s)
{
  if (s.contains("%action"))
  {
     s_stdActionCaptions = readStdActionCaptions();

     QString action = key;
     action.replace("action/", "");
     QString caption = s_stdActionCaptions[action];
     if (!caption.isEmpty())
     {
        QString result = s;
        result.replace("%action", caption);
        return result;
     }
  }
  return s;
}

bool
ComponentAction::load(KConfigGroup *actionGroup)
{
  QString _type = actionGroup->readEntry("Type", QString() );
  if (_type == "immutable")
    type = ActImmutable;
  else if (_type == "action restriction")
    type = ActRestrict;
  else if (_type == "resource restriction")
    type = ActResource;
  else if (_type == "module")
    type = ActModule;
  else if (_type == "custom")
    type = ActCustom;
  else if (_type == "config")
    type = ActConfig;
  else
  {
#ifndef NDEBUG
    if (_type.isEmpty())
       kFatal() << "'type' attribute missing or empty in action.";
    else
       kFatal() << "Unknown 'type' attribute '" << _type << "' in action.";
#endif
    return false;
  }

  file = actionGroup->readEntry("File", QString() );
  group = actionGroup->readEntry("Group", QString() );
  key = actionGroup->readEntry("Key", QString() );
  defaultValue = actionGroup->readEntry("DefaultValue", QString() );
  caption = expand( actionGroup->readEntry("Name", QString() ) );
  description = expand( actionGroup->readEntry("Description", QString() ) );
  version = expand( actionGroup->readEntry("Version", QString() ) );
  
      /* FIXME: Port to new file format...
    else if (e.tagName() == "action")
    {
      ComponentAction *subAction = new ComponentAction;
      if (subAction->load(e))
      {
        subActions.append(subAction);
      }
      else
      {
        delete subAction;
      }
    }
*/
  return true;
}


ComponentData::ComponentData()
{
}

ComponentData::~ComponentData()
{
    qDeleteAll(actions);
}

bool ComponentData::loadActions(KConfig *groupFile )
{
  foreach( QString groupName, groupFile->groupList() )
  {
    if( groupName.startsWith("Action-") )
    {
      KConfigGroup actionGroup = groupFile->group(groupName);
      ComponentAction *action = new ComponentAction;
      if (action->load(&actionGroup) )
      {
	actions.append(action);
      }
      else
      {
	delete action;
      }
      
    }
  }
  return true;
}

void
ComponentExecData::load(KConfigGroup *actionGroup)
{
   exec = actionGroup->readEntry("Binary", QString() );
   dbus = actionGroup->readEntry("DBus", QString() );
   options = actionGroup->readEntry("Options", QString() ).split(',');
   args = actionGroup->readEntry("Args", QString() ).split(',');
}

void
ComponentData::loadSetup(KConfigGroup *setupGroup)
{
  mutableFiles = setupGroup->readEntry("Mutable", QString() ).split(',');
  ignoreFiles = setupGroup->readEntry("Ignore", QString() ).split(',');
}

bool ComponentData::load(KConfig *groupFile)
{
  id = groupFile->name();
  
  KConfigGroup groupInfo = groupFile->group("Group");
  icon = groupInfo.readEntry("Icon", "package-x-generic");
  caption = groupInfo.readEntry("Name", QString() );
  description= groupInfo.readEntry("Description", QString() );

  KConfigGroup groupSetup = groupFile->group("Setup");
  
  KConfigGroup groupPreview = groupFile->group("Preview");

  setup.load( &groupSetup );
  loadSetup( &groupSetup );
  preview.load( &groupPreview );
  loadActions( groupFile );
  
  return true;
}

KioskData::KioskData()
{
}

KioskData::~KioskData()
{
    qDeleteAll(m_componentData);
}

bool KioskData::load()
{
  foreach( QString groupFile, KGlobal::dirs()->findAllResources("data", "kiosktool/*.kiosk", KStandardDirs::NoDuplicates) )
  {   
    kDebug() << "Loading:" << groupFile;
    
    KConfig groupConfigFile(groupFile, KConfig::SimpleConfig);
    ComponentData *componentData = new ComponentData;
    if (componentData->load(&groupConfigFile))
    {
	m_componentData.insert(componentData->id, componentData);
	m_componentList.append(componentData->id);
    }
    else
    {
	delete componentData;
    }      
  }
  return true;
}
