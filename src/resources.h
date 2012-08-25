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

#ifndef RESOURCES_H
#define RESOURCES_H

#include <string>
#include <qwidget.h>
#include <qlabel.h>
#include <q3listbox.h> 
#include <qcombobox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <q3buttongroup.h>
//Added by qt3to4:
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>
#include <Q3PopupMenu>


#include "preview.h"

class ResourcesWin;
class Preview;

class AddResource : public QWidget
{
    Q_OBJECT
public:
    AddResource( QWidget *parent=0, const char *name=0 , ResourcesWin *res=0);
    
    QLabel *filename;
    QLabel *name;
    QLineEdit *number;
    Q3ButtonGroup *type;
    ResourcesWin *resources_win;
    Preview *preview;
    int restype;
    string file;
    void open(char *filename);
public slots:
    void ok_cb();
    void cancel_cb();
    void select_type(int);
    void edit_cb(const QString &);
};



class ResourcesWin : public QWidget
{
    Q_OBJECT
public:
    ResourcesWin( QWidget* parent = 0, const char*  name=0, int winnum=0);
    Q3ListBox *list;
    int selected;
    int ResourceNum;
    Preview *preview;
    unsigned char ResourceIndex[256];
    bool closing;
public slots:
    void select_resource_type( int i);
    void highlight_resource( int i);
    void select_resource( int i);
    void set_current(int i);
    void add_resource(void);
    void extract_resource(void);
    void extract_all_resource(void);
    void delete_resource(void);
    void renumber_resource(void);
    void new_resource_window(void);
    void export_resource(void);
    void import_resource(void);
    
 protected:
    bool first;
    QLabel *msg;
    QComboBox *type;
    QWidget *previewPane;
    AddResource *addmenu;
    
    Q3PopupMenu *resourceMenu;
    int importMenuItemID;
    
    int winnum;
    void closeEvent( QCloseEvent *e );
    void showEvent(  QShowEvent * );
    void hideEvent(  QHideEvent * );  
    void deinit();
};

#endif
