/*
 *   componentPage.h
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
#ifndef _COMPONENTPAGE_H_
#define _COMPONENTPAGE_H_

#include "ui_componentPage.h"
#include "pageWidget.h"
#include <QByteArray>

class KProcess;
class Component;
class ComponentData;

class ComponentPage : public QWidget, public Ui::ComponentPageUI, public PageWidget
{
  Q_OBJECT
public:
  ComponentPage( ComponentData *data, Component *component, const QString &profileName,  QWidget* parent = 0 );
  ~ComponentPage();

  virtual void load();
  virtual bool save();

  virtual void setFocus();

  virtual QString subCaption();

public slots:
  //void slotSetup();
//   void slotPreview();

protected slots:
  void slotShowNotice();
  void slotSetupDone();
  void slotPreviewDone();
  void slotShowAction(QListWidgetItem *item);
  void slotSetupAppRegistered( const QByteArray &appid);
  void slotPreviewAppRegistered( const QByteArray &appid);
  void slotUpdateValue( );

protected:
  void prepareMutableFiles();

private:
  KProcess *m_process;
  ComponentData *m_data;
  Component *m_component;
  bool m_saveSettings;
  QString m_profileName;
};

#endif
