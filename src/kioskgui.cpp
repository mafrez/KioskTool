/*
 * kioskgui.cpp
 *
 *   Copyright (C) 2003,2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
 *   Copyright (C) 2015 Jiří Holomek <jiri.holomek@google.com>
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

#include <config.h>

#include "kioskgui.h"

#include <QDir>
#include <QLineEdit>
#include <QObject>
#include <QStackedWidget>
#include <QPushButton>
#include <QPixmap>
#include <QMenu>

#include <KDE/KApplication>
#include <KDE/KActionCollection>
#include <KDE/KAction>
#include <KDE/KComboBox>
#include <KDE/KConfig>
#include <KDE/KFileDialog>
#include <KDE/KGlobal>
#include <KDE/KIconLoader>
#include <KDE/KListWidget>
#include <KDE/KLocale>
#include <KDE/KMenuBar>
#include <KDE/KMessageBox>
#include <KDE/KStatusBar>
#include <kstdaccel.h>
#include <KDE/KStandardAction>
#include <KDE/KStandardDirs>
#include <KDE/KUrlRequester>
#include <KDE/KToggleAction>
#include <KDE/KAuth/Action>

#include <kdebug.h>

#include "kioskConfigDialog.h"
#include "profilePropsDialog.h"
#include "kioskdata.h"
#include "kioskrun.h"
#include "kiosksync.h"
#include <QStandardItemModel>
#include "ProfileManager.h"
#include "ProfileSelectorDialog.h"
#include "profileeditgui.h"

void doMbox(QString display)
{
    QMessageBox msgBox;
    msgBox.setText(display);
    msgBox.exec();
}

KioskGui::KioskGui()
        : KXmlGuiWindow(), m_data(0), m_profile(), m_componentData(0)
{
    m_run = new KioskRun(this);
    m_widget = new MainWidget(this);
    m_emergencyExit = false;

    userModel = new QStandardItemModel(this);
    KConfigGroup config( KGlobal::config(),"General");

    groupModel = new QStandardItemModel(this);
    profileModel = new QStandardItemModel(this);
    m_profmanager = new ProfileManager(m_run, userModel, groupModel, profileModel);

    // check if there is a Kiosk profile infrastructure in place
    if(m_profmanager->needsSetup())
    {
        // no = we need one to continue
        KMessageBox::information(this, i18n("<qt>There is no profile mapping file present. An empty one will be created.</qt>"));
        if(!m_profmanager->InitialSetup(true))
        {
            KMessageBox::sorry(this, i18n("<qt>Kiosk Admin Tool can't continue without the profile mapping file.</qt>"));
            m_emergencyExit = true;
            this->close();
        }
    }

    // set up the filter proxys
    userFilter = new UIDFilterModel(this,m_profmanager,config.readEntry("FirstUIDShown", DEFAULT_MIN_UID));
    userFilter->setSourceModel(userModel);
    connect(userModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), userFilter, SLOT(invalidate()));

    groupFilter = new GIDFilterModel(this,m_profmanager,config.readEntry("FirstGIDShown", DEFAULT_MIN_GID));
    groupFilter->setSourceModel(groupModel);
    connect(groupModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), groupFilter, SLOT(invalidate()));
    setupActions();

    // tell the KMainWindow that this is indeed the main widget
    setCentralWidget(m_widget);
    createGUI();

    connect(m_widget->btnAssignGroupProfile, SIGNAL(clicked()), this, SLOT(slotAssignGroupProfile()));
    connect(m_widget->btnClearGroupProfile, SIGNAL(clicked()), this, SLOT(slotClearGroupProfile()));

    // TO BE IMPLEMENTED JH
    //connect(m_widget->btnMoveGroupUp, SIGNAL(clicked()), this, SLOT(slotPromoteGroup()));
    //connect(m_widget->btnMoveGroupDown, SIGNAL(clicked()), this, SLOT(slotDemoteGroup()));
    //connect(m_widget->btnCopyGroupProfile, SIGNAL(clicked()), this, SLOT(slotCopyGroupProfile()));
    //connect(m_widget->btnPasteGroupProfile, SIGNAL(clicked()), this, SLOT(slotPasteGroupProfile()));

    connect(m_widget->btnAssignUserProfile, SIGNAL(clicked()), this, SLOT(slotAssignUserProfile()));
    connect(m_widget->btnClearUserProfile, SIGNAL(clicked()), this, SLOT(slotClearUserProfile()));

    // TO BE IMPLEMENTED JH
    //connect(m_widget->btnCopyUserProfile, SIGNAL(clicked()), this, SLOT(slotCopyUserProfile()));
    //connect(m_widget->btnPasteUserProfile, SIGNAL(clicked()), this, SLOT(slotPasteUserProfile()));

    connect(m_widget->btnNewProfile, SIGNAL(clicked()), this, SLOT(slotCreateProfile()));
    connect(m_widget->btnDeleteProfile, SIGNAL(clicked()), this, SLOT(slotDeleteProfile()));
    connect(m_widget->btnSetupProfile, SIGNAL(clicked()), this, SLOT(slotEditProfile()));
    connect(m_widget->btnProfileProperties, SIGNAL(clicked()), this, SLOT(slotProfileProperties()));

    m_widget->btnCommit1->setGuiItem(KStandardGuiItem::Save);
    m_widget->btnCommit2->setGuiItem(KStandardGuiItem::Save);
    m_widget->btnCommit3->setGuiItem(KStandardGuiItem::Save);

    m_widget->btnDiscard1->setGuiItem(KStandardGuiItem::discard());
    m_widget->btnDiscard2->setGuiItem(KStandardGuiItem::discard());
    m_widget->btnDiscard3->setGuiItem(KStandardGuiItem::discard());
    slotDirtyChanged(false);

    m_widget->userList->setModel(userFilter);
    m_widget->groupList->setModel(groupFilter);
    m_widget->profileList->setModel(profileModel);

    connect(m_widget->btnCommit1, SIGNAL(clicked()), this, SLOT(slotCommit()));
    connect(m_widget->btnCommit2, SIGNAL(clicked()), this, SLOT(slotCommit()));
    connect(m_widget->btnCommit3, SIGNAL(clicked()), this, SLOT(slotCommit()));

    connect(m_widget->btnDiscard1, SIGNAL(clicked()), this, SLOT(slotDiscard()));
    connect(m_widget->btnDiscard2, SIGNAL(clicked()), this, SLOT(slotDiscard()));
    connect(m_widget->btnDiscard3, SIGNAL(clicked()), this, SLOT(slotDiscard()));

    connect(m_profmanager, SIGNAL(dirtyChanged(bool)),this,SLOT(slotDirtyChanged(bool)));

    // apply the saved mainwindow settings, if any, and ask the mainwindow
    // to automatically save settings if changed: window size, toolbar
    // position, icon size, etc.
    setAutoSaveSettings();
    QTimer::singleShot(0, this, SLOT(slotCheckEtcSkel()));
}

KioskGui::~KioskGui()
{
    delete m_data;
}

void KioskGui::slotCheckEtcSkel()
{
    QString etcSkel = "/etc/skel/" KDE_DEFAULT_HOME;
    KioskSync skelDir;
    skelDir.addDir(etcSkel, KUrl());
    QStringList skelFiles = skelDir.listFiles();
    if (!skelFiles.isEmpty())
    {
        KMessageBox::informationList(this,
                                     i18n("<qt>Your system contains KDE configuration settings in the "
                                          "skeleton directory <b>%1</b>. These files are copied to the "
                                          "personal KDE settings directory of newly created users.<p>"
                                          "This may interfere with the correct operation of user profiles.<p>"
                                          "Unless a setting has been locked down, settings that have been copied "
                                          "to the personal KDE settings directory of a user will override "
                                          "a default setting configured in a profile.<p>"
                                          "<b>If this is not the intended behavior, please remove the offending "
                                          "files from the skeleton folder on all systems that you want to "
                                          "administer with user profiles.</b><p>"
                                          "The following files were found under <b>%2</b>:", etcSkel, etcSkel),
                                     skelFiles,
                                     QString(),
                                     "etc_skel_warning");
    }
}

void KioskGui::setupActions()
{
    actionCollection()->addAction( KStandardAction::Quit, this, SLOT( close() ) );
    KStandardAction::preferences(this, SLOT(slotConfig()), actionCollection());

    gidFilterAction = new KAction(this);
    gidFilterAction->setText(i18n("Filter &groups"));
    gidFilterAction->setIcon(KIcon("kiosktool_filt_gid"));
    gidFilterAction->setShortcut(Qt::CTRL + Qt::Key_G);
    gidFilterAction->setCheckable(true);
    actionCollection()->addAction("filter_gids", gidFilterAction);
    connect(gidFilterAction, SIGNAL(toggled(bool)), this, SLOT(toggle_gid_filter(bool)));

    uidFilterAction = new KAction(this);
    uidFilterAction->setText(i18n("Filter &users"));
    uidFilterAction->setIcon(KIcon("kiosktool_filt_uid"));
    uidFilterAction->setShortcut(Qt::CTRL + Qt::Key_U);
    uidFilterAction->setCheckable(true);
    actionCollection()->addAction("filter_uids", uidFilterAction);
    connect(uidFilterAction, SIGNAL(toggled(bool)), this, SLOT(toggle_uid_filter(bool)));

    KStandardAction::quit(kapp, SLOT(quit()), actionCollection());
    updateActions();
}

void KioskGui::toggle_gid_filter(bool enabled)
{
    KConfigGroup grp( KGlobal::config(), "General");
    grp.writeEntry("filterGID", enabled);
    updateFilters();
}

void KioskGui::toggle_uid_filter(bool enabled)
{
    KConfigGroup grp( KGlobal::config(), "General");
    grp.writeEntry("filterUID", enabled);
    updateFilters();
}

void KioskGui::slotPKLAConvert()
{
    QVariantMap helperargs;
    KAuth::Action action("org.kde.kiosk.pklahelper.convertall");
    action.setHelperID("org.kde.kiosk.pklahelper");
    action.setArguments(helperargs);

    KAuth::ActionReply reply = action.execute();

    if (reply.failed())
    {
        if (reply.type() == KAuth::ActionReply::KAuthError)
        {
            // There has been an internal KAuth error
            KMessageBox::sorry(this,
                               i18n("<qt>Unable to authenticate/execute the action: %1 %2.</qt>",
                                    reply.errorCode(),
                                    reply.errorDescription()));
        }
        else
        {
            // There has been an internal KAuth error
            KMessageBox::sorry(this,
                               i18n("<qt>Internal error: %1 %2.</qt>",
                                    reply.errorCode(),
                                    reply.errorDescription()));
        }
    }
    else
    {
        QVariantMap retdata = reply.data();
        if(retdata.isEmpty())
        {
            KMessageBox::sorry(this,
                               i18n("<qt>Reply is empty, helper didn't work :(</qt>"));
        }
        else
        {
            KMessageBox::information(this, i18n("<qt>Conversion was successful.</qt>"));
        }
    }
}

void KioskGui::updateActions()
{
    KConfigGroup config( KGlobal::config(), "General");

    // Deprecated JH
    //bool uploadEnabled = !config.readEntry("uploadURL").isEmpty();

    uidFilterAction->setChecked(config.readEntry("filterUID", true));
    gidFilterAction->setChecked(config.readEntry("filterGID", true));
}

void KioskGui::updateFilters()
{
    KConfigGroup config( KGlobal::config(),"General");
    if(config.readEntry("filterUID", true))
    {
        K_UID minUID = config.readEntry("FirstUIDShown", DEFAULT_MIN_UID);
        userFilter->setFilterMinimumUID(minUID);
    }
    else
    {
        userFilter->setFilterMinimumUID(0);
    }

    if(config.readEntry("filterGID", true))
    {
        K_GID minGID = config.readEntry("FirstGIDShown", DEFAULT_MIN_GID);
        groupFilter->setFilterMinimumGID(minGID);
    }
    else
    {
        groupFilter->setFilterMinimumGID(0);
    }
}

void KioskGui::saveProperties(KConfigGroup& /*config*/ )
{
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored
}

void KioskGui::readProperties(const KConfigGroup & /*config*/ )
{
    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'
}

void KioskGui::slotCommit()
{
    if (!m_profmanager->Save())
    {
        doMbox("Couldn't save profile mappings!");
    }
}

void KioskGui::slotDiscard()
{
    if (m_profmanager->Reload())
    {
        m_profmanager->UpdateUserModel();
        m_profmanager->UpdateGroupModel();
        m_profmanager->UpdateProfileModel();
    }
}

void KioskGui::slotProfileProperties()
{
    QItemSelectionModel * smod= m_widget->profileList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows();
    if (!sel_rows.isEmpty())
    {
        QString profile = profileModel->data(sel_rows[0]).toString();
        int result = KMessageBox::Continue;

        if (m_profmanager->isDirty())
        {
            result = KMessageBox::warningContinueCancel(this,
                     i18n("<qt>You are about to change the basic properties of the <b>%1</b> profile.<p>"
                          "This will discard any changes you made in the profile assignments.", profile),
                     QString::null, KStandardGuiItem::discard());
        }
        if (result == KMessageBox::Continue)
        {
            ProfilePropsDialog dlg(this, profile, m_profmanager->getAllProfiles());
            if (dlg.exec())
            {
                QString newprofname;
                if (dlg.save(newprofname))
                {
                    m_profmanager->renameProfile(profile, newprofname);
                    m_profmanager->Reload();
                    updateActions();
                }
            }
            slotDiscard();
        }
    }
}

void KioskGui::slotCreateProfile()
{
    int result = KMessageBox::Continue;

    if (m_profmanager->isDirty())
    {
        result = KMessageBox::warningContinueCancel(this,
                 i18n("<qt>You are about to create a new profile.<p>"
                      "This will discard any changes you made in the profile assignments."),
                 QString::null, KStandardGuiItem::discard());
    }
    if (result == KMessageBox::Continue)
    {
        ProfilePropsDialog dlg(this, QString(), m_profmanager->getAllProfiles());
        if (dlg.exec())
        {
            QString newprofname;
            if (dlg.save(newprofname))
            {
                m_profmanager->Reload();
                updateActions();
            }
        }
        slotDiscard();
    }
}

void KioskGui::slotEditProfile()
{
    QItemSelectionModel * smod= m_widget->profileList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows();
    if (!sel_rows.isEmpty())
    {
        QString profile = profileModel->data(sel_rows[0]).toString();
        ProfileEditGui dlg(this, profile, m_run);
        if (dlg.exec())
        {
            /*
            if (dlg.save())
            {
                m_profmanager->Reload();
                updateActions();
            }
            */
        }
    }
}

void KioskGui::slotDeleteProfile()
{
    QItemSelectionModel * smod= m_widget->profileList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows();
    if (!sel_rows.isEmpty())
    {
        QModelIndex rowIndex = sel_rows[0];
        QVariant index = profileModel->data(rowIndex);
        if(index.toString() == "default")
        {
            KMessageBox::sorry(this, i18n("<qt>The default profile is not to be deleted.</qt>"));
            return;
        }
        int result = KMessageBox::warningContinueCancel(this,
                     i18n("<qt>You are about to delete the profile <b>%1</b>.<p>"
                          "Are you sure you want to do this?<p>"
                          "You can revert this by using the <b>discard</b> button,<br>which will <b>discard all the changes</b>.", index.toString()),
                     QString::null, KStandardGuiItem::del());

        if (result == KMessageBox::Continue)
        {
            if (m_profmanager->DeleteProfile(index.toString()))
            {
                // FIXME: bogus
                //smod->select(groupModel->index(rowIndex.row() - 1, 0),QItemSelectionModel::Rows);
                // blah
            }
        }
    }
    userFilter->invalidate();
}

void KioskGui::slotConfig()
{
    KioskConfigDialog dlg(this);
    if (dlg.exec())
    {
        dlg.save();
        updateActions();
        updateFilters();
    }
}

void KioskGui::slotDirtyChanged(bool dirty)
{
    m_widget->btnCommit1->setEnabled(dirty);
    m_widget->btnCommit2->setEnabled(dirty);
    m_widget->btnCommit3->setEnabled(dirty);

    m_widget->btnDiscard1->setEnabled(dirty);
    m_widget->btnDiscard2->setEnabled(dirty);
    m_widget->btnDiscard3->setEnabled(dirty);
}


void KioskGui::slotCopyUserProfile()
{
    doMbox("Copy User Profile : Unimplemented");
}

void KioskGui::slotPasteUserProfile()
{
    doMbox("Paste User Profile : Unimplemented");
}

void KioskGui::slotCopyGroupProfile()
{
    doMbox("Copy Group Profile : Unimplemented");
}

void KioskGui::slotPasteGroupProfile()
{
    doMbox("Paste Group Profile : Unimplemented");
}

void KioskGui::slotAssignUserProfile()
{
    QItemSelectionModel * smod= m_widget->userList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows(1);
    if (!sel_rows.isEmpty())
    {
        QString user = userFilter->data(sel_rows[0]).toString();
        ProfileSelectorDialog dlg(this, i18n("Select a profile for user %1:",user), profileModel);
        if (dlg.exec())
        {
            QString profile = dlg.getSelectedProfile();
            m_profmanager->AssignUserProfile(user,profile);
        }
    }
}

void KioskGui::slotClearUserProfile()
{
    QItemSelectionModel * smod= m_widget->userList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows(1);
    if (!sel_rows.isEmpty())
    {
        m_profmanager->ClearUserProfile(userFilter->data(sel_rows[0]).toString());
    }
}

void KioskGui::slotAssignGroupProfile()
{
    QItemSelectionModel * smod= m_widget->groupList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows(2);
    if (!sel_rows.isEmpty())
    {
        QString grpname = groupFilter->data(sel_rows[0]).toString();
        ProfileSelectorDialog dlg(this, i18n("Select a profile for group %1:",grpname), profileModel);
        if (dlg.exec())
        {
            QString profile = dlg.getSelectedProfile();
            qDebug() << "group" << grpname << "assign" << profile;
            if(!grpname.isEmpty())
                m_profmanager->AssignGroupProfile(grpname,profile);
        }
    }
}

void KioskGui::slotClearGroupProfile()
{
    QItemSelectionModel * smod= m_widget->groupList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows(2);
    if (!sel_rows.isEmpty())
    {
        m_profmanager->ClearGroupProfile(groupFilter->data(sel_rows[0]).toString());
    }
}

void KioskGui::slotPromoteGroup()
{
    QItemSelectionModel * smod= m_widget->groupList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows();
    if (!sel_rows.isEmpty())
    {
        QModelIndex rowIndex = sel_rows[0];
        QVariant index = groupModel->data(rowIndex);
        if (!index.toString().isEmpty())
        {
            if (m_profmanager->PromoteGroup(index.toInt() - 1))
            {
                // FIXME: doesn't work for some reason ... maybe it needs to wait for the model to propagate data to the view
                smod->select(groupModel->index(rowIndex.row() - 1, 0),QItemSelectionModel::Rows);
            }
        }
    }
}

void KioskGui::slotDemoteGroup()
{
    QItemSelectionModel * smod= m_widget->groupList->selectionModel();
    QModelIndexList sel_rows = smod->selectedRows();
    if (!sel_rows.isEmpty())
    {
        QModelIndex rowIndex = sel_rows[0];
        QVariant index = groupModel->data(rowIndex);
        if (!index.toString().isEmpty())
        {
            if (m_profmanager->DemoteGroup(index.toInt() - 1))
            {
                // FIXME: doesn't work for some reason ... maybe it needs to wait for the model to propagate data to the view
                smod->select(groupModel->index(rowIndex.row() + 1, 0),QItemSelectionModel::Rows);
            }
        }
    }
}


bool KioskGui::queryClose()
{
    if(m_emergencyExit) return true;
    if (!m_profmanager->Save())
    {
        int result = KMessageBox::warningContinueCancel(
                         this,
                         i18n("Your changes could not be saved, do you want to quit anyway?"),
                         QString::null,
                         KStandardGuiItem::quit());

        if (result == KMessageBox::Continue)
            return true;
        return false;
    }

    return true;
}

// FIXME: move logic elsewhere, it doesn't belong into the GUI part!
// FIXME: bugged beyond recognition
void KioskGui::uploadAllProfiles()
{

    KConfigGroup config( KGlobal::config(), "General");
    QString uploadPrefix = config.readEntry("uploadPrefix");
    QString uploadURL = config.readEntry("uploadURL");

    KioskSync sync(this);

    QStringList profiles = KioskRun::self()->allProfiles();

    foreach( QString profile, profiles)
    {
        QString description;
        QString installUser;
        QString installDir;

        KioskRun::self()->getProfileInfo(profile, description, installDir, installUser);

//      sync.addDir(installDir, KUrl("ftp://localhost/kde/profiles"));

        QString dir = installDir;
        if (dir.startsWith(uploadPrefix))
            dir = dir.mid(uploadPrefix.length());
        if (dir.startsWith("/"))
            dir = dir.mid(1);

        KUrl url(uploadURL);
        url.setPath(url.path( KUrl::AddTrailingSlash)+dir);
        sync.addDir(installDir, url);
        qDebug() << "sync" << installDir << url.pathOrUrl();
    }


    //FIXME: possibly bugged! caused segfaults during testing.
    if (sync.sync())
    {
        KMessageBox::information(this, i18n("<qt>All profiles have been successfully uploaded to <b>%1</b>",uploadURL));
    }

}

void KioskGui::uploadCurrentProfile()
{
}

void KioskGui::help()
{

}

#include "kioskgui.moc"
