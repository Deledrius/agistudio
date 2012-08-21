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
#include "viewedit.h"
#include "menu.h"
#include "wutil.h"
#include "preview.h"

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include <qapplication.h>
#include <qsplitter.h> 
#include <q3frame.h> 
#include <qmessagebox.h> 
#include <q3filedialog.h> 
#include <qstringlist.h> 
#include <qlayout.h>
#include <qpixmap.h>
#include <qpainter.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <Q3GridLayout>
#include <QHideEvent>
#include <QResizeEvent>
#include <Q3PopupMenu>
#include <QMouseEvent>
#include <Q3VBoxLayout>
#include <Q3BoxLayout>
#include <QShowEvent>
#include <QPaintEvent>
#include <QCloseEvent>

#include "zoom_minus_x.xpm"
#include "zoom_plus_x.xpm"
#include "right_x.xpm"
#include "left_x.xpm"
#include "rightarrow_x.xpm"
#include "leftarrow_x.xpm"
#include "uparrow_x.xpm"
#include "downarrow_x.xpm"

Cel saveCel = Cel();  //cel "clipboard"
bool cel_copied = false;

//*********************************************
ViewEdit::ViewEdit( QWidget *parent, const char *name,int win_num, ResourcesWin *res)
    : QWidget( parent, name, Qt::WDestructiveClose)
{

  setCaption("View Editor");

  winnum = win_num;
  resources_win = res;
  view = new View();

  Q3PopupMenu *file = new Q3PopupMenu( this );
  Q_CHECK_PTR( file );

  file->insertItem( "New", this, SLOT(open()) );
  file->insertItem( "Load from file", this, SLOT(open_file()) );
  file->insertItem( "Save to game", this, SLOT(save_to_game()) );
  file->insertItem( "Save to game as...", this, SLOT(save_to_game_as()) );
  file->insertItem( "Save to file", this, SLOT(save_file()) );
  file->insertSeparator();
  file->insertItem( "Delete", this, SLOT(delete_view()) );
  file->insertSeparator();
  file->insertItem( "Close", this, SLOT(close()) );

  Q3PopupMenu *edit = new Q3PopupMenu( this );
  Q_CHECK_PTR( edit );

  edit->insertItem( "Undo", this, SLOT(undo_cel()) );
  edit->insertSeparator();
  edit->insertItem( "&Copy cel", this, SLOT(copy_cel()) ,Qt::CTRL+Qt::Key_C );
  edit->insertItem( "&Paste cel", this, SLOT(paste_cel()), Qt::CTRL+Qt::Key_V );

  Q3PopupMenu *loop = new Q3PopupMenu( this );
  Q_CHECK_PTR( loop );

  loop->insertItem( "Next", this, SLOT(next_loop()) );
  loop->insertItem( "Previous", this, SLOT(previous_loop()) );
  loop->insertItem( "First", this, SLOT(first_loop()) );
  loop->insertItem( "Last", this, SLOT(last_loop()) );
  loop->insertSeparator();
  loop->insertItem( "Insert before", this, SLOT(insert_loop_before()) );
  loop->insertItem( "Insert after", this, SLOT(insert_loop_after()) );
  loop->insertItem( "Append", this, SLOT(append_loop()) );
  loop->insertItem( "Delete", this, SLOT(delete_loop()) );
  loop->insertItem( "Clear", this, SLOT(clear_loop()) );

 
  Q3PopupMenu *cel = new Q3PopupMenu( this );
  Q_CHECK_PTR( cel );

  cel->insertItem( "Next", this, SLOT(next_cel()) );
  cel->insertItem( "Previous", this, SLOT(previous_cel()) );
  cel->insertItem( "First", this, SLOT(first_cel()) );
  cel->insertItem( "Last", this, SLOT(last_cel()) );
  cel->insertSeparator();
  cel->insertItem( "Insert before", this, SLOT(insert_cel_before()) );
  cel->insertItem( "Insert after", this, SLOT(insert_cel_after()) );
  cel->insertItem( "Append", this, SLOT(append_cel()) );
  cel->insertItem( "Delete", this, SLOT(delete_cel()) );
  cel->insertItem( "Clear", this, SLOT(clear_cel()) );
  cel->insertSeparator();
  cel->insertItem( "Flip Horizontally", this, SLOT(fliph_cel()) );
  cel->insertItem( "Flip Vertically", this, SLOT(flipv_cel()) );

  QMenuBar *menu = new QMenuBar(this);  
  Q_CHECK_PTR( menu );
  menu->insertItem( "File", file );
  menu->insertItem( "Edit", edit );
  menu->insertItem( "Loop", loop );
  menu->insertItem( "Cel", cel );
  menu->insertItem( "Animate", this, SLOT(animate_cb()) );
  menu->setSeparator( QMenuBar::InWindowsStyle );

  Q3BoxLayout *all =  new Q3HBoxLayout(this,10);
  all->setMenuBar(menu);

  Q3BoxLayout *left = new Q3VBoxLayout(all,1);

  
  QPixmap pright=QPixmap(right_x);
  QPixmap pleft=QPixmap(left_x);


  Q3Frame *frame1 = new Q3Frame(this);
  frame1->setFrameStyle(Q3Frame::Box | Q3Frame::Sunken);
  frame1->setLineWidth(1);
  frame1->setMinimumSize(200,180);
  frame1->setMargin(10);
  left->addWidget(frame1);

  
  int maxrow1 = 9,maxcol1 = 4;
  Q3GridLayout *grid1 = new Q3GridLayout( frame1, maxrow1,maxcol1, 1 );

  int i;

  for(i=0;i<maxcol1;i++){
    grid1->setColStretch(i,1);
    grid1->addColSpacing(i,1);
  }

  for(i=0;i<maxrow1;i++){
    grid1->setRowStretch(i,1);
    grid1->addRowSpacing(i,2);
  }
 

  int row=1;int col=0;

  QLabel *looplabel = new QLabel("Loop:",frame1);
  grid1->addWidget(looplabel,row,col,Qt::AlignRight); col++;


  loopnum = new QLabel("0/0",frame1);
  grid1->addWidget(loopnum,row,col,Qt::AlignLeft);   col++;


  QPushButton *loopleft = new QPushButton(frame1);
  //  loopleft->setFocusPolicy(ClickFocus);
  loopleft->setPixmap(pleft);
  connect( loopleft, SIGNAL(clicked()), SLOT(previous_loop()) );
  grid1->addWidget(loopleft,row,col,Qt::AlignRight);    col++;

  QPushButton *loopright = new QPushButton(frame1);
  //  loopright->setFocusPolicy(ClickFocus);
  loopright->setPixmap(pright);
  connect( loopright, SIGNAL(clicked()), SLOT(next_loop()) );
  grid1->addWidget(loopright,row,col,Qt::AlignLeft);    col++;

  row++;col=0;

  QLabel *cellabel = new QLabel("Cel:",frame1);
  grid1->addWidget(cellabel,row,col,Qt::AlignRight);  col++;

  celnum = new QLabel("0/0",frame1);
  grid1->addWidget(celnum,row,col,Qt::AlignLeft);    col++;

  QPushButton *celleft = new QPushButton(frame1);
  //  celleft->setFocusPolicy(ClickFocus);
  celleft->setPixmap(pleft);
  connect( celleft, SIGNAL(clicked()), SLOT(previous_cel()) );
  grid1->addWidget(celleft,row,col,Qt::AlignRight);     col++;

  QPushButton *celright = new QPushButton(frame1);
  celright->setPixmap(pright);
  //  celright->setFocusPolicy(ClickFocus);
  connect( celright, SIGNAL(clicked()), SLOT(next_cel()) );
  grid1->addWidget(celright,row,col,Qt::AlignLeft); col++;
  
  row++;col=0;

  QLabel *lwidth = new QLabel("Width:",frame1);
  grid1->addWidget(lwidth,row,col,Qt::AlignRight);     col++;

  width = new QLineEdit(frame1);
  width->setMinimumWidth(40);
  width->setMaximumWidth(60);
  connect( width, SIGNAL(returnPressed()), SLOT(change_width_height()) );
  grid1->addWidget(width,row,col,Qt::AlignLeft);      col++;

  QPushButton *widthleft = new QPushButton(frame1);
  //  widthleft->setFocusPolicy(ClickFocus);
  widthleft->setPixmap(pleft);
  //  widthleft->setFocusPolicy(ClickFocus);
  connect( widthleft, SIGNAL(clicked()), SLOT(dec_width()) );
  grid1->addWidget(widthleft,row,col,Qt::AlignRight);  col++;

  QPushButton *widthright = new QPushButton(frame1);
  //  widthright->setFocusPolicy(ClickFocus);
  widthright->setPixmap(pright);
  //  widthright->setFocusPolicy(ClickFocus);
  connect( widthright, SIGNAL(clicked()), SLOT(inc_width()) );
  grid1->addWidget(widthright,row,col,Qt::AlignLeft);   col++;

  row++;col=0;
  
  QLabel *lheight = new QLabel("Height:",frame1);
  grid1->addWidget(lheight,row,col,Qt::AlignRight);    col++;

  height = new QLineEdit(frame1);
  height->setMinimumWidth(40);
  height->setMaximumWidth(60);
  connect( height, SIGNAL(returnPressed()), SLOT(change_width_height()) );
  grid1->addWidget(height,row,col,Qt::AlignLeft);     col++;

  QPushButton *heightleft = new QPushButton(frame1);
  //  heightleft->setFocusPolicy(ClickFocus);
  heightleft->setPixmap(pleft);
  //  heightleft->setFocusPolicy(ClickFocus);
  connect( heightleft, SIGNAL(clicked()), SLOT(dec_height()) );
  grid1->addWidget(heightleft,row,col,Qt::AlignRight);  col++;

  QPushButton *heightright = new QPushButton(frame1);
  //  heightright->setFocusPolicy(ClickFocus);
  heightright->setPixmap(pright);
  //  heightright->setFocusPolicy(ClickFocus);
  connect( heightright, SIGNAL(clicked()), SLOT(inc_height()) );
  grid1->addWidget(heightright,row,col,Qt::AlignLeft);  col++;


  row++;col=0;
  is_descriptor = new QCheckBox("Description",frame1);
  //  is_descriptor->setFocusPolicy(ClickFocus);
  connect( is_descriptor, SIGNAL(clicked()), SLOT(is_descriptor_cb()) );

  grid1->addMultiCellWidget(is_descriptor,row,row,0,2,Qt::AlignCenter);   
  
  edit_descriptor = new QPushButton(frame1);
  //  edit_descriptor->setFocusPolicy(ClickFocus);
  edit_descriptor->setText("Edit");
  edit_descriptor->setMaximumHeight(20);
  edit_descriptor->setEnabled(false);
  connect( edit_descriptor, SIGNAL(clicked()), SLOT(show_description()) );
  grid1->addMultiCellWidget(edit_descriptor,row,row,maxcol1-2,maxcol1-1,Qt::AlignCenter);


  row++;col=0;


  QLabel *mirrorloop = new QLabel("This loop mirrors: ", frame1, 0);
  grid1->addMultiCellWidget(mirrorloop,row,row,0,maxcol1-1,Qt::AlignCenter);

  
  row++;col=0;


  mirror_loop = new QComboBox( FALSE, frame1,0 );
  mirror_loop->insertItem(" no other loop ");
  mirror_loop->setMinimumSize(100,20);
  connect( mirror_loop, SIGNAL(activated(int)), this, SLOT(change_mirror(int)) );
 
  grid1->addMultiCellWidget(mirror_loop,row,row,0,maxcol1-1,Qt::AlignCenter);  
  

  Q3Frame *frame2 = new Q3Frame(this);
  frame2->setFrameStyle(Q3Frame::Box | Q3Frame::Sunken);
  frame2->setLineWidth(1);
  frame2->setMinimumSize(180,100);
  frame2->setMargin(1);
  left->addWidget(frame2);


  Q3BoxLayout *h_frame2 = new Q3HBoxLayout(frame2, 10);


  Q3GridLayout *grid2 = new Q3GridLayout( h_frame2, 3, 2, 1);

  for(i=0;i<2;i++){
    grid2->setColStretch(i,1);
    grid2->addColSpacing(i,1);
  }
  for(i=0;i<3;i++){
    grid2->setRowStretch(i,1);
    grid2->addRowSpacing(i,2);
  }


  QPushButton *zoom_minus = new QPushButton(frame2);
  //  zoom_minus->setFocusPolicy(ClickFocus);
  zoom_minus->setPixmap(QPixmap(zoom_minus_x));
  connect( zoom_minus, SIGNAL(clicked()), SLOT(zoom_minus()) );
  grid2->addWidget(zoom_minus,0,0,Qt::AlignLeft);

  QPushButton *zoom_plus = new QPushButton(frame2);
  //  zoom_plus->setFocusPolicy(ClickFocus);
  zoom_plus->setPixmap(QPixmap(zoom_plus_x));
  connect( zoom_plus, SIGNAL(clicked()), SLOT(zoom_plus()) );
  grid2->addWidget(zoom_plus,0,1,Qt::AlignRight);
  

  view_draw = new QRadioButton("Draw",frame2);
  //  view_draw->setFocusPolicy(ClickFocus);
  view_draw->setChecked(true);
  drawing_mode=V_DRAW;
  grid2->addMultiCellWidget(view_draw,1,1,0,1,Qt::AlignLeft);  

  view_fill = new QRadioButton("Fill",frame2);
  //  view_fill->setFocusPolicy(ClickFocus);
  grid2->addMultiCellWidget(view_fill,2,2,0,1,Qt::AlignLeft);  


  Q3ButtonGroup *bg = new Q3ButtonGroup(frame2);
  bg->setExclusive(true);
  bg->hide();
  bg->insert(view_draw);
  bg->insert(view_fill,1);
  connect( bg, SIGNAL(clicked(int)), SLOT(change_mode(int)) );


  Q3GridLayout *grid3 = new Q3GridLayout( h_frame2, 3, 3, 0);

  for(i=0;i<3;i++){
    grid3->setColStretch(i,1);
    grid3->addColSpacing(i,0);
  }
  for(i=0;i<3;i++){
    grid3->setRowStretch(i,1);
    grid3->addRowSpacing(i,0);
  }


  QPushButton *view_up = new QPushButton(frame2);
  //  view_up->setFocusPolicy(ClickFocus);
  view_up->setPixmap(QPixmap(uparrow_x));
  connect( view_up, SIGNAL(clicked()), SLOT(shift_up()) );
  grid3->addWidget(view_up,0,1,Qt::AlignBottom|Qt::AlignHCenter);
  
  QPushButton *view_left = new QPushButton(frame2);
  //  view_left->setFocusPolicy(ClickFocus);
  view_left->setPixmap(QPixmap(leftarrow_x));
  connect( view_left, SIGNAL(clicked()), SLOT(shift_left()) );
  grid3->addWidget(view_left,1,0,Qt::AlignRight|Qt::AlignVCenter);
  
  QPushButton *view_right = new QPushButton(frame2);
  //  view_right->setFocusPolicy(ClickFocus);
  view_right->setPixmap(QPixmap(rightarrow_x));
  connect( view_right, SIGNAL(clicked()), SLOT(shift_right()) );
  grid3->addWidget(view_right,1,2,Qt::AlignLeft|Qt::AlignVCenter);
  
  QPushButton *view_down = new QPushButton(frame2);
  //  view_down->setFocusPolicy(ClickFocus);
  view_down->setPixmap(QPixmap(downarrow_x));
  connect( view_down, SIGNAL(clicked()), SLOT(shift_down()) );
  grid3->addWidget(view_down,2,1,Qt::AlignTop|Qt::AlignHCenter);
  


  Q3Frame *frame3 = new Q3Frame(this);
  frame3->setFrameStyle(Q3Frame::Box | Q3Frame::Sunken);
  frame3->setLineWidth(1);
  frame3->setMinimumSize(420,300);
  frame3->setMargin(4);
  all->addWidget(frame3,1);
  

  Q3BoxLayout *right = new Q3VBoxLayout(frame3,10);

  //  QScrollView *canvas = new QScrollView(frame3);
  canvas = new Canvas(frame3,0,this);
  canvas->setMinimumSize(400,200);
  right->addWidget(canvas,1);
  canvas->setFocusPolicy(Qt::ClickFocus);
  setFocusProxy(canvas);

  Q3Frame *frame4 = new Q3Frame(frame3);
  frame4->setFrameStyle(Q3Frame::Box | Q3Frame::Sunken);
  frame4->setLineWidth(1);
  frame4->setMinimumSize(400,80);
  frame4->setMargin(10);
  right->addWidget(frame4);
  

  int maxcol2 = 6;
  Q3GridLayout *grid4 = new Q3GridLayout( frame4, 2, maxcol2, 2);
  
  for(i=0;i<maxcol2;i++){
    grid4->setColStretch(i,1);
    grid4->addColSpacing(i,4);
  }
  for(i=0;i<2;i++){
    grid4->setRowStretch(i,1);
    grid4->addRowSpacing(i,2);
  }

 

  QLabel *trans_color = new QLabel("Transparency colour:",frame4);
  trans_color->setMaximumHeight(20);
  grid4->addWidget(trans_color,0,0,Qt::AlignLeft); 
 
  transcolor = new QWidget(frame4);
  transcolor->setPalette( QPalette( egacolor[0] ) );  
  transcolor->setMinimumSize(40,16);
  transcolor->setMaximumSize(100,30);
  grid4->addWidget(transcolor,0,1,Qt::AlignCenter);   

  QPushButton *set_trans_color = new QPushButton(frame4);
  //  set_trans_color->setFocusPolicy(ClickFocus);
  set_trans_color->setText("Set");
  set_trans_color->setMaximumHeight(20);
  connect( set_trans_color, SIGNAL(clicked()), SLOT(set_transcolor()) );
  grid4->addWidget(set_trans_color,0,2,Qt::AlignRight);

  QWidget *dummy = new QWidget(frame4);
  grid4->addMultiCellWidget(dummy,0,3,maxcol2-1,Qt::AlignCenter);
  
  palette = new Palette(frame4);
  palette->setMinimumSize(250,40);
  palette->setMaximumSize(350,80);
  //  palette->setPalette( QPalette( QColor(255, 255, 80) ) );
  grid4->addMultiCellWidget(palette,1,1,0,maxcol2-2,Qt::AlignLeft);

  description = NULL;

  changed=false;
  undo=false;
  undoCel=Cel();
  animate=NULL;
  canvas->setFocus();

  adjustSize();
  hide();
}

//*********************************************
void ViewEdit::save(char *filename)
{

  view->save(filename);
  changed = false;

}

//*********************************************
void ViewEdit::display()
{

  showlooppar();
  showcelpar();
  
  if(view->Description != ""){
    is_descriptor->setChecked(true);
    edit_descriptor->setEnabled(true);
  }
  else{
    is_descriptor->setChecked(false);
    edit_descriptor->setEnabled(false);
  } 
  if(description)description->hide();
  DisplayView();
  show();
}

//*********************************************
void ViewEdit::open(int ResNum)
{
  
  if(view->open(ResNum))return ;
  ViewNum = ResNum;
  sprintf(tmp,"View editor: view.%d",ViewNum);
  setCaption(tmp);
  changed=false;
  display();

}

//*********************************************
void ViewEdit::open(char *filename)
{

  if(view->open(filename))return;
  ViewNum = -1;
  sprintf(tmp,"View editor");
  setCaption(tmp);  
  changed=false;
  display();
}
//*********************************************
void ViewEdit::DisplayView()
{
  int w,h;
  w = canvas->x0+canvas->cur_w*canvas->pixsize*2+10;
  h = canvas->y0+canvas->cur_h*canvas->pixsize+10;

  int i=view->loops[view->CurLoop].mirror;
  if(i!=-1){
    canvas->DrawCel(view->loops[i].cels[view->CurCel].width,view->loops[i].cels[view->CurCel].height,view->loops[i].cels[view->CurCel].data,true);      
  }
  else{
    canvas->DrawCel(view->loops[view->CurLoop].cels[view->CurCel].width,view->loops[view->CurLoop].cels[view->CurCel].height,view->loops[view->CurLoop].cels[view->CurCel].data,false);
  }
  if(view->loops[view->CurLoop].cels[view->CurCel].transcol != transcol){
    set_transcolor(view->loops[view->CurLoop].cels[view->CurCel].transcol);
  }

}

//*********************************************
void ViewEdit::DisplayView(int pixsize)
{

  int w,h;
  w = canvas->x0+canvas->cur_w*pixsize*2+10;
  h = canvas->y0+canvas->cur_h*pixsize+10;

  if(canvas->contentsWidth() < w || canvas->contentsHeight() < h){
    canvas->resizeContents(w,h);
  }


  int i=view->loops[view->CurLoop].mirror;
  if(i!=-1){
    canvas->DrawCel(view->loops[i].cels[view->CurCel].width,view->loops[i].cels[view->CurCel].height,view->loops[i].cels[view->CurCel].data,true,pixsize);      
  }
  else{
    canvas->DrawCel(view->loops[view->CurLoop].cels[view->CurCel].width,view->loops[view->CurLoop].cels[view->CurCel].height,view->loops[view->CurLoop].cels[view->CurCel].data,false,pixsize);
  }
  if(view->loops[view->CurLoop].cels[view->CurCel].transcol != transcol){
    set_transcolor(view->loops[view->CurLoop].cels[view->CurCel].transcol);
  }

}

//*********************************************
void ViewEdit::showlooppar()
{
  
  sprintf(tmp,"%d/%d",view->CurLoop,view->NumLoops-1);
  loopnum->setText(tmp);
  showmirror();

}

//*********************************************
void ViewEdit::showmirror()
{

  int m=view->loops[view->CurLoop].mirror;
  int m1=view->loops[view->CurLoop].mirror1;

  mirror_loop->clear();
  mirror_loop->insertItem("no other loop");
  mirror_loop->setCurrentItem(0);
  int item=1;
  for(int i=0;i<view->NumLoops;i++){
    if(i==view->CurLoop)continue;
    if((view->loops[i].mirror==-1 && view->loops[i].mirror1==-1)||i==m||i==m1){

      sprintf(tmp,"Loop %d",i);
      mirror_loop->insertItem(tmp);
      if(m==i)
        mirror_loop->setCurrentItem(item);
      else if(m==-1&&m1==i)
        mirror_loop->setCurrentItem(item);
      item++;

    }
  }

}

//*********************************************
void ViewEdit::showcelpar()
{

  sprintf(tmp,"%d/%d",view->CurCel,view->loops[view->CurLoop].NumCels-1);
  celnum->setText(tmp);
  sprintf(tmp,"%d",view->loops[view->CurLoop].cels[view->CurCel].width);
  width->setText(tmp);
  sprintf(tmp,"%d",view->loops[view->CurLoop].cels[view->CurCel].height);
  height->setText(tmp);


}

//*********************************************
void ViewEdit::deinit()
{
  if(description){
    description->close(true);
    description=NULL;
  }
  if(animate)animate->closeall();
  delete view;
  winlist[winnum].type=-1;
  if(window_list && window_list->isVisible())window_list->draw();
}


//*********************************************
void ViewEdit::hideEvent( QHideEvent * )
{
  
  if(description){
    description->close(true);
    description=NULL;
  }
  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void ViewEdit::showEvent( QShowEvent * )
{

  if(window_list && window_list->isVisible())window_list->draw();

}

//***********************************************
void ViewEdit::closeEvent( QCloseEvent *e )
{
  
  if(changed){
    if(ViewNum != -1){
      sprintf(tmp,"Save changes to view.%d ?",ViewNum);
    }
    else{
      sprintf(tmp,"Save changes to view ?");      
    }
    strcat(tmp,"\n(view will be saved to game)");
      
    switch ( QMessageBox::warning( this, "View editor",
                                   tmp,
                                   "Yes",
                                   "No",
                                   "Cancel",
                                   0, 2) ) {
    case 0: // yes
      save_to_game();
      deinit();
      e->accept();

      //     else
      //e->ignore();
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
void ViewEdit::open_file()
{
  
  Q3FileDialog *f = new Q3FileDialog(0,"Open",true);  
  const char *filters[] = {"view*.*","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Open view");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      open((char *)f->selectedFile().latin1());
  }

}

//*********************************************
void ViewEdit::open()
{
  
  setCaption("View editor");
  view->newView();
  ViewNum = -1;
  showlooppar();
  showcelpar();
  DisplayView();
  changed=false;
  show();
}

//*********************************************
void ViewEdit::save_file()
{


  Q3FileDialog *f = new Q3FileDialog(0,"Save",true);  
  const char *filters[] = {"view*.*","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Save view");
  f->setMode(Q3FileDialog::AnyFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      save((char *)f->selectedFile().latin1());
  }

}

//*********************************************
void ViewEdit::save_to_game()
{
  if(ViewNum != -1){
    view->save(ViewNum);
    if(resources_win){
      if(resources_win->preview==NULL)resources_win->preview=new Preview();
      resources_win->preview->open(ViewNum,VIEW);
    }
    changed=false;
  }
  else
    save_to_game_as();

}

//*********************************************
void ViewEdit::save_to_game_as()
{

  AskNumber *view_number = new AskNumber(0,0,"View number","Enter view number: [0-255]");    

  if(!view_number->exec())return;

  QString str = view_number->num->text();
  int num = atoi((char *)str.latin1());
  
  if(num<0||num>255){
    menu->errmes("View number must be between 0 and 255 !");
    return ;
  }
  if(game->ResourceInfo[VIEW][num].Exists){
    sprintf(tmp,"Resource view.%d already exists. Replace it ?",num);
    
    switch( QMessageBox::warning( this, "View", tmp,
                                  "Replace", "Cancel",
                                  0,      // Enter == button 0
                                  1 ) ) { // Escape == button 1
    case 0: 
      view->save(num);
      changed=false;
      ViewNum = num;
      if(resources_win){
        if(resources_win->preview==NULL)resources_win->preview=new Preview();
        resources_win->preview->open(ViewNum,VIEW);
      }
      break;
    case 1:       
      break;
    }
  }
  else{
    view->save(num);
    changed=false;
    ViewNum = num;
    if(resources_win){
      resources_win->select_resource_type(VIEW);
      resources_win->set_current(num);
    }
    open(num);
  }

}

//*********************************************
void ViewEdit::delete_view()
{
  int k;

  if(ViewNum==-1)return;

  sprintf(tmp,"Really delete view %d ?",ViewNum);
  switch( QMessageBox::warning( this, "View", tmp,
                                "Delete", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0: 
    game->DeleteResource(VIEW,ViewNum);
    if(resources_win){
      k = resources_win->list->currentItem();
      resources_win->select_resource_type(VIEW);
      resources_win->list->setCurrentItem(k);
    }
    break;
  case 1:       
    break;
  } 

}

//*********************************************
void ViewEdit::flipv_cel()
{
  
  view->loops[curIndex()].cels[view->CurCel].mirrorv();
  DisplayView();
  changed=true;
}

//*********************************************
void ViewEdit::fliph_cel()
{
  
  view->loops[curIndex()].cels[view->CurCel].mirrorh();
  DisplayView();
  changed=true;
}

//*********************************************
void ViewEdit::copy_cel()
{
  saveCel.deinit();
  saveCel.copy(view->loops[curIndex()].cels[view->CurCel]);
  cel_copied=true;
}

//*********************************************
void ViewEdit::paste_cel()
{
  if(!cel_copied)return;
  saveundo();
  view->loops[curIndex()].cels[view->CurCel].copy(saveCel);
  showcelpar();
  DisplayView();
  changed=true;
}

//*********************************************
void ViewEdit::next_loop()
{

  if(view->CurLoop<view->NumLoops-1){
    view->CurLoop++;
    view->CurCel=0;
    showlooppar();
    showcelpar();
    DisplayView();
  }


}

//*********************************************
void ViewEdit::previous_loop()
{

   if(view->CurLoop>0){
    view->CurLoop--;
    view->CurCel=0;
    showlooppar();
    showcelpar();
    DisplayView();
  }


}

//*********************************************
void ViewEdit::first_loop()
{

  view->CurLoop=0;
  showlooppar();
  showcelpar();
  DisplayView();

}

//*********************************************
void ViewEdit::last_loop()
{

  view->CurLoop=view->NumLoops-1;
  showlooppar();
  showcelpar();
  DisplayView();


}

//*********************************************
void ViewEdit::insert_loop_before()
{

  if(view->NumLoops<MaxLoops-1){
    view->insertLoop_before();
    showlooppar();
    showcelpar();
    DisplayView();  
    changed=true;
  }
  else{
    menu->errmes("Max of 16 loops already reached.");
  }

}

//*********************************************
void ViewEdit::insert_loop_after()
{

  if(view->NumLoops<MaxLoops-1){
    view->insertLoop_after();
    showlooppar();
    showcelpar();
    DisplayView();  
    changed=true;
  }
  else{
    menu->errmes("Max of 16 loops already reached.");
  }

}

//*********************************************
void ViewEdit::append_loop()
{
  if(view->NumLoops<MaxLoops-1){
    view->appendLoop();
    showlooppar();
    showcelpar();
    DisplayView();  
    changed=true;
  }
  else{
    menu->errmes("Max of 16 loops already reached.");
  }

}

//*********************************************

void ViewEdit::delete_loop()
{
  if(view->NumLoops>1){
    view->deleteLoop();
    if(view->CurLoop>view->NumLoops-1){
      view->CurLoop--;
    }
    showlooppar();
    showcelpar();
    DisplayView();  
    changed=true;
  }
}

//*********************************************
void ViewEdit::clear_loop()
{

  view->loops[view->CurLoop].clear();
  if(view->loops[view->CurLoop].mirror != -1){
    view->loops[view->loops[view->CurLoop].mirror].clear();
  }  
  showlooppar();
  showcelpar();
  DisplayView();  
  changed=true;

}

//*********************************************
void ViewEdit::change_mirror(int i)
{ 

  if(i==0){

    printf("unset mirror %d\n",view->CurLoop);

    if (view->loops[view->CurLoop].mirror !=-1)
      view->unsetMirror(view->CurLoop);
    else if (view->loops[view->CurLoop].mirror1 !=-1)
      view->unsetMirror(view->loops[view->CurLoop].mirror1);
  }
  else{
    QString str = mirror_loop->currentText();
    int k = atoi((char *)str.latin1()+5);
    printf("set %d to mirror %d\n",view->CurLoop,k);
    if(view->loops[view->CurLoop].mirror != k){


      for(int j=0;j<view->NumLoops;j++){
        if(view->loops[j].mirror == view->CurLoop){
          view->unsetMirror(j);
        }
        if(view->loops[j].mirror1 == view->CurLoop){
          view->loops[j].mirror1 = -1;
        }
      }


      if(view->loops[k].mirror != -1)
        view->unsetMirror(k);
      view->setMirror(view->CurLoop,k);

    }
  }  

  showlooppar();
  DisplayView();
  changed=true;
}


//**********************************************************
void ViewEdit::next_cel()
{

  if(view->CurCel<view->loops[view->CurLoop].NumCels-1){
    view->CurCel++;
    showcelpar();
    DisplayView();
  }

}

//*********************************************
void ViewEdit::next_cel_cycle()
{

  if(view->loops[view->CurLoop].NumCels<=1)return;
  if(view->CurCel<view->loops[view->CurLoop].NumCels-1){
    view->CurCel++;
  }
  else{
    view->CurCel=0;    
  }
  showcelpar();
  DisplayView();

}

//*********************************************
void ViewEdit::previous_cel()
{
  if(view->CurCel>0){
    view->CurCel--;
    showcelpar();
    DisplayView();
  }

}

//*********************************************
void ViewEdit::prev_cel_cycle()
{
  if(view->loops[view->CurLoop].NumCels<=1)return;
  if(view->CurCel>0){
    view->CurCel--;
  }
  else{
    view->CurCel=view->loops[view->CurLoop].NumCels-1;
  }
  showcelpar();
  DisplayView();

}

//*********************************************
void ViewEdit::first_cel()
{

  view->CurCel=0;
  showcelpar();
  DisplayView();


}

//*********************************************
void ViewEdit::last_cel()
{

  view->CurCel=view->loops[view->CurLoop].NumCels-1;
  showcelpar();
  DisplayView();

}

//*********************************************
void ViewEdit::insert_cel_before()
{

  if(view->loops[view->CurLoop].NumCels < MaxCels-1){
    view->loops[view->CurLoop].insertCel_before(view->CurCel);
    if(view->loops[view->CurLoop].mirror != -1){
      view->loops[view->loops[view->CurLoop].mirror].insertCel_before(view->CurCel);
    }
    showcelpar();
    DisplayView();    
    changed=true;
  }
  else{
    menu->errmes("Max of 32 cels already reached in this loop.");
  }
}

//*********************************************
void ViewEdit::insert_cel_after()
{

  if(view->loops[view->CurLoop].NumCels < MaxCels-1){
    view->loops[view->CurLoop].insertCel_after(view->CurCel);
    if(view->loops[view->CurLoop].mirror != -1){
      view->loops[view->loops[view->CurLoop].mirror].insertCel_after(view->CurCel);
    }
    showcelpar();
    DisplayView();    
    changed=true;
  }
  else{
    menu->errmes("Max of 32 cels already reached in this loop.");
  }
}

//*********************************************
void ViewEdit::append_cel()
{

  if(view->loops[view->CurLoop].NumCels < MaxCels-1){
    view->loops[view->CurLoop].appendCel();
    if(view->loops[view->CurLoop].mirror != -1){
      view->loops[view->loops[view->CurLoop].mirror].appendCel();
    }
    showcelpar();
    DisplayView();    
    changed=true;
  }
  else{
    menu->errmes("Max of 32 cels already reached in this loop.");
  }

}

//*********************************************
void ViewEdit::delete_cel()
{

  if(view->loops[view->CurLoop].NumCels > 1){
    view->loops[view->CurLoop].deleteCel(view->CurCel);
    if(view->loops[view->CurLoop].mirror != -1){
      view->loops[view->loops[view->CurLoop].mirror].deleteCel(view->CurCel);
    }
    if(view->CurCel>=view->loops[view->CurLoop].NumCels)
      view->CurCel--;
    showcelpar();
    DisplayView(); 
    changed=true;
  }
}

//*********************************************
void ViewEdit::dec_width()
{
  int w;

  if((w=view->loops[view->CurLoop].cels[view->CurCel].width)>1){
    w--;
    view->loops[view->CurLoop].cels[view->CurCel].setW(w);
    sprintf(tmp,"%d",w);
    width->setText(tmp);
    DisplayView();
    changed=true;
  }
}

//*********************************************
void ViewEdit::inc_width()
{

    int w=view->loops[view->CurLoop].cels[view->CurCel].width+1;
    if(w<160){
      view->loops[view->CurLoop].cels[view->CurCel].setW(w);
      sprintf(tmp,"%d",w);
      width->setText(tmp);
      DisplayView();
      changed=true;
    }
    else{
      menu->errmes("Maximum width is 160.");
    }
}

//*********************************************
void ViewEdit::dec_height()
{

  int h;
  if((h=view->loops[view->CurLoop].cels[view->CurCel].height)>1){
    h--;
    view->loops[view->CurLoop].cels[view->CurCel].setH(h);
    sprintf(tmp,"%d",h);
    height->setText(tmp);
    DisplayView();
    changed=true;
  }
}

//*********************************************
void ViewEdit::inc_height()
{

    int h=view->loops[view->CurLoop].cels[view->CurCel].height+1;

    if(h<168){
      view->loops[view->CurLoop].cels[view->CurCel].setH(h);
      sprintf(tmp,"%d",h);
      height->setText(tmp);
      DisplayView();
      changed=true;
    }
    else{
      menu->errmes("Maximum height is 168.");
    }
}

//*********************************************
void ViewEdit::change_width_height()
{

  QString str = width->text();
  int w = atoi((char *)str.latin1());
  view->loops[view->CurLoop].cels[view->CurCel].setW(w);

  str = height->text();
  int h = atoi((char *)str.latin1());
  view->loops[view->CurLoop].cels[view->CurCel].setH(h);

  DisplayView();
  changed=true;
  width->clearFocus();
  height->clearFocus();
  setFocus();
}


//*********************************************
void ViewEdit::shift_right()
{
  
  if(view->loops[view->CurLoop].mirror==-1){
    view->loops[view->CurLoop].cels[view->CurCel].right();
  }
  else{
    view->loops[curIndex()].cels[view->CurCel].left();
  }
  DisplayView();
  changed=true;
}

//*********************************************
void ViewEdit::shift_left(){

  if(view->loops[view->CurLoop].mirror==-1){
      view->loops[view->CurLoop].cels[view->CurCel].left();
  }
  else{
    view->loops[curIndex()].cels[view->CurCel].right();
  }
  DisplayView();
  changed=true;
}

//*********************************************
void ViewEdit::shift_up()
{
  view->loops[curIndex()].cels[view->CurCel].up();
  DisplayView();
  changed=true;
}

//*********************************************
void ViewEdit::shift_down()
{
  view->loops[curIndex()].cels[view->CurCel].down();
  DisplayView();
  changed=true;
}

//*********************************************
void ViewEdit::fillCel(int x,int y,byte color)
{

  saveundo();  
  view->loops[curIndex()].cels[view->CurCel].fill(x,y,color);
  DisplayView();    
  changed=true;
}

//*********************************************
void ViewEdit::clear_cel()
{
  saveundo();    
  view->loops[curIndex()].cels[view->CurCel].clear();
  DisplayView();    
  changed=true;
}  

//*********************************************
void ViewEdit::saveundo()
{
  undoCel.deinit();
  undoCel.copy(view->loops[curIndex()].cels[view->CurCel]);
  undo=true;

}

//*********************************************
void ViewEdit::undo_cel()
{
  if(undo){
    view->loops[curIndex()].cels[view->CurCel].copy(undoCel);
    undo=false;
  }
  DisplayView();
}

/*************************************************/
int ViewEdit::curIndex()
{
  int i=view->loops[view->CurLoop].mirror;
  if(i==-1)return view->CurLoop;
  else return i;
}

/*************************************************/
void ViewEdit::show_description()
{
  if(!description)description = new Description(0,0,this);
  description->set();
  description->show();
}

/*******************************************************/
void ViewEdit::change_mode(int m)
{

  drawing_mode=m;


}
/*******************************************************/
void ViewEdit::change_mode1(int m)
{

  drawing_mode=m;
  if(m==V_DRAW)
    view_draw->setChecked(true);
  else
    view_fill->setChecked(true);

}
/*******************************************************/
void ViewEdit::is_descriptor_cb()
{

  if(is_descriptor->isChecked()){
    edit_descriptor->setEnabled(true);
  }
  else{
    edit_descriptor->setEnabled(false);
  }

}
/*******************************************************/
void ViewEdit::set_transcolor()
{

  transcolor->setPalette( QPalette( egacolor[palette->left] ) );  
  transcol=palette->left;
  view->loops[view->CurLoop].cels[view->CurCel].transcol = transcol;
}

/*******************************************************/
void ViewEdit::set_transcolor(int col)
{

  transcolor->setPalette( QPalette( egacolor[col] ) );  
  transcol=col;
  view->loops[view->CurLoop].cels[view->CurCel].transcol = transcol;
}

/*******************************************************/
void ViewEdit::zoom_minus()
  //zoom_out
{

  if(canvas->pixsize>1){
    DisplayView(canvas->pixsize-1);
  }

}
/*******************************************************/
void ViewEdit::zoom_plus()
  //zoom_in
{

  if(canvas->pixsize<10){
    DisplayView(canvas->pixsize+1);
  }

}

/*******************************************************/
bool ViewEdit::focusNextPrevChild ( bool ) 
{

  if(width->hasFocus()){
    height->setFocus();
  }
  else if(height->hasFocus()){
    width->setFocus();
  }
  else{
    canvas->setFocus();
  }
  return true;

}
/*******************************************************/
void ViewEdit::animate_cb()
{

  if(animate==NULL)animate=new Animate(0,0,0,this);
  animate->show();

}
/*******************************************************/
Animate::Animate( QWidget *parent, const char *name, Preview *p, ViewEdit *v)
    : QWidget( parent, name )
{

  viewedit = v;
  preview = p;
  setCaption("Animate");
  Q3BoxLayout *b = new Q3VBoxLayout(this,10);

  Q3HBoxLayout *b1 = new Q3HBoxLayout(b,4);
  QLabel *l = new QLabel("Delay (ms)",this);
  b1->addWidget(l);
  delay = new QLineEdit(this);
  delay->setText("200");
  delay->setMaximumWidth(100);
  b1->addWidget(delay);

  Q3ButtonGroup *fb = new Q3ButtonGroup(2,Qt::Horizontal,"",this);
  fb->setExclusive(true);
  forward = new QRadioButton("Forward",fb);
  forward->setChecked(true);
  backward = new QRadioButton("Backward",fb);
  connect(fb,SIGNAL(clicked(int)),SLOT(fb_cb()));
  b->addWidget(fb);

  Q3HBoxLayout *b2 = new Q3HBoxLayout(b,4);
  button = new QPushButton(this);
  button->setText("Start");
  b2->addWidget(button);
  connect(button,SIGNAL(clicked()),SLOT(start_stop()));
  QPushButton *close = new QPushButton(this);
  close->setText("Close");
  b2->addWidget(close);
  connect(close,SIGNAL(clicked()),SLOT(hide()));
  
  timer = new QTimer(this);
  connect(timer,SIGNAL(timeout()), SLOT(next_cel()) );

}

/*******************************************************/
void Animate::start_stop()
{

  if(timer->isActive()){
    timer->stop();
    button->setText("Start");
  }
  else{
    QString str = delay->text();
    num = atoi((char *)str.latin1());
    button->setText("Stop");
    fwd = forward->isChecked();
    timer->start(num);    
  }

}
/*******************************************************/
void Animate::fb_cb()
{

  fwd = forward->isChecked();

}
/*******************************************************/
void Animate::next_cel()
{

  if(viewedit){
    if(fwd)viewedit->next_cel_cycle();
    else viewedit->prev_cel_cycle();
  }
  else{
    if(fwd)preview->next_cel_cycle();
    else preview->prev_cel_cycle();
  }
}
/*******************************************************/
void Animate::closeall()
{
  
   if(timer->isActive())timer->stop();
   close(true);
}

/*******************************************************/
Description::Description( QWidget *parent, const char *name , ViewEdit *v)
    : QWidget( parent, name ,Qt::WDestructiveClose)
{

  setCaption("View description");
  viewedit = v;

  Q3BoxLayout *d1 = new Q3VBoxLayout(this,10);
  d1->addSpacing(10);
  
  smallview = new ViewIcon(this,0,viewedit);
  smallview->setMinimumSize(64,64);

  d1->addWidget(smallview,0,Qt::AlignCenter);

  desc = new Q3MultiLineEdit(this);
  desc->setMinimumSize(300,100);
  d1->addWidget(desc,1);  

  Q3BoxLayout *d2 = new Q3HBoxLayout(d1,10);  
  d2->addSpacing(10);

  QPushButton *ok = new QPushButton(this);
  ok->setText("OK");
  ok->setMaximumWidth(80);
  connect( ok, SIGNAL(clicked()), SLOT(ok_cb()) );
  d2->addWidget(ok);    

  QPushButton *cancel = new QPushButton(this);
  cancel->setText("Cancel");
  cancel->setMaximumWidth(80);
  connect( cancel, SIGNAL(clicked()), SLOT(cancel_cb()) );
  d2->addWidget(cancel);    

  adjustSize();
  hide();
  getmaxcol();

}

//*********************************************
void Description::getmaxcol()
 //get maximum number of columns on screen (approx.) 
 //to wrap the long lines
{

  QFontMetrics f = fontMetrics();
  maxcol = desc->width()/f.width('a');

}

//*********************************************
void Description::resizeEvent( QResizeEvent * )
{
  getmaxcol();
  set();
}

//*********************************************
void Description::set()
{
  int n;

  desc->clear();

  if(viewedit->view->Description == ""){    
    return;
  }
  
  string ThisLine = "";
  string ThisMessage = viewedit->view->Description;

  do{
    if(ThisMessage.length() + ThisLine.length() > maxcol){
      n = maxcol - ThisLine.length();
      do{ n--; }while(!(n == 0 || ThisMessage[n]==' '));
      if (n <= 0)n = maxcol-ThisLine.length();
      ThisLine += ThisMessage.substr(0,n);      
      ThisMessage = (n < (int)ThisMessage.length())?ThisMessage.substr(n+1):"";
      desc->insertLine(ThisLine.c_str(),-1);
      ThisLine = "";
    }
    else{
      ThisLine += ThisMessage;
      ThisMessage = "";
    }    
  }while(ThisMessage != "");     

  if(ThisLine != ""){
    desc->insertLine(ThisLine.c_str(),-1);
  }
  
}

//*********************************************
void Description::ok_cb()
{
  int i;

  QString str = desc->text();
  char *s = (char *)str.latin1();
  tmp[0]=0;
  for(i=0;*s;s++){
    if(*s!='\n')
      tmp[i++]=*s;
    else if(i>1 && tmp[i-1]!=' '){
      tmp[i++]=' ';
    }
  }
  tmp[i]=0;

  if(strcmp(viewedit->view->Description.c_str(),tmp)){
    viewedit->view->Description = string(tmp);
    viewedit->changed=true;
  }
  hide();

}
//*********************************************

void Description::cancel_cb()
{
  hide();
}
//**********************************************
Canvas::Canvas ( QWidget *parent, const char *name, ViewEdit *v)
    : Q3ScrollView( parent, name )
{

  viewedit = v;
  x0=10;y0=10;
  pixsize=2;
  cur_mirror = false;
  pixmap = QPixmap();
  cur_w=cur_h=0;

}

//*********************************************
void Canvas::setSize(int w,int h)
{
  if(cur_w != w || cur_h != h){
    pixmap.resize(w*pixsize*2,h*pixsize);
    cur_w=w;
    cur_h=h;
  }
}

//*********************************************
void Canvas::viewportMousePressEvent(QMouseEvent* event)
{
  int x, y;
  viewportToContents( event->x(),  event->y(), x, y );

  if (event->button() & Qt::LeftButton){
    CurColor = viewedit->palette->left;
  }  
  else if (event->button() & Qt::RightButton){
    CurColor = viewedit->palette->right;        
  }
  UpdateCel(x-x0,y-y0);
  viewedit->changed=true;
}

//*********************************************
void Canvas::viewportMouseMoveEvent(QMouseEvent* event)
{
  int x, y;

  viewportToContents( event->x(),  event->y(), x, y );  
  UpdateCel(x-x0,y-y0);
}

//*********************************************
void Canvas::drawContents(QPainter* p, int , int , int, int )
{
 
  if(cur_w==0 ||cur_h==0)return;
  p->drawPixmap( x0, y0, pixmap );  

}      

//*********************************************
void Canvas::DrawCel(int w,int h,byte *celdata,bool mirror, int size)
{
  int x,y,ww,hh,w0,h0,ww0,hh0;

  w0=cur_w;
  h0=cur_h;
  ww0=(x0+w0)*2*pixsize;
  hh0=(y0+h0)*pixsize;
  pixsize=size;
  pixmap.resize(cur_w*pixsize*2,cur_h*pixsize);
  ww=(x0+w)*2*pixsize;
  hh=(y0+h)*pixsize;

  QPainter p(&pixmap); 

  data=celdata;

  if(mirror){      
    for(y=0;y<h;y++){
      for(x=0;x<w*2;x+=2){		    
        p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[data[y*w*2+w*2-2-x]]);
      }
    }     
  }
  else{
    for(y=0;y<h;y++){
      for(x=0;x<w*2;x+=2){		
        p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[data[y*w*2+x]]);
      }
    }
  }
  repaintContents(x0,y0,MAX(ww,ww0),MAX(hh,hh0),true);

}

//*********************************************
void Canvas::DrawCel(int w,int h,byte *celdata,bool mirror)
{
  int x,y,ww,hh,w0,h0,ww0=0,hh0=0;
  bool changed;


  if(cur_w!=w || cur_h!=h){
    changed=true;
    w0=cur_w;
    h0=cur_h;
    ww0=(x0+w0)*2*pixsize;
    hh0=(y0+h0)*pixsize;
    setSize(w,h);
  }
  else{
    changed = false;
  }

  ww=(x0+w)*2*pixsize;
  hh=(y0+h)*pixsize;

  QPainter p(&pixmap); 

  cur_mirror = mirror;
  data = celdata;

  if(mirror){      
    for(y=0;y<h;y++){
      for(x=0;x<w*2;x+=2){		
        p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[data[y*w*2+w*2-2-x]]);
      }
    }     
  }
  else{
    for(y=0;y<h;y++){
      for(x=0;x<w*2;x+=2){		
        p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[data[y*w*2+x]]);
      }
    }
  }

  if(changed)
    repaintContents(x0,y0,MAX(ww,ww0),MAX(hh,hh0),true);
  else
    repaintContents(x0,y0,ww,hh,false);
}

//*********************************************
void Canvas::UpdateCel(int x,int y){

  int xn=x/pixsize/2;
  int yn=y/pixsize;
      
  if(xn>=0&&xn<cur_w && yn>=0&&yn<cur_h){

    QPainter p(&pixmap); 
    if(viewedit->drawing_mode == V_DRAW){
          
      x=xn*2*pixsize;
      y=yn*pixsize;
          
      p.fillRect(x,y,pixsize*2,pixsize,egacolor[CurColor]);
      repaintContents(x0+x,y0+y,pixsize*2,pixsize,false);
      if(cur_mirror){
        data[yn*cur_w*2+cur_w*2-2-xn*2]=CurColor;
        data[yn*cur_w*2+cur_w*2-2-xn*2+1]=CurColor;
      }
      else{
        data[yn*cur_w*2+xn*2]=CurColor;
        data[yn*cur_w*2+xn*2+1]=CurColor;
      }
    }
    else{ //FILL      
      if(cur_mirror)
        viewedit->fillCel(cur_w-1-xn,yn,CurColor);
      else
        viewedit->fillCel(xn,yn,CurColor);      
    }
    
  }
    
}

//*********************************************
void Canvas::keyPressEvent( QKeyEvent *k )
{

  //  printf("key ! %d\n",k->key());  
  switch(k->key()){
  case Qt::Key_Q:
    viewedit->previous_loop();
    break;
  case Qt::Key_W:
    viewedit->next_loop();
    break;
  case Qt::Key_A:
    viewedit->previous_cel();
    break;
  case Qt::Key_S:
    viewedit->next_cel();
    break;
  case Qt::Key_Z:
    viewedit->zoom_minus();
    break;
  case Qt::Key_X:
    viewedit->zoom_plus();
    break;
  case Qt::Key_T:
    viewedit->set_transcolor();
    break;
  case Qt::Key_D:
    viewedit->change_mode1(V_DRAW);
    break;
  case Qt::Key_F:
    viewedit->change_mode1(V_FILL);
    break;
  case Qt::Key_I:
    viewedit->shift_up();
    break;
  case Qt::Key_K:
    viewedit->shift_down();
    break;
  case Qt::Key_J:
    viewedit->shift_left();
    break;
  case Qt::Key_L:
    viewedit->shift_right();
    break;
  default:
    k->ignore();
    break;
  }

}

//*********************************************
bool Canvas::focusNextPrevChild ( bool ) 
{
  
  setFocus();
  return true;

}
//********************************************
ViewIcon::ViewIcon ( QWidget *parent, const char *name , ViewEdit *v)
    : QWidget( parent, name )
{

  viewedit=v;

}

//*********************************************
void ViewIcon::paintEvent(QPaintEvent *)
{

  int x,y;

  QPainter p(this);

  int w = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].width;
  int h = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].height;
  bool mirror = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].mirror;
  byte *data = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].data;

  int pixsize=2;

  int W = viewedit->description->width();
  setGeometry((W-pixsize*w*2)/2,10,pixsize*w*2,pixsize*h);

  //  if(pixsize*w*2>width() || pixsize*h>height()){
  //    resize(pixsize*w*2,pixsize*h);
  
  
  if(mirror){      
    for(y=0;y<h;y++){
      for(x=0;x<w*2;x+=2){		    
        p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[data[y*w*2+w*2-2-x]]);
      }
    }     
  }
  else{
    for(y=0;y<h;y++){
      for(x=0;x<w*2;x+=2){		
        p.fillRect(x*pixsize,y*pixsize,pixsize*2,pixsize,egacolor[data[y*w*2+x]]);
      }
    }
  }
  

}      



