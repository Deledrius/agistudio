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


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>

#include <QCloseEvent>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QBoxLayout>

#include "game.h"
#include "object.h"
#include "objedit.h"
#include "menu.h"


//*****************************************
// Inventory object editor
ObjEdit::ObjEdit(QWidget *parent, const char *name, int win_num)
    : QMainWindow(parent), winnum(win_num), filename(), changed(false),
      CurObject(), objlist(new ObjList())
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    connect(actionNew, &QAction::triggered, this, &ObjEdit::new_file);
    connect(actionOpen, &QAction::triggered, this, &ObjEdit::open_file);
    connect(actionSave, &QAction::triggered, this, &ObjEdit::save_file);
    connect(actionSaveAs, &QAction::triggered, this, &ObjEdit::save_as_file);
    connect(actionClose, &QAction::triggered, this, &ObjEdit::close);

    connect(listWidgetObjects, &QListWidget::itemSelectionChanged, this, &ObjEdit::select_object);
    connect(lineEditObjectName, &QLineEdit::returnPressed, this, &ObjEdit::name_cb);
    connect(pushButtonAdd, &QPushButton::clicked, this, &ObjEdit::add_cb);
    connect(pushButtonDelete, &QPushButton::clicked, this, &ObjEdit::del_cb);
    connect(lineEditRoomNumber, &QLineEdit::returnPressed, this, &ObjEdit::num_cb);
    connect(pushButtonLeft, &QPushButton::clicked, this, &ObjEdit::left_cb);
    connect(pushButtonRight, &QPushButton::clicked, this, &ObjEdit::right_cb);
}

//*****************************************
void ObjEdit::open()
{
    open(QString("%1/OBJECT").arg(game->dir.c_str()).toStdString());
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
        switch (QMessageBox::warning(this, tr("Object Editor"), tr("Save changes to OBJECT file?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel)) {
            case QMessageBox::Save:
                save_file();
                deinit();
                e->accept();
                break;
            case QMessageBox::Discard:
                deinit();
                e->accept();
                break;
            default: // Cancel
                e->ignore();
                break;
        }
    } else {
        deinit();
        e->accept();
    }
}

//*****************************************
void ObjEdit::open(const std::string &name)
{
    int ret = objlist->read(name, false);
    if (ret)
        return;

    filename = name;

    listWidgetObjects->clear();
    for (int i = 0; i < objlist->ItemNames.count(); i++)
        listWidgetObjects->addItem(QString("%1. %2").arg(i).arg(objlist->ItemNames.at(i)));
    listWidgetObjects->show();
    listWidgetObjects->setCurrentItem(0);
    changed = false;
}

//*****************************************
void ObjEdit::open_file()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open Object"), game->dir.c_str(), tr("Object Files (object);;All Files (*)"));
    if (!filename.isNull())
        open(filename.toStdString());
}

//*****************************************
void ObjEdit::save_file()
{
    if (filename == "")
        save_as_file();
    else {
        objlist->save(filename, actionEncrypted->isChecked());
        changed = false;
    }
}

//*****************************************
void ObjEdit::save_as_file()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Object"), filename.c_str(), tr("Object Files (object);;All Files (*)"));
    if (!fileName.isNull()) {
        objlist->save(fileName.toStdString(), actionEncrypted->isChecked());
        changed = false;
    }
}

//*****************************************
void ObjEdit::new_file()
{
    objlist->clear();
    CurObject = 0;
    listWidgetObjects->clear();
    listWidgetObjects->addItem("0. ?");
    listWidgetObjects->setCurrentRow(CurObject);
    changed = false;
}

//*****************************************
void ObjEdit::select_object()
{
    int n = listWidgetObjects->currentRow();
    lineEditObjectName->setText(objlist->ItemNames.at(n));
    lineEditRoomNumber->setText(QString::number(objlist->RoomNum[n]));
    CurObject = n;
}

//*****************************************
void ObjEdit::add_cb()
{
    objlist->ItemNames.append("?");
    CurObject = objlist->ItemNames.count() - 1;
    objlist->RoomNum[CurObject] = 0;
    listWidgetObjects->addItem(QString("%1. ?").arg(QString::number(CurObject)));
    listWidgetObjects->setCurrentRow(CurObject);
    changed = true;
}

//*****************************************
void ObjEdit::del_cb()
{
    objlist->ItemNames.replace(CurObject, "?");
    auto item = listWidgetObjects->item(CurObject);
    item->setText(QString("%1. ?").arg(QString::number(CurObject)));
    changed = true;
}

//*****************************************
void ObjEdit::left_cb()
{
    if (objlist->RoomNum[CurObject] > 0) {
        objlist->RoomNum[CurObject]--;
        lineEditRoomNumber->setText(QString::number(objlist->RoomNum[CurObject]));
        changed = true;
    }
}

//*****************************************
void ObjEdit::right_cb()
{
    if (objlist->RoomNum[CurObject] < 255) {
        objlist->RoomNum[CurObject]++;
        lineEditRoomNumber->setText(QString::number(objlist->RoomNum[CurObject]));
        changed = true;
    }
}

//*****************************************
void ObjEdit::num_cb()
{
    bool ok;
    int k = lineEditRoomNumber->text().toInt(&ok);
    if (ok && (k > 0 && k < 256)) {
        objlist->RoomNum[CurObject] = k;
        changed = true;
    } else
        lineEditRoomNumber->setText(QString::number(objlist->RoomNum[CurObject]));
}

//*****************************************
void ObjEdit::name_cb()
{
    std::string str = lineEditObjectName->text().toStdString();
    objlist->ItemNames.replace(CurObject, str.c_str());
    auto item = listWidgetObjects->item(CurObject);
    item->setText(QString("%1. %2").arg(QString::number(CurObject)).arg(str.c_str()));
    changed = true;
}
