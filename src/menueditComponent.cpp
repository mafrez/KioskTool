/*
 *   menueditComponent.cpp
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

#include "menueditComponent.h"

#include <QDir>
#include <QDomElement>
#include <QFileInfo>

#include <kapplication.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <KDE/KTemporaryFile>
#include "kioskrun.h"
#include "kiosksync.h"

MenuEditComponent::MenuEditComponent( QObject *parent)
 : Component(parent)
{
}

MenuEditComponent::~MenuEditComponent()
{
}

void
MenuEditComponent::slotSetupPrepare()
{
   (void) KioskRun::self()->locateLocal("xdgconf-menu", "applications-kmenuedit.menu"); // Create dir
}

void
MenuEditComponent::slotSetupStarted()
{
}

static QDomDocument loadDoc(const QString &fileName)
{
   QDomDocument doc;

   QFile file( fileName );
   if ( !file.open( QIODevice::ReadOnly ) )
   {
      kWarning() << "Could not open " << fileName << endl;
      return doc;
   }
   QString errorMsg;
   int errorRow;
   int errorCol;
   if ( !doc.setContent( &file, &errorMsg, &errorRow, &errorCol ) ) {
      kWarning() << "Parse error in " << fileName << ", line " << errorRow << ", col " << errorCol << ": " << errorMsg << endl;
      file.close();
      return doc;
   }
   file.close();
   return doc;
}

static bool saveDoc(const QString &fileName, QDomDocument doc)
{
   KSaveFile saveFile(fileName);

   if( !saveFile.open(QIODevice::WriteOnly ) )
      return false;

   saveFile.write( doc.toString().toUtf8() );

   if (!saveFile.finalize())
   {
      kWarning() << "Could not write " << fileName << endl;
      return false;
   }

   return true;
}


bool
MenuEditComponent::setupFinished()
{
   bool result;
   QString menuEditFile = KioskRun::self()->locateLocal("xdgconf-menu", "applications-kmenuedit.menu");
   QString menuFile = KioskRun::self()->locate("xdgconf-menu", "applications.menu");
   QString menuFileSave = KioskRun::self()->locateSave("xdgconf-menu", "applications.menu");

   kDebug() << "MenuEditComponent: menuEditFile = " << menuEditFile;
   kDebug() << "MenuEditComponent: menuFile = " << menuFile;
   kDebug() << "MenuEditComponent: menuFileSave = " << menuFileSave;

   QDomDocument docChanges = loadDoc(menuEditFile);
   if (docChanges.isNull())
   {
       kDebug() << "No menu changes.";
       return true;
   }

   QDomDocument doc = loadDoc(menuFile);
   if (doc.isNull())
   {
       kWarning() << "Can't find menu file!";
       return true;
   }

   QDomElement docElem = doc.documentElement();
   QDomNode n = docElem.firstChild();
   QDomNode next;
   for(; !n.isNull(); n = next )
   {
      QDomElement e = n.toElement(); // try to convert the node to an element.
      next = n.nextSibling();

      if ((e.tagName() == "MergeFile") && (e.text() == "applications-kmenuedit.menu"))
         break;
   }
   QDomNode insertionPoint = n;
   if (insertionPoint.isNull())
   {
      kWarning() << "Application menu fails to include applications-kmenuedit.menu" << endl;
      return false;
   }
   QDomElement docChangesElem = docChanges.documentElement();
   n = docChangesElem.firstChild();
   for(; !n.isNull(); n = next )
   {
      QDomElement e = n.toElement(); // try to convert the node to an element.
      next = n.nextSibling();

      docElem.insertBefore(n, insertionPoint);
   }

   KTemporaryFile tempFile;
   tempFile.close();

   saveDoc(tempFile.fileName(), doc);
   result = KioskRun::self()->install(tempFile.fileName(), menuFileSave);
   if (!result) return false;


   // Install .desktop files
   {
      QString legacyApplications = KioskRun::self()->locateLocal("apps", QString());
      QString legacySaveApplications = KioskRun::self()->locateSave("apps", QString());

      KioskSync legacyDir(KApplication::activeWindow());
      legacyDir.addDir(legacyApplications, KUrl());

      QStringList newLegacyApplications = legacyDir.listFiles();

      foreach( QString newLegacyApplication, newLegacyApplications )
      {
         if (newLegacyApplication.endsWith(".desktop") ||
             newLegacyApplication.endsWith(".kdelnk") ||
             newLegacyApplication.endsWith(".directory"))
         {
            kDebug() << "MenueditComponent: New legacy file " << (legacyApplications + newLegacyApplication);
            result = KioskRun::self()->install(legacyApplications + newLegacyApplication, legacySaveApplications + newLegacyApplication);
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

      foreach( QString newXdgApplication, newXdgApplications )
      {
         if (newXdgApplication.endsWith(".desktop") || newXdgApplication.endsWith(".kdelnk"))
         {
            kDebug() << "MenueditComponent: New .desktop file " << (xdgApplications + newXdgApplication);
            result = KioskRun::self()->install(xdgApplications + newXdgApplication, xdgSaveApplications + newXdgApplication);
            if (!result) return false;
         }
      }
   }

   // Install .directory files
   {
      QString xdgDirectories = KioskRun::self()->locateLocal("xdgdata-dirs", QString());
      QString xdgSaveDirectories = KioskRun::self()->locateSave("xdgdata-dirs", QString());

      QDir dir(xdgDirectories);
      QStringList newXdgDirectories = dir.entryList(QDir::TypeMask|QDir::NoDotAndDotDot, QDir::Unsorted);

      foreach( QString newXdgDirectory, newXdgDirectories  )
      {
         if (newXdgDirectory.endsWith(".directory"))
         {
            kDebug() << "MenueditComponent: New .directory file " << (xdgDirectories + newXdgDirectory);
            result = KioskRun::self()->install(xdgDirectories + newXdgDirectory, xdgSaveDirectories + newXdgDirectory);
            if (!result) return false;
         }
      }
   }

   KioskRun::self()->forceSycocaUpdate();

   return true;
}



#include "menueditComponent.moc"
