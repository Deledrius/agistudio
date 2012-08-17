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
#include "words.h"
#include "menu.h"
#include "wordsedit.h"
#include "resources.h"

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
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QCloseEvent>
#include <Q3BoxLayout>
#include <QShowEvent>
#include <QLabel>
#include <Q3PopupMenu>
#include <QHideEvent>
#include <Q3VBoxLayout>


WordsEdit::WordsEdit( QWidget *parent, const char *name, int win_num, ResourcesWin *res )
    : QWidget( parent, name ,Qt::WDestructiveClose )
{
  
  setCaption("WORDS.TOK Editor");
  wordlist = new WordList();

  winnum = win_num;
  resources_win = res;
  wordsfind = NULL;
  
  Q3PopupMenu *file = new Q3PopupMenu( this );
  Q_CHECK_PTR( file );

  file->insertItem( "New", this, SLOT(new_file()) );
  file->insertItem( "Open", this, SLOT(open_file()) );
  file->insertItem( "Merge", this, SLOT(merge_file()) );
  file->insertItem( "Save", this, SLOT(save_file()) );
  file->insertItem( "Save As", this, SLOT(save_as_file()) );
  file->insertSeparator();
  file->insertItem( "Close", this, SLOT(close()) );

  Q3PopupMenu *words = new Q3PopupMenu( this );
  Q_CHECK_PTR( words );

  words->insertItem( "Add word group", this, SLOT(add_group_cb()) );
  words->insertItem( "Delete word group", this, SLOT(delete_group_cb()) );
  words->insertItem( "Change group number", this, SLOT(change_group_number_cb()) );
  words->insertSeparator();
  words->insertItem( "Add word", this, SLOT(add_word_cb()) );
  words->insertItem( "Delete word", this, SLOT(delete_word_cb()) );
  words->insertSeparator();
  words->insertItem( "Count word groups", this, SLOT(count_groups_cb()) );
  words->insertItem( "Count words", this, SLOT(count_words_cb()) );
  words->insertItem( "&Find...", this, SLOT(find_cb()) , Qt::CTRL+Qt::Key_F);


  QMenuBar *menu = new QMenuBar(this);  
  Q_CHECK_PTR( menu );
  menu->insertItem( "File", file );
  menu->insertItem( "Words", words );
  menu->setSeparator( QMenuBar::InWindowsStyle );

  Q3BoxLayout *all =  new Q3VBoxLayout(this,10);

  QSplitter *split = new QSplitter(Qt::Horizontal,this);


  QWidget *left = new QWidget(split);
  Q3BoxLayout *lgroup = new Q3VBoxLayout(left,4);   
  QLabel *labelgroup = new QLabel("Word groups",left,"Word groups");
  labelgroup->setAlignment(Qt::AlignCenter);  
  lgroup->addWidget(labelgroup);
  
  listgroup = new Q3ListBox(left);
  listgroup->setColumnMode (1);
  listgroup->setMinimumSize(200,300);
  connect( listgroup, SIGNAL(highlighted(int)), SLOT(select_group(int)) );
  connect( listgroup, SIGNAL(selected(int)), SLOT(select_group(int)) );

  lgroup->addWidget(listgroup);


  QWidget *right = new QWidget(split);
  Q3BoxLayout *lwords =  new Q3VBoxLayout(right,4);
  labelword = new QLabel("Word group",right,"Word group");
  labelword->setAlignment(Qt::AlignCenter);
  lwords->addWidget(labelword);

  listwords = new Q3ListBox(right);
  listwords->setColumnMode (1);
  listwords->setMinimumSize(200,300);
  connect( listwords, SIGNAL(highlighted(int)), SLOT(select_word(int)) );
  connect( listwords, SIGNAL(selected(int)), SLOT(select_word(int)) );
  lwords->addWidget(listwords);
  
  lineword = new QLineEdit(right);
  lineword->setEnabled(false);
  connect( lineword, SIGNAL(returnPressed()), SLOT(do_add_word()) );
  lwords->addWidget(lineword);  
  

  all->addWidget(split);  

  Q3BoxLayout *buttons =  new Q3HBoxLayout(all,20);

  add_group = new QPushButton("Add group",this);
  connect( add_group, SIGNAL(clicked()), SLOT(add_group_cb()) );
  buttons->addWidget(add_group);

  delete_group = new QPushButton("Delete group",this);
  connect( delete_group, SIGNAL(clicked()), SLOT(delete_group_cb()) );
  buttons->addWidget(delete_group);

  add_word = new QPushButton("Add word",this);
  connect( add_word, SIGNAL(clicked()), SLOT(add_word_cb()) );
  buttons->addWidget(add_word);

  delete_word = new QPushButton("Delete word",this);
  connect( delete_word, SIGNAL(clicked()), SLOT(delete_word_cb()) );
  buttons->addWidget(delete_word);

  Q3BoxLayout *buttons1 =  new Q3HBoxLayout(all,20);

  change_group_number = new QPushButton("&Change group number",this);
  connect( change_group_number, SIGNAL(clicked()), SLOT(change_group_number_cb()) );
  buttons1->addWidget(change_group_number);

  find = new QPushButton("Find",this);
  connect( find, SIGNAL(clicked()), SLOT(find_cb()) );
  buttons1->addWidget(find);
  

  adjustSize();

  changed=false;
  filename="";
  SelectedGroup=0;
  FindLastGroup = FindLastWord = -1;
}
//********************************************************
void WordsEdit::deinit()
{
  delete wordlist;
  winlist[winnum].type=-1;
  if(window_list && window_list->isVisible())window_list->draw();
}


//*********************************************
void WordsEdit::hideEvent( QHideEvent * )
{
  
  if(wordsfind){
    wordsfind->close(true);
    wordsfind=NULL;
  }
  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void WordsEdit::showEvent( QShowEvent * )
{

  if(window_list && window_list->isVisible())window_list->draw();

}

//********************************************************
void WordsEdit::closeEvent( QCloseEvent *e )
{
  
  if(changed){
      
    switch ( QMessageBox::warning( this, "WORDS.TOK edit",
                                   "Save changes to WORDS.TOK ?",
                                   "Yes",
                                   "No",
                                   "Cancel",
                                   0, 2) ) {
    case 0: // yes
      save_file() ;
        deinit();
        e->accept();

        //      else
        // e->ignore();
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
//********************************************************
void WordsEdit::update_all()
{

  listgroup->clear();
  listwords->clear();
  if (wordlist->NumGroups > 0){
    for (int i = 0;i< wordlist->NumGroups;i++){
      sprintf(tmp,"%d. ",wordlist->WordGroup[i].GroupNum);
      for(int k=0;k<wordlist->WordGroup[i].Words.num;k++){
        if(k>0)strcat(tmp," | ");
        strcat(tmp,wordlist->WordGroup[i].Words.at(k).c_str());
      }
      QString str = tmp;
      listgroup->insertItem( str );
    }
  }
  show();
  changed=false;
}

//********************************************************
void WordsEdit::open(char *filenam)
{
  int ret = wordlist->read(filenam);
  if(ret)return ;
  filename = filenam;
  update_all();
  return;
}

//********************************************************
void WordsEdit::open()
{
  
  sprintf(tmp,"%s/words.tok",game->dir.c_str());
  open(tmp);

}

//********************************************************
void WordsEdit::add_group_cb(void)
{

  AskNumber *addgroup = new AskNumber(0,0,"Add group","Enter group number:");
  if(!addgroup->exec())return;

  int i;
  QString str = addgroup->num->text();
  int num = atoi((char *)str.latin1());

  if(num < 0 || num > 65535){
    menu->errmes("Wordsedit","You must enter an integer from 0 to 65535.");
    return;    
  }


  if((i=wordlist->add_group(num))==-1)return;

  str.sprintf("%d. ",num);
  listgroup->insertItem(str,i);
  listgroup->setCurrentItem(i);
  changed=true;  
}

//********************************************************
void WordsEdit::delete_group_cb(void)
{

  int rm = listgroup->currentItem();
  if(rm != -1){
    wordlist->delete_group(rm);
    listgroup->removeItem(rm);
    if(wordlist->NumGroups>0){
      listgroup->setCurrentItem(rm);
      select_group(rm);
    }
    else listwords->clear();
    changed=true;
  }
}

//********************************************************
void WordsEdit::print_group(int curgroup)
{
  sprintf(tmp,"%d. ",wordlist->WordGroup[curgroup].GroupNum);
  for(int i=0;i<wordlist->WordGroup[curgroup].Words.num;i++){
    if(i>0)strcat(tmp," | ");
    strcat(tmp,wordlist->WordGroup[curgroup].Words.at(i).c_str());
  }
}

//********************************************************
void WordsEdit::update_group(int curgroup)
{
  
  print_group(curgroup);
  listgroup->changeItem(tmp,curgroup);

}

//********************************************************
void WordsEdit::delete_word_cb(void)
{

  QString str = lineword->text();
  char *word = (char *)str.latin1();
  int k=wordlist->delete_word(word,SelectedGroup);
  if(k!=-1){
    lineword->setText("");
    listwords->removeItem(k);
    update_group(SelectedGroup);
    changed=true;
    return;
  }
  
}

//********************************************************
void WordsEdit::change_group_number_cb(void)
{


  AskNumber *newnumber = new AskNumber(0,0,"Change group number","Enter group number:");
  if(!newnumber->exec())return;
  int i;
  QString str = newnumber->num->text();
  int num = atoi((char *)str.latin1());

  if(num < 0 || num > 65535){
    menu->errmes("Wordsedit","You must enter an integer from 0 to 65535.");
    return;    
  }
  int currentgroup = listgroup->currentItem();
  if((i=wordlist->change_number(currentgroup,num))==-1)return ;

  listgroup->removeItem(currentgroup);
  listgroup->insertItem("",i);
  update_group(i);
  changed=true;
}

//********************************************************
void WordsEdit::find_cb(void)
{

  if(wordsfind == NULL){
    wordsfind = new WordsFind(0,0,this);
  }
  wordsfind->show();
  wordsfind->find_field->setFocus();
}

//********************************************************
int WordsEdit::find_down(char *word)
{ 

  for (int i = FindLastGroup;i< wordlist->NumGroups;i++){
    for(int k = FindLastWord;k<wordlist->WordGroup[i].Words.num;k++){
      if(!strcmp(wordlist->WordGroup[i].Words.at(k).c_str(),word)){
        FindLastWord = k;
        FindLastGroup = i;
        return 1;
      }
    }
    FindLastWord=0;
  }
  FindLastGroup = wordlist->NumGroups-1;
  FindLastWord = wordlist->WordGroup[FindLastGroup].Words.num-1;
  return 0;

}

//********************************************************
int WordsEdit::find_up(char *word)
{  

  for (int i = FindLastGroup;i>= 0;i--){
    for(int k = FindLastWord;k>=0;k--){
      if(!strcmp(wordlist->WordGroup[i].Words.at(k).c_str(),word)){
        FindLastWord = k;
        FindLastGroup = i;
        return 1;
      }
    }
    if(i>0)FindLastWord=wordlist->WordGroup[i-1].Words.num-1;
  }
  FindLastWord=0;
  FindLastGroup=0;
  return 0;

}

//********************************************************
void WordsEdit::select_group(int num)
{
  QString str;
  str.sprintf("Word group %d",wordlist->WordGroup[num].GroupNum);
  labelword->setText(str);

  SelectedGroup = num;

  listwords->clear();
  for(int k=0;k<wordlist->WordGroup[num].Words.num;k++){
	string str2 = wordlist->WordGroup[num].Words.at(k);
    const char *str1 = str2.c_str();
    listwords->insertItem(str1);
  }
  delete_word->setEnabled(false);
  lineword->setText("");
  lineword->setEnabled(false);
  listwords->show();

}
//********************************************************
void WordsEdit::select_word(int num)
{

  lineword->setText(wordlist->WordGroup[SelectedGroup].Words.at(num).c_str());
  delete_word->setEnabled(true);  
  
}

//********************************************************
void WordsEdit::add_word_cb(void)
{

  lineword->setEnabled(true);
  lineword->setText("new word");
  lineword->selectAll();
  lineword->setFocus();
}

//********************************************************
void WordsEdit::do_add_word(void)
{
 
  QString str = lineword->text();
  char *word = (char *)str.latin1();
  
  FindLastWord = 0;
  FindLastGroup = 0;
  int curgroup = listgroup->currentItem();
  if(find_down(word)){
    sprintf(tmp,"This word already exists (in group %d).\nDo you wish to remove this occurance and add it to this group ?",wordlist->WordGroup[FindLastGroup].GroupNum);
    
    switch( QMessageBox::information( this, "Remove duplicate word ?",
                                      tmp,
                                      "Yes", "No", 
                                      0,      // Enter == button 0
                                      1 ) ) { // Escape == button 1
    case 0: //yes
      wordlist->WordGroup[FindLastGroup].Words.del(FindLastWord);
      update_group(FindLastGroup);
      changed=true;
      break;
    case 1: //no
      return;
    }
  }

  wordlist->WordGroup[curgroup].Words.addsorted(word); 
  changed=true;
  select_group(curgroup);
  update_group(curgroup);

}

//********************************************************
void WordsEdit::count_groups_cb(void)
{

  sprintf(tmp,"There are %d word groups.",wordlist->NumGroups);
  QMessageBox::information( this, "AGI studio",
                                      tmp,
                                      "OK", 
                                      0, 0);

}

//********************************************************
void WordsEdit::count_words_cb(void)
{
  int n = 0;
  for (int i = 0;i< wordlist->NumGroups;i++){
    n+=wordlist->WordGroup[i].Words.num;
  }
  sprintf(tmp,"There are %d words.",n);
  QMessageBox::information( this, "AGI studio",
                                      tmp,
                                      "OK", 
                                      0, 0);

}
//********************************************************

void WordsEdit::open_file()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Open",true);  
  const char *filters[] = {"words.tok","*.tok","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Open");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->dir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      open((char *)f->selectedFile().latin1());
  }

}

//********************************************************
void WordsEdit::save_as_file()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Save",true);  
  const char *filters[] = {"words.tok","*.tok","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Save");
  f->setMode(Q3FileDialog::AnyFile);
  f->setDir(game->dir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      save((char *)f->selectedFile().latin1());
  }

}

//********************************************************
void WordsEdit::new_file()
{

  setCaption("WORDS.TOK Editor");
  wordlist->clear();
  listgroup->clear(); 
  listwords->clear();
  for(int i=0;i<3;i++){
    print_group(i);
    listgroup->insertItem(tmp);
  }
  filename="";

}

//********************************************************
void WordsEdit::save(char *filename)
{

  if (wordlist->NumGroups == 0){
    menu->errmes("Wordsedit","Error: Could not save the file as there are no word groups.");
    return;
  }
  if(!wordlist->save(filename)){
    changed=false;
  }

}

//********************************************************
void WordsEdit::save_file()
{

  if(filename!="")
    save((char *)filename.c_str());
  else
    save_as_file();

}

//********************************************************
void WordsEdit::merge_file()
{

  WordList w = WordList();
  Q3FileDialog *f = new Q3FileDialog(0,"Open",true);  
  const char *filters[] = {"words.tok","*.tok","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Open");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->dir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() ){
      int ret = w.read((char *)f->selectedFile().latin1());
      if(ret)return ;
      wordlist->merge(w);
      update_all();
    }
  }
  
}

//********************************************************
WordsFind::WordsFind( QWidget *parent, const char *name , WordsEdit *w)
    : QWidget( parent, name )
{

  wordsedit = w;
  wordlist=w->wordlist;
  setCaption("Find");
  Q3BoxLayout *all =  new Q3VBoxLayout(this,10);  

  Q3BoxLayout *txt = new Q3HBoxLayout(all,4);

  QLabel *label = new QLabel("Find what:",this);  
  txt->addWidget(label);
  
  find_field = new QLineEdit(this);
  find_field->setMinimumWidth(200);
  connect( find_field, SIGNAL(returnPressed()), SLOT(find_first_cb()) );
  txt->addWidget(find_field);

  Q3BoxLayout *left1 =  new Q3HBoxLayout(all,10);

  Q3ButtonGroup *direction = new Q3ButtonGroup(2,Qt::Vertical,"Dir",this);
  up = new QRadioButton("Up",direction);
  up->setChecked(false);
  down = new QRadioButton("Down",direction);
  down->setChecked(true);
  left1->addWidget(direction);

  Q3ButtonGroup *from = new Q3ButtonGroup(2,Qt::Vertical,"From",this);
  start = new QRadioButton("Start",from);
  start->setChecked(true);
  current = new QRadioButton("Current",from);
  current->setChecked(false);
  left1->addWidget(from);

  Q3ButtonGroup *type = new Q3ButtonGroup(2,Qt::Vertical,"Match",this);
  exact = new QRadioButton("Exact",type);
  exact->setChecked(false);
  substring = new QRadioButton("Substr",type);
  substring->setChecked(true);
  left1->addWidget(type);

  Q3BoxLayout *right =  new Q3VBoxLayout(left1,5);  
  find_first = new QPushButton("Find",this);
  right->addWidget(find_first);
  connect( find_first, SIGNAL(clicked()), SLOT(find_first_cb()) );
  find_next = new QPushButton("Find next",this);
  connect( find_next, SIGNAL(clicked()), SLOT(find_next_cb()) );
  right->addWidget(find_next);
  cancel = new QPushButton("Cancel",this);
  connect( cancel, SIGNAL(clicked()), SLOT(cancel_cb()) );
  right->addWidget(cancel);

  adjustSize();

  FindLastWord=-1;
  FindLastGroup=-1;
  
}

//********************************************************
int WordsFind::find_down(char *word)
{ 
  bool sub = substring->isChecked();

  for (int i = FindLastGroup;i< wordlist->NumGroups;i++){
    for(int k = FindLastWord;k<wordlist->WordGroup[i].Words.num;k++){
      if((sub && strstr(wordlist->WordGroup[i].Words.at(k).c_str(),word))||
         !strcmp(wordlist->WordGroup[i].Words.at(k).c_str(),word)){
        FindLastWord = k;
        FindLastGroup = i;
        return 1;
      }
    }
    FindLastWord=0;
  }
  FindLastGroup = wordlist->NumGroups-1;
  FindLastWord = wordlist->WordGroup[FindLastGroup].Words.num-1;
  return 0;

}

//********************************************************
int WordsFind::find_up(char *word)
{  
  bool sub = substring->isChecked();

  for (int i = FindLastGroup;i>= 0;i--){
    for(int k = FindLastWord;k>=0;k--){
      if((sub && strstr(wordlist->WordGroup[i].Words.at(k).c_str(),word))||
         !strcmp(wordlist->WordGroup[i].Words.at(k).c_str(),word)){
        FindLastWord = k;
        FindLastGroup = i;
        return 1;
      }
    }
    if(i>0)FindLastWord=wordlist->WordGroup[i-1].Words.num-1;
  }
  FindLastWord=0;
  FindLastGroup=0;
  return 0;

}

//********************************************************
void WordsFind::find_first_cb()
{
  int ret;
  QString str = find_field->text();
  char *word = (char *)str.latin1();

  if(down->isChecked()){
    if(start->isChecked()){
      FindLastGroup=0;
      FindLastWord=0;
    }
    else{
      if(wordsedit->listgroup->currentItem()!=-1)
        FindLastGroup=wordsedit->listgroup->currentItem();
      else
        FindLastGroup=0;
      if(wordsedit->listwords->currentItem()!=-1)
        FindLastWord=wordsedit->listwords->currentItem();
      else
        FindLastWord=0;
    }
    ret = find_down(word);
  }
  else{    
    if(start->isChecked()){
      FindLastGroup=wordlist->NumGroups -1;
      FindLastWord = wordlist->WordGroup[FindLastGroup].Words.num-1;
    }
    else{
      if(wordsedit->listgroup->currentItem()!=-1)
        FindLastGroup=wordsedit->listgroup->currentItem();
      else
        FindLastGroup=wordlist->NumGroups -1;
      if(wordsedit->listwords->currentItem()!=-1)
        FindLastWord=wordsedit->listwords->currentItem();
      else
        FindLastWord=wordlist->WordGroup[FindLastGroup].Words.num-1;            
    }
    ret = find_up(word);
  }
  
  if(ret){
    wordsedit->listgroup->setCurrentItem(FindLastGroup);
    wordsedit->listwords->setCurrentItem(FindLastWord);
  }
  else{
    menu->errmes("Find","'%s' not found !",word);
  }
  
}
//********************************************************
void WordsFind::find_next_cb()
{
  int ret;
  QString str = find_field->text();
  char *word = (char *)str.latin1();

  if(FindLastGroup==-1||FindLastWord==-1){
    find_first_cb();
    return;
  }

  if(down->isChecked()){
    if(FindLastWord+1 >= wordlist->WordGroup[FindLastGroup].Words.num){
      if(FindLastGroup+1 >= wordlist->NumGroups){        
        menu->errmes("Find","'%s' not found !",word);    
        return ;
      }
      else{
        FindLastWord = 0;
        FindLastGroup++;    
      }
    }
    else{
      FindLastWord++;
    }         
    ret = find_down(word);
  }
  else{
    if(FindLastWord -1 < 0){
      if(FindLastGroup-1 < 0){        
        menu->errmes("Find","'%s' not found !",word);    
        return;
      }
      else{
        FindLastGroup--;
        FindLastWord = wordlist->WordGroup[FindLastGroup].Words.num-1;
      }
    }
    else{
      FindLastWord--;
    }  
    ret = find_up(word);
  }
 
  if(ret){
    wordsedit->listgroup->setCurrentItem(FindLastGroup);
    wordsedit->listwords->setCurrentItem(FindLastWord);
  }
  else{
    menu->errmes("Find","'%s' not found !",word);    
  }

}

//************************************************
void WordsFind::cancel_cb()
{

  hide();

}

//************************************************
ReplaceWord::ReplaceWord(string word,int OldGroupNum, int NewGroupNum, QWidget *parent, QString name )
  : QDialog( parent, name, TRUE ,Qt::WDestructiveClose)
{

  setCaption("Replace word");

  Q3BoxLayout *all = new Q3VBoxLayout(this,10);
  sprintf(tmp,"The word %s already exists in group %d of the currently open file.",word.c_str(),OldGroupNum);
  QLabel *l1 = new QLabel(tmp,this);
  all->add(l1);

  sprintf(tmp,"Do you wish to replace it with the occurance in the merge file (group %d )?",NewGroupNum);
  QLabel *l2 = new QLabel(tmp,this);
  all->add(l2);
  
  Q3BoxLayout *b = new Q3HBoxLayout(all,10);

  QPushButton *yes = new QPushButton( "Yes", this );
  connect( yes, SIGNAL(clicked()), SLOT(yes() ));
  b->addWidget(yes);
  QPushButton *yes_to_all = new QPushButton( "Yes to all", this );
  connect( yes_to_all, SIGNAL(clicked()), SLOT(yes_to_all() ));
  b->addWidget(yes_to_all);
  QPushButton *no = new QPushButton( "No", this );
  connect( no, SIGNAL(clicked()), SLOT(no() ));
  b->addWidget(no);
  QPushButton *no_to_all = new QPushButton( "No to all", this );
  connect( no_to_all, SIGNAL(clicked()), SLOT(no_to_all() ));
  b->addWidget(no_to_all);

  adjustSize();

}
//************************************************
void ReplaceWord::yes()
{
  done(mrYes);
}
//************************************************
void ReplaceWord::yes_to_all()
{
  done(mrYesToAll);
}
//************************************************
void ReplaceWord::no()
{
  done(mrNo);
}
//************************************************
void ReplaceWord::no_to_all()
{
  done(mrNoToAll);
}
//************************************************
