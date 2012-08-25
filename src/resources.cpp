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

#include "resources.h"
#include "game.h"
#include "logedit.h"
#include "viewedit.h"
#include "preview.h"
#include "menu.h"
#include "midi.h"
#include "bmp2agipic.h"

#include <q3widgetstack.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QLabel>
#include <QHideEvent>
#include <Q3PopupMenu>
#include <Q3VBoxLayout>
#include <Q3BoxLayout>
#include <QShowEvent>
#include <QCloseEvent>

#include <stdio.h>
#ifdef _WIN32
#define strncasecmp(a, b, l) _stricmp(a, b)
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>


//**********************************************************
ResourcesWin::ResourcesWin(  QWidget* parent, const char*  name, int win_num ):
  QWidget(parent,name,Qt::WDestructiveClose)
{
  setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ));
  setCaption("resources");
  winnum = win_num;

  Q3PopupMenu *window = new Q3PopupMenu( this );
  Q_CHECK_PTR( window );
  window->insertItem ( "New", this, SLOT(new_resource_window()) );
  window->insertSeparator();
  window->insertItem ( "Close", this, SLOT(close()) );


  resourceMenu = new Q3PopupMenu( this );
  Q_CHECK_PTR( resourceMenu );
  resourceMenu->insertItem ( "&Add", this, SLOT(add_resource()) );
  resourceMenu->insertItem ( "&Extract", this, SLOT(extract_resource()) );
  resourceMenu->insertItem ( "&Delete", this, SLOT(delete_resource()) );
  resourceMenu->insertItem ( "&Renumber", this, SLOT(renumber_resource()) );
  importMenuItemID = resourceMenu->insertItem ( "Import &bitmap", this, SLOT(import_resource()) );
  resourceMenu->setItemEnabled(importMenuItemID, false);
  resourceMenu->insertItem ( "Extract all", this, SLOT(extract_all_resource()) );

  QMenuBar* resourceMenubar = new QMenuBar(this);
  Q_CHECK_PTR( resourceMenubar );
  resourceMenubar->insertItem ("Window", window );
  resourceMenubar->insertItem( "&Resource", resourceMenu );
  resourceMenubar->setSeparator( QMenuBar::InWindowsStyle );

  Q3BoxLayout *hbox =  new Q3HBoxLayout( this, 10 );
  hbox->setMenuBar(resourceMenubar);

  Q3BoxLayout *resbox =  new Q3VBoxLayout( hbox, 10 );

  type = new QComboBox(FALSE, this, "type");

  type->insertItem( "LOGIC" );
  type->insertItem( "PICTURE" );
  type->insertItem( "VIEW" );
  type->insertItem( "SOUND" );
  connect( type, SIGNAL(activated(int)), SLOT(select_resource_type(int)) );
  type->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ));
  resbox->addWidget(type);

  list = new Q3ListBox(this,"list");
  list->setColumnMode (1);
  list->setMinimumSize(200,300);
  connect( list, SIGNAL(highlighted(int)), SLOT(highlight_resource(int)) );
  connect( list, SIGNAL(selected(int)), SLOT(select_resource(int)) );
  list->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::MinimumExpanding ));

  selected = game->res_default;
  resbox->addWidget(list);

  msg = new QLabel( this, "message" );
  msg->setAlignment( Qt::AlignLeft );
  resbox->addWidget(msg);

  addmenu = NULL;
  preview = NULL;
  closing = false;

  Q3BoxLayout *prevbox =  new Q3VBoxLayout(hbox,10);
  Q3GroupBox* group = new Q3GroupBox( 1, Qt::Vertical, "Preview", this );
  group->setMinimumSize(340+10*2,280+10*2);
  group->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  previewPane = group;
  prevbox->addWidget(previewPane);

  adjustSize();
}

//********************************************************
void ResourcesWin::select_resource_type( int ResType )
{
  QString str;
  int i,k;

  type->setCurrentItem(ResType);
  selected = ResType;

  // enable import item if resource type supports it
  resourceMenu->setItemEnabled(importMenuItemID, selected == PICTURE );

  // Show a resource list
  list->hide();
  list->clear();
  for(i=0,k=0;i<256;i++){
    if(game->ResourceInfo[ResType][i].Exists){
      str.sprintf( "%s.%3d", ResTypeName[ResType],i);
      list->insertItem( str );
      ResourceIndex[k++]=i;
    }
  }
  ResourceNum = k;
  list->show();
  first=true;
}

//********************************************************
void ResourcesWin::highlight_resource( int k )
{
  int i = ResourceIndex[k];
  int size = game->GetResourceSize(selected,i);

  QString str;
  str.sprintf("%d bytes",size);
  msg->setText(str);

  if(preview==NULL)
    preview = new Preview(previewPane,0,this);

  preview->open(i,selected);
  preview->show();
}
//********************************************************
void ResourcesWin::select_resource( int k )
{
  int i = ResourceIndex[k];
  int size = game->GetResourceSize(selected,i);
  int n;
  extern void play_sound(int ResNum);

  QString str;
  str.sprintf("%d bytes",size);
  msg->setText(str);

  if((n=get_win())==-1)return;
  switch(selected){
  case LOGIC:
    winlist[n].w.l=new LogEdit(NULL,NULL,n);
    winlist[n].type=LOGIC;
    winlist[n].w.l->open(i);
    break;
  case PICTURE:
    winlist[n].w.p=new PicEdit(NULL,NULL,n);
    winlist[n].type=PICTURE;
    winlist[n].w.p->open(i);
    break;
  case VIEW:
    winlist[n].w.v = new ViewEdit(NULL,NULL,n);
    winlist[n].type=VIEW;
    winlist[n].w.v->open(i);
    break;
  case SOUND:
    play_sound(i);
    break;
  }
}

//********************************************************
void ResourcesWin::set_current( int n )
{

  for(int i=0;i<ResourceNum;i++){
    if(ResourceIndex[i]==n){
      list->setCurrentItem(i);
      break;
    }
  }

}
//*********************************************
void ResourcesWin::deinit( )
{
  closing = true;
  if(preview){
    preview->close();
    preview=NULL;
  }
  for(int i=0;i<MAXWIN;i++){
    if(winlist[i].type==-1)continue;
    switch(winlist[i].type){
    case LOGIC:
      if(winlist[i].w.l->resources_win==this)winlist[i].w.l->resources_win=NULL;
      break;
    case VIEW:
      if(winlist[i].w.v->resources_win==this)winlist[i].w.v->resources_win=NULL;
      break;
    case PICTURE:
      if(winlist[i].w.p->resources_win==this)winlist[i].w.p->resources_win=NULL;
      break;
    }
  }

  winlist[winnum].type=-1;
  menu->dec_res();
  if(window_list && window_list->isVisible())window_list->draw();

}
//*********************************************
void ResourcesWin::closeEvent( QCloseEvent *e )
{

  deinit();
  e->accept();

}
//*********************************************
void ResourcesWin::hideEvent( QHideEvent * )
{

  if(preview && preview->isVisible())preview->showMinimized();
  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void ResourcesWin::showEvent( QShowEvent * )
{

  if(preview && !preview->isVisible())preview->showNormal();
  if(window_list && window_list->isVisible())window_list->draw();

}

//**********************************************
void ResourcesWin::new_resource_window()
{

  menu->new_resource_window();
}

//**********************************************
void ResourcesWin::delete_resource()
{

  int restype = selected;
  int k = list->currentItem();
  int resnum = ResourceIndex[k];

  sprintf(tmp,"Really delete %s.%d ?",ResTypeName[restype],resnum);
  switch( QMessageBox::warning( this, "Delete resource", tmp,
                                "Delete", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0:
    game->DeleteResource(restype,resnum);
    select_resource_type(restype);
    if(k>0)
      list->setCurrentItem(k-1);
    else
      list->setCurrentItem(0);
    break;
  case 1:
    break;
  }

}

//**********************************************
void ResourcesWin::renumber_resource()
{

  int k = list->currentItem();
  if(k==-1)return;

  AskNumber *ask_number = new AskNumber(0,0,"Resource number",
                        "Enter new resource number [0-255]: ");

  if(!ask_number->exec())return;

  QString str = ask_number->num->text();
  int newnum = atoi((char *)str.latin1());
  int restype = selected;
  int resnum = ResourceIndex[k];
  if(game->ResourceInfo[restype][newnum].Exists){
    sprintf(tmp,"Resource %s.%d already exists. Replace it ?",ResTypeName[restype],newnum);
  }
  switch( QMessageBox::warning( this, "Renumber resource", tmp,
                                "Replace", "Cancel",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0:
    game->ReadResource(restype,resnum);
    game->DeleteResource(restype,resnum);
    game->AddResource(restype,newnum);
    select_resource_type(restype);
    break;
  case 1:
    break;
  }
}

static QImage openBitmap( const char* title )
{
  Q3FileDialog *f = new Q3FileDialog(0,"Load visible bitmap",true);
  const char *filters[] = {"All files (*)",NULL};
  f->setFilters(filters);

  QImage pic;
  
  f->setCaption(title);
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted )
    if ( !f->selectedFile().isEmpty() )
    {
      pic = QImage( f->selectedFile());
      if ( pic.isNull())
        menu->errmes("Error loading bitmap.");
    }

  if (!pic.isNull())
    if ( (pic.width() != 320 && pic.width() != 160) || pic.height()<168 )
    {
      menu->errmes("Bitmap size must be 320x168 or 160x168.\nHeight can be more but will be cropped to 168.");
    }
  return pic;
}

//**********************************************
void ResourcesWin::import_resource()
{
  QImage vis, pri;

  vis = openBitmap("Open visible bitmap");
  if ( vis.isNull())
    return;
  pri = openBitmap("Open PRIORITY bitmap (or press cancel)");
  
  AskNumber *ask_number = new AskNumber(0,0,"Resource number",
                        "Enter new resource number [0-255]: ");
  if(!ask_number->exec()) return;

  QString str = ask_number->num->text();
  int newnum = atoi((char *)str.latin1());
  int restype = selected;
  
  int replace = 0;
  if(game->ResourceInfo[restype][newnum].Exists)
  {
    sprintf(tmp,"Resource %s.%d already exists. Replace it ?",ResTypeName[restype],newnum);
    replace = QMessageBox::warning( this, "Overwrite resource", tmp, "Replace", "Cancel", 0, 1 );
  }
  switch( replace )
  {
  case 0:
    {
      if(game->ResourceInfo[restype][newnum].Exists)
        game->DeleteResource(restype, newnum);
      
      QByteArray res;
      const char* err = bitmapToAGIPicture( vis, pri, &res );
      if ( err )
      {
        menu->errmes(err);
        return;
      }    
      ResourceData.Size = res.size();
      memcpy( ResourceData.Data, res.data(), res.size());
            
      game->AddResource(restype, newnum); // uses data from ResourceData
      select_resource_type(restype);
      
      for ( int k=0; k<255; ++k )
        if ( ResourceIndex[k] == newnum )
          list->setSelected(k, true);
    }
    break;
  case 1:
    break;
  }
}

//**********************************************
static void extract(char *filename,int restype,int resnum)
{
  if(game->ReadResource(restype,resnum))return ;

  FILE *fptr=fopen(filename,"wb");
  if(fptr==NULL){
    menu->errmes("Can't open file %s ! ",filename);
    return ;
  }
  if(restype==LOGIC && game->save_logic_as_text){
    Logic *logic = new Logic();
    int err=logic->decode(resnum);
    if(!err)fprintf(fptr,"%s",logic->OutputText.c_str());
    delete logic;
  }
  else{
    fwrite(ResourceData.Data,ResourceData.Size,1,fptr);
  }
  fclose(fptr);
}

//**********************************************
void ResourcesWin::extract_resource()
{

  int restype = selected;
  int k = list->currentItem();
  int resnum = ResourceIndex[k];

  Q3FileDialog *f = new Q3FileDialog(0,"Extract resource",true);
  f->setCaption("Extract resource");
  f->setMode(Q3FileDialog::AnyFile);
  f->setDir(game->srcdir.c_str());
  sprintf(tmp,"%s.%03d",ResTypeName[restype],resnum);
  f->setSelection(tmp);

  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() ){
      extract((char *)f->selectedFile().latin1(),restype,resnum);
    }
  }
}

//**********************************************
void ResourcesWin::extract_all_resource()
{
  char filename[256];

  int restype = selected;
  sprintf(tmp,"Do you really want to extract all %s resources ?",ResTypeName[restype]);
  switch( QMessageBox::warning( this, "Extract all", tmp,
                                "Yes", "No",
                                0,      // Enter == button 0
                                1 ) ) { // Escape == button 1
  case 0:
    break;
  case 1:
    return;
  }


   for(int resnum=0;resnum<256;resnum++){
     if(game->ResourceInfo[restype][resnum].Exists){
       sprintf(filename,"%s/%s.%03d",game->srcdir.c_str(),ResTypeName[restype],resnum);
       extract(filename,restype,resnum);
     }
   }

}

//**********************************************
void ResourcesWin::add_resource()
{

  Q3FileDialog *f = new Q3FileDialog(0,"Add resource",true);

  static const char *types[6] = {"logic*.*","picture*.*","view*.*","sound*.*","All files (*)",NULL};

  const char *filters[6];

  switch(selected){
  case LOGIC:
    filters[0]=types[LOGIC];
    filters[1]=types[PICTURE];
    filters[2]=types[VIEW];
    filters[3]=types[SOUND];
    break;
  case PICTURE:
    filters[1]=types[LOGIC];
    filters[0]=types[PICTURE];
    filters[2]=types[VIEW];
    filters[3]=types[SOUND];
    break;
  case VIEW:
    filters[1]=types[LOGIC];
    filters[2]=types[PICTURE];
    filters[0]=types[VIEW];
    filters[3]=types[SOUND];
    break;
  case SOUND:
    filters[1]=types[LOGIC];
    filters[2]=types[PICTURE];
    filters[3]=types[VIEW];
    filters[0]=types[SOUND];
    break;
  }
  filters[4]=types[4];
  filters[5]=NULL;

  f->setFilters((const char **)filters);
  f->setCaption("Add resource");
  f->setMode(Q3FileDialog::ExistingFile);
  f->setDir(game->srcdir.c_str());
  if ( f->exec() == QDialog::Accepted ) {
    if ( !f->selectedFile().isEmpty() ){
      if(addmenu==NULL)addmenu = new AddResource(0,0,this);
      addmenu->open((char *)f->selectedFile().latin1());
    }
  }


}

//**********************************************
void ResourcesWin::export_resource()
{
  int k = list->currentItem();
  if(k<0) return;
  int i = ResourceIndex[k];

  switch(selected) {
  case SOUND:
    if(game->ReadResource(SOUND,i))
      menu->errmes("Couldn't read sound resource ! ");
    else
      showSaveAsMidi( this, ResourceData.Data );
    break;
  default:
    qWarning("BUG in code. Export not supported for this resource type!");
    break;
  }
}

//**********************************************
AddResource::AddResource( QWidget *parent, const char *nam ,ResourcesWin *res )
    : QWidget( parent, nam )
{

  resources_win = res;
  setCaption("Add resource");
  Q3BoxLayout *box = new Q3VBoxLayout(this,10);

  filename = new QLabel("Filename:",this);
  // filename->setAutoResize(true);  // TODO: REPLACE WITH A LAYOUT!
  box->addWidget(filename);

  Q3BoxLayout *box0 = new Q3HBoxLayout(box,4);

  QLabel *lname = new QLabel("Name of resource:",this);
  box0->addWidget(lname);

  name = new QLabel("",this);
  // name->setAutoResize(true);  // TODO: REPLACE WITH A LAYOUT!
  box0->addWidget(name);

  type = new Q3ButtonGroup(4,Qt::Horizontal,"Type",this);
  type->setExclusive(true);
  QRadioButton *logic = new QRadioButton(type);
  logic->setText("LOGIC");
  logic->setChecked(true);
  restype=0;
  QRadioButton *picture = new QRadioButton(type);
  picture->setText("PICTURE");
  QRadioButton *view = new QRadioButton(type);
  view->setText("VIEW");
  QRadioButton *sound = new QRadioButton(type);
  sound->setText("SOUND");

  connect( type, SIGNAL(clicked(int)), SLOT(select_type(int)) );

  box->addWidget(type);

  Q3BoxLayout *box1 = new Q3HBoxLayout(box,10);

  QLabel *lnumber = new QLabel("Number: [0-255]",this);
  box1->addWidget(lnumber);

  number = new QLineEdit(this);
  number->setMinimumWidth(60);
  connect( number, SIGNAL(textChanged(const QString &)), SLOT(edit_cb(const QString &)) );
  box1->addWidget(number);

  Q3BoxLayout *box2 = new Q3HBoxLayout(box,40);

  QPushButton *ok = new QPushButton("OK",this);
  connect( ok, SIGNAL(clicked()), SLOT(ok_cb()) );
  box2->addWidget(ok);

  QPushButton *cancel = new QPushButton("Cancel",this);
  connect( cancel, SIGNAL(clicked()), SLOT(cancel_cb()) );
  box2->addWidget(cancel);

}

//**********************************************
void AddResource::open(char *file_name)
{
  char *ptr;

  file = string(file_name);
  if((ptr = strrchr(file_name,'/'))==NULL)ptr=file_name;
  else ptr++;
  sprintf(tmp,"Filename: %s",ptr);
  filename->setText(tmp);
  if(!strncasecmp(ptr,"logic",5)){
    restype=LOGIC;
    type->setButton(restype);
  }
  else if(!strncasecmp(ptr,"picture",7)){
    restype=PICTURE;
    type->setButton(restype);
  }
  else if(!strncasecmp(ptr,"view",4)){
    restype=VIEW;
    type->setButton(VIEW);
  }
  else if(!strncasecmp(ptr,"sound",5)){
    restype=SOUND;
    type->setButton(SOUND);
  }

  select_type(restype);
  show();

}

//**********************************************
void AddResource::edit_cb(const QString &str)
{

  int num = atoi((char *)str.latin1());
  sprintf(tmp,"%s.%d",ResTypeName[restype],num);
  name->setText(tmp);

}

//**********************************************
static int load_resource(const char *filename,int restype)
{
  extern TStringList InputLines;
  char *ptr;
  byte b;
  FILE *fptr = fopen(filename,"rb");
  if(fptr==NULL){
    menu->errmes("Can't open file %s ! ",filename);
    return 1;
  }
  struct stat buf;
  fstat(fileno(fptr),&buf);
  int size=buf.st_size;
  if(size >= MaxResourceSize){
    menu->errmes("File %s is too big ! ",filename);
    fclose(fptr);
    return 1;
  }

  if(restype==LOGIC){
    //check if file is binary or ascii
    fread(ResourceData.Data,MIN(size,64),1,fptr);
    for(int i=0;i<MIN(size,64);i++){
      b=ResourceData.Data[i];
      if(b>0x80||(b<0x20&&b!=0x0a&&b!=0x0d&&b!=0x09)){  //file is binary
        fseek(fptr,0,SEEK_SET);
        ResourceData.Size = size;
        fread(ResourceData.Data,ResourceData.Size,1,fptr);
        fclose(fptr);
        return 0;
      }
    }
    //file is ascii - trying to compile
    fseek(fptr,0,SEEK_SET);
    Logic *logic = new Logic();
    InputLines.lfree();
    while(fgets(tmp,1024,fptr)!=NULL){
      if((ptr=strchr(tmp,0x0a)))*ptr=0;
      if((ptr=strchr(tmp,0x0d)))*ptr=0;
      InputLines.add(tmp);
    }
    fclose(fptr);
    int err = logic->compile();
    delete logic;
    if(err)return 1;
   }
  else{
    ResourceData.Size = size;
    fread(ResourceData.Data,ResourceData.Size,1,fptr);
    fclose(fptr);
  }
  return 0;

}

//**********************************************
void AddResource::ok_cb()
{

  QString str = number->text();
  int num = atoi((char *)str.latin1());

  if(num<0||num>255){
    menu->errmes("Resource number must be between 0 and 255 !");
    return ;
  }

  if(game->ResourceInfo[restype][num].Exists){
    sprintf(tmp,"Resource %s.%d already exists. Replace it ?",ResTypeName[restype],num);

    switch( QMessageBox::warning( this, "Add resource", tmp,
                                  "Replace", "Cancel",
                                  0,      // Enter == button 0
                                  1 ) ) { // Escape == button 1
    case 0:
      if(!load_resource(file.c_str(),restype))
        game->AddResource(restype,num);
      break;
    case 1:
      break;
    }
  }
  else{
    if(!load_resource(file.c_str(),restype)){
      game->AddResource(restype,num);
      if( resources_win->selected == restype)
        resources_win->select_resource_type(restype);
    }
  }
  hide();

}

//**********************************************
void AddResource::cancel_cb()
{

  hide();

}

//**********************************************
void AddResource::select_type(int type)
{
  restype = type;

  QString str = number->text();
  int num = atoi((char *)str.latin1());

  sprintf(tmp,"%s.%d",ResTypeName[restype],num);
  name->setText(tmp);

}

