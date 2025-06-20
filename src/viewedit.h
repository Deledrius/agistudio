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

#ifndef VIEWEDIT_H
#define VIEWEDIT_H


#include <QPixmap>
#include <QScrollArea>
#include <QWidget>

#include "view.h"

#include "ui/ui_viewedit.h"


#define V_DRAW 0
#define V_FILL 1

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPaintEvent;
class QPushButton;
class QRadioButton;
class QTextEdit;

class Palette;
class Preview;
class ResourcesWin;
class ViewEdit;

//********************************************************
class ViewIcon : public QWidget
//small non-editable picture in the description window
{
    Q_OBJECT
public:
    ViewIcon(QWidget *parent = 0, const char *name = 0, ViewEdit *v = 0);
protected:
    ViewEdit *viewedit;
    void paintEvent(QPaintEvent *);
};

//********************************************************
class Canvas : public QScrollArea
// View drawing area
{
    Q_OBJECT
public:
    Canvas(QWidget *parent = 0, const char *name = 0, ViewEdit *v = 0);
    int x0, y0;
    int pixsize;
    int cur_w, cur_h;
    void DrawCel(int w, int h, byte *data, bool mirror);
    void DrawCel(int w, int h, byte *data, bool mirror, int pixsize);
    void setSize(int w, int h);
    void UpdateCel(int x, int y);
protected:
    int CurColor;
    QLabel *imagecontainer;
    QPixmap pixmap;
    bool cur_mirror;
    byte *data;
    ViewEdit *viewedit;
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void drawContents(QPainter *p, int, int, int, int) const;
    bool focusNextPrevChild(bool next) ;
};

//********************************************************
class Description: public QWidget
{
    Q_OBJECT
public:
    Description(QWidget *parent = 0, const char *name = 0, ViewEdit *v = 0);
public slots:
    void set();
    void ok_cb();
    void cancel_cb();
    void getmaxcol();
protected:
    ViewIcon *smallview;
    ViewEdit *viewedit;
    QTextEdit *desc;
    unsigned int maxcol;
    void resizeEvent(QResizeEvent *);
};

//********************************************************
class Animate: public QWidget
{
    Q_OBJECT
public:
    Animate(QWidget *parent = 0, const char *name = 0, Preview *p = 0, ViewEdit *v = 0);
    QTimer *timer;
public slots:
    void start_stop();
    void next_cel();
    void closeall();
    void fb_cb();
protected:
    QLineEdit *delay;
    QPushButton *button;
    QRadioButton *forward, *backward;
    int num;
    ViewEdit *viewedit;
    Preview *preview;
    bool fwd;
};

//********************************************************
class ViewEdit : public QMainWindow, private Ui::ViewEdit
{
    Q_OBJECT
public:
    explicit ViewEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0, ResourcesWin *res = 0);
    Description *description;
    Palette *palette;
    View *view;
    ResourcesWin *resources_win;
    int drawing_mode;
    bool changed;
    void open(int ResNum);
    void fillCel(int x, int y, byte color);
    void change_mode1(int);

    void open();
    void open_file();
    void save_file();
    void save_to_game();
    void save_to_game_as();

    void next_cel_cycle();
    void prev_cel_cycle();

protected:
    void delete_view();

    void undo_cel();
    void copy_to_clipboard();
    void paste_from_clipboard();

    void next_loop();
    void previous_loop();
    void first_loop();
    void last_loop();
    void insert_loop_before();
    void insert_loop_after();
    void append_loop();
    void delete_loop();
    void clear_loop();

    void change_mirror(int i);

    void next_cel();
    void previous_cel();
    void first_cel();
    void last_cel();
    void insert_cel_before();
    void insert_cel_after();
    void append_cel();
    void delete_cel();
    void clear_cel();
    void flipv_cel();
    void fliph_cel();
    void load_cel();

    void set_transcolor();
    void change_mode(int);
    void is_descriptor_cb();
    void show_description();

    void shift_right();
    void shift_left();
    void shift_up();
    void shift_down();

    void zoom_minus();
    void zoom_plus();

    void change_width_height();
    void inc_width();
    void dec_width();
    void inc_height();
    void dec_height();

    void animate_cb();

    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void keyPressEvent(QKeyEvent *);
    void open(const std::string &filename);
    void save(const std::string &filename);
    void deinit();
    void display();
    void DisplayView();
    void DisplayView(int pixsize);
    void set_transcolor(int col);
    void showmirror();
    void showlooppar();
    void showcelpar();
    int curIndex() const;
    void saveundo();
    bool focusNextPrevChild(bool next);
    void show_help();

    Canvas *canvas;
    Animate *animate;
    int ViewNum;
    int transcol;
    Cel undoCel;
    bool undo;
    int winnum;
};

#endif
