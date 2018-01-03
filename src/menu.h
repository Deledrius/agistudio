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

#ifndef MENU_H
#define MENU_H

#include <stdarg.h>
#include <string>
#include <QWidget>
#include <QMenuBar>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QRadioButton>
#include <QMainWindow>
#include <QStatusBar>
#include <QCloseEvent>

#include "wordsedit.h"
#include "objedit.h"
#include "viewedit.h"
#include "logedit.h"
#include "picedit.h"
#include "resources.h"
#include "helpwindow.h"

class WindowList : public QWidget
{
    Q_OBJECT
public:
    WindowList(QWidget *parent = 0, const char *name = 0);
    QListWidget *win;
public slots:
    void draw();
    void select_cb(int);
    void select_ok();
    void del_cb();
};


class About: public QWidget
{
    Q_OBJECT
public:
    About(QWidget *parent = 0, const char *name = 0);
};


class Menu : public QMainWindow
{
    Q_OBJECT
public:
    Menu(QWidget *parent = 0, const char *name = 0);
    QStatusBar *status;
    ResourcesWin *resources_win;
    void errmes(const char *, const char *, ...);
    void errmes(const char *, ...);
    void warnmes(const char *, ...);

    void enable(void);
    void disable(void);
    void show_resources();
    void enable_resources();
    void disable_resources();
    void inc_res(ResourcesWin *res);
    void dec_res();
    bool templ;

public slots:
    void open_game(void);
    void close_game(void);
    void quit_cb(void);
    void run_game(void);
    void settings(void);

    void from_template(void);
    void blank(void);

    void add_resource(void);
    void extract_resource(void);
    void delete_resource(void);
    void renumber_resource(void);
    void rebuild_vol(void);
    void recompile_all(void);
    void new_resource_window();

    void view_editor(void);
    void logic_editor(void);
    void text_editor(void);
    void object_editor(void);
    void words_editor(void);
    void picture_editor(void);
    void sound_player(void);

    void help_contents(void);
    void help_index(void);
    bool help_topic(const QString &topic);
    void about_it(void);
    void about_qt(void);
    void closeEvent(QCloseEvent *e);

    int save_all(void);
    void save_and_run(void);
    void window_list_cb(void);
protected:
    QMenuBar *menubar;
    QMessageBox *err, *warn;
    QPushButton *create;
    QFileDialog *f;
    QAction *open, *close_, *run, *view, *logic, *text, *obj, *words, *pic;
    int num_res;
    int n_res;
    QAction *id[24];
    int max_disabled;
};

extern Menu *menu;

typedef struct {
    union {
        LogEdit *l;
        ViewEdit *v;
        WordsEdit *w;
        ObjEdit *o;
        PicEdit *p;
        TextEdit *t;
        ResourcesWin *r;
        Preview *pr;
        HelpWindow *h;
    } w;
    int type;
} WinList;

#define MAXWIN 64
extern WinList winlist[MAXWIN];
extern int get_win();
extern WindowList *window_list;

void OpenGameDir(QWidget *parent = 0, bool newgame = false);

#endif  //MENU_H
