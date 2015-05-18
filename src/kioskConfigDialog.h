/*
 *   kioskConfigDialog.h
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
#ifndef _KIOSKCONFIGDIALOG_H_
#define _KIOSKCONFIGDIALOG_H_

#include <KDE/KDialog>
#include "ui_kioskConfigDialog.h"

class KioskConfigDialogUI : public QWidget, public Ui::KioskConfigDialogUI
{
public:
  KioskConfigDialogUI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class KioskConfigDialog : public KDialog
{
  Q_OBJECT
public:
  KioskConfigDialog(QWidget *parent);
  ~KioskConfigDialog();

  bool save();


protected slots:
  void updateExample();

protected:
  void init();

private:
  KioskConfigDialogUI *w;
};

#endif
