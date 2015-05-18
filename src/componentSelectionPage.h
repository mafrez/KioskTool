/*
 *   componentSelectionPage.h
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
#ifndef _COMPONENTSELECTIONPAGE_H_
#define _COMPONENTSELECTIONPAGE_H_

#include "ui_componentSelectionPage.h"
#include "pageWidget.h"

class KioskData;
class QListWidgetItem;

class ComponentSelectionPageUI : public QWidget, public Ui::ComponentSelectionPageUI
{
public:
  ComponentSelectionPageUI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class ComponentSelectionPage : public ComponentSelectionPageUI
{
  Q_OBJECT
public:
  ComponentSelectionPage( KioskData *data, QWidget* parent = 0 );
  ~ComponentSelectionPage();

  virtual void load();
  virtual bool save();
  void discard();

  virtual void setFocus();

  virtual QString subCaption();

  void setCurrentComponent(const QString &);
  QString currentComponent();

signals:
  void componentActivated();

protected:
  void loadComponentList();
  bool hasSelection();

protected slots:
  void slotComponentActivated(QListWidgetItem *item);
  void slotComponentActivated();

private:
  KioskData *m_data;
};

#endif
