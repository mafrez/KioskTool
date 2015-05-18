/*
 *   kioskrun.h
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
#ifndef _KIOSKRUN_H_
#define _KIOSKRUN_H_

#include <QObject>
#include <QRegExp>
#include <QStringList>
#include <QTimer>
#include <QByteArray>
#include <QHash>
#include <KDE/KProgressDialog>

class KConfigRawEditor;
class ImmutableStatus;
class KConfig;
class KProcess;

class KioskGui;

class KioskRun : public QObject
{
  friend class KioskGui;

  Q_OBJECT
public:
  static KioskRun* self() { return s_self; }

  void setKdeDirs(const QStringList &dirs);
  void setUser(const QString &user);

  QString homeDir() { return m_homeDir; }
  QStringList kdeDirs() { return m_kdeDirs; }
  QString desktopPath() { return m_desktopPath; }

  // Locate existing anywhere
  QString locate(const char *resource, const QString &filename=QString::null);

  // Locate for saving
  QString locateSave(const char *resource, const QString &filename=QString::null);

  // Locate for reading saved changed
  QString locateLocal(const char *resource, const QString &filename=QString::null);

  // Prepare runtime environment for run()
  bool prepare();

  // Update sycoca database in runtime environment
  void updateSycoca();

  // Request sycoca update in install environment after flushing config files
  void scheduleSycocaUpdate();

  // Request sycoca update in install environment
  void forceSycocaUpdate();

  // Run a program inside the runtime test environment
  KProcess* run(const QString &cmd, const QStringList &args=QStringList());

  // Open config file in the install directory
  //KConfig *configFile(const QString &filename);

  //Get the configuration by filename based on the current profile.
  KConfigRawEditor *config( const QString &configName, const QString &profile );

  // Make config files temporary mutable.
  void makeMutable(const QString &profile, bool bMutable);

  // Returns whether specific config group is immutable,
  // or entire file if group is empty
  bool isConfigImmutable(const QString &profile, const QString &filename, const QString &group);

  // Make specific config group immutable,
  // or entire file if group is empty
  void setConfigImmutable(const QString &profile, const QString &filename, const QString &group, bool bImmutable);

  // Close all opened config files.
  bool flushConfigCache();

  // Clear all config data in memory
  void clearConfigCache();

  // Return all config files created by the user
  QStringList newConfigFiles();

  // Merge new settings from the test directory into the installation directory
  void mergeConfigFile(const QString &filename);

  // Lookup the setting for a custom action
  bool lookupCustomAction(const QString &profile, const QString &action);

  // Change the setting for a custom action
  void setCustomAction(const QString &profile, const QString &action, bool checked);

  // Create installation directory and its parent dirs
  bool createDir(const QString &dir);

  // Install file
  bool install(const QString &file, const QString &destination);

  // Delete file
  bool remove(const QString &destination);

  // Move file or directory
  bool move(const QString &source, const QString &destination, const QStringList &files);

  // Delete directory in test home dir
  void deleteDir(const QString &);

  // Open /etc/kderc for writing
  KConfig *openKderc();

  // Install new /etc/kderc
  bool closeKderc();

  // Read information of profile @p profile
  void getProfileInfo(const QString &profile, QString &description, QString &installDir, QString &installUser);

  // Store information for profile @p profile
  bool setProfileInfo(const QString &profile, const QString &description, const QString &installDir, const QString &installUser, bool b=false, bool deleteFiles=true);

  // Get new, non-existing, profile name
  QString newProfile();

  // Delete profile @p profile
  bool deleteProfile(const QString &profile, bool deleteFiles = true);

  // Get list of all existing profiles
  QStringList allProfiles();

  // Maps a single group or user to a one or more profiles
  typedef QMap<QString,QStringList> ProfileMapping;

  // Read mappings between groups/users and profiles
  bool getUserProfileMappings( ProfileMapping &groups, ProfileMapping &users, QStringList &groupOrder);

  // Store mappings between groups/users and profiles
  bool setUserProfileMappings( const ProfileMapping &groups, const ProfileMapping &users, const QStringList &groupOrder);

  // Read profile prefix
  QString getProfilePrefix();

  // Store profile prefix
  bool setProfilePrefix(const QString &prefix);

  // Create upload directory
  bool createRemoteDir(const KUrl &dir);

  // Create upload directory and all its parent dirs and be polite if ask = true
  bool createRemoteDirRecursive(const KUrl &dir, bool ask);

  // Upload file
  bool uploadRemote(const QString &file, const KUrl &dest);

protected:
  KioskRun( QWidget* parent = 0 );
  ~KioskRun();

  bool setupRuntimeEnv();
  void shutdownRuntimeEnv();
  void setupConfigEnv();
  void shutdownConfigEnv();
  void applyEnvironment(KProcess *p);

  QString saveImmutableStatus(const QString &filename);
  bool restoreImmutableStatus(const QString &filename, bool force);

  void setCustomRestrictionFileBrowsing(const QString &profile, bool restrict);

private:
  static KioskRun* s_self;
  QString m_homeDir;
  QString m_configDir;
  QString m_desktopPath;
  QString m_user;
  QStringList m_kdeDirs;
  QStringList m_xdgDataDirs;
  QStringList m_xdgConfigDirs;
  KComponentData *m_instance;
  KComponentData *m_saveInstance;
//   QHash<QString,KConfig*> m_saveConfigCache;
  QHash<QString,KConfigRawEditor*> m_configCache;
//   QHash<QString,ImmutableStatus*> m_immutableStatusCache;
  bool m_noRestrictions;
  bool m_forceSycocaUpdate;
  bool m_isRoot;

  QString m_kderc;
  QString m_localKderc;
  KConfig *m_localKdercConfig;
  QWidget* m_mainWidget;
};


class KioskRunProgressDialog : public KProgressDialog
{
   Q_OBJECT
public:
   KioskRunProgressDialog(QWidget *parent, const QString &caption, const QString &text);
public slots:
   void slotProgress();
   void slotFinished();

private:
   QTimer m_timer;
   int m_timeStep;
};


#endif
