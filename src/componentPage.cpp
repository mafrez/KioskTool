/*
 *   componentPage.cpp
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

#include "componentPage.h"

#include <QCursor>
#include <QFileInfo>
#include <QLabel>
#include <QByteArray>
#include <QTextEdit>
#include <QPushButton>


#include <KDE/KApplication>
#include <kdebug.h>
#include <KDE/KListWidget>
#include <KDE/KLocale>
#include <KDE/KMessageBox>
#include <KDE/KProcess>
#include <KDE/KStandardGuiItem>
#include <KDE/KToolInvocation>

#include "component.h"
#include "kioskdata.h"
#include "kioskrun.h"

ComponentPage::ComponentPage( ComponentData *data, Component *component , const QString &profileName, QWidget* parent )
 : QWidget(parent), PageWidget(profileName, this), m_data(data), m_component(component), m_profileName(profileName)
{
  setupUi(this);
  m_process = 0;
  //connect(pbSetup, SIGNAL(clicked()), this, SLOT(slotSetup()));
  //connect(pbPreview, SIGNAL(clicked()), this, SLOT(slotPreview()));

  pbSetup->setText(i18n("&Setup %1",m_data->caption));
  pbPreview->setText(i18n("&Preview %1",m_data->caption));

  if (m_data->preview.exec.isEmpty())
     pbPreview->hide();

  if (m_data->setup.exec.isEmpty())
     pbSetup->hide();

  if (!pbPreview->isHidden() && !pbSetup->isHidden())
  {
     static bool firstTime = true;

     if (firstTime)
     {
        firstTime = false;
        QTimer::singleShot(0, this, SLOT(slotShowNotice()));
     }
  }

  fillActionList(listComponentConfig, m_data);

  connect(listComponentConfig, SIGNAL(itemClicked(QListWidgetItem*)),
          this, SLOT(slotShowAction(QListWidgetItem *)));
  connect(normalKey, SIGNAL(clicked()), SLOT(slotUpdateValue()));
  connect(deletedKey, SIGNAL(clicked()), SLOT(slotUpdateValue()));
  connect(imutableKey, SIGNAL(clicked()), SLOT(slotUpdateValue()));
  connect(shellExpansionKey, SIGNAL(clicked()), SLOT(slotUpdateValue()));
  connect(defaultValue, SIGNAL(textChanged(QString)), SLOT(slotUpdateValue()));

  slotShowAction(listComponentConfig->currentItem());
}

ComponentPage::~ComponentPage()
{
   delete m_component;
}

void ComponentPage::slotShowNotice()
{
   KMessageBox::information(this,
         i18n("Selecting the Setup or Preview option may cause the panel and/or the desktop to be temporarily shut down. "
              "To prevent data loss please make sure you are not actively using these components."),
         i18n("Attention"), "shutdown_notice");
}

void ComponentPage::load()
{
}

bool ComponentPage::save()
{
   return saveActionListChanges(listComponentConfig);
}

void ComponentPage::setFocus()
{
   listComponentConfig->setFocus();
}

QString ComponentPage::subCaption()
{
  return i18n("Setup %1",m_data->caption);
}
/*
void ComponentPage::slotSetup()
{
   if (m_process)
   {
      m_process->kill();
      delete m_process;
   }
#ifdef PORTED
   QByteArray dcopApp = m_data->setup.dcop.utf8();
   QByteArray dcopObj = "qt/" + dcopApp;
   if (!dcopApp.isEmpty() && m_data->setup.hasOption("restart"))
      DCOPRef(dcopApp, dcopObj).call("quit");
#endif

   QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
   if (!KioskRun::self()->prepare())
   {
      QApplication::restoreOverrideCursor();
      KMessageBox::sorry(this,
          i18n("<qt>There was an unexpected problem with the runtime environment.</qt>"));
      return;
   }
   KioskRun::self()->makeMutable(m_profileName, true);
   if (!save())
   {
      QApplication::restoreOverrideCursor();
      return;
   }
   if (m_component)
      m_component->slotSetupPrepare();
   KioskRun::self()->updateSycoca();
   QApplication::restoreOverrideCursor();

#ifdef PORTED
   if (!dcopApp.isEmpty())
   {
      KioskRun::self()->dcopClient()->setNotifications(true);
      connect(KioskRun::self()->dcopClient(), SIGNAL(applicationRegistered( const QByteArray& )),
           this, SLOT(slotSetupAppRegistered( const QByteArray& )));
   }
#endif

   QStringList args;
   if (m_data->setup.hasOption("nofork"))
      args << "--nofork";

   args += m_data->setup.args;

   m_process = KioskRun::self()->run(m_data->setup.exec, args);
   int result = KMessageBox::questionYesNo(this,
         i18n("<qt>You can now configure %1. "
              "When you are finished click <b>Save</b> to make the new configuration permanent.",
              m_data->caption), i18n("%1 Setup",m_data->caption),
              KStandardGuiItem::save(), KStandardGuiItem::discard());
   m_saveSettings = (result == KMessageBox::Yes);
#ifdef PORTED
   if (!dcopApp.isEmpty())
      KioskRun::self()->dcopRef(dcopApp, dcopObj).call("quit");
#endif

   if (m_process->state() == QProcess::Running)
   {
      connect(m_process, SIGNAL(processExited(KProcess *)), this, SLOT(slotPreviewDone()));
   }
   else
   {
      slotSetupDone();
   }
}
*/
void ComponentPage::slotSetupDone()
{
   delete m_process;
   m_process = 0;

#ifdef PORTED
   KioskRun::self()->dcopClient()->setNotifications(false);
   disconnect(KioskRun::self()->dcopClient(), SIGNAL(applicationRegistered( const QByteArray& )),
              this, SLOT(slotSetupAppRegistered( const QByteArray& )));
#endif

   KioskRun::self()->makeMutable(m_profileName, false);
   if (m_saveSettings)
   {
      bool result = true;
      if (m_component)
         result = m_component->setupFinished();

      if (!result) return;

   /*
      // Find new config files.
      QStringList newFiles = KioskRun::self()->newConfigFiles();
      for(QStringList::ConstIterator it = newFiles.constBegin();
          it != newFiles.constEnd(); ++it)
      {
         if (m_data->ignoreFiles.contains(*it))
         {
            kDebug() << "Ignoring new config file " << (*it) << endl;
            continue;
         }
         KioskRun::self()->mergeConfigFile(*it);
      }
   */

   }

   KioskRun::self()->flushConfigCache();

   if (m_data->setup.hasOption("restart"))
      KToolInvocation::kdeinitExec(m_data->setup.exec);
}

#ifdef PORTED

void ComponentPage::slotSetupAppRegistered( const QByteArray &appid)
{
   QByteArray dcopApp = m_data->setup.dcop.utf8();
   if (dcopApp == appid)
   {
      kDebug() << appid << " is up and running" << endl;
      if (m_component)
         m_component->slotSetupStarted();
   }
}

#else

void ComponentPage::slotSetupAppRegistered( const QByteArray &)
{
}

#endif
/*
void ComponentPage::slotPreview()
{
   if (m_process)
   {
      m_process->kill();
      delete m_process;
   }
#ifdef PORTED
   QByteArray dcopApp = m_data->preview.dcop.utf8();
   QByteArray dcopObj = "qt/" + dcopApp;
   if (!dcopApp.isEmpty() && m_data->preview.hasOption("restart"))
      DCOPRef(dcopApp, dcopObj).call("quit");
#endif

   QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
   KioskRun::self()->prepare();
   save();
   KioskRun::self()->updateSycoca();
   QApplication::restoreOverrideCursor();

#ifdef PORTED
   if (!dcopApp.isEmpty())
   {
      KioskRun::self()->dcopClient()->setNotifications(true);
      connect(KioskRun::self()->dcopClient(), SIGNAL(applicationRegistered( const QByteArray& )),
           this, SLOT(slotPreviewAppRegistered( const QByteArray& )));
   }
#endif

   QStringList args;
   if (m_data->preview.hasOption("nofork"))
      args << "--nofork";

   args += m_data->preview.args;

   m_process = KioskRun::self()->run(m_data->preview.exec, args);
   KMessageBox::information(this,
         i18n("<qt>This is how %1 will behave and look with the new settings. "
              "Any changes you now make to the settings will not be saved.<p>"
              "Click <b>Ok</b> to return to your own personal %2 configuration.",
              m_data->caption, m_data->caption), i18n("%1 Preview",m_data->caption));
#ifdef PORTED
   if (!dcopApp.isEmpty())
      KioskRun::self()->dcopRef(dcopApp, dcopObj).call("quit");
#endif

   if (m_process->state() == QProcess::Running )
   {
      connect(m_process, SIGNAL(processExited(KProcess *)), this, SLOT(slotPreviewDone()));
   }
   else
   {
      slotPreviewDone();
   }
}
*/
#ifdef PORTED

void ComponentPage::slotPreviewAppRegistered( const QByteArray &appid)
{
   QByteArray dcopApp = m_data->preview.dcop.utf8();
   if (dcopApp == appid)
   {
      kDebug() << appid << " is up and running" << endl;
      if (m_component)
         m_component->slotPreviewStarted();
   }
}

#else

void ComponentPage::slotPreviewAppRegistered( const QByteArray &)
{
}

#endif



void ComponentPage::slotPreviewDone()
{
#ifdef PORTED
   KioskRun::self()->dcopClient()->setNotifications(false);
   disconnect(KioskRun::self()->dcopClient(), SIGNAL(applicationRegistered( const QByteArray& )),
              this, SLOT(slotPreviewAppRegistered( const QByteArray& )));
#endif

   delete m_process;
   m_process = 0;
   if (m_data->preview.hasOption("restart"))
      KToolInvocation::kdeinitExec(m_data->preview.exec);
}

void ComponentPage::slotShowAction(QListWidgetItem *item)
{
   ComponentActionItem *actionItem = dynamic_cast<ComponentActionItem*>(item);
   QString description;
   if (actionItem)
   {
      description = "<h2>" + actionItem->action()->caption +"</h2>";
      description += actionItem->action()->description;
      if( actionItem->action()->type == ComponentAction::ActConfig )
      {
            defaultValueGroup->setEnabled(true);
            defaultValue->clear();
            imutableKey->setChecked(false);
            normalKey->setChecked(true);
            switch( actionItem->action()->defaultValueProperties )
            {
              case KConfigRawEditor::KConfigEntryData::Normal:
                normalKey->setChecked(true);
                break;
              case KConfigRawEditor::KConfigEntryData::Expandable:
                shellExpansionKey->setChecked(true);
                break;
              case KConfigRawEditor::KConfigEntryData::Deleted:
                deletedKey->setChecked(true);
                break;
              case KConfigRawEditor::KConfigEntryData::ImmutableAndExpandable:
                shellExpansionKey->setChecked(true);
                imutableKey->setChecked(true);
                break;
              case KConfigRawEditor::KConfigEntryData::ImmutableAndDeleted:
                deletedKey->setChecked(true);
                imutableKey->setChecked(true);
                break;
              case KConfigRawEditor::KConfigEntryData::Immutable:
                imutableKey->setChecked(true);
                break;
            }
            defaultValue->setText( actionItem->action()->defaultValue );
      }
      else
            defaultValueGroup->setEnabled(false);
   }
   componentDescription->setText(description);
}

void ComponentPage::slotUpdateValue(  )
{
    ComponentActionItem *actionItem = dynamic_cast<ComponentActionItem*>( listComponentConfig->currentItem() );
    if( actionItem )
    {
        actionItem->action()->defaultValue = defaultValue->text();
        if( imutableKey->isChecked() && normalKey->isChecked() )
        {
            actionItem->action()->defaultValueProperties = KConfigRawEditor::KConfigEntryData::Immutable;
        }
        else if( !imutableKey->isChecked() && normalKey->isChecked() )
        {
            actionItem->action()->defaultValueProperties = KConfigRawEditor::KConfigEntryData::Normal;
        }
        else if( imutableKey->isChecked() && deletedKey->isChecked() )
        {
            actionItem->action()->defaultValueProperties = KConfigRawEditor::KConfigEntryData::ImmutableAndDeleted;
        }
        else if( !imutableKey->isChecked() && deletedKey->isChecked() )
        {
            actionItem->action()->defaultValueProperties = KConfigRawEditor::KConfigEntryData::Deleted;
        }
        else if( imutableKey->isChecked() && shellExpansionKey->isChecked() )
        {
            actionItem->action()->defaultValueProperties = KConfigRawEditor::KConfigEntryData::ImmutableAndExpandable;
        }
        else if( !imutableKey->isChecked() && shellExpansionKey->isChecked() )
        {
            actionItem->action()->defaultValueProperties = KConfigRawEditor::KConfigEntryData::Expandable;
        }

        if( defaultValue->text().isEmpty() && normalKey->isChecked() )
            actionItem->setCheckState(Qt::Unchecked);
        else
            actionItem->setCheckState(Qt::Checked);
    }
}

#include "componentPage.moc"
