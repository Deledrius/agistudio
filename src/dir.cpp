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
 *  A special exception to the GNU General Public License is made for
 *  linking to a non-GPL library, specifically, the Windows version of
 *  Qt.  For more information on Qt, see www.trolltech.com.
 */

#include "game.h"
#include "menu.h"
#include "dir.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <glob.h>
#endif


//directory browser; since there is no way in QFileDialog to 
//show only directories

Dir::Dir( QWidget *parent, const char *name,bool mode)
    : QWidget( parent, name)
{

  newgame=mode;   //if true - can create new dirs
  QBoxLayout *all = new QVBoxLayout(this,20);

  QLabel *l;
  if(newgame){
    setCaption("New game");
    l = new QLabel("Please select the directory\nyou wish to put the new game in:",this);
  }
  else{
    setCaption("Open game");
    l = new QLabel("Please select the game directory",this);
  }
  all->addWidget(l);

  list = new QListBox(this);
  list->setColumnMode (1);
  list->setMinimumSize(200,300);
  connect( list, SIGNAL(highlighted(int)), SLOT(highlight_dir(int)) );
  connect( list, SIGNAL(selected(int)), SLOT(select_dir(int)) );
  all->addWidget(list);

  selected = new QLineEdit(this);
  all->addWidget(selected);

  QBoxLayout *b = new QHBoxLayout(all,20);
  if(newgame){
    QPushButton *create = new QPushButton("Create dir",this);
    connect(create, SIGNAL(clicked()), SLOT(create_dir()));
    b->addWidget(create);
  }
  QPushButton *ok = new QPushButton("OK",this);
  connect(ok, SIGNAL(clicked()), SLOT(ok_cb()));
  b->addWidget(ok);
  QPushButton *cancel = new QPushButton("Cancel",this);
  connect(cancel, SIGNAL(clicked()), SLOT(close()));
  b->addWidget(cancel);

  d.setFilter( QDir::Dirs );
  adjustSize();
}

//*********************************************************
void Dir::open()
{
  
  const QFileInfoList *lst = d.entryInfoList();
  QFileInfoListIterator it( *lst );      // create list iterator
  QFileInfo *fi;                          // pointer for traversing
  
  list->clear();
  while ( (fi=it.current()) ) {           // for each file...
    sprintf(tmp, "%s", fi->fileName().data() );
    if(strcmp(tmp,"."))list->insertItem(tmp);
    ++it;
  }

  show();

}
//*********************************************************
void Dir::highlight_dir(int k)
{

  QString str = list->text(k);
  selected->setText(str);

}
//*********************************************************
void Dir::select_dir(int k)
{

  QString str = list->text(k);
  if(d.cd(str)==true){    
    open();
  }

}
//*********************************************************
void Dir::create_dir()
{
  AskText *name = new AskText(0,0,"Create directory","Enter directory name:");
  if(!name->exec())return;
  QString str = name->text->text();

  if(d.mkdir(str,false)==true){
    d.cd(str); //force to reread the cur dir...
    d.cdUp();
    open();
    for(int i=0;i<(int)list->count();i++){
      if(list->text(i) == str){
        list->setCurrentItem(i);
        break;
      }
    }
  }

}
//*********************************************************
void Dir::ok_cb()
{
  
  QString str = selected->text();  
  if(str.length()==0)return;

  if(game->isOpen){  //close the currently edited game 
    menu->close_game();
    if(game->isOpen)return;
  }

  string name = string((char *)d.absPath().latin1()) + "/" + string((char *)str.latin1()) ;
  if(newgame){
    int game_exists;
    sprintf(tmp,"%s/*vol.?",name.c_str());   //check for an existing game
#ifdef _WIN32
    struct _finddata_t c_file;
    long hFile;
    game_exists = !((hFile = _findfirst(tmp,&c_file)) == -1L);
    if (game_exists) _findclose(hFile);
#else
    glob_t globbuf;
    glob(tmp, 0, NULL, &globbuf);
    game_exists = globbuf.gl_pathc>0;
#endif
    if (game_exists) {
      sprintf(tmp,"There seems to be already an AGI game in %s !\nDo you want to erase it ?",name.c_str());
      switch ( QMessageBox::warning( this, "New game",
                                     tmp,
                                     "Yes",
                                     "No",
                                     "Cancel",
                                     0, 2) ) {
      case 0: // yes
        break;
      case 1: // no
        return;
      default: // cancel
        return;
      }    
    }
  }

  int err;
  if(newgame){
    if(menu->templ)
      err = game->from_template(name);
    else
      err = game->newgame(name);
  }
  else{
    err = game->open(name);
  }
  if(!err)menu->show_resources();
  hide();

}
















