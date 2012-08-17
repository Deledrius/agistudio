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

#ifndef DIR_H
#define DIR_H

#include <qwidget.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qnamespace.h>
#include <qmessagebox.h>
#include <q3multilineedit.h> 
#include <qevent.h> 
#include <q3scrollview.h> 
#include <qpixmap.h>
#include <qimage.h>
#include <qstatusbar.h>
#include <qcheckbox.h>
#include <qdir.h>

#include "util.h"
#include "wutil.h"

//directory browser; since there is no way in QFileDialog to 
//show only directories

class Dir: public QWidget
{

   Q_OBJECT
public:
   Dir( QWidget *parent=0, const char *name=0,bool newgame=false);
   Q3ListBox *list;  //dir list
   QLineEdit *selected;  //selected directory
   QDir d;
   bool newgame;   //true - create new game, false - open existing game
   void open();   
public slots:
     void highlight_dir(int);
     void select_dir(int);
     void create_dir();
     void ok_cb();
};

#endif
