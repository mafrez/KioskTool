/*
 *   kiosksync.h
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
#ifndef _KIOSKSYNC_H_
#define _KIOSKSYNC_H_

#include <QObject>
#include <QStringList>
#include <QList>

#include <KDE/KUrl>

#include "kioskrun.h"

class QWidget;

class KioskSync : public QObject
{
  Q_OBJECT
public:
  KioskSync( QWidget* parent = 0 );
  ~KioskSync();

  void addDir(const QString &src, const KUrl &dest);
  bool sync(bool incremental = false);
  // Returns all files found in the directories
  QStringList listFiles();

signals:
  void finished();
  void status(const QString &);
  void warning(const QString &);

protected:
  void scanChangedFiles(const QString &_dir, const QString &prefix);
  bool syncFile(const QString &prefix, const QString &file, const KUrl &dest);
  void addChangedDir(const QString &dir);

private:
  struct SyncDir
  {
     SyncDir()
     { }

     SyncDir(const QString &_src, const KUrl &_dest) : src(_src), dest(_dest)
     { }

     SyncDir(const SyncDir &dir) : src(dir.src), dest(dir.dest)
     { }

     QString src;
     KUrl dest;
  };

  typedef QList<SyncDir> SyncDirList;

  SyncDirList m_syncDirs;
  QWidget *m_parent;
  KConfig *m_timestamps;
  KConfigGroup m_timestampsGrp;
  QStringList m_changedFiles;
  QStringList m_changedDirs;
  bool m_incremental;
};

#endif
