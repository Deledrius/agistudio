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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <qwidget.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <q3listbox.h>
#include <qmessagebox.h>
#include <qlineedit.h> 
#include <qradiobutton.h> 
#include <q3buttongroup.h>
#include <qcheckbox.h> 
#include <q3multilineedit.h> 
#include <qpixmap.h>
#include <qlineedit.h>
#include <qcombobox.h> 
#include <qevent.h> 
#include <q3tabdialog.h>

#include "util.h"
#include "wutil.h"
#include "view.h"




class Options : public Q3TabDialog
{
    Q_OBJECT
public:
    Options( QWidget *parent=0, const char *name=0);
    QComboBox *type,/* *style,*/ *picstyle;
    QCheckBox *messages,*elses,*special;
    QRadioButton *text,*binary;
    QLineEdit *relname,*absname;
    QRadioButton *reldir,*absdir;
    QLineEdit *command,*templatedir,*helpdir;

public slots:
    void set_general();
    void set_logedit();
    void set_directories();
    void set_interpreter();
    void set_settings();

    void apply();
    void defaults();

    void browse_abs();
    void browse_template();
    void browse_help();
    void browse_interpreter();

    void set_reldir();
    void set_absdir();
};


extern Options *options;
extern char tmp[];

#endif
