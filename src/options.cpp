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

#include <QApplication>
#include <QFileDialog>
#include <QGroupBox>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

Options *options;

//***********************************************
Options::Options(QWidget *parent, const char *name)
    : QDialog(parent)
{
    setWindowTitle("Settings");

    tabs = new QTabWidget(this);

    set_general();
    set_directories();
    set_logedit();
    set_interpreter();

    QVBoxLayout *dlgLayout = new QVBoxLayout(this);
    bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, SIGNAL(accepted()), SLOT(apply()));
    connect(bb, SIGNAL(rejected()), SLOT(defaults()));

    dlgLayout->addWidget(tabs);
    dlgLayout->addWidget(bb);
    setLayout(dlgLayout);

    set_settings();
}

//***********************************************
void Options::set_general()
{
    QWidget *general = new QWidget(this);

    QBoxLayout *all = new QVBoxLayout(general);

    QLabel *l = new QLabel("Default resource type:", general);
    all->addWidget(l);
    type = new QComboBox(general);
    type->addItem("LOGIC");
    type->addItem("PICTURE");
    type->addItem("VIEW");
    type->addItem("SOUND");
    all->addWidget(type);

    QLabel *l2 = new QLabel("Picedit Style:", general);
    all->addWidget(l2);
    picstyle = new QComboBox(general);
    picstyle->addItem("One window");
    picstyle->addItem("Two windows");
    all->addWidget(picstyle);

    QHBoxLayout *extractbox = new QHBoxLayout();
    extractbox->setAlignment(Qt::AlignLeft);
    QGroupBox *extract = new QGroupBox(tr("Extract logic as:"), general);
    text = new QRadioButton(tr("Text"), extract);
    binary = new QRadioButton(tr("Binary"), extract);
    extractbox->addWidget(text);
    extractbox->addWidget(binary);
    all->addWidget(extract);
    extract->setLayout(extractbox);

    tabs->addTab(general, "General");
}

//***********************************************
void Options::set_logedit()
{
    QWidget *logedit = new QWidget(this);

    QBoxLayout *all = new QVBoxLayout(logedit);
    messages = new QCheckBox("Show all messages at end (not just unused ones)", logedit);
    all->addWidget(messages);
    elses = new QCheckBox("Show all elses as gotos", logedit);
    all->addWidget(elses);
    special = new QCheckBox("Show special syntax (e.g. v30=4)", logedit);
    all->addWidget(special);

    all->setAlignment(Qt::AlignTop);

    tabs->addTab(logedit, "Logic editor");
}

//***********************************************
void Options::set_directories()
{
    QWidget *dirs = new QWidget(this);

    QVBoxLayout *all = new QVBoxLayout(dirs);
    dirs->setLayout(all);

    QGroupBox *src = new QGroupBox(tr("Logic source directory"), dirs);
    QGridLayout *s1 = new QGridLayout(this);;
    src->setLayout(s1);

    reldir = new QRadioButton("[Game_dir/]", src);
    connect(reldir, SIGNAL(clicked()), SLOT(set_reldir()));
    relname = new QLineEdit(src);
    s1->addWidget(reldir, 0, 0);
    s1->addWidget(relname, 0, 1);

    absdir = new QRadioButton("Full path", src);
    connect(absdir, SIGNAL(clicked()), SLOT(set_absdir()));
    absname = new QLineEdit(src);
    QPushButton *browse = new QPushButton("Browse", src);
    connect(browse, SIGNAL(clicked()), SLOT(browse_abs()));
    s1->addWidget(absdir, 1, 0);
    s1->addWidget(absname, 1, 1);
    s1->addWidget(browse, 1, 2);

    all->addWidget(src);
    all->addWidget(dirs);

    QBoxLayout *b1 = new QHBoxLayout(dirs);

    QLabel *lt = new QLabel("Template:", dirs);
    b1->addWidget(lt);

    templatedir = new QLineEdit(dirs);
    b1->addWidget(templatedir);

    QPushButton *browse1 = new QPushButton("Browse", dirs);
    b1->addWidget(browse1);
    connect(browse1, SIGNAL(clicked()), SLOT(browse_template()));
    all->addLayout(b1);

    QBoxLayout *b2 = new QHBoxLayout(dirs);

    QLabel *lh = new QLabel("Help:", dirs);
    b2->addWidget(lh);

    helpdir = new QLineEdit(dirs);
    b2->addWidget(helpdir);

    QPushButton *browse2 = new QPushButton("Browse", dirs);
    b2->addWidget(browse2);
    connect(browse2, SIGNAL(clicked()), SLOT(browse_help()));
    all->addLayout(b2);

    tabs->addTab(dirs, "Directories");
}

//***********************************************
void Options::set_interpreter()
{
    QWidget *interp = new QWidget(this);

    QBoxLayout *all = new QVBoxLayout(interp);
    QLabel *l = new QLabel("Interpreter command line:\n(will be invoked with the current directory == game_directory)", interp);
    all->addWidget(l);

    QBoxLayout *b1 = new QHBoxLayout(interp);
    command = new QLineEdit(interp);
    b1->addWidget(command);

    QPushButton *browse = new QPushButton("Browse", interp);
    connect(browse, SIGNAL(clicked()), SLOT(browse_interpreter()));
    b1->addWidget(browse);

    all->addLayout(b1);

    all->setAlignment(Qt::AlignTop);

    tabs->addTab(interp, "Interpreter");
}

//***********************************************
void Options::apply()
{
    game->res_default = type->currentIndex();
    game->save_logic_as_text = text->isChecked();
    game->show_all_messages = messages->isChecked();
    game->show_elses_as_gotos = elses->isChecked();
    game->show_special_syntax = special->isChecked();
    game->reldir = reldir->isChecked();
    game->command = command->text().toStdString();
    if (game->reldir)
        game->srcdirname = relname->text().toStdString();
    else
        game->srcdirname = absname->text().toStdString();
    game->templatedir = templatedir->text().toStdString();
    game->helpdir = helpdir->text().toStdString();
    game->picstyle = picstyle->currentIndex();
    game->save_settings();
    hide();
}

//***********************************************
void Options::defaults()
{
    game->defaults();
    set_settings();
    hide();
}

//***********************************************
void Options::set_settings()
{
    type->setCurrentIndex(game->res_default);
    text->setChecked(game->save_logic_as_text);
    messages->setChecked(game->show_all_messages);
    elses->setChecked(game->show_elses_as_gotos);
    special->setChecked(game->show_special_syntax);
    reldir->setChecked(game->reldir);
    absdir->setChecked(!(game->reldir));
    command->setText(game->command.c_str());
    if (game->reldir) {
        relname->setText(game->srcdirname.c_str());
        absname->clear();
    } else {
        absname->setText(game->srcdirname.c_str());
        relname->clear();
    }
    templatedir->setText(game->templatedir.c_str());
    helpdir->setText(game->helpdir.c_str());
    picstyle->setCurrentIndex(game->picstyle);
}

//***********************************************
void Options::browse_abs()
{
    QString s(QFileDialog::getExistingDirectory());
    if (s.isNull())
        return;
    absname->setText(s);
}

//***********************************************
void Options::browse_template()
{
    QString s(QFileDialog::getExistingDirectory());
    if (s.isNull())
        return;
    templatedir->setText(s);
}

//***********************************************
void Options::browse_help()
{
    QString s(QFileDialog::getExistingDirectory());
    if (s.isNull())
        return;
    helpdir->setText(s);
}

//***********************************************
void Options::browse_interpreter()
{
    QString s(QFileDialog::getOpenFileName());
    if (s.isNull())
        return;
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
