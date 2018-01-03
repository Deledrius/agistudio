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

#ifndef ROOMGEN_H
#define ROOMGEN_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <qnamespace.h>
#include <QMessageBox>
#include <QTextEdit>
#include <QEvent>
#include <QStatusBar>
#include <QCheckBox>

#include "util.h"
#include "wutil.h"
#include "logic.h"


class RoomGenEntry: public QDialog
{
    Q_OBJECT
public:
    RoomGenEntry(QWidget *parent = 0, const char *name = 0);
    QLineEdit *entry_text, *look_text;
};

class RoomGenFirst: public QDialog
{
    Q_OBJECT
public:
    RoomGenFirst(QWidget *parent = 0, const char *name = 0);
    QLineEdit *x, *y;
    QCheckBox *status, *input;
};

class RoomGenPos: public QDialog
{
    Q_OBJECT
public:
    RoomGenPos(QWidget *parent = 0, const char *name = 0);
    QLineEdit *x, *y;
};


class RoomGenMessage: public QDialog
{
    Q_OBJECT
public:
    RoomGenMessage(QWidget *parent = 0, const char *name = 0);
    void name(const char *title, const char *text);
    QLabel *l;
    QLineEdit *message;
};


class RoomGenEdge: public QDialog
{
    Q_OBJECT
public:
    RoomGenEdge(QWidget *parent = 0, const char *name = 0);
    QCheckBox *c_edge[4];
    QCheckBox *m_edge[4];
    QPushButton *b_edge[4];
    RoomGenMessage *message;
    std::string e_mes[4];
public slots:
    void left_message();
    void right_message();
    void bot_message();
    void hor_message();
};


class RoomGen: public QDialog
{
    Q_OBJECT
public:
    RoomGen(QWidget *parent = 0, const char *name = 0);
    RoomGenEntry *room_entry;
    RoomGenFirst *room_first;
    RoomGenPos *ego_advanced;
    RoomGenEdge *edge_advanced;

    QLineEdit *lnum, *pnum, *hnum;
    QCheckBox *draw_ego, *first_room, *inc_def, *gen_comm;
    QLineEdit *from[4], *x[4], *y[4];
    QLineEdit *edge[4];
    QLineEdit *title;
    std::string text;
    bool incomplete_input();
    int rn[4], xn[4], yn[4]; //coming from room rn, position ego at xn,yn
    int en[4];  //edge controls (goto room)
    int ln, pn; //logic number, picture number
    int hn;    //horizon
    int xa, ya; //absolute (unconditional) position
    int x1, y1; //first room ego position
    bool empty_e[4];  //add empty edge controls
    bool display_e[4]; //display edge messages
    std::string e_mes[4]; //edge messages
    std::string entry_mes, look_mes; //room entry & look messages
    bool status, input; //1st room - status bar, player input

    bool bad_int(QLineEdit *w, int *res, int nmin, int nmax, bool ignore, const char *text);
    bool bad_int(int res, int nmin, int nmax, bool ignore, const char *text);
    bool bad_input();

public slots:
    void ego_advanced_cb();
    void edge_advanced_cb();
    void entry_cb();
    void first_cb();
    void ok_cb();
    void lnum_cb();
    void first_room_cb();
};

#endif

