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

#include "options.h"
#include "game.h"

#include <stdlib.h>

#include <qapplication.h>
#include <q3filedialog.h>
//Added by qt3to4:
#include <Q3BoxLayout>
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>
#include <QLabel>

Options *options;

//***********************************************
Options::Options( QWidget *parent, const char *name)
    : Q3TabDialog( parent, name)
{

  setCaption("Settings");

  set_general();
  set_directories();
  set_logedit();
  set_interpreter();

  connect( this, SIGNAL( applyButtonPressed() ), SLOT( apply() ) );
  setDefaultButton();
  connect( this, SIGNAL( defaultButtonPressed() ), SLOT( defaults() ) );
  setCancelButton();
  set_settings();

}

//***********************************************
void Options::set_general()
{

  QWidget *general = new QWidget(this);  
  Q3BoxLayout *all = new Q3VBoxLayout(general,10);

  Q3GroupBox *b1 = new Q3GroupBox(2,Horizontal,"",general);

  QLabel *l = new QLabel("Default resource type",b1);
  l->setText("Default resource type");  //to avoid compilation warning
  
  type = new QComboBox(false,b1,"type");
  type->insertItem( "LOGIC" );
  type->insertItem( "PICTURE" );
  type->insertItem( "VIEW" );
  type->insertItem( "SOUND" );

  QLabel *l2 = new QLabel("Picedit style",b1);
  l2->setText("Picedit style");  //to avoid compilation warning

  picstyle = new QComboBox(false,b1,"picstyle");
  picstyle->insertItem( "One window" );
  picstyle->insertItem( "Two windows" );

  all->addWidget(b1);

  Q3ButtonGroup *extract = new Q3ButtonGroup(2,Horizontal,"Extract logic as",general);
  extract->setMaximumSize(200,100);
  extract->setExclusive(true);
  text = new QRadioButton("Text",extract);
  binary = new QRadioButton("Binary",extract);

  all->addWidget(extract);


  addTab(general,"General");

}

//***********************************************
void Options::set_logedit()
{

  QWidget *logedit = new QWidget(this);

  Q3BoxLayout *all = new Q3VBoxLayout(logedit,10);
  messages = new QCheckBox("Show all messages at end (not just unused ones)",logedit);
  all->addWidget(messages);
  elses = new QCheckBox("Show all elses as gotos",logedit);
  all->addWidget(elses);
  special = new QCheckBox("Show special syntax (e.g. v30=4)",logedit);
  all->addWidget(special);
  addTab(logedit,"Logic editor");

}

//***********************************************
void Options::set_directories()
{

  QWidget *dirs = new QWidget(this);
  Q3BoxLayout *all = new Q3VBoxLayout(dirs,10);
  Q3GroupBox *src = new Q3GroupBox(3,Horizontal,"Logic source directory",dirs);
  reldir = new QRadioButton("[Game_dir/]",src);
  connect(reldir,SIGNAL(clicked()),SLOT(set_reldir()));  
  relname = new QLineEdit(src);

  QLabel *l = new QLabel(" ",src); //dummy label to keep table alignment
  l->setText(" ");  //to avoid compilation warning

  absdir = new QRadioButton("Full path",src);
  connect(absdir,SIGNAL(clicked()),SLOT(set_absdir()));  
  absname = new QLineEdit(src);
  QPushButton *browse = new QPushButton("Browse",src);
  connect(browse,SIGNAL(clicked()),SLOT(browse_abs()));

  all->addWidget(src);

  Q3BoxLayout *b1 = new Q3HBoxLayout(all);

  QLabel *lt = new QLabel("Template:",dirs);
  b1->addWidget(lt);

  templatedir = new QLineEdit(dirs);
  b1->addWidget(templatedir);

  QPushButton *browse1 = new QPushButton("Browse",dirs);
  b1->addWidget(browse1);
  connect(browse1,SIGNAL(clicked()),SLOT(browse_template()));

  Q3BoxLayout *b2 = new Q3HBoxLayout(all);

  QLabel *lh = new QLabel("Help:",dirs);
  b2->addWidget(lh);

  helpdir = new QLineEdit(dirs);
  b2->addWidget(helpdir);

  QPushButton *browse2 = new QPushButton("Browse",dirs);
  b2->addWidget(browse2);
  connect(browse2,SIGNAL(clicked()),SLOT(browse_help()));

  addTab(dirs,"Directories");
}

//***********************************************
void Options::set_interpreter()
{

  QWidget *interp = new QWidget(this);  
  Q3BoxLayout *all = new Q3VBoxLayout(interp,10);  
  QLabel *l = new QLabel("Interpreter command line:\n(will be invoked with the\ncurrent directory == game_directory)",interp);
  all->addWidget(l);

  Q3BoxLayout *b1 = new Q3HBoxLayout(all);
  command = new QLineEdit(interp);
  b1->addWidget(command);

  QPushButton *browse = new QPushButton("Browse",interp);  
  browse->setMaximumSize(80,60);
  connect(browse,SIGNAL(clicked()),SLOT(browse_interpreter()));
  b1->addWidget(browse);  
  
  addTab(interp,"Interpreter");

}

//***********************************************
void Options::apply()
{
  
  game->res_default=type->currentItem();
  game->save_logic_as_text=text->isChecked();
  game->show_all_messages=messages->isChecked();
  game->show_elses_as_gotos=elses->isChecked();
  game->show_special_syntax=special->isChecked();
  game->reldir=reldir->isChecked();
  game->command=string((char *)command->text().latin1());
  if(game->reldir){
    game->srcdirname=string((char *)relname->text().latin1());
  }
  else{
    game->srcdirname=string((char *)absname->text().latin1());
  }
  game->templatedir=string((char *)templatedir->text().latin1());
  game->helpdir=string((char *)helpdir->text().latin1());
  game->picstyle=picstyle->currentItem();
  game->save_settings();
  hide();

}

//***********************************************
void Options::defaults()
{

  game->defaults();
  set_settings();

}

//***********************************************
void Options::set_settings()
{

  type->setCurrentItem(game->res_default);
  text->setChecked(game->save_logic_as_text);  
  messages->setChecked(game->show_all_messages);
  elses->setChecked(game->show_elses_as_gotos);
  special->setChecked(game->show_special_syntax);
  reldir->setChecked(game->reldir);
  absdir->setChecked(!(game->reldir));
  command->setText(game->command.c_str());
  if(game->reldir){
    relname->setText(game->srcdirname.c_str());
    absname->clear();
  }
  else{
    absname->setText(game->srcdirname.c_str());
    relname->clear();
  }      
  templatedir->setText(game->templatedir.c_str());
  helpdir->setText(game->helpdir.c_str());
  picstyle->setCurrentItem(game->picstyle);
}

//***********************************************
void Options::browse_abs()
{

  QString s (Q3FileDialog::getExistingDirectory ());
  if(s.isNull())return;
  absname->setText(s);

}

//***********************************************
void Options::browse_template()
{
  
  QString s (Q3FileDialog::getExistingDirectory ());
  if(s.isNull())return;
  templatedir->setText(s);

}

//***********************************************
void Options::browse_help()
{

  QString s (Q3FileDialog::getExistingDirectory ());
  if(s.isNull())return;
  helpdir->setText(s);

}

//***********************************************
void Options::browse_interpreter()
{
  
  QString s (Q3FileDialog::getOpenFileName ());
  if(s.isNull())return;
  command->setText(s);

}

//***********************************************
void Options::set_reldir()
{
  absdir->setChecked(false);

}

//***********************************************
void Options::set_absdir()
{
  reldir->setChecked(false);

}

//***********************************************
