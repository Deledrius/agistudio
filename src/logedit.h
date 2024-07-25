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

#include "logic.h"
#include "resources.h"

#include "ui/ui_textedit.h"
#include "ui/ui_wordsfind.h"


class QCheckBox;
class QCloseEvent;
class QHideEvent;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QResizeEvent;
class QShowEvent;
class QStatusBar;
class QTextEdit;

// Find strings in the editor window
class FindEdit : public QMainWindow, private Ui::WordsFind
{
    Q_OBJECT
public:
    explicit FindEdit(QWidget *parent = 0, const char *name = 0, QTextEdit *edit = 0);
    QTextEdit *editor;
    int curline;
    void focusEditLine()
    {
        lineFind->setFocus();
    }
public slots:
    void find_first_cb();
    void find_next_cb();
    void cancel_cb();
};

class LogicSyntaxHL;
class RoomGen;

// Logic editor
class LogEdit : public QMainWindow, private Ui::TextEditor
{
    Q_OBJECT
public:
    explicit LogEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0, ResourcesWin *res = 0, bool readonly = false);
    FindEdit *findedit;
    ResourcesWin *resources_win;
    RoomGen *roomgen;
    Logic *logic;
    LogicSyntaxHL *syntax_hl;
    std::string filename;
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
    void update_line_num();
    void wrap_lines();
protected:
    int LogicNum;
    int winnum;
    bool changed;
    int open(const std::string &filename);
    void save(const std::string &filename);
    void deinit();
    void delete_file(int num);
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void setNewTitle(const QString &);
};

// A simple text editor
class TextEdit : public QMainWindow, private Ui::TextEditor
{
    Q_OBJECT
public:
    explicit TextEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0);
    FindEdit *findedit;
    std::string filename;
    int open(const std::string &filename);
    void save(const std::string &filename);
    void setPosition(int newpos);
public slots:
    void new_text();
    void clear_all();
    void open();
    void save();
    void save_as();
    void find_cb();
    void find_again();
    void wrap_lines();
protected:
    std::string OutputText;
    int winnum;
    bool changed;
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void deinit();
    void setNewTitle(const QString &);
};


#endif
