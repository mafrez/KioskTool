/*
 *   profilePropsDialog.cpp
 *
 *   Copyright (C) 2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
 *   Copyright (C) 2010 Petr Mrázek <peterix@gmail.com>
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

#include "profilePropsDialog.h"

#include <QCheckBox>

#include <kapplication.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <knuminput.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <kglobal.h>
#include <KDE/KUser>
#include <KDE/KMessageBox>
#include <KDE/KDebug>

#include "kioskrun.h"
#include "kiosksync.h"
static QStringList userList()
{
    KUser thisUser;
    QStringList result;
    result << thisUser.loginName();
    result << "root";

    KConfigGroup config(KGlobal::config(),"General");
    result += config.readEntry("PreviousUsers",QStringList());
    result.sort();

    // Remove dupes
    return result.toSet().toList();
}


ProfilePropsDialog::ProfilePropsDialog(QWidget* parent, const QString& profile, QStringList allProfiles)
        : KDialog(parent ), m_profile(profile), m_allProfiles(allProfiles)
{
    setCaption( i18n("Configure Kiosk Admin Tool") );
    setButtons( KDialog::Ok|KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    setModal( true );
    w = new ProfilePropsUI(this);
    /*
    w->lineProfilePrefix->setMode(KFile::Directory | KFile::LocalOnly);
    w->lineUpload->setMode(KFile::Directory);
    */
    setMainWidget(w);

    init();
    /*
    connect(w->lineProfilePrefix, SIGNAL(textChanged( const QString& )), SLOT(updateExample()));
    connect(w->lineUpload, SIGNAL(textChanged( const QString& )), SLOT(updateExample()));
    connect(w->lineUploadPrefix, SIGNAL(textChanged( const QString& )), SLOT(updateExample()));
    */
}

ProfilePropsDialog::~ProfilePropsDialog()
{
    delete w;
}

void ProfilePropsDialog::slotProfileNameChanged()
{
    QString profile = w->editProfileName->text();
    if (m_fixedProfileDir)
    {
        QString profilePrefix = KioskRun::self()->getProfilePrefix();
        QString installDir = profilePrefix+profile+"/";
        w->labelInstallDir->setText(installDir);
    }
    // TODO:  enableButtonOk(!profile.isEmpty());
}

void ProfilePropsDialog::init()
{
    bool bNewProfile = false;
    if (m_profile.isEmpty())
    {
        m_profile = KioskRun::self()->newProfile();
        bNewProfile = true;
    }

    QString profilePrefix = KioskRun::self()->getProfilePrefix();
    m_fixedProfileDir = !profilePrefix.isEmpty();
    
    connect(w->editProfileName, SIGNAL(textChanged(const QString&)),
            this, SLOT(slotProfileNameChanged()));

#if 0
    connect(kurlInstallDir, SIGNAL(textChanged(const QString&)),
            this, SLOT(updateButtons()));
#endif

    w->comboUser->setEditable(true);
    w->comboUser->addItems(userList());

    QRegExp rx( "[^/ :]*" );
    QValidator* validator = new QRegExpValidator( rx, this );

    w->editProfileName->setValidator(validator);
    w->editProfileName->setFocus();

    QString description;
    QString installDir;
    QString installUser;

    KioskRun::self()->getProfileInfo(m_profile, description, installDir, installUser);

    if (!bNewProfile)
    {
        m_origProfile = m_profile;
        m_origInstallDir = installDir;
    }

    w->editProfileName->setText(m_profile);
    w->editDescription->setText(description);
    if (m_fixedProfileDir)
    {
        delete w->kurlInstallDir;
        w->labelInstallDir->setReadOnly(true);
        w->labelInstallDir->setText(installDir);
        setTabOrder(w->editDescription, w->comboUser);
        setTabOrder(w->comboUser, w->labelInstallDir);
    }
    else
    {
        delete w->labelInstallDir;
        w->kurlInstallDir->setMode(KFile::Directory);
        w->kurlInstallDir->setUrl(installDir);
        setTabOrder(w->editDescription, w->comboUser);
        setTabOrder(w->comboUser, w->kurlInstallDir);
    }
    w->comboUser->setEditText(installUser);
}

bool ProfilePropsDialog::save(QString & out)
{
    QString user = w->comboUser->currentText();
    KUser userInfo(user);
    if (!userInfo.isValid())
    {
        KMessageBox::sorry(this, i18n("<qt>The user <b>%1</b> is not an existing user.</qt>").arg(user));
        w->comboUser->setFocus();
        return false;
    }

    m_profile = w->editProfileName->text();
    if(m_profile != m_origProfile)
    {
        if(m_allProfiles.contains(m_profile))
        {
            KMessageBox::sorry(this, i18n("<qt>There already is a profile with the name <b>%1</b>.</qt>").arg(m_profile));
            w->editProfileName->setFocus();
            return false;
        }
    }
    out = m_profile;
    QString description = w->editDescription->text();
    QString installDir;
    if (m_fixedProfileDir)
    {
        installDir = w->labelInstallDir->text();
    }
    else
    {
        installDir = w->kurlInstallDir->url().path();
    }

    if (!installDir.endsWith("/"))
        installDir.append("/");

    if (!m_origInstallDir.isEmpty() && (installDir != m_origInstallDir))
    {
        KioskSync origInstallDir;
        origInstallDir.addDir(m_origInstallDir, KUrl());
        QStringList fileList = origInstallDir.listFiles();
        fileList.removeAll(".kdeprofile");
        if (!fileList.isEmpty())
        {
            int msgResult = KMessageBox::warningContinueCancelList(this,
                            i18n("<qt>The directory for this profile has changed "
                                 "from <b>%1</b> to <b>%2</b>.<p>"
                                 "The following files under <b>%3</b> will be moved to <b>%4</b>")
                            .arg(m_origInstallDir, installDir, m_origInstallDir, installDir),
                            fileList,
                            i18n("Profile Directory Changed"));
            if (msgResult != KMessageBox::Continue)
                return false;
        }
        KioskRun::self()->setUser(user);
        if (!KioskRun::self()->move(m_origInstallDir, installDir, fileList))
            return false;
        if (QDir(m_origInstallDir).exists())
        {
            if (!KioskRun::self()->remove(m_origInstallDir))
                return false;
        }
    }
    QString installUser = user;

    bool result = KioskRun::self()->setProfileInfo( m_profile, description, installDir, installUser);

    kDebug() << "here" << result << m_profile << description << installDir << installUser;

    if (result && !m_origProfile.isEmpty() && (m_origProfile != m_profile))
    {
        result = KioskRun::self()->deleteProfile( m_origProfile, false );
    }

    // Store this user for easy access later
    KConfigGroup config(KGlobal::config(),"General");
    QStringList previousUsers= config.readEntry("PreviousUsers",QStringList());
    if (!previousUsers.contains(user))
    {
        previousUsers << user;
        config.writeEntry("PreviousUsers", previousUsers);
        config.sync();
    }

    return result;
    /*
    QString uploadURL;
    QString uploadPrefix;
    QString prefix;
    int minUID = 0;

    uploadPrefix = w->lineUploadPrefix->text();

    if (w->checkUpload->isChecked())
      uploadURL = w->lineUpload->url().path();

    if (w->checkProfilePrefix->isChecked())
      prefix = w->lineProfilePrefix->url().path();

    if (w->checkUID->isChecked())
      minUID = w->numUID->value();

    KConfigGroup grp( KGlobal::config(), "General");
    grp.writeEntry("uploadURL", uploadURL);
    grp.writeEntry("uploadPrefix", uploadPrefix);
    grp.writeEntry("FirstUIDShown", minUID);
    grp.sync();

    return KioskRun::self()->setProfilePrefix(prefix);
    */
    return false;
}

#include "profilePropsDialog.moc"
