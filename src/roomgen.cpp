/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  The idea and most of the design of RoomGen module are copied from the
 *  "AGI Base Logic Generator" utility by Joel McCormick.
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

#include "logedit.h"
#include "game.h"
#include "menu.h"
#include "roomgen.h"

#include <string>
#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include <qapplication.h>
#include <q3filedialog.h>
#include <q3grid.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3BoxLayout>
#include <Q3GridLayout>
#include <Q3Frame>
#include <QLabel>
#include <Q3VBoxLayout>

static const char *dirs[4] = {"left","right","bottom","horizon"};

//***********************************************
RoomGen::RoomGen( QWidget *parent, const char *name)
    : QDialog( parent, name, true )
{
  int i;
  setCaption("AGI Base Logic Generator");

  Q3BoxLayout *all = new Q3VBoxLayout(this,5);

  Q3HBoxLayout *upper = new Q3HBoxLayout(all,10);

  Q3Grid *gtxt = new Q3Grid( 2, this);
  gtxt->setSpacing(4);
  upper->addWidget(gtxt);

  QLabel *llog = new QLabel("Logic Number:",gtxt);
  llog=llog; //avoid compilation warning
  lnum = new QLineEdit(gtxt);
  lnum->setFixedWidth(40);
  lnum->setText("0");
  //lnum->selectAll();
  connect(lnum,SIGNAL(textChanged(const QString &)),SLOT(lnum_cb()));

  QLabel *lpic = new QLabel("Picture Number:",gtxt);
  lpic=lpic;
  pnum = new QLineEdit(gtxt);
  pnum->setFixedWidth(40);
  pnum->setText("0");
  //pnum->selectAll();

  QLabel *lhor = new QLabel("Horizon at Y:",gtxt);
  lhor=lhor;
  hnum = new QLineEdit(gtxt);
  hnum->setFixedWidth(40);
  hnum->setText("36");
  //hnum->selectAll();

  Q3VBoxLayout *check = new Q3VBoxLayout(upper,2);
  draw_ego = new QCheckBox("Draw Ego initially",this);
  draw_ego->setChecked(true);
  check->addWidget(draw_ego);
  first_room = new QCheckBox("First room",this);
  connect(first_room,SIGNAL(clicked()),SLOT(first_room_cb()));
  check->addWidget(first_room);
  inc_def = new QCheckBox("Include defines.txt",this);
  check->addWidget(inc_def);
  inc_def->setChecked(true);
  gen_comm = new QCheckBox("Generate comments",this);
  gen_comm->setChecked(true);
  check->addWidget(gen_comm);

  Q3VBoxLayout *but = new Q3VBoxLayout(upper,2);
  QPushButton *entry = new QPushButton("Entry and looking",this);
  connect(entry,SIGNAL(clicked()),SLOT(entry_cb()));
  but->addWidget(entry);
  QPushButton *first = new QPushButton("First room controls",this);
  connect(first,SIGNAL(clicked()),SLOT(first_cb()));
  but->addWidget(first);

  Q3BoxLayout *ledge = new Q3HBoxLayout(all,1);

  //  QGroupBox *ego_pos_b = new QGroupBox(1,Horizontal,"Ego Positioning (-1 = ignore)",this);
  Q3Frame *ego_pos_b = new Q3Frame(this);
  ego_pos_b->setFrameStyle( Q3Frame::Box | Q3Frame::Sunken );
  ego_pos_b->setLineWidth(1);
  ego_pos_b->setMargin(2);

  Q3GridLayout *ego_pos = new Q3GridLayout(ego_pos_b,6,6,10,4);

  QLabel *ll = new QLabel("Ego Positioning (-1 = ignore)",ego_pos_b);
  ego_pos->addMultiCellWidget(ll,0,0,0,5,AlignCenter);


  int row=1,col;

  QLabel *l_from = new QLabel("Coming from",ego_pos_b);
  ego_pos->addMultiCellWidget(l_from,row,row,0,1,AlignLeft);

  QLabel *l_pos = new QLabel("Position Ego at",ego_pos_b);
  ego_pos->addMultiCellWidget(l_pos,row,row,2,5,AlignCenter);

  QLabel *l_lroom[4],*l_x[4],*l_y[4];
  row++;

  for(i=0;i<4;i++,row++){
    col=0;
    l_lroom[i] = new QLabel("room:",ego_pos_b);
    ego_pos->addWidget(l_lroom[i],row,col,AlignRight); col++;
    from[i] = new QLineEdit(ego_pos_b);
    from[i]->setFixedWidth(40);
    from[i]->setText("-1");
    //    from[i]->selectAll();
    ego_pos->addWidget(from[i],row,col,AlignLeft); col++;


    l_x[i] = new QLabel("X:",ego_pos_b);
    ego_pos->addWidget(l_x[i],row,col,AlignRight); col++;
    x[i] = new QLineEdit(ego_pos_b);
    x[i]->setText("-1");
    //    x[i]->selectAll();
    x[i]->setFixedWidth(40);
    ego_pos->addWidget(x[i],row,col,AlignLeft); col++;

    l_y[i] = new QLabel("Y:",ego_pos_b);
    ego_pos->addWidget(l_y[i],row,col,AlignRight); col++;
    y[i] = new QLineEdit(ego_pos_b);
    y[i]->setText("-1");
    //    y[i]->selectAll();
    y[i]->setFixedWidth(40);
    ego_pos->addWidget(y[i],row,col,AlignLeft); col++;
  }


  QPushButton *ego_adv = new QPushButton("Advanced",ego_pos_b);
  connect(ego_adv,SIGNAL(clicked()),SLOT(ego_advanced_cb()));
  ego_pos->addMultiCellWidget(ego_adv,row,row,0,5,AlignCenter);

  ledge->addWidget(ego_pos_b,1);


  Q3Frame *edge_control_b = new Q3Frame(this);
  edge_control_b->setFrameStyle( Q3Frame::Box | Q3Frame::Sunken );
  edge_control_b->setLineWidth(1);
  edge_control_b->setMargin(4);

  Q3GridLayout *edge_control = new Q3GridLayout(edge_control_b,6,2,10,1);

  QLabel *ll1 = new QLabel("Edge Controls (-1 = ignore)",edge_control_b);
  edge_control->addMultiCellWidget(ll1,0,0,0,1,AlignCenter);

  row=1;
  col=0;

  QLabel *l_if = new QLabel("If Ego touches:",edge_control_b);
  edge_control->addWidget(l_if,row,col,AlignLeft); col++;
  QLabel *l_goto = new QLabel("Goto Room:",edge_control_b);
  edge_control->addWidget(l_goto,row,col,AlignCenter); col++;
  row++;

  QLabel *l_e[4];
  const char *dirs[4] = {"Left Edge:","Right Edge:","Bottom Edge:","Horizon Edge:"};

  for(i=0;i<4;i++,row++){
    col=0;
    l_e[i] = new QLabel(dirs[i],edge_control_b);
    edge_control->addWidget(l_e[i],row,col,AlignLeft); col++;
    edge[i] = new QLineEdit(edge_control_b);
    edge[i]->setFixedWidth(40);
    edge[i]->setText("-1");
    //    edge[i]->selectAll();
    edge_control->addWidget(edge[i],row,col,AlignCenter); col++;
  }

  col=0;
  QPushButton *edge_adv = new QPushButton("Advanced",edge_control_b);
  connect(edge_adv,SIGNAL(clicked()),SLOT(edge_advanced_cb()));
  edge_control->addMultiCellWidget(edge_adv,row,row,0,1,AlignCenter);

  ledge->addWidget(edge_control_b,0);

  Q3HBoxLayout *ltitle = new Q3HBoxLayout(all,4);

  QLabel *lcom = new QLabel("Logic Title (for comments):",this);
  ltitle->addWidget(lcom);
  title = new QLineEdit(this);
  ltitle->addWidget(title);

  Q3HBoxLayout *last = new Q3HBoxLayout(all,20);

  QPushButton *ok = new QPushButton("OK",this);
  ok->setMaximumSize(80,40);
  connect(ok,SIGNAL(clicked()),SLOT(ok_cb()));
  last->addWidget(ok,AlignRight);

  QPushButton *cancel = new QPushButton("Cancel",this);
  cancel->setMaximumSize(80,40);
  connect(cancel,SIGNAL(clicked()),SLOT(reject()));
  last->addWidget(cancel,AlignLeft);

  adjustSize();

  room_entry = NULL;
  room_first = NULL;
  ego_advanced = NULL;
  edge_advanced = NULL;
  xa=ya=-1;
  for(i=0;i<4;i++){
    empty_e[i]=false;
    display_e[i]=false;
    e_mes[i]="You can't go that way.";
  }
  entry_mes="";
  look_mes="";
  status=input=true;
  x1=y1=-1;
}
//************************************************
bool RoomGen::bad_int(QLineEdit *w,int *res,int nmin,int nmax,bool ignore,const char *text)
{

  *res=-1;
  QString str=w->text();

  if(!str.isEmpty())
    *res=atoi((char *)str.latin1());
  if((*res==-1) && ignore)return false;
  if((*res<nmin)||(*res>nmax)){
    menu->errmes("%s must be between %d and %d !",text,nmin,nmax);
    return true;
  }
  return false;

}
//************************************************
bool RoomGen::bad_int(int res,int nmin,int nmax,bool ignore,const char *text)
{

  if(res==-1 && ignore)return false;
  if(res<nmin||res>nmax){
    menu->errmes("%s must be between %d and %d !",text,nmin,nmax);
    return true;
  }
  return false;

}

//************************************************
bool RoomGen::bad_input()
{

  QString str;
  int i;

  if(bad_int(lnum,&ln,0,255,false,"Logic number"))return true;
  if(bad_int(pnum,&pn,0,255,false,"Picture number"))return true;
  if(bad_int(hnum,&hn,0,167,false,"Horizon value"))return true;


  for(i=0;i<4;i++){
    rn[i]=xn[i]=yn[i]=-1;
    str=from[i]->text();
    if(!str.isEmpty())rn[i]=atoi((char *)str.latin1());
    str=x[i]->text();
    if(!str.isEmpty())xn[i]=atoi((char *)str.latin1());
    str=y[i]->text();
    if(!str.isEmpty())yn[i]=atoi((char *)str.latin1());
    if(rn[i]==-1||xn[i]==-1||yn[i]==-1){
      if(!(rn[i]==-1&&xn[i]==-1&&yn[i]==-1)){
        menu->errmes("Ego positioning: incomplete input !");
        return true;
      }
    }
    else{
      if(bad_int(rn[i],0,255,false,"Ego positioning:\nRoom number"))return true;
      if(bad_int(xn[i],0,319,false,"Ego positioning:\nX"))return true;
      if(bad_int(yn[i],0,167,false,"Ego positioning:\nY"))return true;
    }
  }

  for(i=0;i<4;i++){
    if(bad_int(edge[i],&en[i],0,255,true,"Edge controls:\nroom number"))return true;
  }

  if(!(xa==-1&&ya==-1)){
    if(bad_int(xa,0,319,false,"Unconditional Ego positioning:\nX"))return true;
    if(bad_int(ya,0,167,false,"Unconditional Ego positioning:\nY"))return true;
  }

  if(!(x1==-1&&y1==-1)){
    if(bad_int(x1,0,319,false,"First room Ego positioning:\nX"))return true;
    if(bad_int(y1,0,167,false,"First room Ego positioning:\nY"))return true;
  }

  return false;

}
//************************************************
void RoomGen::ok_cb()
{
  int i,k;
  int level=0;

  if(bad_input())return;

  bool com=gen_comm->isChecked();

  text="// ****************************************************************\n\
//\n";
  QString str=title->text();
  if(str.isEmpty()){
    sprintf(tmp,"// Logic %d\n",ln);
  }
  else{
    sprintf(tmp,"// Logic %d: %s\n",ln,str.latin1());
  }
  text+=tmp;
  text+="// \n// ****************************************************************\n";

  if(inc_def->isChecked()){
    text+="#include \"defines.txt\"\n";
  }

  text+="if (new_room) {\n";

  if(ln==pn){
    text+="  load.pic(room_no);\n\
  draw.pic(room_no);\n\
  discard.pic(room_no);\n";
  }
  else{
    sprintf(tmp,"  v255=%d;\n",pn);
    text+=tmp;
    text+="  load.pic(v255);\n\
  draw.pic(v255);\n\
  discard.pic(v255);\n";
  }
  if(com)text+="  //ADD ADDITIONAL INITIALIZATION CODE HERE\n";


  if(hn!=36){
    sprintf(tmp,"  set.horizon(%d);\n",hn);
    text+=tmp;
  }

  if(first_room->isChecked()){
    text+="  if ((prev_room_no == 0 || prev_room_no == 1)){\n";
    if(com)text+="    //THIS IS THE FIRST ROOM, ADD ANY ONE-TIME INITIALIZATION HERE\n";
    if(x1!=-1){
      sprintf(tmp,"    position(ego, %d, %d);\n",x1,y1);
      text+=tmp;
    }
    if(status)text+="    status.line.on();\n";
    if(input)text+="    accept.input();\n";
    text+="  }\n";
    level++;
  }

  for(i=0;i<4;i++){
    if(rn[i]==-1)continue;
    if(level){
      for(k=0;k<level;k++){
        text+="  ";
      }
      text+="else {\n";
    }
    for(k=0;k<level;k++){
      text+="  ";
    }
    sprintf(tmp,"  if (prev_room_no == %d){\n",rn[i]);
    text+=tmp;
    for(k=0;k<level;k++){
      text+="  ";
    }
    sprintf(tmp,"    position(ego, %d, %d);\n",xn[i],yn[i]);
    text+=tmp;
    for(k=0;k<level;k++){
      text+="  ";
    }
    text+="  }\n";
    level++;
  }
  if(xa!=-1){
    if(level){
      for(k=0;k<level;k++){
        text+="  ";
      }
      text+="else {\n";
    }
    for(k=0;k<level;k++){
      text+="  ";
    }
    sprintf(tmp,"  position(ego, %d, %d);\n",xa,ya);
    text+=tmp;
    if(level){
      for(k=0;k<level;k++){
        text+="  ";
      }
      text+="}\n";
    }
  }

  for(i=level-1;i>0;i--){
    for(k=0;k<i;k++){
      text+="  ";
    }
    text+="}\n";
  }

  if(draw_ego->isChecked()){
    text+="  draw(ego);\n";
  }
  text+="  show.pic();\n";

  if(entry_mes.length()>0){
    sprintf(tmp,"  print(\"%s\");\n",entry_mes.c_str());
    text+=tmp;
  }
  text+="}\n";

  for(i=0;i<4;i++){
    if(en[i]==-1){
      if(display_e[i]){
        sprintf(tmp,"if (ego_edge_code == %s_edge){\n",dirs[i]);
        text+=tmp;
        sprintf(tmp,"  print(\"%s\");\n",e_mes[i].c_str());
        text+=tmp;
        text+="  ego_dir = 0;\n}\n";
      }
      else if(empty_e[i]){
        sprintf(tmp,"if (ego_edge_code == %s_edge){\n",dirs[i]);
        text+=tmp;
        if(com){
          sprintf(tmp,"  //ADD EGO-TOUCHING-%s CODE HERE\n",dirs[i]);
          text+=tmp;
        }
        sprintf(tmp,"}\n");
        text+=tmp;
      }
    }
    else{
      sprintf(tmp,"if (ego_edge_code == %s_edge){\n",dirs[i]);
      text+=tmp;
      if(com){
        sprintf(tmp,"  //ADD ADDITIONAL %s EXIT CODE HERE\n",dirs[i]);
        text+=tmp;
      }
      sprintf(tmp,"  new.room(%d);\n}\n",en[i]);
      text+=tmp;
    }
  }

  text+="if (input_recieved && unknown_word_no == 0 && !input_parsed) {\n";
  if(look_mes.length()>0){
    text += "  if (said(\"look\")){\n";
    sprintf(tmp,"    print(\"%s\");\n  }\n",look_mes.c_str());
    text+=tmp;
  }
  text +="}\n\nreturn();\n";


  done(Accepted);

}
//************************************************
void RoomGen::ego_advanced_cb()
{

  if(ego_advanced==NULL)ego_advanced = new RoomGenPos();
  sprintf(tmp,"%d",xa);
  ego_advanced->x->setText(tmp);
  sprintf(tmp,"%d",ya);
  ego_advanced->y->setText(tmp);
  if(ego_advanced->exec()){
    xa=atoi((char *)ego_advanced->x->text().latin1());
    ya=atoi((char *)ego_advanced->y->text().latin1());
  }

}
//************************************************

void RoomGen::edge_advanced_cb()
{
  int i;

  if(edge_advanced==NULL)edge_advanced = new RoomGenEdge();
  for(i=0;i<4;i++){
    edge_advanced->c_edge[i]->setChecked(empty_e[i]);
    edge_advanced->m_edge[i]->setChecked(display_e[i]);
    edge_advanced->e_mes[i]=e_mes[i];
  }
  if(edge_advanced->exec()){
    for(i=0;i<4;i++){
      empty_e[i]=edge_advanced->c_edge[i]->isChecked();
      display_e[i]=edge_advanced->m_edge[i]->isChecked();
      if(display_e[i]){
        e_mes[i]=edge_advanced->e_mes[i];
      }
    }
  }

}

//************************************************

void RoomGen::entry_cb()
{

  if(room_entry==NULL)room_entry = new RoomGenEntry();
  room_entry->entry_text->setText(entry_mes.c_str());
  room_entry->look_text->setText(look_mes.c_str());
  if(room_entry->exec()){
    entry_mes=room_entry->entry_text->text().latin1();
    look_mes=room_entry->look_text->text().latin1();
  }


}
//************************************************

void RoomGen::first_room_cb()
{

  if(first_room->isChecked()){
    first_cb();
  }

}
//************************************************


void RoomGen::first_cb()
{

  if(room_first==NULL)room_first = new RoomGenFirst();
  room_first->status->setChecked(status);
  room_first->input->setChecked(input);
  sprintf(tmp,"%d",x1);
  room_first->x->setText(tmp);
  sprintf(tmp,"%d",y1);
  room_first->y->setText(tmp);
  if(room_first->exec()){
    status=room_first->status->isChecked();
    input=room_first->input->isChecked();
    x1=atoi((char *)room_first->x->text().latin1());
    y1=atoi((char *)room_first->y->text().latin1());
  }


}
//************************************************
void RoomGen::lnum_cb()
{

  pnum->setText(lnum->text());

}
//************************************************
RoomGenEntry::RoomGenEntry( QWidget *parent, const char *name)
    : QDialog( parent, name, true )
{

  setCaption("Room Entry and Looking");

  Q3BoxLayout *all = new Q3VBoxLayout(this,5);

  QLabel *entry = new QLabel("On room entry:",this);
  all->addWidget(entry);


  Q3BoxLayout *l1 = new Q3HBoxLayout(all,1);
  QLabel *print1 = new QLabel("print(\"",this);
  l1->addWidget(print1);
  entry_text = new QLineEdit(this);
  entry_text->setMinimumWidth(300);
  l1->addWidget(entry_text);
  QLabel *print11 = new QLabel("\");",this);
  l1->addWidget(print11);


  QLabel *ifsaid = new QLabel("if said(\"look\")){",this);
  all->addWidget(ifsaid);

  QWidget *place = new QWidget(this);

  Q3BoxLayout *l2 = new Q3HBoxLayout(place,1);
  QLabel *print2 = new QLabel("print(\"",place);
  l2->addWidget(print2);
  look_text = new QLineEdit(place);
  look_text->setMinimumWidth(300);
  l2->addWidget(look_text);
  QLabel *print22 = new QLabel("\");",place);
  l2->addWidget(print22);

  all->addWidget(place);

  QLabel *p = new QLabel("}",this);
  all->addWidget(p);


  Q3BoxLayout *last = new Q3HBoxLayout(all,10);

  QPushButton *ok = new QPushButton("OK",this);
  ok->setMaximumSize(80,40);
  connect(ok,SIGNAL(clicked()),SLOT(accept()));
  last->addWidget(ok,AlignRight);

  QPushButton *cancel = new QPushButton("Cancel",this);
  cancel->setMaximumSize(80,40);
  connect(cancel,SIGNAL(clicked()),SLOT(reject()));
  last->addWidget(cancel,AlignLeft);

  adjustSize();

}


//************************************************
RoomGenFirst::RoomGenFirst( QWidget *parent, const char *name)
    : QDialog( parent, name, true )
{

  setCaption("Controls for First Room");

  Q3BoxLayout *all = new Q3VBoxLayout(this,5);

  QLabel *egopos = new QLabel("Ego positioning (-1 = ignore)",this);
  all->addWidget(egopos);


  Q3GridLayout *l1 = new Q3GridLayout(all,6,1,5);
  QLabel *lx = new QLabel("X:",this);
  l1->addWidget(lx,0,0,AlignRight);
  x = new QLineEdit(this);
  x->setFixedWidth(40);
  x->setText("-1");
  x->selectAll();
  l1->addWidget(x,0,1,AlignLeft);

  QLabel *ly = new QLabel("Y:",this);
  l1->addWidget(ly,0,2,AlignRight);
  y = new QLineEdit(this);
  y->setFixedWidth(40);
  y->setText("-1");
  y->selectAll();
  l1->addWidget(y,0,3,AlignLeft);

  QLabel *place = new QLabel(" ",this);
  l1->addWidget(place,0,4,AlignCenter);
  l1->setColStretch(4,1);

  status = new QCheckBox("Turn on the status bar",this);
  status->setChecked(true);
  all->addWidget(status);

  input = new QCheckBox("Accept player input",this);
  input->setChecked(true);
  all->addWidget(input);

  Q3BoxLayout *last = new Q3HBoxLayout(all,10);

  QPushButton *ok = new QPushButton("OK",this);
  ok->setMaximumSize(80,40);
  connect(ok,SIGNAL(clicked()),SLOT(accept()));
  last->addWidget(ok,AlignRight);

  QPushButton *cancel = new QPushButton("Cancel",this);
  cancel->setMaximumSize(80,40);
  connect(cancel,SIGNAL(clicked()),SLOT(reject()));
  last->addWidget(cancel,AlignLeft);

  adjustSize();


}

//************************************************
RoomGenPos::RoomGenPos( QWidget *parent, const char *name)
    : QDialog( parent, name, true )
{

  setCaption("Controls for First Room");

  Q3BoxLayout *all = new Q3VBoxLayout(this,5);

  QLabel *l = new QLabel("Absolute (Unconditional) Position:",this);
  all->addWidget(l);

  Q3GridLayout *l1 = new Q3GridLayout(all,6,1,5);
  QLabel *lx = new QLabel("X:",this);
  l1->addWidget(lx,0,0,AlignRight);
  x = new QLineEdit(this);
  x->setFixedWidth(40);
  x->setText("-1");
  x->selectAll();
  l1->addWidget(x,0,1,AlignLeft);

  QLabel *ly = new QLabel("Y:",this);
  l1->addWidget(ly,0,2,AlignRight);
  y = new QLineEdit(this);
  y->setFixedWidth(40);
  y->setText("-1");
  y->selectAll();
  l1->addWidget(y,0,3,AlignLeft);

  QLabel *place = new QLabel(" ",this);
  l1->addWidget(place,0,4,AlignCenter);
  l1->setColStretch(4,1);


  QLabel *com = new QLabel(
"Unconditional positioning is useful for positioning ego\n\
in the same place no matter what room it comes from.\n\
It can be used in conjunction with conditional positioning\n\
and the tp command in debug mode to position ego in a place\n\
where you won't get stuck behind control lines, etc."
,this);
  all->addWidget(com);

  Q3BoxLayout *last = new Q3HBoxLayout(all,10);

  QPushButton *ok = new QPushButton("OK",this);
  ok->setMaximumSize(80,40);
  connect(ok,SIGNAL(clicked()),SLOT(accept()));
  last->addWidget(ok,AlignRight);

  QPushButton *cancel = new QPushButton("Cancel",this);
  cancel->setMaximumSize(80,40);
  connect(cancel,SIGNAL(clicked()),SLOT(reject()));
  last->addWidget(cancel,AlignLeft);

  adjustSize();


}

//************************************************
RoomGenEdge::RoomGenEdge( QWidget *parent, const char *name)
    : QDialog( parent, name, true )
{
  int i;

  setCaption("Edge Code Advanced Controls");

  Q3BoxLayout *all = new Q3VBoxLayout(this,5);

  QLabel *com = new QLabel(
"It may be desirable to have an edge code that does not lead to different room.\n\
For example, you may wish to have the game print a message instead, as:\n\
\n\
if (ego_edge_code == horizon_edge){\n\
    print(\"You cannot go that way.\");\n\
    ego_dir = 0;\n\
}"
,this);
  all->addWidget(com);

  Q3GroupBox *edge = new Q3GroupBox(2,Horizontal,"Empty edge controls",this);

  for(i=0;i<4;i++){
    sprintf(tmp,"Include empty code for %s edge",dirs[i]);
    c_edge[i]=new QCheckBox(tmp,edge);
  }

  all->addWidget(edge);

  Q3GroupBox *messages = new Q3GroupBox(4,Horizontal,
"Messages (if Display is not checked, message will be ignored)",this);
  for(i=0;i<4;i++){
    m_edge[i] = new QCheckBox("Display",messages);
    b_edge[i] = new QPushButton(messages);
    sprintf(tmp,"message for %s edge",dirs[i]);
    b_edge[i]->setText(tmp);
    switch(i){
    case 0:
      connect(b_edge[i],SIGNAL(clicked()),SLOT(left_message())); break;
    case 1:
      connect(b_edge[i],SIGNAL(clicked()),SLOT(right_message())); break;
    case 2:
      connect(b_edge[i],SIGNAL(clicked()),SLOT(bot_message())); break;
    case 3:
      connect(b_edge[i],SIGNAL(clicked()),SLOT(hor_message())); break;
    }
  }

  all->addWidget(messages);

  Q3BoxLayout *last = new Q3HBoxLayout(all,10);

  QPushButton *ok = new QPushButton("OK",this);
  ok->setMaximumSize(80,40);
  connect(ok,SIGNAL(clicked()),SLOT(accept()));
  last->addWidget(ok,AlignRight);

  QPushButton *cancel = new QPushButton("Cancel",this);
  cancel->setMaximumSize(80,40);
  connect(cancel,SIGNAL(clicked()),SLOT(reject()));
  last->addWidget(cancel,AlignLeft);

  adjustSize();

  message=NULL;

}

void RoomGenEdge::left_message()
{

  if(message==NULL)message = new RoomGenMessage();
  message->name("Left",e_mes[0].c_str());
  if(message->exec()){
    e_mes[0] = (char *)message->message->text().latin1();
  }
}

void RoomGenEdge::right_message()
{

  if(message==NULL)message = new RoomGenMessage();
  message->name("Right",e_mes[1].c_str());
  if(message->exec()){
    e_mes[1] = (char *)message->message->text().latin1();
  }
}

void RoomGenEdge::bot_message()
{

  if(message==NULL)message = new RoomGenMessage();
  message->name("Bottom",e_mes[2].c_str());
  if(message->exec()){
    e_mes[2] = (char *)message->message->text().latin1();
  }
}

void RoomGenEdge::hor_message()
{

  if(message==NULL)message = new RoomGenMessage();
  message->name("Horizon",e_mes[3].c_str());
  if(message->exec()){
    e_mes[3] = (char *)message->message->text().latin1();;
  }
}

//************************************************
RoomGenMessage::RoomGenMessage( QWidget *parent, const char *name)
    : QDialog( parent, name, true )
{


  Q3BoxLayout *all = new Q3VBoxLayout(this,5);

  l = new QLabel("",this);
  //l->setAutoResize(true); // TODO: REPLACE WITH A LAYOUT!
  all->addWidget(l);
  message = new QLineEdit(this);
  all->addWidget(message);

  Q3BoxLayout *last = new Q3HBoxLayout(all,10);

  QPushButton *ok = new QPushButton("OK",this);
  ok->setMaximumSize(80,40);
  connect(ok,SIGNAL(clicked()),SLOT(accept()));
  last->addWidget(ok,AlignRight);

  QPushButton *cancel = new QPushButton("Cancel",this);
  cancel->setMaximumSize(80,40);
  connect(cancel,SIGNAL(clicked()),SLOT(reject()));
  last->addWidget(cancel,AlignLeft);

  adjustSize();

}

void RoomGenMessage::name(const char *title,const char *text)
{

  sprintf(tmp,"%s Message",title);
  setCaption(tmp);
  sprintf(tmp,"Enter a message to display when ego\ntouches %c%s edge:",tolower(title[0]),title+1);
  l->setText(tmp);
  message->setText(text);
  message->selectAll();

}
