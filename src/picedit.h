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

#include <qwidget.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qnamespace.h>
#include <qmessagebox.h>
#include <q3multilineedit.h> 
#include <qevent.h> 
#include <q3scrollview.h> 
#include <qpixmap.h>
#include <qimage.h>
#include <qstatusbar.h>
#include <qcheckbox.h>
#include <qpaintdevice.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QShowEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QHideEvent>
#include <QKeyEvent>

#include "util.h"
#include "wutil.h"
#include "picture.h"
#include "resources.h"

class PicEdit;

class Palette1 : public QWidget
{
    Q_OBJECT
public:
    Palette1( QWidget *parent=0, const char *name=0, PicEdit *w=0);
    int left,right;
 protected:
    PicEdit *picedit;

    void paintEvent(class QPaintEvent *);
    void mousePressEvent(QMouseEvent* event);
};

//************************************************
class PCanvas : public Q3ScrollView
{
    Q_OBJECT
public:
    PCanvas( QWidget *parent=0, const char *name=0, PicEdit *w=0);
    int pixsize;
    int cur_w,cur_h;
    bool bg_loaded,bg_on;
    bool linedraw;
    bool pri_lines;
    int x0,y0,x1,y1;
    void line(bool mode);
    void load_bg(char *filename);
    void draw(int ResNum);
    void update();
    void setSize(int w,int h);
    void setPixsize(int pixsize);
  protected:
    int CurColor;
    Picture *picture;
    QPixmap pixmap;
    QImage bgpix;
    PicEdit *picedit;
    void closeEvent( QCloseEvent *e );
    void showEvent( QShowEvent *);
    void hideEvent( QHideEvent *);
    void keyPressEvent( QKeyEvent * );
    void viewportMousePressEvent(QMouseEvent* e);
    void viewportMouseMoveEvent(QMouseEvent* e);
    void drawContents ( QPainter * p, int clipx, int clipy, int clipw, int cliph ) ;
    bool focusNextPrevChild ( bool next ) ;    
};

//************************************************
class ViewData: public QWidget
{
    Q_OBJECT
public:
    ViewData( QWidget *parent=0, const char *name=0,Picture *p=0);
public slots:
    void read();
 protected:
    QCheckBox *comments,*wrap;
    Q3MultiLineEdit *codes;
    TStringList data;
    Picture *picture;
    int maxcol;

    void getmaxcol();
    void resizeEvent( QResizeEvent * );
    void KeyPressEvent( QKeyEvent * );
};

//************************************************
class PicEdit : public QWidget
{
    Q_OBJECT
public:
    PicEdit( QWidget *parent=0, const char *name=0,int winnum=0,ResourcesWin *res=0);
    void open(int ResNum);
    Picture *picture;
    Q3ButtonGroup *tool;
    QRadioButton *line,*step,*pen,*fill,*brush;
    QRadioButton *pic,*pri;
    QStatusBar *status;
    QWidget *pricolor;
    QCheckBox *bg,*prilines;
    ResourcesWin *resources_win;
    bool changed;
    bool closing,hiding,showing;
    int pri_mode;
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
    void change_tool(int);
    void change_size(int);
    void change_shape(int);
    void change_type(int);

    void home_cb();
    void end_cb();
    void left_cb();
    void right_cb();
    void del_cb();
    void wipe_cb();

    void set_pos();
    void show_pos();

    void editor_help();

 protected:
    int PicNum;
    int winnum;
    PCanvas *canvas;
    Palette1 *palette;
    QLineEdit *pos,*codeline,*comments;
    ViewData *viewdata;
    void open(char *filename);
    void save(char *filename);
    void deinit();
    void closeEvent( QCloseEvent *e );
    void showEvent( QShowEvent *);
    void hideEvent( QHideEvent *);
    void update_palette();
    void update_tools();
    bool focusNextPrevChild ( bool next ) ;    
};


#endif


