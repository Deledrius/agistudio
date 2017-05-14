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

#include "menu.h"
#include "game.h"
#include "wordsedit.h"
#include "objedit.h"
#include "viewedit.h"
#include "logedit.h"
#include "resources.h"
#include "wutil.h"
#include "preview.h"
#include "picedit.h"
#include "dir.h"
#include "options.h"
#include "helpwindow.h"

#include <stdio.h>
#ifdef _WIN32
  #include <direct.h>
  #include <process.h>
  #include <windows.h>
#else
  #include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include <QApplication>
#include <QPixmap>
#include <QToolButton>
#include <QTooltip>
#include <QLabel>
#include <QCloseEvent>
#include <QToolBar>

#include "toolbar_open.xpm"
#include "toolbar_close.xpm"
#include "toolbar_run.xpm"
#include "toolbar_viewedit.xpm"
#include "toolbar_logedit.xpm"
#include "toolbar_picedit.xpm"
#include "toolbar_wordsedit.xpm"
#include "toolbar_objedit.xpm"
#include "toolbar_textedit.xpm"
#include "logo.xpm"
#include "app_icon.xpm"

extern char tmp[];
Menu *menu;
WindowList *window_list;
static About *about;
WinList winlist[MAXWIN];  //list of all open resource editor windows

//*************************************************
Menu::Menu( QWidget *parent, const char *name )
    : QMainWindow(parent)
{
  int n=0;

  setWindowTitle("AGI Studio");
  setWindowIcon(QPixmap(app_icon));

  QMenu *new_game = new QMenu(this);
  Q_CHECK_PTR( new_game );
  new_game->setTitle("&New");
  new_game->addAction( "From &Template", this, SLOT(from_template()) );
  new_game->addAction( "&Blank", this, SLOT(blank()) );

  QMenu *game = new QMenu(this);
  Q_CHECK_PTR( game );
  game->setTitle("&Game");
  game->addMenu(new_game);
  game->addAction( "&Open", this, SLOT(open_game()) );
  id[n++] = game->addAction( "&Close", this, SLOT(close_game()) );
  id[n++] = game->addAction( "&Run", this, SLOT(run_game()), Qt::CTRL+Qt::Key_R );
  game->addSeparator();
  game->addAction( "&Settings...", this, SLOT(settings()) );
  game->addSeparator();
  game->addAction("E&xit", this, SLOT(quit_cb()), Qt::ALT + Qt::Key_F4);

  QMenu *resource = new QMenu(this);
  Q_CHECK_PTR( resource );
  resource->setTitle("&Resource");
  id[n++] = resource->addAction("New window", this, SLOT(new_resource_window()));
  resource->addSeparator();
  n_res=n;
  id[n++] = resource->addAction( "&Add", this, SLOT(add_resource()) );
  id[n++] = resource->addAction( "&Extract", this, SLOT(extract_resource()) );
  id[n++] = resource->addAction( "&Delete", this, SLOT(delete_resource()) );
  id[n++] = resource->addAction( "&Renumber", this, SLOT(renumber_resource()) );
  resource->addSeparator();
  id[n++] = resource->addAction( "Re&build VOL files", this, SLOT(rebuild_vol()) );
  id[n++] = resource->addAction( "Recompile all", this, SLOT(recompile_all()) );

  QMenu *tools = new QMenu(this);
  Q_CHECK_PTR( tools );
  tools->setTitle("&Tools");
  id[n++] = tools->addAction( "&View Editor", this, SLOT(view_editor()) );
  id[n++] = tools->addAction( "&Logic Editor", this, SLOT(logic_editor()) );
  id[n++] = tools->addAction( "&Text Editor", this, SLOT(text_editor()) );
  id[n++] = tools->addAction( "&Object Editor", this, SLOT(object_editor()) );
  id[n++] = tools->addAction( "&Words.tok Editor", this, SLOT(words_editor()) );
  id[n++] = tools->addAction( "&Picture Editor", this, SLOT(picture_editor()) );
  id[n++] = tools->addAction( "&Sound Player", this, SLOT(sound_player()) );

  QMenu *help = new QMenu(this);
  Q_CHECK_PTR(help);
  help->setTitle("&Help");
  help->addAction("&Contents", this, SLOT(help_contents()));
  help->addAction("&Index", this, SLOT(help_index()), Qt::Key_F1);
  help->addSeparator();
  help->addAction("About", this, SLOT(about_it()));
  help->addAction("About QT", this, SLOT(about_qt()));

  QMenu *window = menuBar()->addMenu("&Window");
  Q_CHECK_PTR(window);
  window->addAction("Save all", this, SLOT(save_all()));
  window->addAction("Save all and run", this, SLOT(save_and_run()));
  window->addAction("Window list", this, SLOT(window_list_cb()));

  menubar = menuBar();
  Q_CHECK_PTR( menubar );
  menubar->addMenu(game);
  menubar->addMenu(resource);
  menubar->addMenu(tools);
  menubar->addMenu(window);
  menubar->addSeparator();
  menubar->addMenu(help);

  QToolBar *toolbar = addToolBar(tr("Main"));

  open = new QAction((QPixmap)toolbar_open, "Open game", toolbar);
  connect(open, &QAction::triggered, this, &Menu::open_game);
  toolbar->addAction(open);

  close_ = new QAction((QPixmap)toolbar_close, "Close game", toolbar);
  connect(close_, &QAction::triggered, this, &Menu::close_game);
  toolbar->addAction(close_);

  run = new QAction((QPixmap)toolbar_run, "Run game", toolbar);
  connect(run, &QAction::triggered, this, &Menu::run_game);
  toolbar->addAction(run);

  view = new QAction((QPixmap)toolbar_viewedit, "View Editor", toolbar);
  connect(view, &QAction::triggered, this, &Menu::view_editor);
  toolbar->addAction(view);

  logic = new QAction((QPixmap)toolbar_logedit, "Logic Editor", toolbar);
  connect(logic, &QAction::triggered, this, &Menu::logic_editor);
  toolbar->addAction(logic);

  text = new QAction((QPixmap)toolbar_textedit, "Text Editor", toolbar);
  connect(text, &QAction::triggered, this, &Menu::text_editor);
  toolbar->addAction(text);

  obj = new QAction((QPixmap)toolbar_objedit, "Object Editor", toolbar);
  connect(obj, &QAction::triggered, this, &Menu::object_editor);
  toolbar->addAction(obj);

  words = new QAction((QPixmap)toolbar_wordsedit, "WORDS.TOK editor", toolbar);
  connect(words, &QAction::triggered, this, &Menu::words_editor);
  toolbar->addAction(words);

  pic = new QAction((QPixmap)toolbar_picedit, "Picture Editor", toolbar);
  connect(pic, &QAction::triggered, this, &Menu::picture_editor);
  toolbar->addAction(pic);

  toolbar->setMovable(false);
  toolbar->show();

  status = statusBar();
  QLabel *msg = new QLabel("", status);
  status->addWidget( msg, 4 );
  status->setSizeGripEnabled(false);

  err = new QMessageBox(QMessageBox::Critical, tr("AGI Studio"), tr(""));
  warn = new QMessageBox(QMessageBox::Warning, tr("AGI Studio"), tr(""));

  max_disabled = n;
  disable();
  adjustSize();
  setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  for(int i=0;i<MAXWIN;i++){
    winlist[i].type=-1;
  }
  make_egacolors();
#ifdef IMGEXT
  imgext=false;
#endif
  window_list=NULL;
  resources_win=NULL;
  num_res=0;
}

//**********************************************
int get_win()
{
  int n;

  for(n=0;n<MAXWIN;n++){
    if(winlist[n].type==-1)break;
  }
  if(n==MAXWIN){
    menu->errmes("Too many open windows !");
    return -1;
  }
  return n;
}

//**********************************************
void Menu::disable()
{

  for(int i=0;i<max_disabled;i++){
    id[i]->setEnabled(false);
  }
  close_->setEnabled(false);
  run->setEnabled(false);
  view->setEnabled(false);
  logic->setEnabled(false);
  obj->setEnabled(false);
  words->setEnabled(false);
  pic->setEnabled(false);
}

//**********************************************
void Menu::enable()
{

  for(int i=0;i<max_disabled;i++){
      id[i]->setEnabled(true);
  }
  close_->setEnabled(true);
  run->setEnabled(true);
  view->setEnabled(true);
  logic->setEnabled(true);
  obj->setEnabled(true);
  words->setEnabled(true);
  pic->setEnabled(true);
}

//**********************************************
void Menu::open_game()
{
  OpenGameDir( 0, false );
}

//**********************************************
void Menu::show_resources()
{

  if(resources_win==NULL){
    int n;
    if((n=get_win())==-1)return;
    resources_win = new ResourcesWin(NULL,"Resources",n);
    winlist[n].type=RESOURCES;
    winlist[n].w.r=resources_win;
    inc_res(resources_win);
  }
  resources_win->select_resource_type(game->res_default);
  resources_win->show();
  enable();


}
//**********************************************
void Menu::inc_res(ResourcesWin *res)
{

  num_res++;
  if(num_res==1){
    resources_win=res;
    enable_resources();
  }
  else{
    resources_win=NULL;
    disable_resources();
  }

}
//**********************************************
void Menu::dec_res()
{

  num_res--;
  if(num_res==1){
    for(int i=0;i<MAXWIN;i++){
      if(winlist[i].type==RESOURCES){
        resources_win=winlist[i].w.r;
        break;
      }
    }
    enable_resources();
  }
  else{
    resources_win=NULL;
    disable_resources();
  }

}
//**********************************************
void Menu::enable_resources()
{
  for(int i=n_res;i<n_res+4;i++){
      id[i]->setEnabled(true);
  }


}
//**********************************************
void Menu::disable_resources()
{

  for(int i=n_res;i<n_res+4;i++){
      id[i]->setEnabled(false);
  }

}

//**********************************************
void Menu::quit_cb()
{

  close();

}

//**********************************************
void Menu::closeEvent( QCloseEvent *e )
{

  if(game->isOpen){
    close_game();
    if(game->isOpen){
      e->ignore();
      return;
    }
  }
  e->accept();

}

//**********************************************
void Menu::close_game()
{
  int i;

  //close all open windows (they will ask to save if something was changed)
  for(i=0;i<MAXWIN;i++){
    if(winlist[i].type!=-1){
      switch(winlist[i].type){
      case LOGIC:
        winlist[i].w.l->close();
        break;
      case PICTURE:
        winlist[i].w.p->close();
        break;
      case VIEW:
        winlist[i].w.v->close();
        break;
      case OBJECT:
        winlist[i].w.o->close();
        break;
      case WORDS:
        winlist[i].w.w->close();
        break;
      case TEXTRES:
        winlist[i].w.t->close();
        break;
      }
    }
  }

  int n;
  for(i=0;i<MAXWIN;i++){
    n=winlist[i].type;
    if(n >= LOGIC && n <= TEXTRES)
      return ;  //some window was not closed
  }


  //ok to close
  for(i=0;i<MAXWIN;i++){
    if(winlist[i].type==RESOURCES){
      winlist[i].w.r->close();
    }
  }

  if(window_list)
    window_list->hide();
  disable();
  game->isOpen=false;

}

//**********************************************
void Menu::run_game()
{

#ifdef _WIN32
  int i;
  _chdir(game->dir.c_str());
#else
  int i=fork();
  if(i==0){
    chdir(game->dir.c_str());
#endif
#define MAX_ARG 32
    char *argv[MAX_ARG];
    strcpy(tmp,game->command.c_str());
    argv[0]=strtok(tmp," ");
    for(i=1;i<MAX_ARG;i++){
      argv[i]=strtok(NULL," ");
      if(argv[i]==NULL)break;
    }
    if(argv[MAX_ARG-1]!=NULL)argv[MAX_ARG-1]=NULL;
#ifdef _WIN32
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi,sizeof(pi));

    if(!CreateProcessA(NULL,tmp,NULL,NULL,false,0,NULL,NULL,&si,&pi)) {
#else
    if(execvp(argv[0],argv)){
#endif
      printf("Couldn't execute command %s !\n",game->command.c_str());
    }
#ifdef _WIN32
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#else
    exit(0);
  }
#endif

}
//**********************************************
int Menu::save_all()
{

  int ret,err=0;
  for(int i=0;i<MAXWIN;i++){
    switch(winlist[i].type){
    case LOGIC:
      ret=winlist[i].w.l->compile_logic();
      if(ret)err=1;
      break;
    case PICTURE:
      winlist[i].w.p->save_to_game();
      break;
    case VIEW:
      winlist[i].w.v->save_to_game();
      break;
    }
  }
  return err;

}
//**********************************************
void Menu::save_and_run()
{
  if(!save_all())
    run_game();

}
//**********************************************
void Menu::window_list_cb()
{

  if(window_list==NULL)window_list=new WindowList();
  window_list->draw();

}
//**********************************************
void Menu::settings()
{

  if(options==NULL)options = new Options();
  options->show();

}

//**********************************************
void Menu::from_template()
{
  menu->templ=true;
  OpenGameDir( 0, true );
}

//**********************************************
void Menu::blank()
{
  menu->templ=false;
  OpenGameDir( 0, true );
}

//**********************************************
void Menu::new_resource_window()
{

  int i,n;
  int sel[4]={0,0,0,0};

  for(i=0;i<MAXWIN;i++){
    if(winlist[i].type==RESOURCES){
      sel[winlist[i].w.r->selected]=1;
    }
  }

  if((n=get_win())==-1)return;
  ResourcesWin *resources_win = new ResourcesWin(NULL,"Resources",n);
  winlist[n].type=RESOURCES;
  winlist[n].w.r=resources_win;
  int res=game->res_default;
  for(i=0;i<4;i++){
    if(sel[i]==0){
      res=i;
      break;
    }
  }
  resources_win->select_resource_type(res);
  resources_win->show();
  num_res++;
  if(num_res>1)disable_resources();

}

//**********************************************
void Menu::add_resource()
{

  resources_win->add_resource();

}

//**********************************************
void Menu::extract_resource()
{

  resources_win->extract_resource();

}

//**********************************************
void Menu::delete_resource()
{

  resources_win->delete_resource();

}

//**********************************************
void Menu::renumber_resource()
{

  resources_win->renumber_resource();

}

//**********************************************
void Menu::rebuild_vol()
{

  switch( QMessageBox::warning( this, "Rebuild VOL files", "Are you sure ?",
                                      "Yes", "No",
                                      0,      // Enter == button 0
                                      1 ) ) { // Escape == button 1
    case 0:
      game->RebuildVOLfiles();
      break;
    case 1:
      break;
    }
}

//**********************************************
void Menu::recompile_all()
{

  switch( QMessageBox::warning( this, "Recompile all", "Do you really want to recompile all logics ?",
                                      "Yes", "No",
                                      0,      // Enter == button 0
                                      1 ) ) { // Escape == button 1
    case 0:
      game->RecompileAll();
      break;
    case 1:
      break;
    }
}

//**********************************************
void Menu::view_editor()
{
  int n;
  if((n=get_win())==-1)return;
  winlist[n].w.v = new ViewEdit(NULL,NULL,n,resources_win);
  winlist[n].type=VIEW;
  winlist[n].w.v->open();

}

//**********************************************
void Menu::logic_editor()
{

  int n;
  if((n=get_win())==-1)return;
  winlist[n].w.l = new LogEdit(NULL,NULL,n,resources_win);
  winlist[n].type=LOGIC;
  winlist[n].w.l->open();

}

//**********************************************
void Menu::text_editor()
{

  int n;
  if((n=get_win())==-1)return;
  winlist[n].w.t = new TextEdit(NULL,NULL,n);
  winlist[n].type = TEXTRES;
  winlist[n].w.t->new_text();

}

//**********************************************
void Menu::object_editor()
{

  int n;
  if((n=get_win())==-1)return;
  winlist[n].w.o = new ObjEdit(NULL,NULL,n);
  winlist[n].type = OBJECT;
  winlist[n].w.o->open();

}

//**********************************************
void Menu::words_editor()
{

  int n;
  if((n=get_win())==-1)return;
  winlist[n].w.w = new WordsEdit(NULL,NULL,n,resources_win);
  winlist[n].type = WORDS;
  winlist[n].w.w->open();

}

//**********************************************
void Menu::picture_editor()
{

  int n;
  if((n=get_win())==-1)return;
  winlist[n].w.p = new PicEdit(NULL,NULL,n,resources_win);
  winlist[n].type=PICTURE;
  winlist[n].w.p->open();

}

//**********************************************
void Menu::sound_player()
{
    extern void play_sound(char *);

    QString fileName = QFileDialog::getOpenFileName(this, tr("Play Sound File"), game->srcdir.c_str(), tr("Sound Files (sound*.*);;All Files (*)"));
    if (!fileName.isNull()) {
        play_sound(fileName.toLatin1().data());
    }
}

//**********************************************
void Menu::help_contents()
  //from QT examples (qbrowser)
{
  sprintf(tmp,"%s/index.html",game->helpdir.c_str());
  if(helpwindow==NULL){
    int n;
    if((n=get_win())==-1)return;
    helpwindow = new HelpWindow(tmp,".");
    winlist[n].type=HELPWIN;
    winlist[n].w.h=helpwindow;
  }
  else helpwindow->setSource(tmp);
  helpwindow->show();
}

//**********************************************
bool Menu::help_topic( const QString& topic )
{
  sprintf(tmp,"%s/%s.html",game->helpdir.c_str(),
    QString(topic).replace(".", "_").toLatin1().constData());

  if (QFile(tmp).exists())
  {
    if(helpwindow1==NULL){
      int n;
      if((n=get_win())==-1) return true;
      helpwindow1 = new HelpWindow(tmp, ".");
      winlist[n].type=HELPWIN;
      winlist[n].w.h=helpwindow1;
    }
    else helpwindow1->setSource(tmp);
    helpwindow1->show();
    helpwindow1->raise();
    return true;
  }
  else
    return false;
}

//**********************************************
void Menu::help_index()
{
  sprintf(tmp,"%s/indexabc.html",game->helpdir.c_str());
  if(helpwindow1==NULL){
    int n;
    if((n=get_win())==-1)return;
    helpwindow1 = new HelpWindow(tmp,".");
    winlist[n].type=HELPWIN;
    winlist[n].w.h=helpwindow1;
  }
  else helpwindow1->setSource(tmp);
  helpwindow1->show();
}

//**********************************************
void Menu::about_it()
{
  if(about==NULL)about=new About();
  about->show();
}

//**********************************************
void Menu::about_qt()
{

  QMessageBox::aboutQt( this, "AGI studio" );

}

//**********************************************
void Menu::errmes(const char *caption, const char *fmt, ...)
{
  char tmp[512];
  va_list argp;

  va_start(argp, fmt);
  vsprintf(tmp,fmt,argp);
  va_end(argp);

  err->setText(QString(tmp));
  err->setWindowTitle(caption);
  err->adjustSize();
  err->show();


}

//*************************************************
void Menu::errmes(const char *fmt, ...)
{
  char tmp[512];
  va_list argp;

  va_start(argp, fmt);
  vsprintf(tmp,fmt,argp);
  va_end(argp);

  err->setText(QString(tmp));
  err->setWindowTitle("AGI studio");
  err->adjustSize();
  err->show();

}


//**********************************************
void Menu::warnmes(const char *fmt, ...)
{
  char tmp[512];
  va_list argp;

  va_start(argp, fmt);
  vsprintf(tmp,fmt,argp);
  va_end(argp);

  warn->setText(QString(tmp));
  warn->setWindowTitle("AGI studio");
  warn->adjustSize();
  warn->show();

}


//**********************************************
#ifdef IMGEXT
void Menu::load_imgext()
  //QT image extensions - to handle more image formats
  //currently it is only jpg and it doesn't work well anyway
{

  qInitImageIO() ;
  imgext = true;

}
#endif

//**********************************************

About::About(QWidget *parent, const char *name )
    : QWidget(parent)
{
  setWindowTitle("About QT AGI Studio");

  QBoxLayout *all = new QVBoxLayout(this);

  QLabel *alogo = new QLabel(this);
  alogo->setPixmap(QPixmap(logo));
  alogo->setAlignment( Qt::AlignHCenter );
  alogo->setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ));
  all->addWidget(alogo);

  QTextEdit* about = new QTextEdit(this);
  about->setReadOnly(true);
  about->setText(
    "<center><b>QT AGI studio v. 1.3.0</b><br>"
    "http://agistudio.sourceforge.net/<br>"
    "<br>"
    "<b>Authors:</b><br>"
    "Helen Zommer (helen@cc.huji.ac.il)<br>"
    "Jarno Elonen (elonen@iki.fi)<br>"
    "<br>"
    "<b>and also:</b><br>"
    "Peter Kelly (pmk@post.com)<br>"
    "Lance Ewing (lance.e@ihug.co.nz)<br>"
    "Claudio Matsuoka (claudio@helllabs.org)<br>"
    "<br>"
    "<b>Windows port by:</b><br>"
    "Nat Budin (natb@brandeis.edu)"
    "<br><br></center>"
    "This program is free software; you can "
    "redistribute it and/or modify it under "
    "the terms of the GNU General Public "
    "License, version 2 or later, as published "
    "by the Free Software Foundation.");
  all->addWidget(about);

  QPushButton *ok = new QPushButton(this);
  ok->setText("OK");
  ok->setMaximumSize(80,40);
  connect(ok, SIGNAL(clicked()), SLOT(hide()));
  all->addWidget(ok);
}

//**********************************************

WindowList::WindowList(QWidget *parent, const char *name )
    : QWidget(parent)
{
  setWindowTitle("Window list");
  QBoxLayout *l =  new QVBoxLayout(this);

  win = new QListWidget(this);
  win->setMinimumSize(100,200);
  connect( win, SIGNAL(selected(int)), SLOT(select_cb(int)) );
  l->addWidget(win);

  QBoxLayout *l1 = new QHBoxLayout();
  l->addLayout(l1);

  QPushButton *select = new QPushButton(this);
  select->setText("Select");
  connect( select, SIGNAL(clicked()), SLOT(select_ok()) );
  l1->addWidget(select);

  QPushButton *del = new QPushButton(this);
  del->setText("Delete");
  connect( del, SIGNAL(clicked()), SLOT(del_cb()) );
  l1->addWidget(del);

  QPushButton *close = new QPushButton(this);
  close->setText("Close list");
  connect( close, SIGNAL(clicked()), SLOT(hide()) );
  l1->addWidget(close);

  adjustSize();
}

//**********************************************

void WindowList::draw()
{
  QString caption;

  win->clear();
  for(int i=0;i<MAXWIN;i++){
    if(winlist[i].type==-1)continue;
    //    printf("i=%d type=%d\n",i,winlist[i].type);
    switch(winlist[i].type){
    case LOGIC:
      caption = QString("Logic editor: ").append(winlist[i].w.l->windowTitle());
      if(winlist[i].w.l->isMinimized())caption.insert(0,"(.) ");
      break;
    case PICTURE:
      caption = winlist[i].w.p->windowTitle();
      if(winlist[i].w.p->isMinimized())caption.insert(0,"(.) ");
      break;
    case VIEW:
      caption = winlist[i].w.v->windowTitle();
      if(winlist[i].w.v->isMinimized())caption.insert(0,"(.) ");
      break;
    case OBJECT:
      caption = winlist[i].w.o->windowTitle();
      if(winlist[i].w.o->isMinimized())caption.insert(0,"(.) ");
      break;
    case WORDS:
      caption = winlist[i].w.w->windowTitle();
      if(winlist[i].w.w->isMinimized())caption.insert(0,"(.) ");
      break;
    case TEXTRES:
      caption = winlist[i].w.t->windowTitle();
      if(winlist[i].w.t->isMinimized())caption.insert(0,"(.) ");
      break;
    case RESOURCES:
      caption = winlist[i].w.r->windowTitle();
      if(winlist[i].w.r->isMinimized())caption.insert(0,"(.) ");
      break;
    case HELPWIN:
      caption = QString("Help");
      if(winlist[i].w.h->isMinimized())caption.insert(0,"(.) ");
      else if(!winlist[i].w.h->isVisible())caption.insert(0,"(~) ");
      break;
    case PREVIEW:
      caption = winlist[i].w.pr->windowTitle();
      if(winlist[i].w.pr->isMinimized())caption.insert(0,"(.) ");
      break;
    }
    win->addItem(caption);
  }
  show();
}

//**********************************************

void WindowList::select_cb(int sel)
{
  QString caption;

  int n=0;
  for(int i=0;i<MAXWIN;i++){
    if(winlist[i].type==-1)continue;
    if(n==sel){
      switch(winlist[i].type){
      case LOGIC:
        if(winlist[i].w.l->isMinimized()){
          winlist[i].w.l->showNormal();
          caption = QString("Logic editor: ").append(winlist[i].w.l->windowTitle());
          win->setCurrentRow(sel);
        }
        winlist[i].w.l->activateWindow();
        winlist[i].w.l->raise();
        break;
      case PICTURE:
        if(winlist[i].w.p->isMinimized()){
          winlist[i].w.p->showNormal();
           caption = winlist[i].w.p->windowTitle();
           win->setCurrentRow(sel);
        }
        winlist[i].w.p->activateWindow();
        winlist[i].w.p->raise();
        break;
      case VIEW:
        if(winlist[i].w.v->isMinimized()){
          winlist[i].w.v->showNormal();
          caption = winlist[i].w.v->windowTitle();
          win->setCurrentRow(sel);
        }
        winlist[i].w.v->activateWindow();
        winlist[i].w.v->raise();
        break;
      case TEXTRES:
        if(winlist[i].w.t->isMinimized()){
          winlist[i].w.t->showNormal();
          caption = winlist[i].w.t->windowTitle();
          win->setCurrentRow(sel);
        }
        winlist[i].w.t->activateWindow();
        winlist[i].w.t->raise();
        break;
      case WORDS:
        if(winlist[i].w.w->isMinimized()){
          winlist[i].w.w->showNormal();
          caption = winlist[i].w.w->windowTitle();
          win->setCurrentRow(sel);
        }
        winlist[i].w.w->activateWindow();
        winlist[i].w.w->raise();
        break;
      case OBJECT:
        if(winlist[i].w.o->isMinimized()){
          winlist[i].w.o->showNormal();
          caption = winlist[i].w.o->windowTitle();
          win->setCurrentRow(sel);
        }
        winlist[i].w.o->activateWindow();
        winlist[i].w.o->raise();
        break;
      case RESOURCES:
        if(winlist[i].w.r->isMinimized()){
          winlist[i].w.r->showNormal();
          caption = winlist[i].w.r->windowTitle();
          win->setCurrentRow(sel);
        }
        winlist[i].w.r->activateWindow();
        winlist[i].w.r->raise();
        break;
      case HELPWIN:
        if(winlist[i].w.h->isMinimized()){
          winlist[i].w.h->showNormal();
          caption = QString("Help");
          win->setCurrentRow(sel);
        }
        else if(!winlist[i].w.h->isVisible()){
          winlist[i].w.h->show();
          caption = winlist[i].w.h->windowTitle();
          win->setCurrentRow(sel);
        }
        winlist[i].w.h->activateWindow();
        winlist[i].w.h->raise();
        break;
      case PREVIEW:
        if(winlist[i].w.pr->isMinimized()){
          winlist[i].w.pr->showNormal();
          caption = winlist[i].w.pr->windowTitle();
          win->setCurrentRow(sel);
        }
        winlist[i].w.pr->activateWindow();
        winlist[i].w.pr->raise();
        break;
      }
      break;
    }
    n++;
  }
}

//**********************************************

void WindowList::select_ok()
{
  if(win->currentRow()!=-1)
    select_cb(win->currentRow());

}

//**********************************************

void WindowList::del_cb()
{
  int sel = win->currentRow();
  if(sel==-1)return;

  int n=0;
  for(int i=0;i<MAXWIN;i++){
    if(winlist[i].type==-1)continue;
    if(n==sel){
      switch(winlist[i].type){
      case LOGIC:
        winlist[i].w.l->close();
        break;
      case VIEW:
        winlist[i].w.v->close();
        break;
      case PICTURE:
        winlist[i].w.p->close();
        break;
      case TEXTRES:
        winlist[i].w.t->close();
        break;
      case WORDS:
        winlist[i].w.w->close();
        break;
      case OBJECT:
        winlist[i].w.o->close();
        break;
      case RESOURCES:
        winlist[i].w.r->close();
        break;
      case HELPWIN:
        winlist[i].w.h->hide();
        break;
      case PREVIEW:
        winlist[i].w.pr->close();
        break;
      }
      break;
    }
    n++;
  }
  draw();
}
