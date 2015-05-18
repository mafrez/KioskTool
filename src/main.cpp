/*
 *   main.cpp
 *
 *   Copyright (C) 2003,2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
 *   Copyright (C) 2010 Petr Mrázek <peterix@gmail.com>
 *   Copyright (C) 2015 Jiří Holomek <jiri.holomek@gmail.com>
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
#include "kioskgui.h"

#include <KDE/KAboutData>
#include <KDE/KApplication>
#include <KDE/KCmdLineArgs>
#include <KDE/KLocale>
#include <KDE/KMessageBox>

static const char *version = "1.99.5";
bool kde_kiosk_exception = false;

#ifndef KDERC
#define KDERC      "/etc/kde4rc"
#endif

int main(int argc, char *argv[])
{
    KAboutData aboutData( "kiosktool",0, ki18n("KIOSK Admin Tool"),
                         version, ki18n("KIOSK Admin Tool" ) , KAboutData::License_GPL_V2,
                         ki18n( "(c) 2003-2015 KioskTool Maintainers" ));
   aboutData.addAuthor(ki18n( "Ian Reinhart Geiser" ),ki18n("Maintainer"), "geiseri@kde.org");
   aboutData.addAuthor(ki18n( "Waldo Bastian" ),ki18n("Original Author"), "bastian@kde.org");
   aboutData.addAuthor(ki18n( "Laurent Montel" ),ki18n("Port to KDE4"), "montel@kde.org");
   aboutData.addAuthor(ki18n( "Petr Mrázek" ),ki18n("Fixes and GUI rework"), "peterix@gmail.com");
   aboutData.addAuthor(ki18n( "Jiří Holomek" ),ki18n("Fixes and deprecated removing"), "jiri.holomek@gmail.com");
   aboutData.setHomepage("http://extragear.kde.org/apps/kiosktool");

   KCmdLineArgs::init(argc, argv, &aboutData);

   KCmdLineOptions options;
   options.add("kderc <file>", ki18n("kderc file to save settings to"), KDERC );
   KCmdLineArgs::addCmdLineOptions( options );

   KApplication a;
   KioskGui *w = new KioskGui;
   w->show();

   int result = a.exec();

   return result;
}
