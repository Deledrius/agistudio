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

#ifndef OBJEDIT_H
#define OBJEDIT_H


#include <QWidget>

#include "object.h"

#include "ui/ui_objedit.h"


class QAction;
class QCloseEvent;
class QHideEvent;
class QListWidget;
class QLineEdit;
class QMenu;
class QPushButton;
class QShowEvent;

//inventory objects editor
class ObjEdit : public QMainWindow, private Ui::ObjEdit
{
    Q_OBJECT
public:
    explicit ObjEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0);
    void open();
public slots:
    void open_file();
    void save_file();
    void save_as_file();
    void new_file();
    void select_object();
    void add_cb();
    void del_cb();
    void left_cb();
    void right_cb();
    void num_cb();
    void name_cb();
protected:
    int winnum;
    int CurObject;
    bool changed;
    std::string filename;
    ObjList *objlist;

    void open(const std::string &);
    void deinit();
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
};

#endif
