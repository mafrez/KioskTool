/*
 * kiosktool-kdedirs.cpp
 *
 *   Copyright (C) 2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License versin 2 as
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


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <qfile.h>

#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kcomponentdata.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>

static QString readEnvPath(const char *env)
{
   QByteArray c_path = getenv(env);
   if (c_path.isEmpty())
      return QString();
   return QFile::decodeName(c_path);
}

static QStringList lookupProfiles(const QString &mapFile)
{
    QStringList profiles;

    if (mapFile.isEmpty() || !QFile::exists(mapFile))
    {
       profiles << "default";
       return profiles;
    }

    struct passwd *pw = getpwuid(geteuid());
    if (!pw)
    {
        profiles << "default";
        return profiles; // Not good
    }

    QByteArray user = pw->pw_name;

    gid_t sup_gids[512];
    int sup_gids_nr = getgroups(512, sup_gids);

    KConfig mapCfg(mapFile);
    KConfigGroup grpUsers = mapCfg.group("Users");
    if (grpUsers.hasKey(user.data()))
    {
        profiles = grpUsers.readEntry(user.data(), QStringList());
        return profiles;
    }

    KConfigGroup general =  mapCfg.group("General");
    QStringList groups =general.readEntry("groups", QStringList());

    KConfigGroup groupsUser =  mapCfg.group("Groups");

    for( QStringList::ConstIterator it = groups.constBegin();
         it != groups.constEnd(); ++it )
    {
        QByteArray grp = (*it).toUtf8();
        // Check if user is in this group
        struct group *grp_ent = getgrnam(grp);
        if (!grp_ent) continue;
        gid_t gid = grp_ent->gr_gid;
        if (pw->pw_gid == gid)
        {
            // User is in this group --> add profiles
            profiles += groupsUser.readEntry(*it, QStringList());
        }
        else
        {
            for(int i = 0; i < sup_gids_nr; i++)
            {
                if (sup_gids[i] == gid)
                {
                    // User is in this group --> add profiles
                    profiles += groupsUser.readEntry(*it, QStringList());
                    break;
                }
            }
        }
    }

    if (profiles.isEmpty())
        profiles << "default";
    return profiles;
}

int main(int argc, char **argv)
{
    KLocale::setMainCatalog("kiosktool");
    KAboutData about("kiosktool-kdedirs",0, ki18n( "kiosktool-kdedirs" ), "1.0", ki18n("A tool to set $KDEDIRS according to the current user profile."), KAboutData::License_GPL, ki18n( "(C) 2004 Waldo Bastian" ));

    KCmdLineArgs::init( argc, argv, &about);

    KCmdLineOptions options;
    options.add("check", ki18n("Output currently active prefixes"));
    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KComponentData a("kiosktool-kdedirs");

    if (args->isSet("check"))
    {
       (void) KGlobal::config(); // Force config file processing
       QString dirs = KGlobal::dirs()->kfsstnd_prefixes();
       printf("%s\n", QFile::encodeName(dirs).data());
       return 0;
    }

    QStringList kdedirList;

    // begin KDEDIRS
    QString kdedirs = readEnvPath("KDEDIRS");
    if (!kdedirs.isEmpty())
    {
        kdedirList = kdedirs.split(':');
    }
    else
    {
	QString kdedir = readEnvPath("KDEDIR");
	if (!kdedir.isEmpty())
        {
           kdedir = KShell::tildeExpand(kdedir);
	   kdedirList.append(kdedir);
        }
    }

    KConfigGroup config( KGlobal::config() , "Directories");
    QString userMapFile = config.readEntry("userProfileMapFile");
    QString profileDirsPrefix = config.readEntry("profileDirsPrefix");
    if (!profileDirsPrefix.isEmpty() && !profileDirsPrefix.endsWith("/"))
        profileDirsPrefix.append('/');
    QStringList profiles = lookupProfiles(userMapFile);

    while(!profiles.isEmpty())
    {
        QString profile = profiles.back();
        KConfigGroup grp( KGlobal::config() ,QString::fromLatin1("Directories-%1").arg(profile));
        profiles.pop_back();
        QStringList list = grp.readEntry("prefixes", QStringList());
        for (QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); it++)
        {
            kdedirList.prepend(*it);
	}
        if (list.isEmpty() && !profile.isEmpty() && !profileDirsPrefix.isEmpty())
        {
	   kdedirList.prepend(profileDirsPrefix + profile);
        }
    }
    printf("%s\n", QFile::encodeName(kdedirList.join(":")).data());

    return 0;
}
