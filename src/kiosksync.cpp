/*
 *   kiosksync.cpp
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

#include "kiosksync.h"

#include <QDir>
#include <QWidget>

#include <kdebug.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>

/*
Chaotic undocumented code, segfaults, side-effects, all-around nastiness. Ripe for a rewrite.

This is supposed to be used as a class that gets a list of source -> target URLs and synchronizes them (from source to target)
Maybe there's a better way to do this.
*/

KioskSync::KioskSync( QWidget* parent )
 : QObject(parent),
   m_parent(parent)
{
}

KioskSync::~KioskSync()
{
}

void
KioskSync::addDir(const QString &_src, const KUrl &dest)
{
   QString src = _src;
   if (!src.endsWith("/"))
      src.append("/");

   m_syncDirs.append(SyncDir(src, dest));
}

bool
KioskSync::sync(bool incremental)
{
   m_incremental = incremental;
   m_timestamps = new KConfig(KStandardDirs::locateLocal("appdata", "profile-data"),KConfig::SimpleConfig);

   bool canceled = false;

   for(SyncDirList::ConstIterator it = m_syncDirs.constBegin();
       it != m_syncDirs.constEnd(); ++it)
   {
      m_changedFiles.clear();
      m_changedDirs.clear();

      // FIXME: bugged
      //m_timestampsGrp.group((*it).src);

      if (!KioskRun::self()->createRemoteDirRecursive((*it).dest, true))
      {
         canceled = true;
         break;
      }

      scanChangedFiles((*it).src, QString::null);

      for(QStringList::ConstIterator it2 = m_changedDirs.constBegin();
          it2 != m_changedDirs.constEnd(); ++it2)
      {
         KUrl dest = (*it).dest;
         dest.setPath(dest.path(KUrl::AddTrailingSlash) + *it2);
         if (!KioskRun::self()->createRemoteDir(dest))
         {
            canceled = true;
            break;
         }
      }

      if (canceled)
         break;

      for(QStringList::ConstIterator it2 = m_changedFiles.constBegin();
          it2 != m_changedFiles.constEnd(); ++it2)
      {
         KUrl dest = (*it).dest;
         dest.setPath(dest.path(KUrl::AddTrailingSlash) + *it2);
         if (!syncFile((*it).src, *it2, dest))
         {
            canceled = true;
            break;
         }
      }
      if (canceled)
         break;
   }
   delete m_timestamps;
   m_timestamps = 0;
   m_changedFiles.clear();
   m_changedDirs.clear();

   return !canceled;
}

QStringList
KioskSync::listFiles()
{
   m_changedFiles.clear();
   m_changedDirs.clear();
   m_incremental = false;
   m_timestamps = 0;

   for(SyncDirList::ConstIterator it = m_syncDirs.constBegin();
       it != m_syncDirs.constEnd(); ++it)
   {
      scanChangedFiles((*it).src, QString::null);
   }
   return m_changedFiles;
}

void
KioskSync::addChangedDir(const QString &dir)
{
   if (dir.isEmpty())
      return;

   if (m_changedDirs.contains(dir))
      return;

   int i = dir.lastIndexOf('/', -2);
   if (i != -1)
   {
       QString parentDir = dir.left(i+1);
       addChangedDir(parentDir);
   }

   kDebug() << "KioskSync: Adding " << dir << endl;
   m_changedDirs.append(dir);
}

void
KioskSync::scanChangedFiles(const QString &_dir, const QString &prefix)
{
   kDebug() << "KioskSync: Scanning " << _dir << endl;
   QDir dir(_dir);
   if (!dir.exists())
   {
       emit warning(i18n("Directory <b>%1</b> does not exist.",_dir));
       return;
   }
   if (!dir.isReadable())
   {
       emit warning(i18n("Directory <b>%1</b> is not readable.",_dir));
       return;
   }

   QStringList subDirs;
   const QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoSymLinks);

   bool dirtyDir = false;
   foreach ( QFileInfo fi, list ) {
       if (fi.isDir())
       {
          QString subDir = fi.fileName();
          if ((subDir != ".") && (subDir != ".."))
             subDirs.append(subDir+"/");
          continue;
       }

       // TODO: Check file
       QString file = prefix + fi.fileName();
       QDateTime lastModified = fi.lastModified();
       // FIXME: bugged
       /*
       if (!m_incremental || !m_timestampsGrp.hasKey(file) ||
           (m_timestampsGrp.readEntry(file, QDateTime()) != lastModified))
           */
       {
          dirtyDir = true;
          m_changedFiles.append(file);
       }
   }
   if (dirtyDir)
      addChangedDir(prefix);

   for( QStringList::ConstIterator it = subDirs.constBegin();
        it != subDirs.constEnd(); ++it)
   {
       QString subDir = *it;
       scanChangedFiles(_dir + subDir, prefix + subDir);
   }
}

bool
KioskSync::syncFile(const QString &prefix, const QString &file, const KUrl &dest)
{
   kDebug() << "KioskSync: Syncing [" << prefix << "]" << file << " --> " << dest.prettyUrl() << endl;

   if (!KioskRun::self()->uploadRemote(prefix+file, dest))
       return false;

   QFileInfo fi(prefix+file);
   // FIXME: bugged
   //m_timestampsGrp.writeEntry(file, fi.lastModified());
   return true;
}

#include "kiosksync.moc"
