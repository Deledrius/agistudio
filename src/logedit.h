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

#ifndef LOGEDIT_H
#define LOGEDIT_H

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
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>

#include "util.h"
#include "wutil.h"
#include "logic.h"
#include "roomgen.h"
#include "resources.h"

//find string in the editor window
class FindEdit : public QWidget
{
    Q_OBJECT
public:
    FindEdit(QWidget *parent = 0, const char *name = 0, QTextEdit *edit = 0, QStatusBar *s = 0);
    QStatusBar *status;
    QPushButton *find_first, *find_next, *cancel;
    QRadioButton *up, *down, *start, *current;
    QCheckBox *match_whole, *match_case;
    QLineEdit *find_field;
    QTextEdit *editor;
    int curline;
public slots:
    void find_first_cb();
    void find_next_cb();
    void cancel_cb();
};

class LogicSyntaxHL;

//Logic editor
class LogEdit : public QWidget
{
    Q_OBJECT
public:
    LogEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0, ResourcesWin *res = 0, bool readonly = false);
    QTextEdit *editor;
    FindEdit *findedit;
    ResourcesWin *resources_win;
    QStatusBar *status;
    RoomGen *roomgen;
    Logic *logic;
    LogicSyntaxHL *syntax_hl;
    std::string filename;
    unsigned int maxcol;
    int open();
    int open(int ResNum);
public slots:
    void new_room();
    void read_logic();
    void save_logic();
    void save_as();
    int compile_logic();
    int compile_all_logic();
    void compile_and_run();
    void change_logic_number();
    void delete_logic();
    void clear_all();
    void find_cb();
    void find_again();
    void goto_cb();
    void context_help();
    void command_help();
    void update_line_num(int para, int pos);
protected:
    int LogicNum;
    int winnum;
    bool changed;
    int open(char *filename);
    void save(char *filename);
    void deinit();
    void delete_file(int num);
    void getmaxcol();
    void resizeEvent(QResizeEvent *);
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
};

//a simple text editor
class TextEdit : public QWidget
{
    Q_OBJECT
public:
    TextEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0);
    QTextEdit *editor;
    FindEdit *findedit;
    QStatusBar *status;
    std::string filename;
    int open(char *filename);
    void save(const char *filename);
public slots:
    void new_text();
    void clear_all();
    void open();
    void save();
    void save_as();
    void find_cb();
    void find_again();
protected:
    std::string OutputText;
    int winnum;
    bool changed;
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void deinit();
};


#endif
