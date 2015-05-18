/*
 *   desktopComponent.h
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
#ifndef _DESKTOPCOMPONENT_H_
#define _DESKTOPCOMPONENT_H_

#include "component.h"

#include <QStringList>
#include <QTimer>

class DesktopComponent: public Component
{
  Q_OBJECT
public:  
  DesktopComponent( QObject *parent = 0);
  virtual ~DesktopComponent();

  virtual bool setupFinished();

public slots:

  virtual void slotSetupPrepare();
  virtual void slotSetupStarted();

protected slots:
  void slotSetupReady();

private:
  QTimer m_timer;
  QString m_iconPositionsFile;
  QStringList m_origDesktopFiles;
};

#endif
