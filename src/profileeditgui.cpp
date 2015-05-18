/*
 * kioskgui.cpp
 *
 *   Copyright (C) 2003,2004 Waldo Bastian <bastian@kde.org>
 *   Copyright (C) 2009 Ian Reinhart Geiser <geiseri@kde.org>
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

#include <config.h>

#include "profileeditgui.h"

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

#include <kdebug.h>

#include "componentSelectionPage.h"
#include "desktopComponent.h"
#include "menueditComponent.h"
#include "screensaverComponent.h"
#include "panelComponent.h"
#include "filetypeeditComponent.h"
#include "componentPage.h"
#include "pageWidget.h"

#include "kioskdata.h"
#include "kioskrun.h"
#include "kiosksync.h"

ProfileEditGui::ProfileEditGui(QWidget* parent, const QString& profile, KioskRun * _krun)
    : KDialog(parent), m_componentSelectionPage(0), m_componentPage(0), m_data(0),
    m_run(_krun), m_profile(profile), m_componentData(0), m_page(PAGE_ENTRY)
{
    m_view = new MainView(this);
    setCaption( i18n("Editing profile \"%1\"", profile) );
    setButtons( 0 );
    setModal( true );

    // tell the KMainWindow that this is indeed the main widget
    setMainWidget(m_view);

    connect(m_view->pbDiscard, SIGNAL(clicked()), this, SLOT(discardPage()));
    connect(m_view->pbFinished, SIGNAL(clicked()), this, SLOT(finishedPage()));
    
    // TO BE IMPLEMENTED JH
    //connect(m_view->pbHelp, SIGNAL(clicked()), this, SLOT(help()));
    //m_view->pbHelp->setGuiItem(KStandardGuiItem::Help);

    KConfigGroup config( KGlobal::config(), "General");
    m_component = config.readEntry("CurrentComponent");

    selectPage(PAGE_COMPONENT_SELECTION);
}

ProfileEditGui::~ProfileEditGui()
{
    delete m_data;
}

void ProfileEditGui::setSubCaption(const QString &caption)
{
    m_view->subCaptionLabel->setText("<h2>"+caption+"</h2>");
}

void ProfileEditGui::selectPage(Page page)
{
    // first handle transition from previous page
    if( m_page ==  PAGE_COMPONENT_SELECTION )
    {
        m_component = m_componentSelectionPage->currentComponent();
        m_componentData = m_data->m_componentData[m_component];
        if (!m_componentData)
           return;
    }
    // this is a fake page that's used when the dialog is created
    if( m_page ==  PAGE_ENTRY )
    {
        KConfigGroup config( KGlobal::config(), "General");
        config.writeEntry("CurrentProfile", m_profile);
        config.sync();

        QString description;
        QString installDir;
        QString installUser;

        KioskRun::self()->getProfileInfo(m_profile, description, installDir, installUser);

        QStringList kdeDirs;
        kdeDirs << installDir;
        m_run->setKdeDirs(kdeDirs);
        m_run->setUser(installUser);
    }

    // swap current page for the new one
    m_page = page;

    // handle transition to the new page
    if ((m_page == PAGE_COMPONENT_SELECTION) && !m_componentSelectionPage)
    {
       m_data = new KioskData;
       if (!m_data->load())
       {
          KMessageBox::error(this, m_data->errorMsg(), i18n("Error accessing Kiosk data"));
       }

       m_componentSelectionPage = new ComponentSelectionPage(m_data, m_view->widgetStack);
       connect(m_componentSelectionPage, SIGNAL(componentActivated()), this, SLOT(slotComponentSelection()));
       m_componentSelectionPage->setCurrentComponent(m_component);

       m_pageMapping[PAGE_COMPONENT_SELECTION] = m_view->widgetStack->insertWidget(PAGE_COMPONENT_SELECTION, m_componentSelectionPage);
       m_componentSelectionPage->listComponent->setFocus();
    }
    else if (m_page == PAGE_COMPONENT)
    {
       delete m_componentPage;

       Component *component = 0;
       if (m_component == "kdesktop")
          component = new DesktopComponent;
       else if (m_component == "kdemenu")
          component = new MenuEditComponent;
       else if (m_component == "screensaver")
          component = new ScreenSaverComponent;
       else if (m_component == "kicker")
          component = new PanelComponent;
       else if (m_component == "filetypes")
          component = new FileTypeEditComponent;

       m_componentPage = new ComponentPage(m_componentData, component, m_profile, m_view->widgetStack);
       m_pageMapping[PAGE_COMPONENT] = m_view->widgetStack->insertWidget(PAGE_COMPONENT,m_componentPage->widget());
       m_componentPage->setFocus();
    }

    // set captions
    if( m_page ==  PAGE_COMPONENT_SELECTION )
    {
       setSubCaption(i18n("Setup Profile \"%1\"",m_profile));
    }
    if( m_page ==  PAGE_COMPONENT )
    {
       setSubCaption(m_componentPage->subCaption());
    }

    // swap page for real
    m_view->widgetStack->setCurrentIndex(m_pageMapping[m_page]);
    loadPage(m_page);

    updateButtons();
}

void ProfileEditGui::updateButtons()
{
    if( m_page ==   PAGE_COMPONENT_SELECTION)
    {
        m_view->pbDiscard->show();
        m_view->pbFinished->show();
        m_view->pbDiscard->setGuiItem(KStandardGuiItem::Discard);
        m_view->pbFinished->setGuiItem(KStandardGuiItem::Save);
    }

    else if( m_page ==   PAGE_COMPONENT)
    {
        m_view->pbDiscard->hide();
        m_view->pbFinished->show();
        m_view->pbFinished->setGuiItem(KStandardGuiItem::Back);
    }
}

void ProfileEditGui::loadPage(Page page)
{
    if( page == PAGE_COMPONENT )
    {
        m_componentPage->load();
    }
}

bool ProfileEditGui::savePage(Page page)
{
    if( page ==  PAGE_COMPONENT_SELECTION)
    {
        return m_componentSelectionPage->save();
    }
    if( page ==  PAGE_COMPONENT )
    {
        if (m_componentPage)
            return m_componentPage->save();
    }
    return true;
}

void ProfileEditGui::finishedPage(bool save)
{
    if( save )
    {
        if( !savePage( m_page ))
            return;
    }
    
    if (m_page == PAGE_COMPONENT_SELECTION)
    {
        if( !save )
            m_componentSelectionPage->discard();
        done(save);
    }
    else if (m_page == PAGE_COMPONENT)
       selectPage(PAGE_COMPONENT_SELECTION);
}

void ProfileEditGui::discardPage()
{
   finishedPage(false);
}

void ProfileEditGui::slotComponentSelection()
{
       selectPage(PAGE_COMPONENT);
}

/*
bool KioskProfileEditGui::queryClose()
{
    if (!savePage(m_page))
    {
       int result = KMessageBox::warningContinueCancel(this,
       		i18n("Your changes could not be saved, do you want to quit anyway?"),
       		QString::null,
       		KStandardGuiItem::quit());
       if (result == KMessageBox::Continue)
          return true;
       return false;
    }
    return true;
}
*/
void ProfileEditGui::help()
{

}

#include "profileeditgui.moc"
