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

#include <cstdlib>
#include <stdio.h>

#include <qapplication.h>
#include <q3mainwindow.h>

#include "menu.h"
#include "game.h"

QApplication *app;
char tmp[MAX_TMP]; //global temporary buffer

static char help[]=
"QT AGI Studio v1.1.\n\
A Sierra On-Line(tm) adventure game creator and editor.\n\
\n\
Usage: agistudio [switches] \n\
\n\
where [switches] are optionally:\n\
\n\
-dir GAMEDIR   : open an existing game in GAMEDIR\n\
-help          : this message\n\
\n";

//***************************************************
int main( int argc, char **argv )
{
  char *gamedir=NULL;
  
  tmp[0]=0;

  for(int i=1;i<argc;i++){
    if(argv[i][0] == '-'){
      if(!strcmp(argv[i]+1,"dir")){
        gamedir = argv[i+1];
      }
      else
      {
        if(strcmp(argv[i]+1,"help")!=0 && strcmp(argv[i]+1,"-help")!=0)
          printf( "Unknown parameter.\n\n" );
        printf(help);
        exit(-2);
      }
    }
  }

  app = new QApplication(argc,argv);
  menu = new Menu(NULL,NULL);
  app->setMainWidget( menu );

  game = new Game();

  menu->show();
  
  if(gamedir){
    int err = game->open(gamedir);
    if(!err)menu->show_resources();
  }
  return app->exec();
}
