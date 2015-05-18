/*
 *   desktopComponent.cpp
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

#include "desktopComponent.h"
#include <unistd.h>
#include <QDir>
#include <QFileInfo>

#include <kdebug.h>
#include <KDE/KMimeType>
#include <KDE/KProcess>
#include <ksimpleconfig.h>
#include <KDE/KStandardDirs>
#include <KDE/KTemporaryFile>
#include <KDE/KDesktopFile>
#include <KDE/KUrl>

#include "kioskrun.h"

DesktopComponent::DesktopComponent( QObject *parent)
 : Component(parent)
{
}

DesktopComponent::~DesktopComponent()
{
}

void
DesktopComponent::slotSetupPrepare()
{
   m_iconPositionsFile = KioskRun::self()->locateLocal("data", "kdesktop/IconPositions");
   ::unlink(QFile::encodeName(m_iconPositionsFile));
   connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotSetupStarted()));
}

void
DesktopComponent::slotSetupStarted()
{
   QString desktop = KioskRun::self()->desktopPath();
   QFileInfo info(desktop);
   if (info.exists())
   {
      disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotSetupStarted()));
      connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotSetupReady()));
      m_timer.start(1000);
   }
   else
   {
      m_timer.start(500);
   }
}

void filterFileList(const QString &path, QStringList *files, QStringList *oldFiles)
{
   foreach( QString file, *files)
   {
      KUrl u;
      u.setPath( path + file);

      KMimeType::Ptr mime = KMimeType::findByUrl(u, 0, true);
      if (mime->name() == "application/x-desktop")
      {
         KDesktopFile cfg(path + file);
         KConfigGroup cg(cfg.desktopGroup());
         if (cg.readEntry("Hidden", false))
         {
            if (oldFiles)
               oldFiles->append(file);
            files->removeAll(file);
            continue;
         }
      }
   }
}

void
DesktopComponent::slotSetupReady()
{
   QString desktop = KioskRun::self()->desktopPath();
   
   QDir dir(desktop);
   m_origDesktopFiles = dir.entryList(QDir::TypeMask|QDir::NoDotAndDotDot, QDir::Unsorted);
   
   filterFileList(desktop, &m_origDesktopFiles, 0);
}

bool
DesktopComponent::setupFinished()
{
   bool result = true;
   
   disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotSetupStarted()));
   disconnect(&m_timer, SIGNAL(timeout()), this, SLOT(slotSetupReady()));
   m_timer.stop();
   
   KConfig newCfg(m_iconPositionsFile, KConfig::SimpleConfig);

   QString desktop = KioskRun::self()->desktopPath();
   
   QDir dir(desktop);
   QStringList newDesktopFiles = dir.entryList(QDir::TypeMask|QDir::NoDotAndDotDot, QDir::Unsorted);
   filterFileList(desktop, &newDesktopFiles, &m_origDesktopFiles);

   KTemporaryFile positionsFile;
   positionsFile.close();

   KConfig positions(positionsFile.fileName(), KConfig::SimpleConfig);
   
   QStringList newGroups = newCfg.groupList();

   QString prefix = "IconPosition::";

   // Save icon positions
   foreach( QString newGroup, newGroups )
   {
      if (!newGroup.startsWith(prefix))
         continue;

      KConfigGroup newCfgGroup = newCfg.group(newGroup);
      KConfigGroup positionsGroup = positions.group(newGroup);
      if (newCfgGroup.hasKey("X"))
      {
         positionsGroup.writeEntry("X", newCfgGroup.readEntry("X"));
         positionsGroup.writeEntry("Y", newCfgGroup.readEntry("Y"));
      }
   }

   // Remove old icons from new list
   foreach( QString origDesktopFile, m_origDesktopFiles )
   {
      if (newDesktopFiles.removeAll(origDesktopFile))
      {
         m_origDesktopFiles.removeAll(origDesktopFile);
         continue;
      }
      
   }

   QString installPath = KioskRun::self()->locateSave("data", "kdesktop/Desktop/");
   QString installPath2 = KioskRun::self()->locateSave("data", "kdesktop/DesktopLinks/");

   // Remove all icons that are no longer
   foreach( QString origDesktopFile, m_origDesktopFiles )
   {
      QString file;
      if (QFile::exists(installPath + origDesktopFile ))
         file = installPath + origDesktopFile;
      else if (QFile::exists(installPath2 + origDesktopFile))
         file = installPath2 + origDesktopFile;

      if (!file.isEmpty())
      {
         result = KioskRun::self()->remove(file);
         if (!result) return false;
         positions.deleteGroup(prefix + origDesktopFile);
      }
      else
      {
         QString installFile = installPath + origDesktopFile;
         file = KioskRun::self()->locate("data", "kdesktop/Desktop/" + origDesktopFile);
         if (file.isEmpty())
         {
            installFile = installPath2 + origDesktopFile;
            file = KioskRun::self()->locate("data", "kdesktop/DesktopLinks/" + origDesktopFile);
         }
         
         if (!file.isEmpty())
         {
             // Hide via "Hidden=True", not sure if this works
             KTemporaryFile tmp;
             tmp.close();
             KDesktopFile cfg(tmp.fileName());
             cfg.desktopGroup().writeEntry("Hidden", true);
             cfg.sync();
             result = KioskRun::self()->install(tmp.fileName(), installFile);
             if (!result) return false;
             positions.deleteGroup(prefix + origDesktopFile );
         }
         else
         {
             kWarning() << "DesktopComponent: Can't remove " << origDesktopFile << endl;
         }
      }
   }
   positions.sync();
   result = KioskRun::self()->install(positionsFile.fileName(), KioskRun::self()->locateSave("data", "kdesktop/Desktop/.directory"));
   if (!result) return false;

   // Add all icons that have been added
   foreach( QString newDesktopFile, newDesktopFiles )
   {
      QString file = KioskRun::self()->desktopPath() + newDesktopFile;
      if (QFile::exists(file))
      {
         result = KioskRun::self()->install(file, installPath + newDesktopFile);
         if (!result) return false;
      }
      else
      {
         kWarning() << "DesktopComponent: Can't find new file " << file << endl;
      }
   }
   return true;
}

#include "desktopComponent.moc"
