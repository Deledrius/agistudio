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
#include "menu.h"
#include "dir.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <glob.h>
#endif

#include <QFileDialog>


//*********************************************************
void OpenGameDir( QWidget *parent, bool newgame )
{
  QString title("Open game");
  if(newgame)
    title = "New game";
  QString dir = QFileDialog::getExistingDirectory(parent, title);
  if ( dir.isNull())
    return;
 
  //close the currently edited game 
  if(game->isOpen)
  { 
    menu->close_game();
    if(game->isOpen)return;
  }  
  
  std::string name = dir.toLocal8Bit().data();
  
  if(newgame)
  {
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
      switch ( QMessageBox::warning( parent, "New game",
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
  if(newgame)
  {
    if(menu->templ)
      err = game->from_template(name);
    else
      err = game->newgame(name);
  }
  else
    err = game->open(name);
  if(!err)
    menu->show_resources();  
}
