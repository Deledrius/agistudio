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

#ifndef PREVIEW_H
#define PREVIEW_H

#include "picture.h"
#include "view.h"
#include "resources.h"

#include <string>
#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QPixmap>
#include <QTextEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>
#include <QPaintEvent>

class Animate;
class Preview;

//****************************************************
class PreviewView : public QWidget
{
    Q_OBJECT
public:
    PreviewView(QWidget *parent = 0, const char *name = 0, Preview *p = 0);
    Preview *preview;
    View *view;
    QPixmap pixmap;
    int cur_w, cur_h;
    int pixsize;
    void draw(int ResNum);
    void update();
    void show_description();
protected:
    void paintEvent(QPaintEvent *);
};

//****************************************************
class PreviewPicture : public QWidget
{
    Q_OBJECT
public:
    PreviewPicture(QWidget *parent = 0, const char *name = 0, Preview *p = 0);
    Preview *preview;
    BPicture *ppicture;
    QPixmap pixmap;
    int drawing_mode;
    void draw(int ResNum);
    void update();
protected:
    void paintEvent(QPaintEvent *);

};

class ResourcesWin;
class LogEdit;

//****************************************************
class Preview : public QStackedWidget
{
    Q_OBJECT
public:
    Preview(QWidget *parent = 0, const char  *name = 0, ResourcesWin *res = 0);
    QTextEdit *description;
    ResourcesWin *resources_win;
    void open(int i, int type);
public slots:
    void double_click();
    void change_mode();
    void previous_loop(void);
    void next_loop(void);
    void previous_cel(void);
    void next_cel(void);
    void prev_cel_cycle(void);
    void next_cel_cycle(void);
    void showlooppar();
    void showcelpar();
    void save_image(QPixmap &img, const char *format);
    void save_pic();
    void save_view();
    void export_resource();
    void animate_cb();
protected:
    QComboBox *formats_pic, *formats_view;
    QWidget *w_logic, *w_sound;
    QWidget *w_picture;

    LogEdit *p_logic;
    PreviewPicture *p_picture;
    QRadioButton *visual, *priority;
    QPushButton *save_pic_butt, *save_view_butt;
    QWidget *w_view;
    PreviewView *p_view;
    QPushButton *loopleft, *loopright, *celleft, *celright;
    QLabel *loopnum, *celnum;
    Animate *animate;
    void deinit();
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
};


#endif
