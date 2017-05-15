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
#include "object.h"
#include "objedit.h"
#include "menu.h"

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <QApplication>
#include <QCloseEvent>
#include <QShowEvent>
#include <QLabel>
#include <QHideEvent>


//*****************************************
//Inventory object editor
ObjEdit::ObjEdit(QWidget *parent, const char *name, int win_num)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Object Editor");
    setMinimumSize(400, 400);

    winnum = win_num;
    objlist = new ObjList();

    QMenu *file = new QMenu(this);
    Q_CHECK_PTR(file);
    file->setTitle("&File");
    file->addAction("&New", this, SLOT(new_file()));
    file->addAction("&Open", this, SLOT(open_file()));
    file->addAction("&Save", this, SLOT(save_file()));
    file->addAction("Save &As", this, SLOT(save_as_file()));
    file->addSeparator();
    file->addAction("&Close", this, SLOT(close()));

    options = new QMenu(this);
    Q_CHECK_PTR(options);
    options->setTitle("&Options");
    encrypted = options->addAction("&Encrypted");
    encrypted->setCheckable(true);
    encrypted->setChecked(true);

    QMenuBar *menu = new QMenuBar(this);
    Q_CHECK_PTR(menu);
    menu->addMenu(file);
    menu->addMenu(options);

    QBoxLayout *all =  new QVBoxLayout(this);
    all->setMenuBar(menu);

    list = new QListWidget(this);
    list->setMinimumSize(400, 400);

    connect(list, SIGNAL(itemSelectionChanged()), this, SLOT(select_object()));
    all->addWidget(list);

    objname = new QLineEdit(this);
    connect(objname, SIGNAL(returnPressed()), SLOT(name_cb()));
    all->addWidget(objname);

    QBoxLayout *down =  new QHBoxLayout(this);
    all->addLayout(down);

    add = new QPushButton("&Add", this);
    connect(add, SIGNAL(clicked()), SLOT(add_cb()));
    down->addWidget(add);
    del = new QPushButton("&Delete", this);
    connect(del, SIGNAL(clicked()), SLOT(del_cb()));
    down->addWidget(del);

    QLabel *label = new QLabel("Room no:", this);
    down->addWidget(label);

    num = new QLineEdit(this);
    connect(num, SIGNAL(returnPressed()), SLOT(num_cb()));
    down->addWidget(num);

    left = new QPushButton("<", this);
    connect(left, SIGNAL(clicked()), SLOT(left_cb()));
    down->addWidget(left);
    right = new QPushButton(">", this);
    connect(right, SIGNAL(clicked()), SLOT(right_cb()));
    down->addWidget(right);

    adjustSize();

    filename = "";
    changed = false;
}

//*****************************************
void ObjEdit::open()
{
    sprintf(tmp, "%s/object", game->dir.c_str());
    open(tmp);
    show();
}

//*****************************************
void ObjEdit::deinit()
{
    delete objlist;
    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void ObjEdit::hideEvent(QHideEvent *)
{
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void ObjEdit::showEvent(QShowEvent *)
{
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*****************************************
void ObjEdit::closeEvent(QCloseEvent *e)
{
    if (changed) {
        switch (QMessageBox::warning(this, "ObjEdit",
                                     "Save changes to OBJECT file ?",
                                     "Yes",
                                     "No",
                                     "Cancel",
                                     0, 2)) {
            case 0: // yes
                save_file();
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
    } else {
        deinit();
        e->accept();
    }
}

//*****************************************
void ObjEdit::open(char *name)
{
    int ret = objlist->read(name, false);
    if (ret)
        return;

    filename = name;

    list->clear();
    for (int i = 0; i < objlist->ItemNames.num; i++) {
        sprintf(tmp, "%d. %s", i, objlist->ItemNames.at(i).c_str());
        list->addItem(tmp);
    }
    list->show();
    list->setCurrentItem(0);
    changed = false;
}

//*****************************************
void ObjEdit::open_file()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open Object"), game->dir.c_str(), tr("Object Files (object);;All Files (*)"));
    if (!filename.isNull())
        open(filename.toLatin1().data());
}

//*****************************************
void ObjEdit::save_file()
{
    if (filename == "")
        save_as_file();
    else {
        objlist->save((char *)filename.c_str(), encrypted->isChecked());
        changed = false;
    }
}

//*****************************************
void ObjEdit::save_as_file()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Object"), filename.c_str(), tr("Object Files (object);;All Files (*)"));
    if (!fileName.isNull()) {
        objlist->save(fileName.toLatin1().data(), encrypted->isChecked());
        changed = false;
    }
}

//*****************************************
void ObjEdit::new_file()
{
    objlist->clear();
    CurObject = 0;
    list->clear();
    list->addItem("0. ?");
    list->setCurrentRow(CurObject);
    changed = false;
}

//*****************************************
void ObjEdit::select_object()
{
    int n = list->currentRow();
    objname->setText(objlist->ItemNames.at(n).c_str());
    sprintf(tmp, "%d", objlist->RoomNum[n]);
    num->setText(tmp);
    CurObject = n;
}

//*****************************************
void ObjEdit::add_cb()
{
    objlist->ItemNames.add("?");
    CurObject = objlist->ItemNames.num - 1;
    objlist->RoomNum[CurObject] = 0;
    sprintf(tmp, "%d. ?", CurObject);
    list->addItem(tmp);
    list->setCurrentRow(CurObject);
    changed = true;
}

//*****************************************
void ObjEdit::del_cb()
{
    objlist->ItemNames.replace(CurObject, "?");
    sprintf(tmp, "%d. ?", CurObject);
    auto item = list->item(CurObject);
    item->setText(tmp);
    changed = true;
}

//*****************************************
void ObjEdit::left_cb()
{
    if (objlist->RoomNum[CurObject] > 0) {
        objlist->RoomNum[CurObject]--;
        sprintf(tmp, "%d", objlist->RoomNum[CurObject]);
        num->setText(tmp);
        changed = true;
    }
}

//*****************************************
void ObjEdit::right_cb()
{
    if (objlist->RoomNum[CurObject] < 255) {
        objlist->RoomNum[CurObject]++;
        sprintf(tmp, "%d", objlist->RoomNum[CurObject]);
        num->setText(tmp);
        changed = true;
    }
}

//*****************************************
void ObjEdit::num_cb()
{
    char *str = num->text().toLatin1().data();
    int k = num->text().toInt();
    if (!strcmp(str, "0") || (k > 0 && k < 256)) {
        objlist->RoomNum[CurObject] = k;
        changed = true;
    } else {
        sprintf(tmp, "%d", objlist->RoomNum[CurObject]);
        num->setText(tmp);
    }
}

//*****************************************
void ObjEdit::name_cb()
{
    std::string str = objname->text().toStdString();
    objlist->ItemNames.replace(CurObject, str.c_str());
    sprintf(tmp, "%d. %s", CurObject, str.c_str());
    auto item = list->item(CurObject);
    item->setText(tmp);
    changed = true;
}
