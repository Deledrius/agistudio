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

#ifndef PICEDIT_H
#define PICEDIT_H


#include <QMainWindow>
#include <QImage>
#include <QPixmap>
#include <QScrollArea>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ui/ui_picedit.h"


class QPaintEvent;
class QMouseEvent;
class QKeyEvent;
class QResizeEvent;
class QLabel;
class QTextEdit;
class QCheckBox;

class PicEdit;
class Picture;
class PCanvas;
class ResourcesWin;

class Palette1 : public QWidget
{
    Q_OBJECT
public:
    Palette1(QWidget *parent = 0, const char *name = 0, PicEdit *w = 0);
    int left, right;
protected:
    PicEdit *picedit;

    void paintEvent(class QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
};

//************************************************
class PCanvas : public QScrollArea
{
    Q_OBJECT
public:
    PCanvas(QWidget *parent = 0, const char *name = 0, PicEdit *w = 0);
    int pixsize;
    int cur_w, cur_h;
    bool bg_loaded, bg_on;
    bool linedraw;
    bool pri_lines;
    int x0, y0, x1, y1;
    void line(bool mode);
    void load_bg(QString &filename);
    void draw(int ResNum);
    void update();
    void setSize(int w, int h);
    void setPixsize(int pixsize);
protected:
    int CurColor;
    Picture *picture;
    QLabel *imagecontainer;
    QPixmap pixmap;
    QImage bgpix;
    PicEdit *picedit;
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void keyPressEvent(QKeyEvent *);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void drawContents(QPainter *p, int clipx, int clipy, int clipw, int cliph) const;
    bool focusNextPrevChild(bool next) ;
};

//************************************************
class ViewData: public QWidget
{
    Q_OBJECT
public:
    ViewData(QWidget *parent = 0, const char *name = 0, Picture *p = 0);
public slots:
    void read();
protected:
    QCheckBox *comments, *wrap;
    QTextEdit *codes;
    QStringList data;
    Picture *picture;
    int maxcol;

    void getmaxcol();
    void resizeEvent(QResizeEvent *);
    void KeyPressEvent(QKeyEvent *);
};

//************************************************
class PicEdit : public QMainWindow, private Ui::PicEdit
{
    Q_OBJECT
public:
    explicit PicEdit(QWidget *parent = nullptr, const char *name = 0, int winnum = 0, ResourcesWin *res = 0);

    void open(int ResNum);
    ResourcesWin *resources_win;
public slots:
    void open();
    void open_file();
    void save_file();
    void save_to_game();
    void save_to_game_as();
    void delete_picture();

    void view_data();
    void background();
    void zoom_minus();
    void zoom_plus();
    void change_drawmode(int);
    void toggle_bgmode(bool);
    void toggle_prilinemode(bool);
    void change_tool(int);
    void change_size(int);
    void change_shape(int);
    void change_type(int);
    void deselect_tool();
    void enable_background(bool mode);
    bool background_enabled() const;

    void on_startButton_clicked();
    void on_endButton_clicked();
    void on_prevButton_clicked();
    void on_nextButton_clicked();
    void on_delButton_clicked();
    void on_wipeButton_clicked();

    void set_pos();
    void show_pos();

    void editor_help();

protected:
    int PicNum;
    int winnum;
    PCanvas *canvas;
    Palette1 *palette;
    ViewData *viewdata;
    Picture *picture;
    QFrame *pricolor;
    bool changed;
    bool closing, hiding, showing;
    int pri_mode;
    void open(const std::string &filename);
    void save(const std::string &filename);
    void deinit();
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void update_palette();
    void update_tools();
    bool focusNextPrevChild(bool next) ;

    friend PCanvas;
    friend Palette1;
};


#endif


