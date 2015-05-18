/*
 *   panelComponent.cpp
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

#include "panelComponent.h"

#include <QDir>
#include <QFileInfo>

#include <kdebug.h>
#include <kmimetype.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include "kioskrun.h"

PanelComponent::PanelComponent( QObject *parent)
 : Component(parent)
{
}

PanelComponent::~PanelComponent()
{
}

void
PanelComponent::slotSetupPrepare()
{
}

void
PanelComponent::slotSetupStarted()
{
}

bool
PanelComponent::setupFinished()
{
   bool result;

   // Install .desktop files
   {
      QString kickerApplications = KioskRun::self()->locateLocal("data", "kicker/");
      QString kickerSaveApplications = KioskRun::self()->locateSave("data", "kicker/");

      QDir dir(kickerApplications);
      QStringList newKickerApplications = dir.entryList(QDir::TypeMask|QDir::NoDotAndDotDot, QDir::Unsorted);

      foreach( QString newKickerApplication, newKickerApplications)
      {
         if (newKickerApplication.endsWith(".desktop"))
         {
            kDebug() << "PanelComponent: New .desktop file = " << (kickerApplications + newKickerApplication);
            result = KioskRun::self()->install(kickerApplications + newKickerApplication, kickerSaveApplications + newKickerApplication);
            if (!result) return false;
         }
      }
   }
   return true;
}

#include "panelComponent.moc"
