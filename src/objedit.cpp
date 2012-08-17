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

#include "game.h"
#include "object.h"
#include "objedit.h"
#include "menu.h"

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <qapplication.h>
#include <q3filedialog.h> 
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QCloseEvent>
#include <Q3BoxLayout>
#include <QShowEvent>
#include <QLabel>
#include <Q3PopupMenu>
#include <QHideEvent>
#include <Q3VBoxLayout>


//*****************************************
//Inventory object editor
ObjEdit::ObjEdit( QWidget *parent, const char *nam, int win_num)
    : QWidget( parent, nam, Qt::WDestructiveClose )
{
  
  setCaption("Object Editor");
  setMinimumSize(400,400);

  winnum = win_num;
  objlist = new ObjList();

  Q3PopupMenu *file = new Q3PopupMenu( this );
  Q_CHECK_PTR( file );

  file->insertItem( "New", this, SLOT(new_file()) );
  file->insertItem( "Open", this, SLOT(open_file()) );
  file->insertItem( "Save", this, SLOT(save_file()) );
  file->insertItem( "Save As", this, SLOT(save_as_file()) );
  file->insertSeparator();
  file->insertItem( "Close", this, SLOT(close()) );

  options = new Q3PopupMenu( this );
  Q_CHECK_PTR( options );
  encrypted = options->insertItem( "Encrypted", this, SLOT(encrypted_cb()) );
  options->setItemChecked(encrypted,true);  

  QMenuBar *menu = new QMenuBar(this);  
  Q_CHECK_PTR( menu );
  menu->insertItem( "File", file );
  menu->insertItem( "Options", options );
  menu->setSeparator( QMenuBar::InWindowsStyle );

  Q3BoxLayout *all =  new Q3VBoxLayout(this,14);  
  all->setMenuBar(menu);    
  
  list = new Q3ListBox(this);
  list->setMinimumSize(400,400);
  list->setColumnMode (1);

  connect( list, SIGNAL(highlighted(int)), SLOT(select_object(int)) );
  connect( list, SIGNAL(selected(int)), SLOT(select_object(int)) );
  all->addWidget(list);

  name = new QLineEdit(this);  
  connect( name, SIGNAL(returnPressed()), SLOT(name_cb()) );
  all->addWidget(name);

  Q3BoxLayout *down =  new Q3HBoxLayout(all,4);
  
  add = new QPushButton("&Add",this);
  connect( add, SIGNAL(clicked()), SLOT(add_cb()) );
  down->addWidget(add);
  del = new QPushButton("&Delete",this);
  connect( del, SIGNAL(clicked()), SLOT(del_cb()) );
  down->addWidget(del);

  QLabel *label = new QLabel("Room no:",this);
  down->addWidget(label);
  
  num = new QLineEdit(this);
  connect( num, SIGNAL(returnPressed()), SLOT(num_cb()) );
  down->addWidget(num);

  left = new QPushButton("<",this);
  connect( left, SIGNAL(clicked()), SLOT(left_cb()) );
  down->addWidget(left);
  right = new QPushButton(">",this);
  connect( right, SIGNAL(clicked()), SLOT(right_cb()) );
  down->addWidget(right);
 
  adjustSize();

  filename = "";
  changed=false; 
}

//*****************************************
void ObjEdit::open()
{

  sprintf(tmp,"%s/object",game->dir.c_str());  
  open(tmp);
  show();

}

//*****************************************
void ObjEdit::deinit()
{
  delete objlist;
  winlist[winnum].type=-1;
  if(window_list && window_list->isVisible())window_list->draw();
}


//*********************************************
void ObjEdit::hideEvent( QHideEvent * )
{

  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void ObjEdit::showEvent( QShowEvent * )
{

  if(window_list && window_list->isVisible())window_list->draw();

}

//*****************************************
void ObjEdit::closeEvent( QCloseEvent *e )
{
  
  if(changed){      
    switch ( QMessageBox::warning( this, "ObjEdit",
                                   "Save changes to OBJECT file ?",
                                   "Yes",
                                   "No",
                                   "Cancel",
                                   0, 2) ) {
    case 0: // yes
      save_file();
      deinit();
      e->accept();
      break;
    case 1: // no
      deinit();
      e->accept();
      break;
    default: // cancel
      e->ignore();
      break;
    }        
  }
  else{
    deinit();
    e->accept();
  }

}

//*****************************************
void ObjEdit::open(char *name)
{
  int ret = objlist->read(name,false);
  if(ret)return;

  filename = name;
  
  list->clear();
  for(int i=0;i<objlist->ItemNames.num;i++){
    sprintf(tmp,"%d. %s",i,objlist->ItemNames.at(i).c_str());
     list->insertItem(tmp);
  }
  list->show();  
  list->setCurrentItem(0);
  changed=false;

}

//*****************************************
void ObjEdit::open_file()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Open",true);  
  const char *filters[] = {"object","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Open");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->dir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      open((char *)f->selectedFile().latin1());
  }
}

//*****************************************
void ObjEdit::save_file()
{
  if(filename==""){
    save_as_file();
  }
  else{
    objlist->save((char *)filename.c_str(),options->isItemChecked(encrypted));
    changed=false;
  }
}

//*****************************************
void ObjEdit::save_as_file()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Save",true);  
  const char *filters[] = {"object","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Save");
  f->setMode(Q3FileDialog::AnyFile);
  f->setDir(game->dir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() ){
      objlist->save((char *)f->selectedFile().latin1(),options->isItemChecked(encrypted));
      changed=false;
    }
  }
}

//*****************************************
void ObjEdit::new_file()
{

  objlist->clear();
  CurObject = 0;
  list->clear();
  list->insertItem("0. ?");
  list->setCurrentItem(CurObject);    
  changed=false;
}

//*****************************************
void ObjEdit::select_object(int n)
{
  name->setText(objlist->ItemNames.at(n).c_str());
  sprintf(tmp,"%d",objlist->RoomNum[n]);
  num->setText(tmp);
  CurObject = n;
}

//*****************************************
void ObjEdit::add_cb()
{

  objlist->ItemNames.add("?");  
  CurObject = objlist->ItemNames.num-1;
  objlist->RoomNum[CurObject]=0;
  sprintf(tmp,"%d. ?",CurObject);
  list->insertItem(tmp);
  list->setCurrentItem(CurObject);  
  changed=true;

}

//*****************************************
void ObjEdit::del_cb()
{

  objlist->ItemNames.replace(CurObject,"?");
  sprintf(tmp,"%d. ?",CurObject);
  list->changeItem(tmp,CurObject);
  changed=true;
}

//*****************************************
void ObjEdit::left_cb()
{

  if(objlist->RoomNum[CurObject]>0){
    objlist->RoomNum[CurObject]--;
    sprintf(tmp,"%d",objlist->RoomNum[CurObject]);
    num->setText(tmp);
    changed=true;
  }

}

//*****************************************
void ObjEdit::right_cb()
{

  if(objlist->RoomNum[CurObject]<255){  
    objlist->RoomNum[CurObject]++;
    sprintf(tmp,"%d",objlist->RoomNum[CurObject]);
    num->setText(tmp);
    changed=true;
  }

}

//*****************************************
void ObjEdit::num_cb()
{

  char *str = (char *)num->text().latin1(); 
  int k = atoi(str);
  if(!strcmp(str,"0") || (k>0&&k<256) ){
    objlist->RoomNum[CurObject] = k;
    changed=true;
  }
  else{
    sprintf(tmp,"%d",objlist->RoomNum[CurObject]);
    num->setText(tmp);    
  }
}

//*****************************************
void ObjEdit::name_cb()
{

  char *str = (char *)name->text().latin1(); 
  objlist->ItemNames.replace(CurObject,str);  
  sprintf(tmp,"%d. %s",CurObject,str);
  list->changeItem(tmp,CurObject);
  changed=true;
}

//*****************************************
void ObjEdit::encrypted_cb()
{

  options->setItemChecked(encrypted,!options->isItemChecked(encrypted));

}
//*****************************************
