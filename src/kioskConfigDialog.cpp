/*
 *   kioskConfigDialog.cpp
 *
 *   Copyright (C) 2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
 *   Copyright (C) 2015 Jiří Holomek <jiri.holomek@gmail.com>
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
#include "kioskConfigDialog.h"

#include <QCheckBox>

#include <kapplication.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <knuminput.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <kglobal.h>

#include "kioskrun.h"


KioskConfigDialog::KioskConfigDialog(QWidget *parent)
 : KDialog(parent )
{
    setCaption( i18n("Configure Kiosk Admin Tool") );
    setButtons( KDialog::Ok|KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    setModal( true );
  w = new KioskConfigDialogUI(this);
  w->lineProfilePrefix->setMode(KFile::Directory | KFile::LocalOnly);

  // Deprecated JH
  // w->lineUpload->setMode(KFile::Directory);

  setMainWidget(w);

  init();
  connect(w->lineProfilePrefix, SIGNAL(textChanged( const QString& )), SLOT(updateExample()));

  // Deprecated JH
  //connect(w->lineUpload, SIGNAL(textChanged( const QString& )), SLOT(updateExample()));
  //connect(w->lineUploadPrefix, SIGNAL(textChanged( const QString& )), SLOT(updateExample()));
}

KioskConfigDialog::~KioskConfigDialog()
{
    delete w;
}

void KioskConfigDialog::init()
{
   QString prefix = KioskRun::self()->getProfilePrefix();

   if (prefix.isEmpty())
   {
      w->checkProfilePrefix->setChecked(false);
      w->lineProfilePrefix->setUrl(KUrl( "/etc/kde-profile/" ));
   }
   else
   {
      w->checkProfilePrefix->setChecked(true);
      w->lineProfilePrefix->setUrl(prefix);
   }

   KConfigGroup grp(KGlobal::config(), "General");

   // Deprecated JH
   /*
   QString uploadURL = grp.readEntry("uploadURL");
   if (uploadURL.isEmpty())
   {
      w->checkUpload->setChecked(false);
      w->lineUpload->setUrl(KUrl( "fish://root@host/" ));
   }
   else
   {
      w->checkUpload->setChecked(true);
      w->lineUpload->setUrl(uploadURL);
   }
   w->lineUploadPrefix->setText(grp.readEntry("uploadPrefix"));
   */

   // FIXME: get rid of magic values. the default here should be configurable by the packagers
   w->checkUID->setChecked(grp.readEntry("filterUID", true));
   w->numUID->setValue(grp.readEntry("FirstUIDShown", DEFAULT_MIN_UID));

   w->checkGID->setChecked(grp.readEntry("filterGID", true));
   w->numGID->setValue(grp.readEntry("FirstGIDShown", DEFAULT_MIN_GID));

   updateExample();
}

void KioskConfigDialog::updateExample()
{
   // Deprecated JH
   //QString uploadPrefix = w->lineUploadPrefix->text();
   QString uploadPrefix = "";
   QString file1 = w->lineProfilePrefix->url().path()+"default";
   QString file2 = file1;
   if (file2.startsWith(uploadPrefix))
      file2 = file2.mid(uploadPrefix.length());
   if (file2.startsWith("/"))
      file2 = file2.mid(1);
   // Deprecated JH
   //QString url = w->lineUpload->url().path();
   QString url = "";
   if (!url.endsWith("/"))
      url += "/";
   url += file2;
   QString example = QString("<qt><center><b>%1</b><br>--><br><b>%2</b></center>").arg(file1, url);

   // Deprecated JH
   //w->lblUploadExample->setText(example);
   //w->lblUploadExample->setFixedSize(QSize(500,fontMetrics().lineSpacing()*3 + 6));
}

bool KioskConfigDialog::save()
{
   QString uploadURL;
   QString uploadPrefix;
   QString prefix;

   uploadPrefix = "";
   // Deprecated JH
   /*
   uploadPrefix = w->lineUploadPrefix->text();

   if (w->checkUpload->isChecked())
      uploadURL = w->lineUpload->url().path();

   if (w->checkProfilePrefix->isChecked())
      prefix = w->lineProfilePrefix->url().path();
   */

   KConfigGroup grp( KGlobal::config(), "General");
   grp.writeEntry("uploadURL", uploadURL);
   grp.writeEntry("uploadPrefix", uploadPrefix);
   grp.writeEntry("filterUID", w->checkUID->isChecked());
   grp.writeEntry("FirstUIDShown", w->numUID->value());
   grp.writeEntry("filterGID", w->checkGID->isChecked());
   grp.writeEntry("FirstGIDShown", w->numGID->value());
   grp.sync();

   return KioskRun::self()->setProfilePrefix(prefix);
}

#include "kioskConfigDialog.moc"
