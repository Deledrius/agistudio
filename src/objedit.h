/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef OBJEDIT_H
#define OBJEDIT_H

#include <qwidget.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <q3listbox.h>
#include <qmessagebox.h>
#include <qlineedit.h> 
#include <qevent.h>
//Added by qt3to4:
#include <Q3PopupMenu>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>

#include "util.h"
#include "object.h"

//inventory objects editor
class ObjEdit : public QWidget
{
    Q_OBJECT
public:
    ObjEdit( QWidget *parent=0, const char *name=0, int winnum=0);
    void open();
public slots:
      void open_file();
      void save_file();
      void save_as_file();
      void new_file();
      void select_object(int);
      void add_cb();
      void del_cb();
      void left_cb();
      void right_cb();
      void num_cb();
      void name_cb();
      void encrypted_cb();
 protected:
    int winnum;
    Q3ListBox *list;
    QLineEdit *name,*num;
    QPushButton *add,*del,*left,*right;
    Q3PopupMenu *options;
    int encrypted;
    int CurObject;
    bool changed;
    string filename;
    ObjList *objlist;

    void open(char *);
    void save(char *);
    void deinit();
    void closeEvent( QCloseEvent *e );
    void showEvent(  QShowEvent * );
    void hideEvent(  QHideEvent * );    
};

#endif
