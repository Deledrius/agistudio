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

#include "logedit.h"
#include "game.h"
#include "menu.h"
#include "agicommands.h"

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
#include <q3syntaxhighlighter.h>
#include <qregexp.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QLabel>
#include <QHideEvent>
#include <QResizeEvent>
#include <Q3PopupMenu>
#include <QTextOStream>
#include <Q3VBoxLayout>
#include <Q3BoxLayout>
#include <QShowEvent>
#include <QCloseEvent>

TStringList InputLines;  //temporary buffer for reading the text from editor
//and sending to compilation

//***********************************************
// Syntax highlight

static QString operators ="!-+=<>/*&|^%",
               wordchars = "0123456789abcdefghijlkmnopqrstuvwxyz"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ._#";

static QColor normal_color = QColor("black"),
              comment_color = QColor("#808080"),
              string_color = QColor("red"),
              number_color = QColor("darkCyan"),
              command_color = QColor("darkBlue"),
              test_color = QColor("brown"),
              operator_color = QColor("blue");

class LogicSyntaxHL : public Q3SyntaxHighlighter
{
public:

  LogicSyntaxHL( Q3TextEdit* te )
    : Q3SyntaxHighlighter( te ) {}

  void highlightWord( const QString& line, int start, int len )
  {
    QColor* col = 0;
    QString word( line.mid(start, len));

    // Number?
    if ( word.find(QRegExp("[^0-9]")) < 0 )
      col = &number_color;
    else
    {
      // AGI command?
      for ( CommandStruct* cmd = AGICommand;
            cmd < AGICommand + (sizeof(AGICommand)/sizeof(CommandStruct));
            ++cmd )
      {
        if ( word == cmd->Name )
        {
          col = &command_color;
          break;
        }
      }
      // AGI test command?
      for ( CommandStruct* cmd = TestCommand;
            cmd < TestCommand + (sizeof(TestCommand)/sizeof(CommandStruct));
            ++cmd )
      {
        if ( word == cmd->Name )
          col = &test_color;
      }
      // Control structure
      if ( word[0]=='#' || word == "if" || word == "else" || word == "goto" )
        col = &operator_color;
    }

    if ( col )
      setFormat( start, len, *col);
  }

  int highlightParagraph ( const QString & text, int endStateOfLastPara )
  {
    setFormat( 0, text.length(), normal_color);

    QString txt = text;

    int comment_depth = endStateOfLastPara;
    bool in_quotes = (comment_depth==-1);
    if ( comment_depth < 0 )
      comment_depth = 0;

    int curchar = 0, lastchar = 0;
    while( !txt.isEmpty())
    {
      // Get next line & shorten
      int eol = txt.find( '\n' );
      if ( eol < 0 )
        eol = txt.length()-1;
      QString line = txt.mid(0,eol);
      txt = txt.mid(eol+1);
      bool in_word = false;

      // Process the line
      int i;
      for ( i=0; i<(int)line.length(); ++i )
      {
        // Single words
        if ( in_word && wordchars.find(line[i]) < 0 )
        {
          highlightWord( line, lastchar, curchar+i-lastchar );
          in_word = false;
        }

        // Comments
        if ( !in_quotes )
        {
          if (comment_depth==0 && line.mid(i,1) == "[")
          {
            setFormat( (curchar+i), line.length()-i+1, comment_color);
            break;
          }
          // More than two chars left?
          if ( i<(int)line.length()-1 )
          {
            if (comment_depth==0 && (line.mid(i,2) == "//" ))
            {
              setFormat( (curchar+i), line.length()-i+1, comment_color);
              break;
            }
            else if ( line.mid(i,2) == "/*")
            {
              if ( comment_depth == 0 )
                lastchar = (curchar+i);
              ++comment_depth;
              ++i;
              continue;
            }
            else if (comment_depth>0 && line.mid(i,2) == "*/" )
            {
              --comment_depth;
              if ( comment_depth == 0 )
              {
                setFormat( lastchar, (curchar+i)-lastchar+2, comment_color);
                lastchar = (curchar+i+2);
              }
              ++i;
              continue;
            }
          }
        }

        // Quotes
        if ( comment_depth == 0 )
        {
          if(line[i]=='\"' && (i==0 || line[i-1] != '\\'))
          {
            if ( in_quotes )
              setFormat( lastchar, (curchar+i)-lastchar+1, string_color);
            lastchar = curchar+i;
            in_quotes = !in_quotes;
          }

          if ( !in_quotes )
          {
            if( !in_word && wordchars.find(line[i]) >= 0 &&
                (i==0 || wordchars.find(line[i-1]) < 0 ))
            {
              in_word = true;
              lastchar = curchar+i;
            }
            else if ( operators.find(line[i]) >= 0 )
              setFormat((curchar+i), 1, operator_color);
          }
        }
      }

      // End of line, format word if still open
      if ( in_word  )
        highlightWord( line, lastchar, line.length()-lastchar );

      curchar += i;
      if ( in_quotes )
        setFormat( lastchar, curchar-lastchar+1, string_color);
    }

    // End of paragraph, format comment block if still open
    if ( comment_depth > 0 )
      setFormat( lastchar, curchar-lastchar+1, comment_color);

    return (in_quotes ? -1 : comment_depth);
  }
};

//***********************************************
LogEdit::LogEdit( QWidget *parent, const char *name, int win_num, ResourcesWin *res, bool readonly)
    : QWidget( parent, name, Qt::WDestructiveClose )
    , findedit(NULL)
    , roomgen(NULL)
{
  setCaption("Logic editor");

  logic = new Logic();
  winnum = win_num;  //my window number
  resources_win = res; //resources window which called me

  editor = new Q3MultiLineEdit(this);

  QFont font("Monospace");
  font.setPointSize(readonly?8:9);
  font.setStyleHint( QFont::TypeWriter );
  editor->setFont( font );

  editor->setSizePolicy( QSizePolicy(
    QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  editor->setWordWrap( Q3TextEdit::NoWrap );
  syntax_hl = new LogicSyntaxHL( editor );

  if ( readonly )
  {
    editor->setReadOnly( readonly );
  }
  else
  {
    editor->setMinimumSize(512,600);
    setMinimumSize(450,400);
  }

  setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  
  Q3BoxLayout *all = new Q3VBoxLayout(this,10);
  all->addWidget(editor);

  if ( !readonly ) {
    Q3PopupMenu *file = new Q3PopupMenu( this );
    Q_CHECK_PTR( file );

    file->insertItem( "Read from file", this, SLOT(read_logic()) );
    file->insertItem( "Save", this, SLOT(save_logic()), Qt::CTRL + Qt::Key_S );
    file->insertItem( "Save as", this, SLOT(save_as()) );
    file->insertItem( "Compile", this, SLOT(compile_logic()) ,Qt::Key_F9 );
    file->insertItem( "Compile all", this, SLOT(compile_all_logic())  );
    file->insertItem( "Compile all and run", this, SLOT(compile_and_run()) ,Qt::Key_F10 );
    file->insertItem( "Change logic number", this, SLOT(change_logic_number()) );
    file->insertSeparator();
    file->insertItem( "Delete", this, SLOT(delete_logic()) );
    file->insertItem( "New room", this, SLOT(new_room()) );
    file->insertSeparator();
    file->insertItem( "Close", this, SLOT(close()) );

    Q3PopupMenu *edit = new Q3PopupMenu( this );
    Q_CHECK_PTR( edit );
    edit->insertItem( "Cut", editor, SLOT(cut()) , Qt::CTRL + Qt::Key_X );
    edit->insertItem( "Copy", editor, SLOT(copy()) , Qt::CTRL + Qt::Key_C );
    edit->insertItem( "Paste", editor, SLOT(paste()) , Qt::CTRL + Qt::Key_V);
    edit->insertSeparator();
    edit->insertItem( "Clear all", this, SLOT(clear_all()) );
    edit->insertSeparator();
    edit->insertItem( "Find", this, SLOT(find_cb()) ,Qt::CTRL + Qt::Key_F);
    edit->insertItem( "Find again", this, SLOT(find_again()) ,Qt::Key_F3);
    edit->insertSeparator();
    edit->insertItem( "Go to line", this, SLOT(goto_cb()) ,Qt::ALT + Qt::Key_G);

    Q3PopupMenu *help = new Q3PopupMenu( this );
    Q_CHECK_PTR( help );
    help->insertItem( "Context help", this, SLOT(context_help()), Qt::Key_F1);
    help->insertItem( "All commands", this, SLOT(command_help()));

    QMenuBar *menu = new QMenuBar(this);
    Q_CHECK_PTR( menu );
    menu->insertItem( "File", file );
    menu->insertItem( "Edit", edit );
    menu->insertItem( "Help", help );
    menu->setSeparator( QMenuBar::InWindowsStyle );

    all->setMenuBar(menu);

    status = new QStatusBar(this);
    QLabel *msg = new QLabel( status, "message" );
    status->addWidget( msg, 4 );
    all->addWidget(status);

    connect( editor, SIGNAL(cursorPositionChanged(int,int)),
             this, SLOT(update_line_num(int,int)));
  }

  getmaxcol();
  changed=false;
  filename="";  
  hide();
}

//***********************************************
void LogEdit::deinit()
{

  if(findedit){
    findedit->close(true);
    findedit=NULL;
  }
  if(roomgen){
    roomgen->close(true);
    roomgen=NULL;
  }
  delete logic;
  logic = 0;
  delete syntax_hl;
  syntax_hl = 0;

  winlist[winnum].type=-1;
  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void LogEdit::hideEvent( QHideEvent * )
{
  
  if(findedit){
    findedit->close(true);
    findedit=NULL;
  }
  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void LogEdit::showEvent( QShowEvent * )
{

  if(window_list && window_list->isVisible())window_list->draw();

}
//***********************************************
void LogEdit::closeEvent( QCloseEvent *e )
{

  if(changed){
    QString str = editor->text();
    if(!strcmp(str.latin1(),logic->OutputText.c_str())){  //not changed
      deinit();
      e->accept();
      return;
    }

    if(LogicNum != -1){
      sprintf(tmp,"Save changes to logic.%d ?",LogicNum);
    }
    else{
      sprintf(tmp,"Save changes to logic ?");      
    }
    switch ( QMessageBox::warning( this, "Logic editor",
                                   tmp,
                                   "Yes",
                                   "No",
                                   "Cancel",
                                   0, 2) ) {
    case 0: // yes
      save_logic(); 
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

//***********************************************
int LogEdit::open()
{

  getmaxcol();
  LogicNum = -1;
  show();
  changed=true;
  return 0;
}

//***********************************************
int LogEdit::open(char *filenam)
{
  getmaxcol();

  FILE *fptr = fopen(filenam,"rb");
  if(fptr!=NULL){
    filename = string(filenam);
    editor->clear();
    char *ptr;
    QString filecont;
    while(fgets(tmp,MAX_TMP,fptr)!=NULL){
      if((ptr=(char*)strchr(tmp,0x0a))) *ptr=0;
      if((ptr=(char*)strchr(tmp,0x0d))) *ptr=0;
      filecont += QString(tmp) + "\n";
    }
    editor->setText( filecont );
    fclose(fptr);
    logic->OutputText=editor->text().latin1();
    if((ptr=(char*)strrchr(filename.c_str(),'/')))ptr++;
    else ptr=(char *)filename.c_str();
    if(LogicNum!=-1)
      sprintf(tmp,"logic.%d (file %s)",LogicNum,ptr);
    else
      sprintf(tmp,"logic (file %s)",ptr);
    setCaption(tmp);
    show();
    changed=true;

    return 0;
  }
  else{
    menu->errmes("Can't open file %s !",filenam);
    //QMessageBox::critical( this, "Agistudio",
    //  QString( "Can't open file '" ) + filenam + "'");
    return 1;
  }
}

//***********************************************
int LogEdit::open(int ResNum)
{
  int err=0;
  QString str;
  FILE *fptr;
  getmaxcol();

  logic->maxcol=maxcol;

  //look for the source file first
  sprintf(tmp,"%s/logic.%03d",game->srcdir.c_str(),ResNum);
  fptr = fopen(tmp,"rb");
  if(fptr==NULL){
    sprintf(tmp,"%s/logic.%d",game->srcdir.c_str(),ResNum);
    fptr = fopen(tmp,"rb");
  }
  if(fptr==NULL){
    sprintf(tmp,"%s/logic%d.txt",game->srcdir.c_str(),ResNum);
    fptr = fopen(tmp,"rb");
  }
  if(fptr!=NULL){    
    LogicNum = ResNum;  
    err=open(tmp);
  }
  else{  //source file not found - reading from the game
    err=logic->decode(ResNum);
    if(!err)editor->setText(logic->OutputText.c_str());
    else {
      sprintf(tmp,"logic.%d",ResNum);
      menu->errmes(tmp,"Errors:\n%s",logic->ErrorList.c_str());
    }
    str.sprintf("logic.%d",ResNum);
    setCaption(str);
    LogicNum = ResNum;
    show();
    changed=true;
  }

  return err;
}

//***********************************************
void LogEdit::save(char *filename)
{

  QString str;
  byte *s;
  FILE *fptr;

  if((fptr=fopen(filename,"wb"))==NULL){
    menu->errmes("Can't open file %s !\n",filename);
    return;
  }
  for(int i=0;i<editor->numLines();i++){
    str = editor->textLine(i);
    if(!str.isNull()){
      s = (byte *)str.latin1();
      fprintf(fptr,"%s\n",s);
    }
  }
  fclose(fptr);
  changed=false;
}

//***********************************************
void LogEdit::save_logic()
{

  if(LogicNum==-1){
    save_as();
  }
  else if(filename != ""){
    save((char *)filename.c_str());
    char *ptr;
    if((ptr=(char*)strrchr(filename.c_str(),'/')))ptr++;
    else ptr=(char *)filename.c_str();
    sprintf(tmp,"File %s",ptr);
    setCaption(tmp);
  }
  else{
    sprintf(tmp,"%s/logic.%03d",game->srcdir.c_str(),LogicNum);
    save(tmp);
    sprintf(tmp,"Logic %d (file)",LogicNum);
    setCaption(tmp);
  }

}
//***********************************************
void LogEdit::save_as()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Save",true);  
  const char *filters[] = {"logic*.*","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Save");
  f->setMode(Q3FileDialog::AnyFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      save((char *)f->selectedFile().latin1());
  }
}

//***********************************************
void LogEdit::read_logic()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Read",true);  
  const char *filters[] = {"logic*.*","All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Read");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      open((char *)f->selectedFile().latin1());
  }
}

//***********************************************
int LogEdit::compile_all_logic()
{
  int ret,err=0;
  for(int i=0;i<MAXWIN;i++){
    if(winlist[i].type==LOGIC){
      ret=winlist[i].w.l->compile_logic();
      if(ret)err=1;
    }
  }
  return err;
}

//***********************************************
int LogEdit::compile_logic()
{
  QString str;
  byte *s;
  int err,i;
  string filename;
  char name[128];
  char tmp1[16],*ptr,*ptr1;

  InputLines.lfree();
  for(i=0;i<editor->numLines();i++){
    str = editor->textLine(i);
    if(!str.isNull() && str.length()>0){
      s = (byte *)str.latin1();
      if(s[0]<0x80)  //i'm getting \221\005 at the last line...
        InputLines.add((char *)str.latin1());
    }
    else{
      InputLines.add("");
    }
  }

  for(i=0;i<MAXWIN;i++){
    if(winlist[i].type==TEXT){
      if(winlist[i].w.t->filename != ""){
        winlist[i].w.t->save();
        winlist[i].w.t->status->message("");
      }
    }
  }

  err=logic->compile();

  if(!err){
    status->message("Compiled OK !", 2000);
    if(LogicNum!=-1){
      game->AddResource(LOGIC,LogicNum);
      save_logic();
      changed=false;
    }
  }
  else{
    if(logic->ErrorList != ""){
      if(LogicNum!=-1)
        sprintf(tmp1,"logic.%d",LogicNum);
      else
        sprintf(tmp1,"logic");      
      strcpy(tmp,logic->ErrorList.c_str());

      if(!strncmp(tmp,"File ",5)){
        ptr=strstr(tmp,"Line ");
        strncpy(name,tmp+5,(int)(ptr-tmp-6));
        name[(int)(ptr-tmp-6)]=0;
        for(i=0;i<MAXWIN;i++){
          if(winlist[i].type==TEXT){
            filename = winlist[i].w.t->filename;
            char *ptr2;
            if((ptr2=(char*)strrchr(filename.c_str(),'/')))ptr2++;
            else ptr2=(char *)filename.c_str();            
            if(!strcmp(ptr2,name)){
              int num=atoi(ptr+5);
              winlist[i].w.t->editor->setCursorPosition(num,0,false);
              ptr1=strchr(ptr,'\n');
              *ptr1=0;
              winlist[i].w.t->status->message(ptr);
              break;
            }
          }
        }
        if(i>=MAXWIN){
          char fullname[256];
          string tmp1=tmp;          
          sprintf(fullname,"%s/%s",game->srcdir.c_str(),name);
          for(i=0;i<MAXWIN;i++){
            if(winlist[i].type==-1){
              winlist[i].w.t = new TextEdit(NULL,NULL,i);
              winlist[i].type=TEXT;
              winlist[i].w.t->open(fullname);
              ptr=(char*)strstr(tmp1.c_str(),"Line ");
              int num=atoi(ptr+5);
              winlist[i].w.t->editor->setCursorPosition(num,0,false);
              ptr1=strchr(ptr,'\n');
              *ptr1=0;
              winlist[i].w.t->status->message(ptr);
              break;
            }
          }
        }
      }
      else{
        ptr=strstr(tmp,"Line ");
        int num=atoi(ptr+5);
        editor->setCursorPosition(num,0,false);
        ptr1=strchr(ptr,'\n');
        *ptr1=0;
        status->message(tmp);
      }
      menu->errmes(tmp1,"Errors:\n%s",logic->ErrorList.c_str());
    }
  }
  return err;
}

//***********************************************
void LogEdit::compile_and_run()
{
  if(!compile_all_logic())
    menu->run_game();
}

//***********************************************
void LogEdit::change_logic_number()
{

  AskNumber *changelogic = new AskNumber(0,0,"Logic number","Enter logic number: [0-255]");    

  if(!changelogic->exec())return;
  QString str = changelogic->num->text();
  int num = atoi((char *)str.latin1());  
  if(num<0||num>255){
    menu->errmes("Logic number must be between 0 and 255 !");
    return ;
  }
  if(game->ResourceInfo[LOGIC][num].Exists){
    sprintf(tmp,"Resource logic.%d already exists. Replace it ?",num);    
    switch( QMessageBox::warning( this, "Logic", tmp,
                                      "Replace", "Cancel",
                                      0,      // Enter == button 0
                                      1 ) ) { // Escape == button 1
    case 0:      
      break;
    case 1:       
      return;
    }
  }

  LogicNum = num;
  compile_logic();
  filename="";
  save_logic();
  if(resources_win){
    resources_win->select_resource_type(LOGIC);
    resources_win->set_current(num);
  }
  open(num);

}

//***********************************************
void LogEdit::delete_logic()
{
  int k;

  if(LogicNum==-1)return;

  sprintf(tmp,"Really delete logic %d ?",LogicNum);
  switch( QMessageBox::warning( this, "Logic", tmp,
                                "Delete", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0: 
    game->DeleteResource(LOGIC,LogicNum);
    delete_file(LogicNum);
    if(resources_win){
      k = resources_win->list->currentItem();
      resources_win->select_resource_type(LOGIC);
      resources_win->list->setCurrentItem(k);
    }
    changed=false;
    close();
    break;
  case 1:       
    break;
  } 
}

//***********************************************
void LogEdit::delete_file(int ResNum)
{
  struct stat buf;

  sprintf(tmp,"%s/logic.%03d",game->srcdir.c_str(),ResNum);
  if(stat(tmp,&buf)==0){
    unlink(tmp);
  }
  sprintf(tmp,"%s/logic.%d",game->srcdir.c_str(),ResNum);
  if(stat(tmp,&buf)==0){
    unlink(tmp);
  }
  sprintf(tmp,"%s/logic%d.txt",game->srcdir.c_str(),ResNum);
  if(stat(tmp,&buf)==0){
    unlink(tmp);
  }

}

//***********************************************
void LogEdit::clear_all()
{
 
  switch( QMessageBox::warning( this, "Logic", "Really clear all ?",
                                "Clear", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0: 
    editor->clear();
    logic->OutputText = "";
    break;
  case 1:       
    break;
  } 

}

//***********************************************
void LogEdit::new_room()
{
  //  FILE *fptr;
 
  switch( QMessageBox::warning( this, "Logic", "Replace the editor contents\nwith the new room template ?",
                                "Replace", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0:
    /*
    sprintf(tmp,"%s/src/newroom.txt",game->templatedir.c_str());
    fptr = fopen(tmp,"rb");
    if(fptr==NULL){
      menu->errmes("Can't open "+string(tmp)+"!");
      return;
    }
    editor->clear();    
    char *ptr; 
    while(fgets(tmp,MAX_TMP,fptr)!=NULL){
      if((ptr=strchr(tmp,0x0a)))*ptr=0;
      if((ptr=strchr(tmp,0x0d)))*ptr=0;
      editor->insertLine(tmp,-1);
    }
    fclose(fptr);
    */
    if(roomgen==NULL)roomgen=new RoomGen();
    if(roomgen->exec()){
      editor->setText(roomgen->text.c_str());
      logic->OutputText=editor->text().latin1();
      changed=true;
    }
    break;
  case 1:       
    break;
  } 

}
//***********************************************
void LogEdit::goto_cb()
{

  AskNumber *ask_number = new AskNumber(0,0,"Go to line",
                        "Go to line: ");

  if(!ask_number->exec())return;

  QString str = ask_number->num->text();
  int linenum = atoi((char *)str.latin1());
  editor->setCursorPosition(linenum,0,false);

}
//***********************************************
void LogEdit::find_cb()
{

  if(findedit==NULL)findedit = new FindEdit(NULL,NULL,editor,status);
  findedit->show();
  findedit->find_field->setFocus();

}

//***********************************************
void LogEdit::find_again()
{

  if(findedit==NULL)find_cb();
  else findedit->find_next_cb();

}

//***********************************************
void LogEdit::getmaxcol()
  //get maximum number of columns on screen (approx.)
  //(for formatting the 'print' messages)
{
  // QFontMetrics f = fontMetrics();
  // maxcol = editor->width()/f.width('a');
  maxcol = 50;
}

//***********************************************
void LogEdit::resizeEvent( QResizeEvent * )
{

  QString str = editor->text();
  getmaxcol();
  editor->setText(str);
}

//***********************************************
void LogEdit::context_help()
{
  int para, index;
  editor->getCursorPosition( &para, &index );
  if ( para<0 || index<0 )
    return;
  QString paratxt = editor->text( para );
  int start = index, end = index;

  if (wordchars.find(paratxt[start]) < 0)
    return;

  // Find the bounds of the whole word
  while( start > 0 && wordchars.find(paratxt[start-1]) >= 0 )
    --start;
  while( end < (int)paratxt.length() && wordchars.find(paratxt[end]) >= 0 )
    ++end;

  QString word = paratxt.mid( start, end-start ).lower();
  if (!menu->help_topic( word ))
    status->message("No help found for '" + word + "'", 2000);
}

//***********************************************
void LogEdit::command_help()
{
  menu->help_topic("commands_by_category");
}

//***********************************************
void LogEdit::update_line_num( int para, int pos )
{
  QString str;
  QTextOStream( &str ) << pos << ", " << para;
  status->message(str);
}


//*******************************************************
TextEdit::TextEdit( QWidget *parent, const char *name,int win_num)
    : QWidget( parent, name ,Qt::WDestructiveClose )
{

  setCaption("Text editor");

  winnum = win_num;
  editor = new Q3MultiLineEdit(this);
  editor->setMinimumSize(380,380);

  QFont font;
  font.setPointSize(10);
  font.setStyleHint( QFont::TypeWriter );
  editor->setFont( font );
  editor->setWordWrap( Q3TextEdit::NoWrap );

  Q3PopupMenu *file = new Q3PopupMenu( this );
  Q_CHECK_PTR( file );

  file->insertItem( "New", this, SLOT(new_text()));
  file->insertItem( "Open", this, SLOT(open()) );
  file->insertItem( "Save", this, SLOT(save()), Qt::CTRL + Qt::Key_S );
  file->insertItem( "Save as", this, SLOT(save_as()) );
  file->insertSeparator();
  file->insertItem( "Close", this, SLOT(close()) );

  Q3PopupMenu *edit = new Q3PopupMenu( this );
  Q_CHECK_PTR( edit );
  edit->insertItem( "Cut", editor, SLOT(cut()) , Qt::CTRL + Qt::Key_X );
  edit->insertItem( "Copy", editor, SLOT(copy()) , Qt::CTRL + Qt::Key_C );
  edit->insertItem( "Paste", editor, SLOT(paste()) , Qt::CTRL + Qt::Key_V);
  edit->insertSeparator();
  edit->insertItem( "Clear all", this, SLOT(clear_all()) );
  edit->insertSeparator();  
  edit->insertItem( "Find", this, SLOT(find_cb()) ,Qt::CTRL + Qt::Key_F);
  edit->insertItem( "Find again", this, SLOT(find_again()) ,Qt::Key_F3);  

  QMenuBar *menu = new QMenuBar(this);  
  Q_CHECK_PTR( menu );
  menu->insertItem( "File", file );
  menu->insertItem( "Edit", edit );
  menu->setSeparator( QMenuBar::InWindowsStyle );
  setMinimumSize(400,400);

  Q3BoxLayout *all = new Q3VBoxLayout(this,10);
  all->setMenuBar(menu);

  all->addWidget(editor);
  
  status = new QStatusBar(this);
  QLabel *msg = new QLabel( status, "message" );
  status->addWidget( msg, 4 );
  all->addWidget(status);

  changed=false;  
  filename = "";
  findedit=NULL;
  OutputText = "";
}

//***********************************************
void TextEdit::deinit()
{
  if(findedit){
    findedit->close(true);
    findedit=NULL;
  }
  winlist[winnum].type=-1;
  if(window_list && window_list->isVisible())window_list->draw();
}

//*********************************************
void TextEdit::hideEvent( QHideEvent * )
{
  
  if(findedit){
    findedit->close(true);
    findedit=NULL;
  }
  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void TextEdit::showEvent( QShowEvent * )
{

  if(window_list && window_list->isVisible())window_list->draw();

}

//***********************************************
void TextEdit::closeEvent( QCloseEvent *e )
{
  
  if(changed){   
    QString str = editor->text();
    if(!strcmp(str.latin1(),OutputText.c_str())){  //not changed
      deinit();
      e->accept();
      return;
    }

    if(filename != "")
      sprintf(tmp,"Do you want to save changes to\n%s ?",filename.c_str());
    else
      strcpy(tmp,"Do you want to save changes ?");
    switch ( QMessageBox::warning( this, "Text editor",
                                   tmp,
                                   "Yes",
                                   "No",
                                   "Cancel",
                                   0, 2) ) {
    case 0: // yes
      save();
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

//***********************************************
void TextEdit::new_text()
{

  if(filename.length()>0){
    //if (changed) 
    sprintf(tmp,"Do you want to save changes to\n%s ?",filename.c_str());
    switch ( QMessageBox::warning( this, "Text editor",
                                   tmp,
                                   "Yes",
                                   "No",
                                   "Cancel",
                                   0, 2) ) {
    case 0: // yes
      save(); //???????????????
      break;
    case 1: // no
      break;
    default: // cancel
      break;
    }        
  }
  
  filename="";
  setCaption("New text");
  editor->clear();
  show();
  changed=true;
  OutputText = "";
}

//***********************************************
void TextEdit::open()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Open",true);  
  const char *filters[] = {"All files (*)",NULL};

  f->setFilters(filters);
  f->setCaption("Open");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() ){
      open((char *)f->selectedFile().latin1());
      show();
      changed=true;
    }
  }
}

//***********************************************
int TextEdit::open(char *filenam)
{
  FILE *fptr = fopen(filenam,"rb"); 
  
  if(fptr!=NULL){
    struct stat buf;
    fstat(fileno(fptr),&buf);
    editor->clear();
    char *ptr;
    QString filecont;
    while(fgets(tmp,MAX_TMP,fptr)!=NULL){
      if((ptr=strchr(tmp,0x0a)))*ptr=0;
      if((ptr=strchr(tmp,0x0d)))*ptr=0;
      filecont += QString(tmp) + "\n";
    }
    editor->setText( filecont );
    fclose(fptr);
    OutputText=editor->text().latin1();
    filename = string(filenam);
    char *name = strrchr(filenam,'/');
    if(name==NULL)name=filenam;
    else name++;
    QString str;
    str.sprintf("file %s",name);
    setCaption(str);    
    show();
    changed=true;
    return 0;
  }
  else{
    menu->errmes("Can't open file %s !",filenam);
    return 1;
  }
}

//***********************************************
void TextEdit::save()
{
  if(filename == ""){
    save_as();
  }
  else{
    save(filename.c_str());
  }
}

//***********************************************
void TextEdit::save_as()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Save",true);  
  const char *filters[] = {"All files (*)",NULL};
  
  f->setFilters(filters);
  f->setCaption("Save");
  f->setMode(Q3FileDialog::AnyFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() )
      save((const char *)f->selectedFile().latin1());
  }
}

//***********************************************
void TextEdit::save(const char *filename)
{

  QString str;
  byte *s;
  FILE *fptr;

  if((fptr=fopen(filename,"wb"))==NULL){
    menu->errmes("Can't open file %s !\n",filename);
    return;
  }
  for(int i=0;i<editor->numLines();i++){
    str = editor->textLine(i);
    if(!str.isNull()){
      s = (byte *)str.latin1();
      fprintf(fptr,"%s\n",s);
    }
  }
  fclose(fptr);
  changed=false;
  char *ptr;
  if((ptr=(char*)strrchr(filename,'/')))ptr++;
  else ptr=(char *)filename;
  sprintf(tmp,"File %s",ptr);
  setCaption(tmp);
}

//***********************************************
void TextEdit::clear_all()
{
 
  switch( QMessageBox::warning( this, "Text", "Really clear all ?",
                                "Clear", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0:
    editor->clear();
    break;
  case 1:
    break;
  }
 }

//***********************************************
void TextEdit::find_cb()
{

  if(findedit==NULL)findedit = new FindEdit(NULL,NULL,editor,status);
  findedit->show();
  findedit->find_field->setFocus();

}

//***********************************************
void TextEdit::find_again()
{

  if(findedit==NULL)find_cb();
  else findedit->find_next_cb();

}

//***********************************************
FindEdit::FindEdit( QWidget *parent, const char *name, Q3MultiLineEdit *edit ,QStatusBar *s)
    : QWidget( parent, name ,Qt::WDestructiveClose)
{
  setCaption("Find");
  //  setMinimumSize(340,140);

  status = s;
  editor = edit;

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


  Q3GroupBox *type = new Q3GroupBox(2,Qt::Vertical,"Match",this);
  match_whole = new QCheckBox("Match exact",type);  
  //  box->addWidget(match_whole);
  match_case = new QCheckBox("Match case",type);
  // box->addWidget(match_case);
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
  curline=0;

}

//***********************************************
void FindEdit::find_first_cb()
{

  if(current->isChecked()){
    int line,col;
    editor->getCursorPosition(&line,&col);
    curline=line;
  }
  else if(down->isChecked()){
    curline=0;
  }
  else{
    curline=editor->numLines()-1;
  }

  find_next_cb();

}

//***********************************************
void FindEdit::find_next_cb()
{

  int k;
  QString str;
  QString w = find_field->text();
  char *word = (char *)w.latin1();
  int len = strlen(word);
  if(len==0)return;
  char *ptr,*ptr0,*ww,*ww0;
  int num=editor->numLines();
  bool mwhole=match_whole->isChecked();
  bool mcase=!(match_case->isChecked());
  bool found;

  if(mcase)toLower(word);

  if(down->isChecked()){  
    if(curline<0)curline=0;
    for(;curline<num;curline++){
      str = editor->textLine(curline);
      ww = (char *)str.latin1();
      ww0=ww;
      if(mcase)toLower(ww);
      if(mwhole){
        ptr0=ww;
        found=false;
        do{
          ptr=strstr(ptr0,word);
          if(!ptr)break;
          ptr0=ptr+len;
          if(ptr>ww && isalnum(*(ptr-1)))continue;
          if(isalnum(*ptr0))continue;
          found=true;
        }while(!found);
        if(!found)continue;
      }
      else{
        if((ptr=strstr(ww,word))==NULL)continue;
      }
      k=int(ptr-ww);
      editor->setCursorPosition(curline,k,false);
      editor->setSelection(curline,k,curline,k+len);
      sprintf(tmp,"%d: %s",curline,ww0);
      status->message(tmp);
      curline++;
      return;      
    }
  }
  else{
    if(curline>=num)curline=num-1;
    for(;curline>=0;curline--){
      str = editor->textLine(curline);
      ww = (char *)str.latin1();
      ww0=ww;
      if(mcase)toLower(ww);
      if((ptr=strstr(ww,word))==NULL)continue;
      if(mwhole){
        ptr0=ww;
        found=false;
        do{
          ptr=strstr(ptr0,word);
          if(!ptr)break;
          ptr0=ptr+len;
          if(ptr>ww && isalnum(*(ptr-1)))continue;
          if(isalnum(*ptr0))continue;
          found=true;
        }while(!found);
        if(!found)continue;
      }
      else{
        if((ptr=strstr(ww,word))==NULL)continue;
      }      
      k=int(ptr-ww);
      editor->setCursorPosition(curline,k,false);
      editor->setSelection(curline,k,curline,k+len);      
      sprintf(tmp,"%d: %s",curline,ww0);
      status->message(tmp);
      curline--;
      return;      
    }
  }
  menu->errmes("Find","'%s' not found !",word);
  status->clear();

}

//***********************************************
void FindEdit::cancel_cb()
{

  hide();
  status->clear();
}
//***********************************************

