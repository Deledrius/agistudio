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
#include "wutil.h"
#include "picture.h"
#include "preview.h"

#include <cstdlib>
#include <qspinbox.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPixmap>
#include <Q3Frame>
#include <QHideEvent>
#include <QResizeEvent>
#include <Q3PopupMenu>
#include <QMouseEvent>
#include <Q3VBoxLayout>
#include <Q3BoxLayout>
#include <QShowEvent>
#include <QPaintEvent>
#include <QCloseEvent>

#include "left1_x.xpm"
#include "left2_x.xpm"
#include "right1_x.xpm"
#include "right2_x.xpm"
#include "zoom_minus_x.xpm"
#include "zoom_plus_x.xpm"

static const char *comment[]={
      "pic colour",  //0xf0
      "pic off", //0xf1
      "pri colour",//0xf2
      "pri off",   //0xf3,
        "Y corner", //0xf4,
        "X corner", //0xf5,
        "abs line", //0xf6,
        "rel line", //0xf7,
        "fill",  //0xf8,
        "pattern", //0xf9,
        "brush", //0xfa,
        " ",    //0xfb,
        " ",    //0xfc,
        " ",    //0xfd,
        " ",    //0xfe,
        "end",  //0xff
        };

static const char *colname[]={
  "black",
  "blue",
  "green",
  "cyan",
  "red",
  "magenta",
  "brown",
  "white",
  "gray",
  "lightblue",
  "lightgreen",
  "lightcyan",
  "lightred",
  "lightmagenta",
  "yellow",
  "brightwhite"
};


//***************************************
PicEdit::PicEdit( QWidget *parent, const char *name,int win_num,ResourcesWin *res )
    : QWidget( parent, name, Qt::WDestructiveClose)
{
  setCaption("Picture Editor");

  winnum = win_num;
  resources_win = res;
  picture = new Picture();

  Q3PopupMenu *file = new Q3PopupMenu( this );
  Q_CHECK_PTR( file );
  file->insertItem( "New", this, SLOT(open()) );
  file->insertItem( "Load from file", this, SLOT(open_file()) );
  file->insertItem( "Save to game", this, SLOT(save_to_game()) );
  file->insertItem( "Save to game as...", this, SLOT(save_to_game_as()) );
  file->insertItem( "Save to file", this, SLOT(save_file()) );
  file->insertSeparator();
  file->insertItem( "Delete", this, SLOT(delete_picture()) );
  file->insertSeparator();
  file->insertItem( "Close", this, SLOT(close()) );

  Q3PopupMenu *util = new Q3PopupMenu( this );
  Q_CHECK_PTR( util );
  util->insertItem("View data", this, SLOT(view_data()) );
  util->insertItem("Background", this, SLOT(background()) );

  Q3PopupMenu *help = new Q3PopupMenu( this );
  Q_CHECK_PTR( help );
  help->insertItem( "Help on picture editor", this, SLOT(editor_help()) );

  QMenuBar *menu = new QMenuBar(this);
  Q_CHECK_PTR( menu );
  menu->insertItem( "File", file );
  menu->insertItem( "Utilities", util );
  menu->insertItem( "Help", help );
  menu->setSeparator( QMenuBar::InWindowsStyle );

  Q3BoxLayout *all =  new Q3HBoxLayout(this,10);
  all->setMenuBar(menu);

  Q3BoxLayout *leftb = new Q3VBoxLayout(all,10);

  tool = new Q3ButtonGroup(5,Qt::Horizontal,0,this);
  line = new QRadioButton("Line",tool);
  line->setChecked(false);
  //  line->setFocusPolicy(ClickFocus);
  pen = new QRadioButton("Pen",tool);
  pen->setChecked(false);
  // pen->setFocusPolicy(ClickFocus);
  step = new QRadioButton("Step",tool);
  step->setChecked(false);
  // step->setFocusPolicy(ClickFocus);
  fill = new QRadioButton("Fill",tool);
  fill->setChecked(false);
  // fill->setFocusPolicy(ClickFocus);
  brush = new QRadioButton("Brush",tool);
  brush->setChecked(false);
  // brush->setFocusPolicy(ClickFocus);
  connect(tool, SIGNAL(clicked(int)), SLOT(change_tool(int)) );


  leftb->addWidget(tool);

  Q3BoxLayout *b1 = new Q3HBoxLayout(leftb,10);

  palette = new Palette1(this,0,this);
  palette->setMinimumSize(250,40);
  palette->setMaximumSize(350,80);
  b1->addWidget(palette);

  Q3BoxLayout *b2 = new Q3HBoxLayout(leftb,10);

  Q3ButtonGroup *shape = new Q3ButtonGroup(2,Qt::Vertical,"Shape",this);
  QRadioButton *circle = new QRadioButton("Circle",shape);
  circle->setChecked(true);
  //  circle->setFocusPolicy(ClickFocus);
  QRadioButton *square = new QRadioButton("Square",shape);
  square->setChecked(false);
  // square->setFocusPolicy(ClickFocus);
  b2->addWidget(shape);
  connect(shape, SIGNAL(clicked(int)), SLOT(change_shape(int)) );

  Q3ButtonGroup *type = new Q3ButtonGroup(2,Qt::Vertical,"Type",this);
  QRadioButton *spray = new QRadioButton("Spray",type);
  spray->setChecked(true);
  //  spray->setFocusPolicy(ClickFocus);
  QRadioButton *solid = new QRadioButton("Solid",type);
  solid->setChecked(false);
  //  solid->setFocusPolicy(ClickFocus);
  b2->addWidget(type);
  connect(type, SIGNAL(clicked(int)), SLOT(change_type(int)) );

  Q3GroupBox *lsize = new Q3GroupBox(1,Qt::Vertical,this);
  lsize->setTitle("Size");
  lsize->setMargin(4);
  b2->addWidget(lsize);

  QSpinBox *size = new QSpinBox(1,7,1,lsize);
  size->setValue(1);
  //  size->setFocusPolicy(ClickFocus);
  connect(size, SIGNAL(valueChanged(int)), SLOT(change_size(int)) );

  Q3BoxLayout *b3 = new Q3HBoxLayout(leftb,1);

  QPushButton *home = new QPushButton(this);
  home->setPixmap(QPixmap(left2_x));
  //  home->setFocusPolicy(ClickFocus);
  connect(home, SIGNAL(clicked()), SLOT(home_cb()) );
  b3->addWidget(home);
  QPushButton *left = new QPushButton(this);
  //left->setFocusPolicy(ClickFocus);
  left->setPixmap(QPixmap(left1_x));
  connect(left, SIGNAL(clicked()), SLOT(left_cb()) );
  b3->addWidget(left);
  pos = new QLineEdit(this);
  pos->setMinimumWidth(64);
  //pos->setFocusPolicy(ClickFocus);
  connect( pos, SIGNAL(returnPressed()), SLOT(set_pos()) );
  b3->addWidget(pos);
  QPushButton *right = new QPushButton(this);
  //right->setFocusPolicy(ClickFocus);
  connect(right, SIGNAL(clicked()), SLOT(right_cb()) );
  right->setPixmap(QPixmap(right1_x));
  b3->addWidget(right);
  QPushButton *end = new QPushButton(this);
  //end->setFocusPolicy(ClickFocus);
  connect(end, SIGNAL(clicked()), SLOT(end_cb()) );
  end->setPixmap(QPixmap(right2_x));
  b3->addWidget(end);
  QPushButton *del = new QPushButton("Del",this);
  //del->setFocusPolicy(ClickFocus);
  connect(del, SIGNAL(clicked()), SLOT(del_cb()) );
  b3->addWidget(del);
  QPushButton *wipe = new QPushButton("Wipe",this);
  //wipe->setFocusPolicy(ClickFocus);
  connect(wipe, SIGNAL(clicked()), SLOT(wipe_cb()) );
  b3->addWidget(wipe);

  Q3BoxLayout *b31 = new Q3HBoxLayout(leftb,4);

  codeline = new QLineEdit(this);
  codeline->setFocusPolicy(Qt::NoFocus);
  b31->addWidget(codeline);

  comments = new QLineEdit(this);
  comments->setFocusPolicy(Qt::NoFocus);
  b31->addWidget(comments);

  Q3BoxLayout *b4 = new Q3HBoxLayout(leftb,2);

  QPushButton *zoom_minus = new QPushButton(this);
  zoom_minus->setPixmap(QPixmap(zoom_minus_x));
  zoom_minus->setFixedSize(32,32);
  //zoom_minus->setFocusPolicy(ClickFocus);
  connect( zoom_minus, SIGNAL(clicked()), SLOT(zoom_minus()) );
  b4->addWidget(zoom_minus);

  QPushButton *zoom_plus = new QPushButton(this);
  zoom_plus->setPixmap(QPixmap(zoom_plus_x));
  zoom_plus->setFixedSize(32,32);
  //zoom_plus->setFocusPolicy(ClickFocus);
  connect( zoom_plus, SIGNAL(clicked()), SLOT(zoom_plus()) );
  b4->addWidget(zoom_plus);

  Q3ButtonGroup *drawmode = new Q3ButtonGroup(2,Qt::Vertical,"Show",this);
  pic = new QRadioButton("Visual",drawmode);
  pic->setChecked(true);
  pri_mode=false;
  picture->set_mode(0);
  pri = new QRadioButton("Priority",drawmode);
  bg = new QCheckBox("Background",drawmode);
  prilines = new QCheckBox("PriorityLines",drawmode);
  connect( drawmode, SIGNAL(clicked(int)), SLOT(change_drawmode(int)) );
  drawmode->setExclusive(false);
  b4->addWidget(drawmode);

  status = new QStatusBar(this);
  QLabel *msg = new QLabel( status, "message" );
  status->addWidget( msg, 4 );
  pricolor = new Q3Frame( status );
  pricolor->setMinimumSize( 8,8 );
  pricolor->setMaximumSize( 8,8 );
  QToolTip::add( pricolor, "Priority 'color' required to mask an EGO on this priority level" );
  status->addWidget( pricolor, 0, true );
  status->setSizeGripEnabled( false );
  leftb->addWidget(status);

  if(game->picstyle==P_TWO){
    canvas = new PCanvas(0,0,this);
    canvas->setMinimumSize(canvas->pixsize*MAX_W+canvas->x0+10,canvas->pixsize*MAX_HH+canvas->x0+10);
    canvas->resizeContents(canvas->pixsize*MAX_W+canvas->x0,canvas->pixsize*MAX_HH+canvas->x0);
    canvas->resize(canvas->pixsize*MAX_W+canvas->x0,canvas->pixsize*MAX_HH+canvas->x0);
    //canvas->setFocusPolicy(ClickFocus);

  }
  else{
    canvas = new PCanvas(this,0,this);
    canvas->setMinimumSize(canvas->pixsize*MAX_W+canvas->x0+10,canvas->pixsize*MAX_HH+canvas->x0+10);
    canvas->resizeContents(canvas->pixsize*MAX_W+canvas->x0,canvas->pixsize*MAX_HH+canvas->x0);
    all->addWidget(canvas,1);
    //canvas->setFocusPolicy(ClickFocus);
    setFocusProxy(canvas);
  }


  changed = false;
  adjustSize();
  viewdata = NULL;
  closing = false;

}
//*********************************************
void PicEdit::save(char *filename)
{
  picture->save(filename);
  changed = false;

}

//*********************************************
void PicEdit::open_file()
{
  Q3FileDialog *f = new Q3FileDialog(0,"Open",true);
  const char *filters[] = {"picture*.*","All files (*)",NULL};

  f->setFilters(filters);
  f->setCaption("Open picture");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      open((char *)f->selectedFile().latin1());
  }

}

//*********************************************
void PicEdit::open(int ResNum)
{
  if(picture->open(ResNum))return;
  PicNum = ResNum;
  sprintf(tmp,"Picture editor: picture.%d",PicNum);
  setCaption(tmp);
  if(canvas->isTopLevel())
    canvas->setCaption(tmp);
  canvas->update();
  show_pos();
  changed=false;
  show();
  canvas->show();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::open(char *filename)
{
  if(picture->open(filename))return;
  PicNum = -1;
  sprintf(tmp,"Picture editor");
  setCaption(tmp);
  if(canvas->isTopLevel())
    canvas->setCaption(tmp);
  canvas->update();
  show_pos();
  changed=false;
  show();
  canvas->show();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::save_file()
{
  Q3FileDialog *f = new Q3FileDialog(0,"Save",true);
  const char *filters[] = {"picture*.*","All files (*)",NULL};

  f->setFilters(filters);
  f->setCaption("Save picture");
  f->setMode(Q3FileDialog::AnyFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      save((char *)f->selectedFile().latin1());
  }
}

//*********************************************
void PicEdit::deinit()
{
  if(viewdata){
    viewdata->close(true);
    viewdata=NULL;
  }
  if(canvas->isTopLevel()){
    closing=true;
    canvas->close(true);
  }
  delete picture;
  winlist[winnum].type=-1;
  if(window_list && window_list->isVisible())window_list->draw();

}
//*********************************************
void PicEdit::showEvent( QShowEvent * )
{
  showing=true;
  if(canvas->isTopLevel() && !canvas->isVisible()){
    canvas->showNormal();
  }
  showing=false;
  if(window_list && window_list->isVisible())window_list->draw();


}
//*********************************************
void PicEdit::hideEvent( QHideEvent * )
{
  hiding=true;
  if(isMinimized() && canvas->isTopLevel() && canvas->isVisible()){
    canvas->showMinimized();
  }
  hiding=false;
  if(viewdata){
    viewdata->close(true);
    viewdata=NULL;
  }
  if(window_list && window_list->isVisible())window_list->draw();


}
//*********************************************
void PicEdit::closeEvent( QCloseEvent *e )
{
  if(changed){
    if(PicNum != -1){
      sprintf(tmp,"Save changes to picture.%d ?",PicNum);
    }
    else{
      sprintf(tmp,"Save changes to picture ?");
    }
    strcat(tmp,"\n(picture will be saved to game)");

    switch ( QMessageBox::warning( this, "Picture editor",
                                   tmp,
                                   "Yes",
                                   "No",
                                   "Cancel",
                                   0, 2) ) {
    case 0: // yes
      save_to_game();
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

//*********************************************
void PicEdit::save_to_game()
{
  if(PicNum != -1){
    picture->save(PicNum);
    if(resources_win){
      if(resources_win->preview==NULL)resources_win->preview=new Preview();
      resources_win->preview->open(PicNum,PICTURE);
    }
    changed=false;
  }
  else
    save_to_game_as();

}

//*********************************************
void PicEdit::save_to_game_as()
{
  AskNumber *picture_number = new AskNumber(0,0,"Picture number","Enter picture number: [0-255]");

  if(!picture_number->exec())return;

  QString str = picture_number->num->text();
  int num = atoi((char *)str.latin1());

  if(num<0||num>255){
    menu->errmes("Picture number must be between 0 and 255 !");
    return ;
  }
  if(game->ResourceInfo[PICTURE][num].Exists){
    sprintf(tmp,"Resource picture.%d already exists. Replace it ?",num);

    switch( QMessageBox::warning( this, "Picture", tmp,
                                  "Replace", "Cancel",
                                  0,      // Enter == button 0
                                  1 ) ) { // Escape == button 1
    case 0:
      picture->save(num);
      PicNum = num;
      if(resources_win){
        if(resources_win->preview==NULL)resources_win->preview=new Preview();
        resources_win->preview->open(PicNum,PICTURE);
      }
      changed=false;
      break;
    case 1:
      break;
    }
  }
  else{
    picture->save(num);
    changed=false;
    PicNum = num;
    if(resources_win){
      resources_win->select_resource_type(PICTURE);
      resources_win->set_current(num);
    }
    open(num);
  }

}

//*********************************************
void PicEdit::delete_picture()
{
  int k;
  if(PicNum==-1)return;
  sprintf(tmp,"Really delete picture %d ?",PicNum);
  switch( QMessageBox::warning( this, "Picture", tmp,
                                "Delete", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0:
    game->DeleteResource(PICTURE,PicNum);
    if(resources_win){
      k = resources_win->list->currentItem();
      resources_win->select_resource_type(PICTURE);
      resources_win->list->setCurrentItem(k);
    }
    break;
  case 1:
    break;
  }

}

//*********************************************
void PicEdit::open()
{
  picture->newpic();
  show_pos();
  if(canvas->isTopLevel())
    canvas->setCaption("picture");
  canvas->update();
  show();
  canvas->show();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::view_data()
{
  if(viewdata==NULL)viewdata=new ViewData(0,0,picture);
  if(PicNum!=-1){
    sprintf(tmp,"View data: picture %d",PicNum);
    viewdata->setCaption(tmp);
  }
  else viewdata->setCaption("View data: picture");
  viewdata->read();

}

//*********************************************
void PicEdit::background()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Load background image",true);
  const char *filters[] = {"All files (*)",NULL};

  f->setFilters(filters);
  f->setCaption("Load background image");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      canvas->load_bg((char *)f->selectedFile().latin1());
  }

}

//*********************************************
void PicEdit::zoom_minus()
{

  if(canvas->pixsize>1){
    canvas->setPixsize(canvas->pixsize-1);
    int w,h;
    w = canvas->cur_w+4;
    h = canvas->cur_h+4;
    canvas->resizeContents(w,h);
  }

}

//*********************************************
void PicEdit::zoom_plus()
{

  if(canvas->pixsize<4){
    canvas->setPixsize(canvas->pixsize+1);
    int w,h;
    w = canvas->cur_w+4;
    h = canvas->cur_h+4;
    canvas->resizeContents(w,h);
  }

}

//*********************************************
void PicEdit::change_drawmode(int mode)
{

  switch(mode){
  case 0:  //draw visual
    pri_mode=false;
    picture->set_mode(0);
    pri->setChecked(false);
    pic->setChecked(true);
    break;
  case 1:  //draw priority
    pri_mode=true;
    picture->set_mode(1);
    pic->setChecked(false);
    pri->setChecked(true);
    break;
  case 2: //draw (also) background
    canvas->bg_on=bg->isChecked();
    if(canvas->bg_on && canvas->bg_loaded)
      picture->bg_on=true;
    else
      picture->bg_on=false;
    break;
  case 3: //priority lines
    canvas->pri_lines=prilines->isChecked();
    break;
  }
  canvas->linedraw=false;
  canvas->update();

}

//*********************************************
void PicEdit::change_tool(int k)
{
  if(canvas->linedraw)canvas->line(false);
  picture->tool_proc(k);
}

//*********************************************
void PicEdit::change_size(int k)
{
  picture->set_brush(0,k);

}

//*********************************************
void PicEdit::change_shape(int k)
{
  picture->set_brush(1,k);

}

//*********************************************
void PicEdit::change_type(int k)
{
  picture->set_brush(2,k);

}

//*********************************************
void PicEdit::show_pos()
  //show current picture buffer position
{
  char *t;
  int code=0,val=-1;
  sprintf(tmp,"%5d",picture->bufPos);
  pos->setText(tmp);
  t=picture->showPos(&code,&val);
  codeline->setText(t);
  if(code>=0xf0&&code<=0xff){  //action: can add comments
    if(code==0xf0||code==0xf2){  //add color name
      sprintf(tmp,"%s %s",comment[code-0xf0],colname[val]);
      comments->setText(tmp);
    }
    else{
      comments->setText(comment[code-0xf0]);
    }
  }
  else comments->clear();

}

//*********************************************
void PicEdit::home_cb()
  //set picture buffer position to start
{
  picture->home_proc();
  canvas->update();
  show_pos();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::end_cb()
  //set picture buffer position to end
{
  picture->end_proc();
  canvas->update();
  show_pos();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::left_cb()
  //set picture buffer position to the previous action
{
  picture->left_proc();
  canvas->update();
  show_pos();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::right_cb()
  //set picture buffer position to the next action
{

  picture->right_proc();
  canvas->update();
  show_pos();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::del_cb()
  //delete action from the picture buffer position
{

  picture->del_proc();
  canvas->update();
  show_pos();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::wipe_cb()
  //delete all action from the picture buffer position to the end
{

  picture->wipe_proc();
  canvas->update();
  show_pos();
  update_palette();
  update_tools();
}

//*********************************************
void PicEdit::set_pos()
  //set picture buffer position
{
  QString str=pos->text();
  char *s = (char *)str.latin1();
  int num = atoi(s);
  if(num!=0 || s[0]=='0'){
    if(!picture->setBufPos(num)){
      canvas->update();
    }
  }
  show_pos();
  update_palette();
  update_tools();
  pos->clearFocus();
  setFocus();
  return;

}
//*********************************************
void PicEdit::update_tools()
{

  switch(picture->tool){
  case T_LINE:
    if(!line->isChecked())line->setChecked(true);
    break;
  case T_STEP:
    if(!step->isChecked())step->setChecked(true);
    break;
  case T_PEN:
    if(!pen->isChecked())pen->setChecked(true);
    break;
  case T_FILL:
    if(!fill->isChecked())fill->setChecked(true);
    break;
  case T_BRUSH:
    if(!brush->isChecked())brush->setChecked(true);
    break;
  default:
    QRadioButton *b = (QRadioButton *)tool->selected();
    if(b)b->setChecked(false);
  }

}
//*********************************************
void PicEdit::update_palette()
{
  bool ch=false;
  if(!picture->picDrawEnabled && palette->left!=-1){
    palette->left=-1;
    ch=true;
  }
  if(picture->picDrawEnabled && palette->left!=picture->picColour){
    palette->left=picture->picColour;
    ch=true;
  }
  if(!picture->priDrawEnabled && palette->right!=-1){
    palette->right=-1;
    ch=true;
  }
  if(picture->priDrawEnabled && palette->right!=picture->priColour){
    palette->right=picture->priColour;
    ch=true;
  }

  if(ch)palette->update();


}
//*********************************************
bool PicEdit::focusNextPrevChild ( bool )
{
  canvas->setFocus();
  return true;

}

//************************************************
void PicEdit::editor_help()
{
  menu->help_topic("picture_editor_main");
}

//************************************************

PCanvas::PCanvas ( QWidget *parent, const char *name, PicEdit *w)
    : Q3ScrollView( parent, name )
  //the area to draw picture
{

  picedit = w;
  picture = picedit->picture;
  pixsize=2;
  x0=5;y0=5;
  cur_w=MAX_W*pixsize;
  cur_h=MAX_HH*pixsize;
  pixmap = QPixmap(cur_w,cur_h);
  viewport()->setMouseTracking(true);
  bg_loaded=false;
  bg_on=false;
  pri_lines=false;
  bgpix = QImage();
}

//*********************************************
void PCanvas::setSize(int w,int h)
{
  if(cur_w != w || cur_h != h){
    pixmap.resize(w*pixsize*2,h*pixsize);
    cur_w=w;
    cur_h=h;
  }
}

//*********************************************
void PCanvas::setPixsize(int s)
{

  pixsize=s;
  cur_w=MAX_W*pixsize;
  cur_h=MAX_HH*pixsize;
  pixmap.resize(cur_w,cur_h);
  QPainter p(&pixmap);
  p.eraseRect(0,0,cur_w,cur_h);
  update();

}
//*********************************************
void PCanvas::viewportMousePressEvent(QMouseEvent* event)
{
  int x,y,xx,yy;

  viewportToContents( event->x(),  event->y(), x, y );
  xx=x;
  yy=y;
  x-=x0;
  y-=y0;

  x/=pixsize;
  y/=pixsize;

  if(x<0)x=0;
  else if(x>=MAX_W)x=MAX_W-1;
  if(y<0)y=0;
  else if(y>=MAX_HH)y=MAX_HH-1;

  if (event->button() & Qt::LeftButton){
    if(picture->button_action(x,y))
      picedit->show_pos();
  }
  else if (event->button() & Qt::RightButton){
    /*
    QRadioButton *b = (QRadioButton *)picedit->tool->selected();
        if(b!=0)b->setChecked(false);
    picture->clear_tools();
    picture->tool = -1;
    */
    picture->init_tool();
  }
  update();
  picedit->changed=true;
}

//*********************************************
void PCanvas::viewportMouseMoveEvent(QMouseEvent* event)
{
  int x,y,xx,yy;

  viewportToContents( event->x(),  event->y(), x, y );
  xx=x;
  yy=y;

  x-=x0;
  y-=y0;

  x/=pixsize;
  y/=pixsize;

  if(x<0)x=0;
  else if(x>=MAX_W)x=MAX_W-1;
  if(y<0)y=0;
  else if(y>=MAX_HH)y=MAX_HH-1;

  x1=x;
  y1=y;

  //  printf("mouse move: %d %d (%d %d)\n",x,y,xx,yy);

  if(picture->move_action(x,y))
    line(true);

  if(x>=0&&y>=0){
    int pri = y/12+1;
    sprintf(tmp,"X=%d  Y=%d  Pri=%d",x/2,y,pri);
    picedit->status->message(tmp);
    picedit->pricolor->setPaletteBackgroundColor( egacolor[pri+1] );
  }

}

//*********************************************
void PCanvas::drawContents(QPainter* p,int ,int ,int ,int )
{

  if(cur_w==0 ||cur_h==0)return;

  p->drawPixmap( x0, y0, pixmap );

}

//*********************************************
void PCanvas::update()
{

  QPainter p(&pixmap);
  int x,y;
  byte c;
  byte *data;

  data = (picedit->pri_mode)?picture->priority:picture->picture;
  if(bg_loaded && bg_on){  //if must draw background
    bool pic = !picedit->pri_mode;
    for(y=0;y<MAX_HH;y++){
      for(x=0;x<MAX_W;x+=2){
        c=data[y*MAX_W+x];
        if((pic&&c==15)||(!pic&&c==4)){  //draw background instead of "empty" areas
          p.fillRect(x*pixsize,y*pixsize,pixsize,pixsize,QColor(bgpix.pixel(x,y)));
          p.fillRect((x+1)*pixsize,y*pixsize,pixsize,pixsize,QColor(bgpix.pixel(x+1,y)));
        }
        else{
          p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[c]);
        }
      }
    }
  }
  else{
    for(y=0;y<MAX_HH;y++){
      for(x=0;x<MAX_W;x+=2){
        p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[data[y*MAX_W+x]]);
      }
    }
  }
  linedraw=false;

  if(pri_lines){
    QPen pen;
    pen.setStyle(Qt::DashLine);
    pen.setWidth(1);  
    
    //p.setPen(Qt::white);
    // p.setRasterOp(XorROP);
    int i=4;
    int step=MAX_HH*pixsize/14;
    for(y=step*3;y<MAX_HH*pixsize;y+=step){
      //pen.setBrush(QColor(255, 0, 0, 127));
      pen.setBrush(egacolor[i++]);
      p.setPen(pen);
      p.drawLine(0,y,MAX_W*pixsize,y);
      pen.setBrush(QColor(0, 0, 0, 127));
      p.setPen(pen);
      p.drawLine(0,y+1,MAX_W*pixsize,y+1);
    }
  }

  repaintContents(x0,y0,x0+MAX_W*pixsize,y0+MAX_HH*pixsize,false);

}

//*********************************************
void PCanvas::line(bool mode)
  //draw/erase 'temporary' line following the cursor (line, pen or step mode)
  //mode==true - erase the old one, draw the new one
  //mode==false - only erase the old one
{
  QPainter p(&pixmap);
  byte c;
  Points *curp,*newp=NULL;
  int i;

  //restore the previous line
  curp = picture->curp;

  if(picture->bg_on){
    //with background - must redraw the line containing background pixels
    //(x-pixels are not doubled !)
    for(i=0;i<curp->n;i++){
      p.fillRect(curp->p[i].x*pixsize,curp->p[i].y*pixsize,
                 pixsize,pixsize,curp->p[i].cc);

    }
  }
  else{
    //no background - just restore the agi image
    for(i=0;i<curp->n;i++){
      c=curp->p[i].c;
      p.fillRect(curp->p[i].x*2*pixsize,curp->p[i].y*pixsize,
                 pixsize*2,pixsize,egacolor[c]);

    }
  }

  if(mode==true){
    linedraw=true;
    newp = picture->newp;
    //draw the 'new' line
    for(int i=0;i<newp->n;i++){
      c=newp->p[i].c;
      p.fillRect(newp->p[i].x*2*pixsize,newp->p[i].y*pixsize,
                 pixsize*2,pixsize,egacolor[c]);

    }
  }
  else{
    linedraw=false;
  }

#if 0
  //find the max rectangle to repaint
  if(curp->n){
    if(picture->bg_on){
      x00=curp->p[0].x;
      y00=curp->p[0].y;
      x11=curp->p[curp->n-1].x;
      y11=curp->p[curp->n-1].y;
    }
    else{
      x00=curp->p[0].x*2;
      y00=curp->p[0].y;
      x11=curp->p[curp->n-1].x*2;
      y11=curp->p[curp->n-1].y;
    }
    if(newp){
      x0_=newp->p[0].x*2;
      y0_=newp->p[0].y;
      x1=newp->p[newp->n-1].x*2;
      y1=newp->p[newp->n-1].y;

      X0=(MIN(MIN(x00,x0_),MIN(x11,x1)))*pixsize;
      Y0=(MIN(MIN(y00,y0_),MIN(y11,y1)))*pixsize;
      X1=(MAX(MAX(x00,x0_),MAX(x11,x1)))*pixsize;
      Y1=(MAX(MAX(y00,y0_),MAX(y11,y1)))*pixsize;
    }
    else{
      X0=(MIN(x00,x11))*pixsize;
      Y0=(MIN(y00,y11))*pixsize;
      X1=(MAX(x00,x11))*pixsize;
      Y1=(MAX(y00,y11))*pixsize;
    }
  }
  else{

    x0_=newp->p[0].x*2*pixsize;
    y0_=newp->p[0].y*pixsize;
    x1=newp->p[newp->n-1].x*2*pixsize;
    y1=newp->p[newp->n-1].y*pixsize;
    X0=MIN(x0_,x1);
    Y0=MIN(y0_,y1);
    X1=MAX(x0_,x1);
    Y1=MAX(y0_,y1);

  }
#endif
  repaintContents(x0,y0,x0+MAX_W*pixsize,y0+MAX_HH*pixsize,false);

}
//*********************************************
void PCanvas::keyPressEvent( QKeyEvent *k )
{
  switch(k->key()){
  case Qt::Key_L:
  case Qt::Key_F1:
    picedit->line->setChecked(true);
    picedit->change_tool(T_LINE);
    break;
  case Qt::Key_P:
  case Qt::Key_F2:
    picedit->pen->setChecked(true);
    picedit->change_tool(T_PEN);
    break;
  case Qt::Key_S:
  case Qt::Key_F3:
    picedit->step->setChecked(true);
    picedit->change_tool(T_STEP);
    break;
  case Qt::Key_F:
  case Qt::Key_F4:
    picedit->fill->setChecked(true);
    picedit->change_tool(T_FILL);
    break;
  case Qt::Key_B:
  case Qt::Key_F5:
    picedit->brush->setChecked(true);
    picedit->change_tool(T_BRUSH);
    break;
  case Qt::Key_Tab:
    picedit->pri_mode=!picedit->pri_mode;
    picedit->change_drawmode(picedit->pri_mode?1:0);
    break;
  case Qt::Key_Home:
    picedit->home_cb();
    break;
  case Qt::Key_End:
    picedit->end_cb();
    break;
  case Qt::Key_Right:
    picedit->right_cb();
    break;
  case Qt::Key_Left:
    picedit->left_cb();
    break;
  case Qt::Key_Delete:
    picedit->del_cb();
    break;
  case Qt::Key_F10:
    if(picedit->bg->isChecked())picedit->bg->setChecked(false);
    else picedit->bg->setChecked(true);
    picedit->change_drawmode(2);
    break;

  default:
    k->ignore();
    break;
  }

}
//*********************************************
bool PCanvas::focusNextPrevChild ( bool )
{

  setFocus();
  return false;

}
//*****************************************
void PCanvas::load_bg(char *filename)
{

  if(!bgpix.load(filename)){  //loads all formats supported by QT
#ifdef IMGEXT
    //if can't load - try image extensions
    //(currently only jpeg; loads with colors all wrong)
    if(!menu->imgext){
      menu->load_imgext();
      if(!bgpix.load(filename)){
        menu->errmes("Can't open file %s !",filename);
        return;
      }
    }
    else{
      menu->errmes("Can't open file %s !",filename);
      return;
    }
#else
    menu->errmes("Can't open file %s !",filename);
    return;
#endif
  }
  bg_loaded=true;

  picture->bgpix=&bgpix;
  picedit->bg->setChecked(true);
  bg_on=true;
  picture->bg_on=true;
  update();

}

//*********************************************
void PCanvas::showEvent( QShowEvent * )
{

  if(isTopLevel() && !picedit->isVisible() && !picedit->showing){
    picedit->showNormal();
  }
  if(window_list && window_list->isVisible())window_list->draw();

}
//*********************************************
void PCanvas::hideEvent( QHideEvent * )
{

  if(isTopLevel() && picedit->isVisible() && !picedit->hiding){
    picedit->showMinimized();
  }

}
//*********************************************
void PCanvas::closeEvent( QCloseEvent *e )
{

  if(picedit->closing){
    e->accept();
    return;
  }
  picedit->closing=true;
  if(isTopLevel()){
    picedit->close();
    e->ignore();
  }

}
/*******************************************************/
Palette1::Palette1( QWidget *parent, const char *name , PicEdit *p)
    : QWidget( parent, name )
{

  left=right=-1;
  picedit = p;

}

//*****************************************
void Palette1::paintEvent( QPaintEvent * )
  //draw palette with the currently selected colors marked 'V' (visual) and 'P' (priority)
{
    QPainter p(this);
    int w,h,x,y,dx,dy,i;

    w = this->width();
    h = this->height();
    dx=w/9;
    dy=h/2;
    w=dx*9;
    h=dy*2;

    for(y=0,i=0;y<h;y+=dy){
      for(x=0;x<w-dx;x+=dx,i++){
        p.fillRect(x,y,dx,dy,egacolor[i]);
        if(i==left){
          p.setPen(i<10?egacolor[15]:egacolor[0]);
          p.drawText(x+dx/4,y+dy/2+2,"V");
        }
        if(i==right){
          p.setPen(i<10?egacolor[15]:egacolor[0]);
          p.drawText(x+dx*2/3,y+dy/2+2,"P");
        }
      }
    }
    p.fillRect(w-dx,0,dx,h,QColor(0x90,0x90,0x90));
    p.setPen(egacolor[0]);
    x=w-dx;
    p.drawText(x+2,h-dy/3,"Off");
    if(left==-1){
      p.drawText(x+dx/4,dy/2+2,"V");
    }
    if(right==-1){
      p.drawText(x+dx*2/3,dy/2+2,"P");
    }
}

//*****************************************
void Palette1::mousePressEvent(QMouseEvent* event)
{
  int w,h,x,y,dx,dy,i;


  w = this->width();
  h = this->height();
  dx=w/9;
  dy=h/2;
  w=dx*9;
  h=dy*2;

  x=event->x()/dx;
  y=event->y()/dy;

  if(x>=8){  //choose "off"
    i=-1;
  }
  else{      //choose color
    i=y*8+x;
  }

  if (event->button() & Qt::LeftButton){
    if(left != i){
      left = i;
      picedit->picture->choose_color(M_LEFT,i);
      repaint();
    }
  }
  else if (event->button() & Qt::RightButton){
    if(right != i){
      right = i;
      picedit->picture->choose_color(M_RIGHT,i);
      repaint();
    }
  }

}

//************************************************
ViewData::ViewData( QWidget *parent, const char *name,Picture *p )
    : QWidget( parent, name)
  //view picture codes
{

  picture = p;
  Q3BoxLayout *all = new Q3VBoxLayout(this,20);
  codes = new Q3MultiLineEdit(this);
  codes->setMinimumSize(300,200);
  codes->setReadOnly(true);
  all->addWidget(codes);

  Q3BoxLayout *b = new Q3HBoxLayout(all,20);
  Q3BoxLayout *left = new Q3VBoxLayout(b,20);
  comments = new QCheckBox("Show comments",this);
  connect(comments, SIGNAL(clicked()), SLOT(read()));
  left->addWidget(comments);
  wrap = new QCheckBox("Line wrap",this);
  connect(wrap, SIGNAL(clicked()), SLOT(read()));
  left->addWidget(wrap);

  Q3BoxLayout *right = new Q3VBoxLayout(b,20);
  QPushButton *close = new QPushButton("Close",this);
  close->setMaximumSize(80,60);
  connect(close, SIGNAL(clicked()), SLOT(close()));
  right->addWidget(close);
  QLabel *dummy = new QLabel("     ",this);
  right->addWidget(dummy);

  data.lfree();

}

//************************************************
void ViewData::resizeEvent( QResizeEvent * )
{

  QString str = codes->text();
  getmaxcol();
  codes->setText(str);
}

//************************************************
void ViewData::getmaxcol()
{

  QFontMetrics f = fontMetrics();
  maxcol = codes->width()/f.width('a');

}

//************************************************
void ViewData::read()
{
  char *str;
  bool comm = comments->isChecked();
  bool wr = wrap->isChecked();
  int c,cc,i,k,len;
  char *ptr,*ptr0;
  bool first;

  codes->setUpdatesEnabled(false);  //to speed up repaint

  picture->viewData(&data);
  getmaxcol();

  codes->clear();
  for(i=0;i<data.num;i++){
	string str2 = data.at(i);
    str = (char *)str2.c_str();
    if(wr){     //wrap long lines
      k=0;
      tmp[0]=0;
      first=true;
      for(ptr=ptr0=str;*ptr;ptr+=3){
        if((first && k+3>=maxcol)||k+6>=maxcol){
          len=(int)(ptr-ptr0);
          strncat(tmp,ptr0,len);
          tmp[len]=0;
          codes->insertLine(tmp,-1);
          strcpy(tmp,"   ");
          first=false;
          ptr0=ptr;
          k=0;
        }
        else k+=3;
      }
      strcat(tmp,ptr0);
      if(comm){  //add comments (action and color when applicable)
        sscanf(str,"%x %x",&c,&cc);
        strcat(tmp," //");
        strcat(tmp,comment[c-0xf0]);
        if(c==0xf0||c==0xf2){
          strcat(tmp," ");
          strcat(tmp,colname[cc]);
        }
      }
      codes->insertLine(tmp,-1);
    }
    else{
      if(comm){  //add comments (action and color when applicable)
        sscanf(str,"%x %x",&c,&cc);
        sprintf(tmp,"%s //%s",str,comment[c-0xf0]);
        if(c==0xf0||c==0xf2){
          strcat(tmp," ");
          strcat(tmp,colname[cc]);
        }
        codes->insertLine(tmp,-1);
      }
      else codes->insertLine(str,-1);
    }
  }
  codes->setUpdatesEnabled(true);
  codes->update();
  show();
}

//************************************************

