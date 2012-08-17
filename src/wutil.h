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

#ifndef WUTIL_H
#define WUTIL_H

#include <qwidget.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <q3listbox.h>
#include <qmessagebox.h>
#include <qlineedit.h> 
#include <qradiobutton.h> 
#include <q3buttongroup.h> 
//Added by qt3to4:
#include <QPaintEvent>
#include <QMouseEvent>
#include <vector>

#include "global.h"

//*****************************************************
class AskNumber : public QDialog
{
    Q_OBJECT
public:
    AskNumber( QWidget *parent=0, const char *name=0,const char *caption=0,const char *prompt=0);
    QLineEdit *num;
};

//*****************************************************
class AskText : public QDialog
{
    Q_OBJECT
public:
    AskText( QWidget *parent=0, const char *name=0,const char *caption=0,const char *prompt=0);
    QLineEdit *text;
};

//*****************************************************
class Palette : public QWidget
{
    Q_OBJECT
public:
    Palette( QWidget *parent=0, const char *name=0);
    void paintEvent(class QPaintEvent *);
    void mousePressEvent(QMouseEvent* event);
    int left,right;
};

//*****************************************************
extern QColor egacolor[];
extern void make_egacolors(void);

#endif
