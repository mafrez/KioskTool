/*
 *   kioskdata.h
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
#ifndef _KIOSKDATA_H_
#define _KIOSKDATA_H_

#include <QStringList>
#include <QList>
#include <QHash>

#include "kconfigraweditor.h"

class KConfig;
class KConfigGroup;

class ComponentAction
{
public:
   ComponentAction();
   ~ComponentAction();
   bool load(KConfigGroup *);

private:
   QString expand(const QString &);

public:
   QString caption;
   QString description;
   enum ActionType {ActImmutable, ActRestrict, ActCustom, ActModule, ActConfig, ActResource };
   ActionType type;
   QString file;
   QString group;
   QString key;
   QList<ComponentAction*> subActions;
   QString defaultValue;
   QString version;
   KConfigRawEditor::KConfigEntryData::DataType defaultValueProperties;
};
class ComponentExecData
{
public:
   void load(KConfigGroup *group);
   bool hasOption(const QString &option) { return options.contains(option); }
public:
   QString exec;
   QString dbus;
   QStringList options;
   QStringList args;
};

class ComponentData
{
public:
   ComponentData();
   ~ComponentData();
   bool load(KConfig *group);
   bool loadActions(KConfig *group);

protected:
   void loadSetup(KConfigGroup *group);

public:
   QString id;
   QString caption;
   QString description;
   QString icon;
   QList<ComponentAction*> actions;
   QStringList mutableFiles;
   QStringList ignoreFiles;
   ComponentExecData setup;
   ComponentExecData preview;
};

class KioskData
{
public:
   KioskData();
   ~KioskData();

   QString errorMsg() { return m_errorMsg; }

   bool load();

public:
   QStringList m_componentList;
   QHash<QString, ComponentData*> m_componentData;

protected:
   QString m_errorMsg;
};

#endif
