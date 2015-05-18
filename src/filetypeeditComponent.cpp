/*
 *   filetypeeditComponent.cpp
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

#include "filetypeeditComponent.h"

#include <QDir>
#include <QDomElement>
#include <QFileInfo>

#include <KDE/KApplication>
#include <kdebug.h>
#include <KDE/KMimeType>
#include <KDE/KSaveFile>
#include <ksimpleconfig.h>
#include <KDE/KStandardDirs>
#include <KDE/KUrl>

#include "kioskrun.h"
#include "kiosksync.h"

FileTypeEditComponent::FileTypeEditComponent( QObject *parent)
 : Component(parent)
{
}

FileTypeEditComponent::~FileTypeEditComponent()
{
}

void
FileTypeEditComponent::slotSetupPrepare()
{
}

void
FileTypeEditComponent::slotSetupStarted()
{
}

bool
FileTypeEditComponent::setupFinished()
{
   bool result;

   // Install mimetype files
   {
      QString mimetypeFiles = KioskRun::self()->locateLocal("mime", QString());
      QString mimetypeSaveFiles = KioskRun::self()->locateSave("mime", QString());

      KioskSync mimeDir(KApplication::activeWindow());
      mimeDir.addDir(mimetypeFiles, KUrl());

      QStringList newMimetypeFiles = mimeDir.listFiles();

      for(QStringList::ConstIterator it = newMimetypeFiles.constBegin();
          it != newMimetypeFiles.constEnd(); ++it)
      {
         if ((*it).endsWith(".desktop"))
         {
            kDebug() << "FileTypeEditComponent: New mimetype file %s" << (mimetypeFiles+(*it));
            result = KioskRun::self()->install(mimetypeFiles+(*it), mimetypeSaveFiles+(*it));
            if (!result) return false;
         }
      }
   }

   // Install legacy .desktop files
   {
      QString legacyApplications = KioskRun::self()->locateLocal("apps", QString());
      QString legacySaveApplications = KioskRun::self()->locateSave("apps", QString());

      KioskSync legacyDir( KApplication::activeWindow() );
      legacyDir.addDir(legacyApplications, KUrl());

      QStringList newLegacyApplications = legacyDir.listFiles();

      for(QStringList::ConstIterator it = newLegacyApplications.constBegin();
          it != newLegacyApplications.constEnd(); ++it)
      {
         if ((*it).endsWith(".desktop") || (*it).endsWith(".kdelnk") || (*it).endsWith(".directory"))
         {
             kDebug() << "MenueditComponent: New legacy file %s" << (legacyApplications+(*it));
            result = KioskRun::self()->install(legacyApplications+(*it), legacySaveApplications+(*it));
            if (!result) return false;
         }
      }
   }

   // Install .desktop files
   {
      QString xdgApplications = KioskRun::self()->locateLocal("xdgdata-apps", QString());
      QString xdgSaveApplications = KioskRun::self()->locateSave("xdgdata-apps", QString());

      QDir dir(xdgApplications);
      QStringList newXdgApplications = dir.entryList(QDir::TypeMask|QDir::NoDotAndDotDot, QDir::Unsorted);

      for(QStringList::ConstIterator it = newXdgApplications.constBegin();
          it != newXdgApplications.constEnd(); ++it)
      {
         if ((*it).endsWith(".desktop") || (*it).endsWith(".kdelnk"))
         {
            kDebug() << "MenueditComponent: New .desktop file %s" << (xdgApplications+(*it));
            result = KioskRun::self()->install(xdgApplications+(*it), xdgSaveApplications+(*it));
            if (!result) return false;
         }
      }
   }

   KioskRun::self()->forceSycocaUpdate();

   return true;
}

#include "filetypeeditComponent.moc"
