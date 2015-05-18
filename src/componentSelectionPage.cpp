/*
 *   componentSelectionPage.cpp
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

#include "componentSelectionPage.h"

#include <QPushButton>
#include <QPixmap>
#include <QListWidgetItem>

#include <KDE/KApplication>
#include <KDE/KConfig>
#include <KDE/KIconLoader>
#include <KDE/KListWidget>
#include <KDE/KLocale>
#include <KDE/KGlobal>

#include "kioskdata.h"
#include "kioskrun.h"
class ComponentWidgetItem : public QListWidgetItem
{
public:
   ComponentWidgetItem( KListWidget * parent, const QString & text, const QPixmap & icon, const QString & _id )
    :  QListWidgetItem( icon, text, parent ), id(_id)
   {
   }

   QString id;
};

ComponentSelectionPage::ComponentSelectionPage( KioskData *data, QWidget* parent )
 : ComponentSelectionPageUI(parent), m_data(data)
{
   loadComponentList();
   connect(listComponent, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(slotComponentActivated(QListWidgetItem *)));

   // DEPRECATED JH
   //connect(pbSetup, SIGNAL(clicked()), this, SLOT(slotComponentActivated()));
}

ComponentSelectionPage::~ComponentSelectionPage()
{
}

void ComponentSelectionPage::load()
{
}

bool ComponentSelectionPage::save()
{
/*
   KSharedConfigPtr config = KGlobal::config();
   KConfigGroup cg(config, "General");
   cg.writeEntry("CurrentComponent", currentComponent());
   config->sync();
   return true;
*/
    return KioskRun::self()->flushConfigCache();
}

void ComponentSelectionPage::discard()
{
    return KioskRun::self()->clearConfigCache();
}

void ComponentSelectionPage::setFocus()
{
}

QString ComponentSelectionPage::subCaption()
{
   return QString::null;
}

void ComponentSelectionPage::loadComponentList()
{
    listComponent->clear();
    foreach( ComponentData *data, m_data->m_componentData )
    {
       QPixmap icon = DesktopIcon( data->icon, KIconLoader::SizeLarge );
       new ComponentWidgetItem(listComponent, data->caption, icon, data->id);
    }
}

bool ComponentSelectionPage::hasSelection()
{
   return !currentComponent().isEmpty();
}

QString ComponentSelectionPage::currentComponent()
{
   if( listComponent->currentItem() )
   {
      ComponentWidgetItem *item = static_cast<ComponentWidgetItem *>(listComponent->currentItem());
      return item->id;
   }
   return QString();
}

void ComponentSelectionPage::setCurrentComponent(const QString &id)
{
   for( int idx = 0; idx < listComponent->count(); ++idx )
   {
       ComponentWidgetItem *item = static_cast<ComponentWidgetItem *>(listComponent->item(idx));
       if( item->id == id )
           listComponent->setCurrentItem( item );
   }
}

void ComponentSelectionPage::slotComponentActivated(QListWidgetItem *item)
{
   if (item)
      emit componentActivated();
}

void ComponentSelectionPage::slotComponentActivated()
{
   if (!currentComponent().isEmpty())
      emit componentActivated();
}

#include "componentSelectionPage.moc"
