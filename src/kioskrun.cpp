/*
 *   kioskrun.cpp
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

#include <config.h>

#include "kioskrun.h"


#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QHash>
#include <QDesktopServices>

#include <KDE/KApplication>
#include <KDE/KCmdLineArgs>
#include <KDE/KConfig>
#include <kdebug.h>
#include <KDE/KLocale>
#include <KDE/KMessageBox>
#include <KDE/KProcess>
#include <KDE/KSaveFile>
#include <ksimpleconfig.h>
#include <KDE/KStandardDirs>
#include <KDE/KUrl>
#include <KDE/KUser>
#include <KDE/KTemporaryFile>
#include <KDE/KToolInvocation>
#include <klauncher_iface.h>
#include "kiosksync.h"
#include "kdedinterface.h"
#include "kconfigraweditor.h"

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>

#include <kglobal.h>
#include <krandom.h>

#undef DEBUG_ENTRIES

KioskRun *KioskRun::s_self = 0;

KioskRun::KioskRun( QWidget* parent )
 : QObject(parent), m_instance(0), m_localKdercConfig(0), m_mainWidget( parent )
{
   m_noRestrictions = false;
   m_forceSycocaUpdate = false;
   s_self = this;
   m_homeDir = QDir::homePath()+"/.kde-test";
   KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
   m_kderc = args->getOption("kderc");
   m_isRoot = (getuid() == 0);
}

KioskRun::~KioskRun()
{
   shutdownRuntimeEnv();
   s_self = 0;
}

void
KioskRun::setUser(const QString &user)
{
   if (m_user == user) return;

   shutdownRuntimeEnv();
   shutdownConfigEnv();
   m_user = user;
}

static void filterDupes(QStringList &list)
{
   QStringList tmp;
   foreach( QString item, list)
   {
      if (!tmp.contains(item))
         tmp.append(item);
   }
   list = tmp;
}

void
KioskRun::setKdeDirs(const QStringList &dirs)
{
   if (m_kdeDirs == dirs) return;

   shutdownRuntimeEnv();
   shutdownConfigEnv();
   m_kdeDirs = dirs;
   QStringList xdgDataDirs = QFile::decodeName(getenv("XDG_DATA_DIRS")).split(':');
   if (xdgDataDirs.isEmpty())
   {
      xdgDataDirs = KGlobal::dirs()->kfsstnd_prefixes().split(':');
      xdgDataDirs.pop_front();
      for(QStringList::Iterator it = xdgDataDirs.begin();
          it != xdgDataDirs.end(); ++it)
      {
         *it += "share";
      }
      xdgDataDirs << "/usr/local/share" << "/usr/share";
   }

   m_xdgDataDirs.clear();
   for(QStringList::ConstIterator it = dirs.constBegin();
       it != dirs.constEnd(); ++it)
   {
      m_xdgDataDirs.append(*it+"/share");
   }
   m_xdgDataDirs += xdgDataDirs;
   filterDupes(m_xdgDataDirs);

   QStringList xdgConfigDirs = QFile::decodeName(getenv("XDG_CONFIG_DIRS")).split(':');
   if (xdgConfigDirs.isEmpty())
   {
      xdgConfigDirs << "/etc/xdg";
      QString sysconfMenuDir = KGlobal::dirs()->findDirs("xdgconf-menu", QString()).last();
      if (sysconfMenuDir.endsWith("/menus/"))
         xdgConfigDirs << sysconfMenuDir.left(sysconfMenuDir.length()-7);

   }

   m_xdgConfigDirs.clear();
   for(QStringList::ConstIterator it = dirs.constBegin();
       it != dirs.constEnd(); ++it)
   {
      m_xdgConfigDirs.append(*it+"/etc/xdg");
   }

   m_xdgConfigDirs += xdgConfigDirs;
   filterDupes(m_xdgConfigDirs);
}

void
KioskRun::deleteDir(const QString &dir)
{
   if (dir.length() <= 1) // Safety
      return;
   if (!dir.startsWith("/")) // Safety
      return;
   Q_ASSERT(dir.startsWith(m_homeDir));

   KIO::DeleteJob *deleteJob =  KIO::del( KUrl(dir) );
   deleteJob->ui()->setWindow( m_mainWidget );
   deleteJob->exec();
}

void
KioskRun::applyEnvironment(KProcess *p)
{
   p->setEnv(QLatin1String("HOME"),  m_homeDir);
   p->setEnv(QLatin1String("KDEHOME"),  QString(m_homeDir + "/" KDE_DEFAULT_HOME) );
   p->setEnv(QLatin1String("KDEROOTHOME"), QString( m_homeDir + "/" KDE_DEFAULT_HOME));
   p->setEnv(QLatin1String("KDEDIRS"),  m_kdeDirs.join(":"));
   p->setEnv(QLatin1String("XDG_DATA_HOME"),  QString( m_homeDir + "/.local/share"));
   p->setEnv(QLatin1String("XDG_DATA_DIRS"),  m_xdgDataDirs.join(":"));
   p->setEnv(QLatin1String("XDG_CONFIG_HOME"), QString(m_homeDir + "/.config"));
   p->setEnv(QLatin1String("XDG_CONFIG_DIRS"),  m_xdgConfigDirs.join(":"));
   p->setEnv(QLatin1String("KDE_KIOSK_NO_PROFILES"), "true");
   if (m_noRestrictions)
      p->setEnv(QLatin1String("KDE_KIOSK_NO_RESTRICTIONS"),  "true");
}

bool
KioskRun::prepare()
{
   bool result = setupRuntimeEnv();

   deleteDir(m_configDir);
   //PORT KDE4
#if 0
   deleteDir(KStandardDirs::locateLocal("data"));
#endif
   deleteDir(m_desktopPath);
   deleteDir(m_homeDir+"/.config");
   deleteDir(m_homeDir+"/.local/share");
   return result;
}

void
KioskRun::updateSycoca()
{
   // Force update
   QString sycocaUpdateFile = KioskRun::self()->locateLocal("services", "update_ksycoca");
   QFile file(sycocaUpdateFile);
   file.remove();
   file.open(QIODevice::WriteOnly);
   file.close();
   org::kde::kded kded( "org.kde.kbuildsycoca", "/kbuildsycoca", QDBusConnection::sessionBus());
   kded.call("recreate");
}

KProcess*
KioskRun::run(const QString &cmd, const QStringList &args)
{
   KProcess *proc = new KProcess(this);

   applyEnvironment(proc);

   *proc << cmd;
   *proc << args;

   proc->start();
   return proc;
}

class SetEnv
{
public:
   SetEnv(const char *key, const QString &value) : m_key(key)
   {
      m_oldValue = getenv(m_key);
      setenv(m_key, QFile::encodeName(value), 1);
   }

   ~SetEnv()
   {
      if (m_oldValue.isEmpty())
         setenv(m_key,"",1);
     else
         setenv(m_key,m_oldValue.data(),1);
   }

private:
   const char* m_key;
   QByteArray m_oldValue;
};

void
KioskRun::setupConfigEnv()
{
   if (m_instance) return;

   // ::KStandardDirs::locateLocal must be called before we change the env. vars!
   QString newTmpDir = ::KStandardDirs::locateLocal("tmp", "kioskdir");
   QString newSocketDir = ::KStandardDirs::locateLocal("socket", "kioskdir");

   SetEnv home("HOME", m_homeDir);
   QString kdeHome = m_homeDir+"/" KDE_DEFAULT_HOME;
   SetEnv kdehome("KDEHOME", kdeHome);
   SetEnv kderoothome("KDEROOTHOME", kdeHome);
   SetEnv kdedirs("KDEDIRS", m_kdeDirs.join(":"));
   SetEnv xdgDataHome("XDG_DATA_HOME", m_homeDir+"/.local/share");
   SetEnv xdgDataDirs("XDG_DATA_DIRS", m_xdgDataDirs.join(":"));
   SetEnv xdgConfigHome("XDG_CONFIG_HOME", m_homeDir+"/.config");
   SetEnv xdgConfigDirs("XDG_CONFIG_DIRS", m_xdgConfigDirs.join(":"));

   ::mkdir(QFile::encodeName(m_homeDir), 0700);
   ::mkdir(QFile::encodeName(kdeHome), 0700);

   // Create temp & socket dirs.
   char hostname[256];
   hostname[0] = 0;
   gethostname(hostname, 255);

   QString tmpDir = QString("%1/%2-%3").arg(kdeHome).arg("tmp").arg(hostname);
   deleteDir(tmpDir);
   ::mkdir(QFile::encodeName(newTmpDir), 0700);
   ::symlink(QFile::encodeName(newTmpDir), QFile::encodeName(tmpDir));

   QString socketDir = QString("%1/%2-%3").arg(kdeHome).arg("socket").arg(hostname);
   deleteDir(socketDir);
   ::mkdir(QFile::encodeName(newSocketDir), 0700);
   ::symlink(QFile::encodeName(newSocketDir), QFile::encodeName(socketDir));

   m_configDir = QString("%1/" KDE_DEFAULT_HOME "/share/config/").arg(m_homeDir);

   m_instance = new KComponentData("kioskrun");
   (void) m_instance->dirs(); // Create KStandardDirs obj

   KConfigGroup paths = KGlobal::config()->group(QLatin1String("Paths"));
   m_desktopPath = paths.readEntry( QLatin1String("Desktop"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation) );

   m_desktopPath = QDir::cleanPath( m_desktopPath );
   if ( !m_desktopPath.endsWith("/") )
      m_desktopPath.append('/');

   {
      SetEnv kdehome("KDEHOME", "-");
      SetEnv kderoothome("KDEROOTHOME", "-");
      SetEnv xdgDataHome("XDG_DATA_HOME", m_xdgDataDirs.first());
      SetEnv xdgConfigHome("XDG_CONFIG_HOME", m_xdgConfigDirs.first());

      m_saveInstance = new KComponentData("kioskrun");
      (void) m_saveInstance->dirs(); // Create KStandardDirs obj
   }
}

QString
KioskRun::locate(const char *resource, const QString &filename)
{
   setupConfigEnv();

   return m_saveInstance->dirs()->findResource(resource, filename);
}

QString
KioskRun::locateSave(const char *resource, const QString &filename)
{
   //setupConfigEnv();

   // split path from filename
   int slash = filename.lastIndexOf('/') + 1;
   QString dir = filename.left(slash);
   QString file = filename.mid(slash);
   return m_saveInstance->dirs()->saveLocation(resource, dir, false) + file;
}

QString
KioskRun::locateLocal(const char *resource, const QString &filename)
{
   setupConfigEnv();

   // split path from filename
   int slash = filename.lastIndexOf('/') + 1;
   QString dir = filename.left(slash);
   QString file = filename.mid(slash);
   return m_instance->dirs()->saveLocation(resource, dir, true) + file;
}


void
KioskRun::shutdownConfigEnv()
{
   if (!m_instance) return;

   delete m_instance;
   m_instance = 0;
}

class ImmutableStatus
{
public:
   bool m_fileScope;
   QHash<QString,int> m_lines;
   QString m_tmpFile;
   bool m_dirty;
};


bool
KioskRun::isConfigImmutable(const QString &profile, const QString &filename, const QString &group)
{
   KConfigRawEditor *conf = config( profile, filename );
   if( group.isEmpty() )
    return conf->isMutable();
   else
    return conf->group(group)->isMutable;
}

void
KioskRun::setConfigImmutable(const QString &profile, const QString &filename, const QString &_group, bool bImmutable)
{

   KConfigRawEditor *conf = config( profile,  filename );
   if( _group.isEmpty() )
    conf->setMutable( bImmutable );
   else
    conf->group(_group)->isMutable = bImmutable;
   m_forceSycocaUpdate = true;
}

static void stripImmutable(QString &ext)
{
   ext.replace("i", "");
   if (ext == "[$]")
      ext = QString();
}

static void addImmutable(QString &ext)
{
   ext.replace("[$", "[$i");
}

/*
QString
KioskRun::saveImmutableStatus(const QString &filename)
{
   ImmutableStatus *status = new ImmutableStatus;
   status->m_fileScope = false;
   status->m_dirty = false;
   m_immutableStatusCache.insert(filename, status);

   KTemporaryFile tmp;
   tmp.setAutoRemove(false);

   if( !tmp.open() )
     return QString();

   QString newPath = tmp.fileName();
   status->m_tmpFile = tmp.fileName();

   tmp.close();

   QString path = m_saveInstance->dirs()->findResource("config", filename);
   if (path.isEmpty())
      return newPath; // Nothing to do

   QFile oldCfg(path);

   if (!oldCfg.open( QIODevice::ReadOnly ))
      return newPath; // Error

   QFile newCfg(newPath);
   if (!newCfg.open( QIODevice::WriteOnly ))
      return newPath; // Error

   QTextStream txtIn(&oldCfg);
   txtIn.setCodec("UTF-8");

   QTextStream pTxtOut(&newCfg);
   pTxtOut.setCodec("UTF-8");

   QRegExp immutable("(\\[\\$e?ie?\\])$");

   // TODO: Use "group+key" instead of "key" as index, otherwise it might not be unique

   while(! txtIn.atEnd())
   {
      QString line = txtIn.readLine().trimmed();

      if (line.startsWith("#"))
      {
        // Comment, do nothing...
      }
      else if (line.startsWith("["))
      {
        int pos = immutable.lastIndexIn(line);
        if (pos != -1)
        {
           QString group = line.left(pos);
           QString ext = immutable.cap(0);
           stripImmutable(ext);
           if (pos == 0)
           {
              status->m_fileScope = true;
              continue;
           }
           status->m_lines[group] =  1;
           line = group + ext;
        }
      }
      else
      {
        int equal = line.indexOf('=');
        if (equal != -1)
        {
          QString key = line.left(equal).trimmed();
          int pos = immutable.lastIndexIn(key);
          if (pos != -1)
          {
             key = key.left(pos);
             QString ext = immutable.cap(0);
             stripImmutable(ext);
             status->m_lines[key] = 1;
             line = key + ext + line.mid(equal);
          }
        }
      }

      pTxtOut << line << endl;
   }
   oldCfg.close();
   newCfg.close();

   return newPath;
}

bool
KioskRun::restoreImmutableStatus(const QString &filename, bool force)
{

    ImmutableStatus *status = m_immutableStatusCache.take(filename);
   if (!status)
   {
      kDebug() << "KioskRun::restoreImmutableStatus(" << filename << ") status info missing" << endl;
      return true;
   }
   if (!force && !status->m_dirty)
   {
      kDebug() << "KioskRun::restoreImmutableStatus(" << filename << ") not changed" << endl;
      delete status;
      return true;
   }
   kDebug() << "KioskRun::restoreImmutableStatus(" << filename << ") restoring" << endl;

   QString path = status->m_tmpFile;

   KSaveFile newCfg(path);
   if( !newCfg.open() )
   {
     m_immutableStatusCache.insert(filename, status);
     return false;
   }

   QTextStream pTxtOut( &newCfg );
   pTxtOut.setCodec("UTF-8");

   QRegExp option("(\\[\\$e\\])$");

   if (status->m_fileScope)
   {
      kDebug() << "Marking file " << filename << " immutable" << endl;
      pTxtOut << "[$i]" << endl;
   }

   QFile oldCfg(path);
   if (oldCfg.open( QIODevice::ReadOnly ))
   {

      QTextStream txtIn(&oldCfg);
      txtIn.setCodec("UTF-8");;

      while(! txtIn.atEnd())
      {
         QString line = txtIn.readLine().trimmed();

         if (line.startsWith("#"))
         {
            // Comment, do nothing...
         }
         else if (line.startsWith("["))
         {
            if (status->m_lines.take(line))
               line += "[$i]";
         }
         else
         {
            int equal = line.indexOf('=');
            if (equal != -1)
            {
               QString key = line.left(equal).trimmed();
               int pos = option.lastIndexIn(key);
               if (pos != -1)
               {
                  key = key.left(pos);
                  QString ext = option.cap(0);
                  if (status->m_lines.take(key))
                     addImmutable(ext);
                  line = key + ext + line.mid(equal);
               }
               else
               {
                  if (status->m_lines.take(key))
                     line = key + "[$i]" + line.mid(equal);
               }
            }
         }

	pTxtOut << line << endl;
      }
      oldCfg.close();
   }

   // Create remaining groups that were marked as immutable
   foreach( QString key, status->m_lines.keys() )
   {
      QString group = key;
      if ( status->m_lines[key] )
         pTxtOut << endl << group << "[$i]" << endl;
   }

   newCfg.close();

   QString installLocation = m_saveInstance->dirs()->saveLocation("config", QString(), false) + filename;
   if (!install(path, installLocation))
   {
      m_immutableStatusCache.insert(filename, status); // Keep it around
      return false;
   }
   delete status;

   return true;
}
*/
bool
KioskRun::flushConfigCache()
{
  foreach( KConfigRawEditor *editor, m_configCache)
    if( !editor->save() )
    {
	// Alert failure to save configs.
	return false;
    }
    if (m_forceSycocaUpdate)
      forceSycocaUpdate();

   qDeleteAll( m_configCache );
   m_configCache.clear();
   return true;

}

void KioskRun::clearConfigCache()
{
   qDeleteAll( m_configCache );
   m_configCache.clear();
}

/*
KConfig *
KioskRun::configFile(const QString &filename)
{
   KConfig *config = m_saveConfigCache[filename];
   if (config)
      return config;

   setupConfigEnv();

   QString saveLocation = saveImmutableStatus(filename);
   config = new KConfig(saveLocation);
   m_saveConfigCache[filename] = config;

   kDebug() << "KioskRun::configFile(" << saveLocation << ") loading file";

   return config;
}
*/

KConfigRawEditor *
KioskRun::config( const QString &profile, const QString &fileName  )
{
    QString prefix = "/share/config/";
    QString installDir;
    QString installUser;
    QString description;

    getProfileInfo( profile,  description, installDir,  installUser);

    KConfigRawEditor *conf = m_configCache[fileName];
    if( conf )
	return conf;
    else
    {
	conf = new KConfigRawEditor( installDir + prefix + fileName );
	conf->load();
	m_configCache[fileName] = conf;
    }

    return conf;
}

void
KioskRun::makeMutable(const QString &profile, bool bMutable)
{
   m_noRestrictions = bMutable;

   QString name = "KDE_KIOSK_NO_RESTRICTIONS";
   QString value = m_noRestrictions ? "true" : "false";
   KToolInvocation::klauncher()->setLaunchEnv(name, value);

   setConfigImmutable(profile, "kdeglobals", "KDE Action Restrictions", true);
}

QStringList
KioskRun::newConfigFiles()
{
   setupConfigEnv();

   QStringList exceptions;
   exceptions << "kconf_updaterc";

   QStringList result;
   QDir dir(m_configDir);
   dir.setFilter( QDir::Files | QDir::NoSymLinks );

   const QFileInfoList list = dir.entryInfoList();
   if (list.isEmpty()) return result;

   foreach ( QFileInfo fi, list ) {
      QString file = fi.fileName();
      if (!file.endsWith("~") && !exceptions.contains(file)) // Skip backup files & exceptions
        result.append(file);
   }
   return result;
}

/*
void
KioskRun::mergeConfigFile(const QString &filename)
{
   KConfig *saveCfg = configFile(filename);

   kDebug() << "KioskRun::mergeConfigFile(" << (m_configDir + filename) << ")" << endl;
   KConfig newCfg(m_configDir + filename);
   foreach( QString group, newCfg.groupList() )
   {
      KConfigGroup grp = saveCfg->group(group);
      QMap<QString, QString> map = newCfg.entryMap(group);
      foreach( QString key, map.keys() )
      {
         grp.writeEntry(key, map[key]);
      }
   }
}
*/
bool
KioskRun::setupRuntimeEnv()
{
    //if (m_dcopClient) return true;

   KioskRunProgressDialog dlg(KApplication::activeWindow(),
                      i18n("Setting Up Configuration Environment"),
                      i18n("Setting up configuration environment."));

   char hostname[256];
   hostname[0] = 0;
   gethostname(hostname, 255);
   QString cacheDir = QString("%1/" KDE_DEFAULT_HOME "/cache-%2").arg(m_homeDir).arg(hostname);

   deleteDir(cacheDir);
   KStandardDirs::makeDir(cacheDir);
   deleteDir(m_homeDir+"/.qt");
   ::unlink(QFile::encodeName(m_homeDir+".kderc"));

   QString iceAuth = QString("%1/.ICEauthority").arg(QDir::homePath());
   setenv("ICEAUTHORITY", QFile::encodeName(iceAuth), 0); // Don't overwrite existing setting

   QString xAuth = QString("%1/.Xauthority").arg(QDir::homePath());
   setenv("XAUTHORITY", QFile::encodeName(xAuth), 0); // Don't overwrite existing setting

   //QString dcopServerFile = m_homeDir+"/.kde/DCOPserver";

   KProcess kdeinit;

   applyEnvironment(&kdeinit);

   kdeinit << "kdeinit4";

   connect(&kdeinit, SIGNAL(processExited(KProcess *)), &dlg, SLOT(slotFinished()));

   kdeinit.start();

   dlg.exec();
#if 0
   QByteArray dcopSrv;
   QFile f(dcopServerFile);
   if (f.open(QIODevice::ReadOnly))
   {
       int size = qMin( 1024, f.size() ); // protection against a huge file
       QByteArray contents( size+1 );
       if ( f.readBlock( contents.data(), size ) == size )
       {
           contents[size] = '\0';
           int pos = contents.find('\n');
           if ( pos == -1 ) // Shouldn't happen
               dcopSrv = contents;
           else
               dcopSrv = contents.left( pos );
       }
   }

   if (dcopSrv.isEmpty())
   {
       kWarning() << "Error reading " << dcopServerFile << endl;
       m_dcopClient = new DCOPClient;
       shutdownRuntimeEnv();
       return false;
   }

   m_dcopClient = new DCOPClient;
   m_dcopClient->setServerAddress(dcopSrv);
   unsetenv("DCOPSERVER"); // Don't propagate it
   m_dcopClient->attach();
#endif
   return true;
}

void
KioskRun::shutdownRuntimeEnv()
{
    //KDE4 port it
#if 0
   if (!m_dcopClient) return;

   delete m_dcopClient;
   m_dcopClient = 0;

   KProcess kdeinit;
   applyEnvironment(&kdeinit);

   kdeinit << "kdeinit4_shutdown";

   kdeinit.start();

   KProcess dcopserver;
   applyEnvironment(&dcopserver);

   dcopserver << "dcopserver_shutdown";

   dcopserver.start();
#endif
}
#if 0
DCOPRef
KioskRun::dcopRef(const QByteArray &appId, const QByteArray &objId)
{
   if (!setupRuntimeEnv())
      return DCOPRef();
   DCOPRef ref(appId, objId);
   ref.setDCOPClient(m_dcopClient);
   return ref;
}
#endif
// Lookup the setting for a custom action
bool
KioskRun::lookupCustomAction(const QString &profile, const QString &action)
{
   KConfigRawEditor *conf = KioskRun::self()->config( profile, "kdeglobals");
   return conf->group("KDE Custom Restrictions")->readEntry(action, false).toBool();
}

// Change the setting for a custom action
void
KioskRun::setCustomAction(const QString &profile, const QString &action, bool checked)
{
    KConfigRawEditor *conf = KioskRun::self()->config( profile, "kdeglobals" );
    if( conf->group("KDE Custom Restrictions")->readEntry(action, false).toBool() != checked )
    {
	conf->group("KDE Custom Restrictions")->writeEntry( action, checked );
	KioskRun::self()->scheduleSycocaUpdate();

	if (action == "restrict_file_browsing")
	{
	    setCustomRestrictionFileBrowsing(profile, checked);
	}

    }
}

// Create directory
bool
KioskRun::createDir(const QString &dir)
{
    //qDebug() << dir;
   if (QDir(dir).exists())
      return true; // Exists already

   KUrl dest;
   if (!m_isRoot || (m_user != "root"))
   {
      dest.setProtocol("fish");
      dest.setHost("localhost");
      dest.setUser(m_user);
   }
   dest.setPath(dir);

   if (dir.length() > 1)
   {
      KUrl parent = dest.upUrl();

      bool result = createDir(parent.path());
      if (!result)
         return false;
   }

    KIO::StatJob *existsJob = KIO::stat( dest, KIO::HideProgressInfo );
    existsJob->ui()->setWindow( m_mainWidget );
    if ( existsJob->exec() == true )
	return true;

    KIO::SimpleJob *mkdirJob = KIO::mkdir(dest, 0755 );
    mkdirJob->ui()->setWindow( m_mainWidget );
    if (mkdirJob->exec() == true)
	return true;

   return false;
}

// Create directory
bool
KioskRun::createRemoteDirRecursive(const KUrl &dest, bool ask)
{
   KIO::StatJob *existsJob = KIO::stat( dest, KIO::HideProgressInfo );
   existsJob->ui()->setWindow( m_mainWidget );
   if ( existsJob->exec() == true )
     return true;


   KUrl parent = dest.upUrl();
   KIO::StatJob *parentExistsJob = KIO::stat( parent, KIO::HideProgressInfo );
   parentExistsJob->ui()->setWindow( m_mainWidget );
   if ( parentExistsJob->exec() == true )
   {
      return createRemoteDir(dest);
   }

   if (ask)
   {
      // Parent doesn't exist,
      int result = KMessageBox::warningContinueCancel(KApplication::activeWindow(),
                i18n("<qt>The directory <b>%1</b> does not yet exist. "
                     "Do you want to create it?", parent.prettyUrl()), QString(),
               KGuiItem(  i18n("Create &Dir") ));
      if (result != KMessageBox::Continue)
         return false;
   }

   QString path = dest.path( KUrl::AddTrailingSlash);
   int i = 0;
   while ( (i = path.indexOf('/',  i + 1)) != -1)
   {
      parent.setPath(path.left(i+1));
      if (! createRemoteDir(parent))
         return false;
   }
   return true;
}

// Create directory
bool
KioskRun::createRemoteDir(const KUrl &dest)
{
     KIO::StatJob *existsJob = KIO::stat( dest, KIO::HideProgressInfo );
     existsJob->ui()->setWindow( m_mainWidget );
     if ( existsJob->exec() == true )
       return true;

     KIO::SimpleJob *mkdirJob = KIO::mkdir(dest, 0755 );
     mkdirJob->ui()->setWindow( m_mainWidget );
     if (mkdirJob->exec() == true)
       return true;

      return false;
}

// Install file
bool
KioskRun::install(const QString &file, const QString &destination)
{
   KUrl dest;
   if (!m_isRoot || (m_user != "root"))
   {
      dest.setProtocol("fish");
      dest.setHost("localhost");
      dest.setUser(m_user);
   }
   dest.setPath(destination);

   if (!createDir(dest.upUrl().path()))
      return false;

    KUrl src;
    src.setPath(file);
    KIO::FileCopyJob *result = KIO::file_copy(src, dest, 0644, KIO::Overwrite|KIO::HideProgressInfo );
    result->ui()->setWindow( m_mainWidget );
    if (result->exec() == true)
    {
        ::unlink(QFile::encodeName(file));
	return true;
    }

   return false;
}

// Upload file
bool
KioskRun::uploadRemote(const QString &file, const KUrl &dest)
{
    KUrl src;
    src.setPath(file);
    KIO::CopyJob *result = KIO::copy(src, dest, KIO::Overwrite|KIO::HideProgressInfo );
    result->ui()->setWindow( m_mainWidget );
    if ( result->exec() )
	return true;
    return false;
}

// Remove file
bool
KioskRun::remove(const QString &destination)
{
   KUrl dest;
   if (!m_isRoot || (m_user != "root"))
   {
      dest.setProtocol("fish");
      dest.setHost("localhost");
      dest.setUser(m_user);
   }
   dest.setPath(destination);

   KIO::DeleteJob *delJob = KIO::del(dest,KIO::HideProgressInfo );
   delJob->ui()->setWindow( m_mainWidget );
   return delJob->exec();
}

// Move file or directory
bool
KioskRun::move(const QString &source, const QString &destination, const QStringList &files)
{
   KUrl src;
   KUrl dest;
   if (!m_isRoot || (m_user != "root"))
   {
      dest.setProtocol("fish");
      dest.setHost("localhost");
      dest.setUser(m_user);
      src.setProtocol("fish");
      src.setHost("localhost");
      src.setUser(m_user);
   }

   foreach( QString file, files )
   {
      src.setPath(source + file);
      dest.setPath(destination + file);

      kDebug() << "Moving " << src << " --> " << dest << endl;
      if (!createRemoteDirRecursive(dest.upUrl(), false))
         return false;
      KIO::CopyJob *moveJob = KIO::move( src, dest, KIO::Overwrite|KIO::HideProgressInfo );
      moveJob->ui()->setWindow( m_mainWidget );
      if ( !moveJob->exec() )
      {
         return false;
      }
   }

   return true;
}

// Read information of profile @p profile
void
KioskRun::getProfileInfo(const QString &profile, QString &description, QString &installDir, QString &installUser)
{
   QString defaultInstallDir = getProfilePrefix();
   if (defaultInstallDir.isEmpty())
   {
      defaultInstallDir = "/etc/kde-profile/";
   }
   if (!defaultInstallDir.endsWith("/"))
      defaultInstallDir.append("/");
   QString tmp = profile;
   tmp.replace(" ", "_");
   tmp.replace(":", "_");
   tmp.replace("/", "_");
   defaultInstallDir += tmp+"/";

   QString group = QString("Directories-%1").arg(profile);
   KConfigGroup grp(KGlobal::config(), group );

   installDir = grp.readEntry("prefixes", defaultInstallDir);
   if (!installDir.endsWith("/"))
      installDir.append("/");

   QString profileInfoFile = installDir + ".kdeprofile";
   if (QFile::exists(profileInfoFile))
   {
       KConfig profileInfo(profileInfoFile, KConfig::SimpleConfig );
       KConfigGroup profileInfoGroup = profileInfo.group("General");
       description = profileInfoGroup.readEntry("Description");
       installUser = profileInfoGroup.readEntry("InstallUser", "root");
       return;
   }

   QString defaultDescription;
   if (profile == "default")
      defaultDescription = i18n("Default profile");

   description = grp.readEntry("ProfileDescription", defaultDescription);
   installUser = grp.readEntry("ProfileInstallUser", "root");
}

KConfig *
KioskRun::openKderc()
{
   if (m_localKdercConfig)
      return m_localKdercConfig;

   KUrl settingsUrl;
   settingsUrl.setPath(m_kderc);

   m_localKderc = ::KStandardDirs::locateLocal("tmp", "kderc_"+KRandom::randomString(5));
   ::unlink(QFile::encodeName(m_localKderc));

   KUrl localCopyUrl;
   localCopyUrl.setPath(m_localKderc);

   if (QFile::exists(settingsUrl.path()))
   {
     KIO::CopyJob *result = KIO::copy(settingsUrl, localCopyUrl, KIO::Overwrite|KIO::HideProgressInfo );
     result->ui()->setWindow( m_mainWidget );
     if( result->exec() == false )
       return 0;
   }

   m_localKdercConfig = new KConfig(m_localKderc, KConfig::SimpleConfig);
   return m_localKdercConfig;
}

bool
KioskRun::closeKderc()
{
   if (!m_localKdercConfig)
      return false;
   m_localKdercConfig->sync();
   delete m_localKdercConfig;
   m_localKdercConfig = 0;

   QString saveUser = m_user;
   m_user = "root";
   bool result = install(m_localKderc, m_kderc);
   m_localKderc = QString();
   m_user = saveUser;
   KGlobal::config()->reparseConfiguration();
   return result;
}

// Store information for profile @p profile
bool
KioskRun::setProfileInfo(const QString &profile, const QString &description, const QString &_installDir, const QString &installUser, bool deleteProfile, bool deleteFiles)
{
   QString installDir = _installDir;
   if (!installDir.endsWith("/"))
      installDir.append("/");

   QString saveProfileInfo = installDir + ".kdeprofile";
   KConfig profileInfo(saveProfileInfo, KConfig::SimpleConfig);
   KConfigGroup profileGroup = profileInfo.group("General");
   QString oldDescription = profileGroup.readEntry("Description");
   QString oldInstallUser = profileGroup.readEntry("InstallUser");

   if (deleteProfile && !installDir.isEmpty())
   {
      bool result = true;
      KioskSync profileDir(KApplication::activeWindow());
      profileDir.addDir(installDir, KUrl());
      QStringList allFiles = profileDir.listFiles();
      allFiles.removeAll(QLatin1String(".kdeprofile"));
      if (allFiles.isEmpty())
      {
         if (QDir(installDir).exists())
         {
            m_user = installUser;
            remove(installDir);
            m_user = QString();
         }
      }
      else if (deleteFiles)
      {
         int msgResult = KMessageBox::warningYesNoCancelList(KApplication::activeWindow(),
                          i18n("<qt>The profile directory <b>%1</b> contains the following files, "
                               "do you wish to delete these files?", installDir),
                          allFiles,
                          i18n("Deleting Profile"),
                          KStandardGuiItem::del(),
                          KGuiItem( i18n("&Keep Files") )
                               );
         switch(msgResult)
         {
           case KMessageBox::Yes:
             // Delete files
             m_user = installUser;
             result = remove(installDir);
             m_user = QString();
             if (!result)
                return false;
             break;

           case KMessageBox::No:
             // Keep files
             break;

           default:
             // Cancel
             return false;
         }
      }

      m_user = installUser;
      if (QFile::exists(saveProfileInfo))
         result = remove(saveProfileInfo);
      m_user = QString();
      if (!result)
         return false;
   }
   else if ((description != oldDescription) ||
       (installUser != oldInstallUser))
   {
      QString localProfileInfo = ::KStandardDirs::locateLocal("tmp", "kdeprofile_"+KRandom::randomString(5));
      ::unlink(QFile::encodeName(localProfileInfo));

      KConfig newProfileInfo(localProfileInfo,KConfig::SimpleConfig );
      KConfigGroup newProfileGroup = newProfileInfo.group("General");
      newProfileGroup.writeEntry("Description", description);
      newProfileGroup.writeEntry("InstallUser", installUser);
      newProfileGroup.sync();
      m_user = installUser;
      bool result = install(localProfileInfo, saveProfileInfo);
      m_user = QString();
      if (!result)
         return false;
   }

   KUser thisUser;
   QString newAdmin = thisUser.loginName()+":"; // This user, all hosts

   KConfigGroup grp( KGlobal::config(), "Directories");
   QString oldAdmin = grp.readEntry("kioskAdmin");

   QString group = QString("Directories-%1").arg(profile);
   KConfigGroup dirgrp( KGlobal::config(),group);

   if ((installDir == dirgrp.readEntry("prefixes")) &&
       (newAdmin == oldAdmin) &&
       !deleteProfile)
      return true; // Nothing to do

   KConfig *cfg = openKderc();
   if (!cfg)
      return false;

   KConfigGroup newDirGrp = cfg->group("Directories");
   newDirGrp.writeEntry("kioskAdmin", newAdmin);

   if (deleteProfile)
   {
      cfg->deleteGroup(group);
   }
   else
   {
       newDirGrp = cfg->group(group);
      // TODO: update prefixes
      newDirGrp.writeEntry("prefixes", installDir);
   }
   cfg->sync();

   return closeKderc();
}

bool
KioskRun::deleteProfile(const QString &profile, bool deleteFiles)
{
   QString description;
   QString installDir;
   QString installUser;
   getProfileInfo(profile, description, installDir, installUser);
   return setProfileInfo(profile, description, installDir, installUser, true, deleteFiles);
}

// Read profile prefix
QString
KioskRun::getProfilePrefix()
{
    KConfigGroup grp( KGlobal::config(), "Directories");

   QString prefix = grp.readEntry("profileDirsPrefix");
   if (!prefix.isEmpty() && !prefix.endsWith("/"))
      prefix.append('/');
   return prefix;
}

// Store profile prefix
bool
KioskRun::setProfilePrefix(const QString &_prefix)
{
   QString prefix = _prefix;

   if (!prefix.isEmpty() && !prefix.endsWith("/"))
      prefix.append('/');

   if (prefix == getProfilePrefix())
      return true; // Nothing to do

   KConfig *cfg = openKderc();
   if (!cfg)
      return false;

   KConfigGroup grp = cfg->group("Directories");
   grp.writeEntry("profileDirsPrefix", prefix);

   grp.sync();

   return closeKderc();
}

QString
KioskRun::newProfile()
{
   QString profilePrefix = getProfilePrefix();

   KSharedConfigPtr config = KGlobal::config();
   for(int p = 1; p; p++)
   {
      QString profile = QString("profile%1").arg(p);
      QString group = QString("Directories-%1").arg(profile);
      if (!config->hasGroup(group))
      {
         if (profilePrefix.isEmpty())
            return profile;

         QString profileDir = profilePrefix + profile;
         if (!QDir(profileDir).exists() && !QFile::exists(profileDir))
            return profile;

         // Keep on looking...
      }
   }
   return QString();
}

QStringList
KioskRun::allProfiles()
{

   KSharedConfigPtr config = KGlobal::config();
   QStringList groups = config->groupList();
   QStringList profiles;
   QStringList directories;
   foreach( QString group, groups )
   {
      if (!group.startsWith("Directories-"))
         continue;
      profiles.append(group.mid(12));
      KConfigGroup grp =  config->group(group);
      QString installDir = grp.readEntry("prefixes");
      if (!installDir.endsWith("/"))
         installDir.append("/");
      directories.append(installDir);
   }

   QString profilePrefix = getProfilePrefix();
   if (!profilePrefix.isEmpty())
   {
      QDir dir(profilePrefix, QString(), QDir::Unsorted, QDir::Dirs);
      QStringList profileDirs = dir.entryList();
      foreach( QString profileDir, profileDirs )
      {
         if (profileDir.startsWith("."))
            continue;
         QString dir = profilePrefix + profileDir + "/";
         if (directories.contains(dir))
         {
            kDebug() << "Skipping " << dir << ", dir already listed" << endl;
            continue;
         }
         if (profiles.contains(profileDir))
         {
            kDebug() << "Skipping " << dir << ", profile [" << profileDir << "] already listed" << endl;
            continue;
         }

         if (!QFile::exists(dir+".kdeprofile"))
         {
            kDebug() << "Skipping " << dir << ", no profile info" << endl;
            continue;
         }
         profiles.append(profileDir);
         directories.append(dir);
      }
   }

   return profiles;
}

bool
KioskRun::getUserProfileMappings( ProfileMapping &groups, ProfileMapping &users, QStringList &groupOrder)
{
   groups.clear();
   users.clear();

   KConfigGroup config( KGlobal::config(), "Directories");
   QString mapFile = config.readEntry("userProfileMapFile");

   if (mapFile.isEmpty() || !QFile::exists(mapFile))
   {
      return false;
   }

   KConfig mapCfg(mapFile, KConfig::SimpleConfig);

   KConfigGroup grp = mapCfg.group("General");
   groupOrder = grp.readEntry("groups", QStringList());

   grp = mapCfg.group("Groups");
   foreach( QString group, groupOrder )
   {
      QStringList profiles = grp.readEntry(group, QStringList());
      if (!profiles.isEmpty())
         groups.insert(group, profiles);
   }

   QMap<QString, QString> cfg_users = mapCfg.entryMap("Users");
   foreach( QString key, cfg_users.keys() )
   {
      QString user = key;
      QStringList profiles = cfg_users[key].split(',');
      if (!profiles.isEmpty())
         users.insert(user, profiles);
   }
   return true;
}

bool
KioskRun::setUserProfileMappings( const ProfileMapping &groups, const ProfileMapping &users, const QStringList &groupOrder)
{
   KConfigGroup config(  KGlobal::config(), "Directories");
   QString mapFile = config.readEntry("userProfileMapFile");
   if (mapFile.isEmpty())
   {
     mapFile = "/etc/kde-user-profile";

     KConfig *cfg = openKderc();
     if (!cfg)
        return false;

     KConfigGroup grp = cfg->group("Directories");
     grp.writeEntry("userProfileMapFile", mapFile);
     if (!closeKderc())
        return false;
   }

   QString localMapFile = ::KStandardDirs::locateLocal("tmp", "kde-user-profile_"+KRandom::randomString(5));
   ::unlink(QFile::encodeName(localMapFile));

   KConfig mapConfig(localMapFile, KConfig::SimpleConfig);
   KConfigGroup generalGrp = mapConfig.group("General");
   generalGrp.writeEntry("groups", groupOrder);

   KioskRun::ProfileMapping::ConstIterator it;

   generalGrp = mapConfig.group("Groups");
   for ( it = groups.constBegin(); it != groups.constEnd(); ++it )
   {
      QString group = it.key();
      generalGrp.writeEntry(group, *it);
   }
   generalGrp = mapConfig.group("Users");
   for ( it = users.constBegin(); it != users.constEnd(); ++it )
   {
      QString user = it.key();
      generalGrp.writeEntry(user, *it);
   }

   mapConfig.sync();

   QString saveUser = m_user;
   m_user = "root";
   bool result = install(localMapFile, mapFile);
   m_user = saveUser;
   return result;
}

void
KioskRun::forceSycocaUpdate()
{
   // Touch $KDEDIR/share/services/update_ksycoca
//FIXME: this needs to be done nicer...
}

void
KioskRun::scheduleSycocaUpdate()
{
   m_forceSycocaUpdate = true;
}

void
KioskRun::setCustomRestrictionFileBrowsing(const QString &profile, bool restrict)
{
   QString file = "kdeglobals";
   QString group = "KDE URL Restrictions";
   KConfigRawEditor *cfg = KioskRun::self()->config( profile, file );
   KConfigRawEditor::KConfigGroupData *grp = cfg->group(group);
   int count = grp->readEntry("rule_count", 0).toInt();
   QStringList urlRestrictions;
   for(int i = 0; i < count; i++)
   {
      QString key = QString("rule_%1").arg(i+1);
      if (grp->hasKey(key))
         urlRestrictions.append( grp->readEntry(key, QString() ).toString() );
   }

   QStringList newRestrictions;
   newRestrictions << "list,,,,file,,,false";
   newRestrictions << "list,,,,file,,$HOME,true";

   foreach( QString newRestriction, newRestrictions)
      urlRestrictions.removeAll(newRestriction);

   if (restrict)
   {
      newRestrictions += urlRestrictions;
      urlRestrictions = newRestrictions;
   }

   count = urlRestrictions.count();
   grp->writeEntry("rule_count", count);

   for(int i = 0; i < count; i++)
   {
      QString key = QString("rule_%1").arg(i+1);
      grp->writeEntry(key, urlRestrictions[i]);
   }
   KioskRun::self()->setConfigImmutable(profile, file, group, true);
}

KioskRunProgressDialog::KioskRunProgressDialog(QWidget *parent, const QString &caption, const QString &text)
 : KProgressDialog(parent, caption, text)
{
    setModal( true );
  connect(&m_timer, SIGNAL(timeout()), this, SLOT(slotProgress()));
  progressBar()->setMaximum(20);
  m_timeStep = 700;
  m_timer.start(m_timeStep);
  setAutoClose(false);
}

void
KioskRunProgressDialog::slotProgress()
{
  int p = progressBar()->value();
  if (p == 18)
  {
     progressBar()->reset();
     progressBar()->setValue(p+1);
     m_timeStep = m_timeStep * 2;
     m_timer.start(m_timeStep);
  }
  else
  {
     progressBar()->setValue(2*p+1);
  }
}

void
KioskRunProgressDialog::slotFinished()
{
  progressBar()->setValue(20);
  m_timer.stop();
  QTimer::singleShot(1000, this, SLOT(close()));
}


#include "kioskrun.moc"

