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


#include <QMainWindow>
#include <QWidget>

#include "ui/ui_mainmenu.h"


class QActionGroup;
class QFileDialog;
class QListWidget;
class QMessageBox;

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


class About : public QWidget
{
    Q_OBJECT
public:
    About(QWidget *parent = 0, const char *name = 0);
};


class ResourcesWin;
class Menu : public QMainWindow, private Ui::Menu
{
    Q_OBJECT
public:
    Menu(QWidget *parent = 0, const char *name = 0);
    ResourcesWin *resources_win;
    void showStatusMessage(const QString &msg);
    void errmes(const char *, const char *, ...);
    void errmes(const char *, ...);
    void warnmes(const char *, ...);

    void enable_game_actions(void);
    void disable_game_actions(void);
    void show_resources();
    void enable_resources();
    void disable_resources();
    void inc_res(ResourcesWin *res);
    void dec_res();
    bool use_template;

    void open_game(void);
    void close_game(void);
    void quit_cb(void);
    void run_game(void);
    void options(void);

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
    QMessageBox *err, *warn;
    QActionGroup *activeGameGroup, *resActionGroup;
    int num_res;
};

extern Menu *menu;

class LogEdit;
class ViewEdit;
class WordsEdit;
class ObjEdit;
class PicEdit;
class TextEdit;
class Preview;
class HelpWindow;

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
