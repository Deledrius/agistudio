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

#include "game.h"
#include "menu.h"
#include "wutil.h"
#include "picture.h"
#include "preview.h"

#include <cstdlib>

#include <QSpinBox>
#include <QApplication>
#include <QPainter>
#include <QTooltip>
#include <QKeyEvent>
#include <QLabel>
#include <QPixmap>
#include <QHideEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QPaintEvent>
#include <QCloseEvent>
#include <QGroupBox>

#include "left1_x.xpm"
#include "left2_x.xpm"
#include "right1_x.xpm"
#include "right2_x.xpm"
#include "zoom_minus_x.xpm"
#include "zoom_plus_x.xpm"

static const char *comment[] = {
    "pic colour",   //0xf0
    "pic off",      //0xf1
    "pri colour",   //0xf2
    "pri off",      //0xf3,
    "Y corner",     //0xf4,
    "X corner",     //0xf5,
    "abs line",     //0xf6,
    "rel line",     //0xf7,
    "fill",         //0xf8,
    "pattern",      //0xf9,
    "brush",        //0xfa,
    " ",            //0xfb,
    " ",            //0xfc,
    " ",            //0xfd,
    " ",            //0xfe,
    "end",          //0xff
};

static const char *colname[] = {
    "black",
    "blue",
    "green",
    "cyan",
    "red",
    "magenta",
    "brown",
    "white",
    "gray",
    "lightblue",
    "lightgreen",
    "lightcyan",
    "lightred",
    "lightmagenta",
    "yellow",
    "brightwhite"
};


//***************************************
PicEdit::PicEdit(QWidget *parent, const char *name, int win_num, ResourcesWin *res)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Picture Editor");

    winnum = win_num;
    resources_win = res;
    picture = new Picture();

    QMenu *file = new QMenu(this);
    Q_CHECK_PTR(file);
    file->setTitle("&File");
    file->addAction("&New", this, SLOT(open()));
    file->addAction("&Load from file", this, SLOT(open_file()));
    file->addAction("&Save to game", this, SLOT(save_to_game()));
    file->addAction("Save to game &as...", this, SLOT(save_to_game_as()));
    file->addAction("Save to file", this, SLOT(save_file()));
    file->addSeparator();
    file->addAction("&Delete", this, SLOT(delete_picture()));
    file->addSeparator();
    file->addAction("&Close", this, SLOT(close()));

    QMenu *util = new QMenu(this);
    Q_CHECK_PTR(util);
    util->setTitle("&Utilities");
    util->addAction("&View data", this, SLOT(view_data()));
    util->addAction("&Background", this, SLOT(background()));

    QMenu *help = new QMenu(this);
    Q_CHECK_PTR(help);
    help->setTitle("&Help");
    help->addAction("Help on picture &editor", this, SLOT(editor_help()));

    QMenuBar *menu = new QMenuBar(this);
    Q_CHECK_PTR(menu);
    menu->addMenu(file);
    menu->addMenu(util);
    menu->addMenu(help);

    QBoxLayout *all =  new QHBoxLayout(this);
    all->setMenuBar(menu);

    QBoxLayout *leftb = new QVBoxLayout(this);
    all->addLayout(leftb);

    QHBoxLayout *toollay = new QHBoxLayout();
    toollay->setAlignment(Qt::AlignLeft);
    QGroupBox *toolbox = new QGroupBox(this);
    tool = new QButtonGroup(this);
    line = new QRadioButton(tr("Line"), toolbox);
    pen = new QRadioButton(tr("Pen"), toolbox);
    step = new QRadioButton(tr("Step"), toolbox);
    fill = new QRadioButton(tr("Fill"), toolbox);
    brush = new QRadioButton(tr("Brush"), toolbox);
    toollay->addWidget(line);
    toollay->addWidget(pen);
    toollay->addWidget(step);
    toollay->addWidget(fill);
    toollay->addWidget(brush);
    tool->addButton(line, T_LINE);
    tool->addButton(pen, T_PEN);
    tool->addButton(step, T_STEP);
    tool->addButton(fill, T_FILL);
    tool->addButton(brush, T_BRUSH);
    leftb->addWidget(toolbox);
    toolbox->setLayout(toollay);
    connect(tool, SIGNAL(buttonClicked(int)), SLOT(change_tool(int)));

    QBoxLayout *b1 = new QHBoxLayout(this);
    leftb->addLayout(b1);

    palette = new Palette1(this, 0, this);
    palette->setMinimumSize(250, 40);
    palette->setMaximumSize(350, 80);
    b1->addWidget(palette);

    QBoxLayout *b2 = new QHBoxLayout(this);
    leftb->addLayout(b2);

    QVBoxLayout *shapebox = new QVBoxLayout();
    QGroupBox *shape = new QGroupBox("Shape", this);
    QRadioButton *circle = new QRadioButton("Circle", shape);
    QRadioButton *square = new QRadioButton("Square", shape);
    circle->setChecked(true);
    shapebox->addWidget(circle);
    shapebox->addWidget(square);
    b2->addWidget(shape);
    shape->setLayout(shapebox);
    connect(shape, SIGNAL(clicked(int)), SLOT(change_shape(int)));


    QVBoxLayout *typebox = new QVBoxLayout();
    QGroupBox *type = new QGroupBox("Type", this);
    QRadioButton *spray = new QRadioButton("Spray", type);
    QRadioButton *solid = new QRadioButton("Solid", type);
    spray->setChecked(true);
    typebox->addWidget(spray);
    typebox->addWidget(solid);
    b2->addWidget(type);
    type->setLayout(typebox);
    connect(type, SIGNAL(clicked(int)), SLOT(change_type(int)));

    QVBoxLayout *sizebox = new QVBoxLayout();
    QGroupBox *lsize = new QGroupBox(tr("Size"), this); //vert
    b2->addWidget(lsize);

    QSpinBox *size = new QSpinBox(lsize);
    size->setMinimum(1);
    size->setMinimum(7);
    size->setValue(1);
    sizebox->addWidget(size);
    lsize->setLayout(sizebox);
    connect(size, SIGNAL(valueChanged(int)), SLOT(change_size(int)));

    QHBoxLayout *b3 = new QHBoxLayout(this);
    leftb->addLayout(b3);

    QPushButton *home = new QPushButton(this);
    home->setIcon(QPixmap(left2_x));
    connect(home, SIGNAL(clicked()), SLOT(home_cb()));
    b3->addWidget(home);
    QPushButton *left = new QPushButton(this);
    left->setIcon(QPixmap(left1_x));
    connect(left, SIGNAL(clicked()), SLOT(left_cb()));
    b3->addWidget(left);
    pos = new QLineEdit(this);
    pos->setMinimumWidth(64);
    connect(pos, SIGNAL(returnPressed()), SLOT(set_pos()));
    b3->addWidget(pos);
    QPushButton *right = new QPushButton(this);
    connect(right, SIGNAL(clicked()), SLOT(right_cb()));
    right->setIcon(QPixmap(right1_x));
    b3->addWidget(right);
    QPushButton *end = new QPushButton(this);
    connect(end, SIGNAL(clicked()), SLOT(end_cb()));
    end->setIcon(QPixmap(right2_x));
    b3->addWidget(end);
    QPushButton *del = new QPushButton("Del", this);
    connect(del, SIGNAL(clicked()), SLOT(del_cb()));
    b3->addWidget(del);
    QPushButton *wipe = new QPushButton("Wipe", this);
    connect(wipe, SIGNAL(clicked()), SLOT(wipe_cb()));
    b3->addWidget(wipe);

    QBoxLayout *b31 = new QHBoxLayout(this);
    leftb->addLayout(b31);

    codeline = new QLineEdit(this);
    codeline->setFocusPolicy(Qt::NoFocus);
    b31->addWidget(codeline);

    comments = new QLineEdit(this);
    comments->setFocusPolicy(Qt::NoFocus);
    b31->addWidget(comments);

    QBoxLayout *b4 = new QHBoxLayout(this);
    leftb->addLayout(b4);

    QPushButton *zoom_minus = new QPushButton(this);
    zoom_minus->setIcon(QPixmap(zoom_minus_x));
    zoom_minus->setFixedSize(32, 32);
    connect(zoom_minus, SIGNAL(clicked()), SLOT(zoom_minus()));
    b4->addWidget(zoom_minus);

    QPushButton *zoom_plus = new QPushButton(this);
    zoom_plus->setIcon(QPixmap(zoom_plus_x));
    zoom_plus->setFixedSize(32, 32);
    connect(zoom_plus, SIGNAL(clicked()), SLOT(zoom_plus()));
    b4->addWidget(zoom_plus);

    QVBoxLayout *drawbox = new QVBoxLayout();
    QGroupBox *drawmode = new QGroupBox("Show", this);
    pic = new QRadioButton("Visual", drawmode);
    pri = new QRadioButton("Priority", drawmode);
    bg = new QCheckBox("Background", drawmode);
    prilines = new QCheckBox("PriorityLines", drawmode);
    drawbox->addWidget(pic);
    drawbox->addWidget(pri);
    drawbox->addWidget(bg);
    drawbox->addWidget(prilines);
    pic->setChecked(true);
    pri_mode = false;
    picture->set_mode(0);
    b4->addWidget(drawmode);
    drawmode->setLayout(drawbox);
    connect(drawmode, SIGNAL(clicked(int)), SLOT(change_drawmode(int)));

    status = new QStatusBar(this);
    QLabel *msg = new QLabel(status);
    status->addWidget(msg, 4);
    pricolor = new QFrame(status);
    pricolor->setMinimumSize(8, 8);
    pricolor->setMaximumSize(8, 8);
    pricolor->setToolTip(tr("Priority 'color' required to mask an EGO on this priority level"));
    status->addWidget(pricolor);
    status->setSizeGripEnabled(false);
    leftb->addWidget(status);

    if (game->picstyle == P_TWO) {
        canvas = new PCanvas(0, 0, this);
        canvas->setMinimumSize(canvas->pixsize * MAX_W + canvas->x0 + 10, canvas->pixsize * MAX_HH + canvas->x0 + 10);
        //canvas->resizeContents(canvas->pixsize*MAX_W+canvas->x0,canvas->pixsize*MAX_HH+canvas->x0);
        canvas->resize(canvas->pixsize * MAX_W + canvas->x0, canvas->pixsize * MAX_HH + canvas->x0);

    } else {
        canvas = new PCanvas(this, 0, this);
        canvas->setMinimumSize(canvas->pixsize * MAX_W + canvas->x0 + 10, canvas->pixsize * MAX_HH + canvas->x0 + 10);
        //canvas->resizeContents(canvas->pixsize*MAX_W+canvas->x0,canvas->pixsize*MAX_HH+canvas->x0);
        all->addWidget(canvas, 1);
        setFocusProxy(canvas);
    }

    changed = false;
    adjustSize();
    viewdata = NULL;
    closing = false;
}

//*********************************************
void PicEdit::save(char *filename)
{
    picture->save(filename);
    changed = false;
}

//*********************************************
void PicEdit::open_file()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Picture File"), game->srcdir.c_str(), tr("Picture Files (picture*.*);;All Files (*)"));
    if (!fileName.isNull())
        open(fileName.toLatin1().data());
}

//*********************************************
void PicEdit::open(int ResNum)
{
    if (picture->open(ResNum))
        return;
    PicNum = ResNum;
    sprintf(tmp, "Picture editor: picture.%d", PicNum);
    setWindowTitle(tmp);
    if (canvas->isTopLevel())
        canvas->setWindowTitle(tmp);
    canvas->update();
    show_pos();
    changed = false;
    show();
    canvas->show();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::open(char *filename)
{
    if (picture->open(filename))
        return;
    PicNum = -1;
    sprintf(tmp, "Picture editor");
    setWindowTitle(tmp);
    if (canvas->isTopLevel())
        canvas->setWindowTitle(tmp);
    canvas->update();
    show_pos();
    changed = false;
    show();
    canvas->show();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::save_file()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Picture File"), game->srcdir.c_str(), tr("Picture Files (picture*.*);;All Files (*)"));
    if (!fileName.isNull())
        save(fileName.toLatin1().data());
}

//*********************************************
void PicEdit::deinit()
{
    if (viewdata) {
        viewdata->close();
        viewdata = NULL;
    }
    if (canvas->isTopLevel()) {
        closing = true;
        canvas->close();
    }
    delete picture;
    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void PicEdit::showEvent(QShowEvent *)
{
    showing = true;
    if (canvas->isTopLevel() && !canvas->isVisible())
        canvas->showNormal();
    showing = false;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void PicEdit::hideEvent(QHideEvent *)
{
    hiding = true;
    if (isMinimized() && canvas->isTopLevel() && canvas->isVisible())
        canvas->showMinimized();
    hiding = false;
    if (viewdata) {
        viewdata->close();
        viewdata = NULL;
    }
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void PicEdit::closeEvent(QCloseEvent *e)
{
    if (changed) {
        if (PicNum != -1)
            sprintf(tmp, "Save changes to picture.%d ?", PicNum);
        else
            sprintf(tmp, "Save changes to picture ?");
        strcat(tmp, "\n(picture will be saved to game)");

        switch (QMessageBox::warning(this, "Picture editor",
                                     tmp,
                                     "Yes",
                                     "No",
                                     "Cancel",
                                     0, 2)) {
            case 0: // yes
                save_to_game();
                deinit();
                e->accept();
                break;
            case 1: // no
                deinit();
                e->accept();
                break;
            default: // cancel
                e->ignore();
                break;
        }
    } else {
        deinit();
        e->accept();
    }
}

//*********************************************
void PicEdit::save_to_game()
{
    if (PicNum != -1) {
        picture->save(PicNum);
        if (resources_win) {
            if (resources_win->preview == NULL)
                resources_win->preview = new Preview();
            resources_win->preview->open(PicNum, PICTURE);
        }
        changed = false;
    } else
        save_to_game_as();
}

//*********************************************
void PicEdit::save_to_game_as()
{
    AskNumber *picture_number = new AskNumber(0, 0, "Picture number", "Enter picture number: [0-255]");

    if (!picture_number->exec())
        return;

    QString str = picture_number->num->text();
    int num = str.toInt();

    if (num < 0 || num > 255) {
        menu->errmes("Picture number must be between 0 and 255 !");
        return ;
    }
    if (game->ResourceInfo[PICTURE][num].Exists) {
        sprintf(tmp, "Resource picture.%d already exists. Replace it ?", num);

        switch (QMessageBox::warning(this, "Picture", tmp,
                                     "Replace", "Cancel",
                                     0,      // Enter == button 0
                                     1)) {   // Escape == button 1
            case 0:
                picture->save(num);
                PicNum = num;
                if (resources_win) {
                    if (resources_win->preview == NULL)
                        resources_win->preview = new Preview();
                    resources_win->preview->open(PicNum, PICTURE);
                }
                changed = false;
                break;
            case 1:
                break;
        }
    } else {
        picture->save(num);
        changed = false;
        PicNum = num;
        if (resources_win) {
            resources_win->select_resource_type(PICTURE);
            resources_win->set_current(num);
        }
        open(num);
    }
}

//*********************************************
void PicEdit::delete_picture()
{
    int k;
    if (PicNum == -1)
        return;
    sprintf(tmp, "Really delete picture %d ?", PicNum);
    switch (QMessageBox::warning(this, "Picture", tmp,
                                 "Delete", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            game->DeleteResource(PICTURE, PicNum);
            if (resources_win) {
                k = resources_win->list->currentRow();
                resources_win->select_resource_type(PICTURE);
                resources_win->list->setCurrentRow(k);
            }
            break;
        case 1:
            break;
    }
}

//*********************************************
void PicEdit::open()
{
    picture->newpic();
    show_pos();
    if (canvas->isTopLevel())
        canvas->setWindowTitle("picture");
    canvas->update();
    show();
    canvas->show();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::view_data()
{
    if (viewdata == NULL)
        viewdata = new ViewData(0, 0, picture);
    if (PicNum != -1) {
        sprintf(tmp, "View data: picture %d", PicNum);
        viewdata->setWindowTitle(tmp);
    } else
        viewdata->setWindowTitle("View data: picture");
    viewdata->read();
}

//*********************************************
void PicEdit::background()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Background Image"), game->srcdir.c_str(), tr("All Files (*)"));
    if (!fileName.isNull())
        canvas->load_bg(fileName.toLatin1().data());
}

//*********************************************
void PicEdit::zoom_minus()
{
    if (canvas->pixsize > 1) {
        canvas->setPixsize(canvas->pixsize - 1);
        int w, h;
        w = canvas->cur_w + 4;
        h = canvas->cur_h + 4;
        canvas->resize(w, h);
    }
}

//*********************************************
void PicEdit::zoom_plus()
{
    if (canvas->pixsize < 4) {
        canvas->setPixsize(canvas->pixsize + 1);
        int w, h;
        w = canvas->cur_w + 4;
        h = canvas->cur_h + 4;
        canvas->resize(w, h);
    }
}

//*********************************************
void PicEdit::change_drawmode(int mode)
{
    switch (mode) {
        case 0:  //draw visual
            pri_mode = false;
            picture->set_mode(0);
            pri->setChecked(false);
            pic->setChecked(true);
            break;
        case 1:  //draw priority
            pri_mode = true;
            picture->set_mode(1);
            pic->setChecked(false);
            pri->setChecked(true);
            break;
        case 2: //draw (also) background
            canvas->bg_on = bg->isChecked();
            if (canvas->bg_on && canvas->bg_loaded)
                picture->bg_on = true;
            else
                picture->bg_on = false;
            break;
        case 3: //priority lines
            canvas->pri_lines = prilines->isChecked();
            break;
    }
    canvas->linedraw = false;
    canvas->update();
}

//*********************************************
void PicEdit::change_tool(int k)
{
    if (canvas->linedraw)
        canvas->line(false);
    picture->tool_proc(k);
}

//*********************************************
void PicEdit::change_size(int k)
{
    picture->set_brush(0, k);
}

//*********************************************
void PicEdit::change_shape(int k)
{
    picture->set_brush(1, k);
}

//*********************************************
void PicEdit::change_type(int k)
{
    picture->set_brush(2, k);
}

//*********************************************
void PicEdit::show_pos()
//show current picture buffer position
{
    char *t;
    int code = 0, val = -1;
    sprintf(tmp, "%5d", picture->bufPos);
    pos->setText(tmp);
    t = picture->showPos(&code, &val);
    codeline->setText(t);
    if (code >= 0xf0 && code <= 0xff) { //action: can add comments
        if (code == 0xf0 || code == 0xf2) { //add color name
            sprintf(tmp, "%s %s", comment[code - 0xf0], colname[val]);
            comments->setText(tmp);
        } else
            comments->setText(comment[code - 0xf0]);
    } else
        comments->clear();
}

//*********************************************
void PicEdit::home_cb()
//set picture buffer position to start
{
    picture->home_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::end_cb()
//set picture buffer position to end
{
    picture->end_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::left_cb()
//set picture buffer position to the previous action
{
    picture->left_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::right_cb()
//set picture buffer position to the next action
{
    picture->right_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::del_cb()
//delete action from the picture buffer position
{
    picture->del_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::wipe_cb()
//delete all action from the picture buffer position to the end
{
    picture->wipe_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::set_pos()
//set picture buffer position
{
    QString str = pos->text();
    char *s = str.toLatin1().data();
    int num = atoi(s);
    if (num != 0 || s[0] == '0') {
        if (!picture->setBufPos(num))
            canvas->update();
    }
    show_pos();
    update_palette();
    update_tools();
    pos->clearFocus();
    setFocus();
    return;
}

//*********************************************
void PicEdit::update_tools()
{
    switch (picture->tool) {
        case T_LINE:
            if (!line->isChecked())
                line->setChecked(true);
            break;
        case T_STEP:
            if (!step->isChecked())
                step->setChecked(true);
            break;
        case T_PEN:
            if (!pen->isChecked())
                pen->setChecked(true);
            break;
        case T_FILL:
            if (!fill->isChecked())
                fill->setChecked(true);
            break;
        case T_BRUSH:
            if (!brush->isChecked())
                brush->setChecked(true);
            break;
        default:
            QRadioButton *b = (QRadioButton *)tool->checkedButton();
            if (b)
                b->setChecked(false);
            break;
    }
}

//*********************************************
void PicEdit::update_palette()
{
    bool ch = false;
    if (!picture->picDrawEnabled && palette->left != -1) {
        palette->left = -1;
        ch = true;
    }
    if (picture->picDrawEnabled && palette->left != picture->picColour) {
        palette->left = picture->picColour;
        ch = true;
    }
    if (!picture->priDrawEnabled && palette->right != -1) {
        palette->right = -1;
        ch = true;
    }
    if (picture->priDrawEnabled && palette->right != picture->priColour) {
        palette->right = picture->priColour;
        ch = true;
    }

    if (ch)
        palette->update();
}

//*********************************************
bool PicEdit::focusNextPrevChild(bool)
{
    canvas->setFocus();
    return true;
}

//************************************************
void PicEdit::editor_help()
{
    menu->help_topic("picture_editor_main");
}


//************************************************
PCanvas::PCanvas(QWidget *parent, const char *name, PicEdit *w)
    : QScrollArea(parent)
      //the area to draw picture
{
    picedit = w;
    picture = picedit->picture;
    pixsize = 2;
    x0 = 5;
    y0 = 5;
    cur_w = MAX_W * pixsize;
    cur_h = MAX_HH * pixsize;
    pixmap = QPixmap(cur_w, cur_h);
    viewport()->setMouseTracking(true);
    bg_loaded = false;
    bg_on = false;
    pri_lines = false;
    bgpix = QImage();
}

//*********************************************
void PCanvas::setSize(int w, int h)
{
    if (cur_w != w || cur_h != h) {
        pixmap = pixmap.scaled(w * pixsize * 2, h * pixsize);
        cur_w = w;
        cur_h = h;
    }
}

//*********************************************
void PCanvas::setPixsize(int s)
{
    pixsize = s;
    cur_w = MAX_W * pixsize;
    cur_h = MAX_HH * pixsize;
    pixmap = pixmap.scaled(cur_w, cur_h);
    QPainter p(&pixmap);
    p.eraseRect(0, 0, cur_w, cur_h);
    update();
}

//*********************************************
void PCanvas::viewportMousePressEvent(QMouseEvent *event)
{
    int x = event->x(), y = event->y();
    int xx = x, yy = y;

    x -= x0;
    y -= y0;

    x /= pixsize;
    y /= pixsize;

    if (x < 0)
        x = 0;
    else if (x >= MAX_W)
        x = MAX_W - 1;
    if (y < 0)
        y = 0;
    else if (y >= MAX_HH)
        y = MAX_HH - 1;

    if (event->button() & Qt::LeftButton) {
        if (picture->button_action(x, y))
            picedit->show_pos();
    } else if (event->button() & Qt::RightButton) {
        QRadioButton *b = (QRadioButton *)picedit->tool->checkedButton();
        if (b != 0)
            b->setChecked(false);
        picture->clear_tools();
        picture->tool = -1;
        picture->init_tool();
    }
    update();
    picedit->changed = true;
}

//*********************************************
void PCanvas::viewportMouseMoveEvent(QMouseEvent *event)
{
    int x = event->x(), y = event->y();
    int xx = x, yy = y;

    x -= x0;
    y -= y0;

    x /= pixsize;
    y /= pixsize;

    if (x < 0)
        x = 0;
    else if (x >= MAX_W)
        x = MAX_W - 1;
    if (y < 0)
        y = 0;
    else if (y >= MAX_HH)
        y = MAX_HH - 1;

    x1 = x;
    y1 = y;

    //  printf("mouse move: %d %d (%d %d)\n",x,y,xx,yy);

    if (picture->move_action(x, y))
        line(true);

    if (x >= 0 && y >= 0) {
        int pri = y / 12 + 1;
        sprintf(tmp, "X=%d  Y=%d  Pri=%d", x / 2, y, pri);
        picedit->status->showMessage(tmp);
        auto pal = picedit->pricolor->palette();
        pal.setColor(QPalette::Background, egacolor[pri + 1]);
        picedit->pricolor->setPalette(pal);
    }
}

//*********************************************
void PCanvas::drawContents(QPainter *p, int, int, int, int)
{
    if (cur_w == 0 || cur_h == 0)
        return;

    p->drawPixmap(x0, y0, pixmap);
}

//*********************************************
void PCanvas::update()
{
    QPainter p(&pixmap);
    int x, y;
    byte c;
    byte *data;

    data = (picedit->pri_mode) ? picture->priority : picture->picture;
    if (bg_loaded && bg_on) { //if must draw background
        bool pic = !picedit->pri_mode;
        for (y = 0; y < MAX_HH; y++) {
            for (x = 0; x < MAX_W; x += 2) {
                c = data[y * MAX_W + x];
                if ((pic && c == 15) || (!pic && c == 4)) { //draw background instead of "empty" areas
                    p.fillRect(x * pixsize, y * pixsize, pixsize, pixsize, QColor(bgpix.pixel(x, y)));
                    p.fillRect((x + 1)*pixsize, y * pixsize, pixsize, pixsize, QColor(bgpix.pixel(x + 1, y)));
                } else
                    p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[c]);
            }
        }
    } else {
        for (y = 0; y < MAX_HH; y++) {
            for (x = 0; x < MAX_W; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * MAX_W + x]]);
        }
    }
    linedraw = false;

    if (pri_lines) {
        QPen pen;
        pen.setStyle(Qt::DashLine);
        pen.setWidth(1);

        //p.setPen(Qt::white);
        // p.setRasterOp(XorROP);
        int i = 4;
        int step = MAX_HH * pixsize / 14;
        for (y = step * 3; y < MAX_HH * pixsize; y += step) {
            //pen.setBrush(QColor(255, 0, 0, 127));
            pen.setBrush(egacolor[i++]);
            p.setPen(pen);
            p.drawLine(0, y, MAX_W * pixsize, y);
            pen.setBrush(QColor(0, 0, 0, 127));
            p.setPen(pen);
            p.drawLine(0, y + 1, MAX_W * pixsize, y + 1);
        }
    }
    repaint(x0, y0, x0 + MAX_W * pixsize, y0 + MAX_HH * pixsize);
}

//*********************************************
void PCanvas::line(bool mode)
//draw/erase 'temporary' line following the cursor (line, pen or step mode)
//mode==true - erase the old one, draw the new one
//mode==false - only erase the old one
{
    QPainter p(&pixmap);
    byte c;
    Points *curp, *newp = NULL;
    int i;

    //restore the previous line
    curp = picture->curp;

    if (picture->bg_on) {
        //with background - must redraw the line containing background pixels
        //(x-pixels are not doubled !)
        for (i = 0; i < curp->n; i++) {
            p.fillRect(curp->p[i].x * pixsize, curp->p[i].y * pixsize,
                       pixsize, pixsize, curp->p[i].cc);

        }
    } else {
        //no background - just restore the agi image
        for (i = 0; i < curp->n; i++) {
            c = curp->p[i].c;
            p.fillRect(curp->p[i].x * 2 * pixsize, curp->p[i].y * pixsize,
                       pixsize * 2, pixsize, egacolor[c]);

        }
    }

    if (mode == true) {
        linedraw = true;
        newp = picture->newp;
        //draw the 'new' line
        for (int i = 0; i < newp->n; i++) {
            c = newp->p[i].c;
            p.fillRect(newp->p[i].x * 2 * pixsize, newp->p[i].y * pixsize,
                       pixsize * 2, pixsize, egacolor[c]);

        }
    } else
        linedraw = false;

#if 0
    //find the max rectangle to repaint
    if (curp->n) {
        if (picture->bg_on) {
            x00 = curp->p[0].x;
            y00 = curp->p[0].y;
            x11 = curp->p[curp->n - 1].x;
            y11 = curp->p[curp->n - 1].y;
        } else {
            x00 = curp->p[0].x * 2;
            y00 = curp->p[0].y;
            x11 = curp->p[curp->n - 1].x * 2;
            y11 = curp->p[curp->n - 1].y;
        }
        if (newp) {
            x0_ = newp->p[0].x * 2;
            y0_ = newp->p[0].y;
            x1 = newp->p[newp->n - 1].x * 2;
            y1 = newp->p[newp->n - 1].y;

            X0 = (MIN(MIN(x00, x0_), MIN(x11, x1))) * pixsize;
            Y0 = (MIN(MIN(y00, y0_), MIN(y11, y1))) * pixsize;
            X1 = (MAX(MAX(x00, x0_), MAX(x11, x1))) * pixsize;
            Y1 = (MAX(MAX(y00, y0_), MAX(y11, y1))) * pixsize;
        } else {
            X0 = (MIN(x00, x11)) * pixsize;
            Y0 = (MIN(y00, y11)) * pixsize;
            X1 = (MAX(x00, x11)) * pixsize;
            Y1 = (MAX(y00, y11)) * pixsize;
        }
    } else {

        x0_ = newp->p[0].x * 2 * pixsize;
        y0_ = newp->p[0].y * pixsize;
        x1 = newp->p[newp->n - 1].x * 2 * pixsize;
        y1 = newp->p[newp->n - 1].y * pixsize;
        X0 = MIN(x0_, x1);
        Y0 = MIN(y0_, y1);
        X1 = MAX(x0_, x1);
        Y1 = MAX(y0_, y1);

    }
#endif
    repaint(x0, y0, x0 + MAX_W * pixsize, y0 + MAX_HH * pixsize);
}

//*********************************************
void PCanvas::keyPressEvent(QKeyEvent *k)
{
    switch (k->key()) {
        case Qt::Key_L:
        case Qt::Key_F1:
            picedit->line->setChecked(true);
            picedit->change_tool(T_LINE);
            break;
        case Qt::Key_P:
        case Qt::Key_F2:
            picedit->pen->setChecked(true);
            picedit->change_tool(T_PEN);
            break;
        case Qt::Key_S:
        case Qt::Key_F3:
            picedit->step->setChecked(true);
            picedit->change_tool(T_STEP);
            break;
        case Qt::Key_F:
        case Qt::Key_F4:
            picedit->fill->setChecked(true);
            picedit->change_tool(T_FILL);
            break;
        case Qt::Key_B:
        case Qt::Key_F5:
            picedit->brush->setChecked(true);
            picedit->change_tool(T_BRUSH);
            break;
        case Qt::Key_Tab:
            picedit->pri_mode = !picedit->pri_mode;
            picedit->change_drawmode(picedit->pri_mode ? 1 : 0);
            break;
        case Qt::Key_Home:
            picedit->home_cb();
            break;
        case Qt::Key_End:
            picedit->end_cb();
            break;
        case Qt::Key_Right:
            picedit->right_cb();
            break;
        case Qt::Key_Left:
            picedit->left_cb();
            break;
        case Qt::Key_Delete:
            picedit->del_cb();
            break;
        case Qt::Key_F10:
            if (picedit->bg->isChecked())
                picedit->bg->setChecked(false);
            else
                picedit->bg->setChecked(true);
            picedit->change_drawmode(2);
            break;

        default:
            k->ignore();
            break;
    }
}

//*********************************************
bool PCanvas::focusNextPrevChild(bool)
{
    setFocus();
    return false;
}

//*****************************************
void PCanvas::load_bg(char *filename)
{
    if (!bgpix.load(filename)) {
        menu->errmes("Can't open file %s !", filename);
        return;
    }
    bg_loaded = true;

    picture->bgpix = &bgpix;
    picedit->bg->setChecked(true);
    bg_on = true;
    picture->bg_on = true;
    update();
}

//*********************************************
void PCanvas::showEvent(QShowEvent *)
{
    if (isTopLevel() && !picedit->isVisible() && !picedit->showing)
        picedit->showNormal();
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void PCanvas::hideEvent(QHideEvent *)
{
    if (isTopLevel() && picedit->isVisible() && !picedit->hiding)
        picedit->showMinimized();
}

//*********************************************
void PCanvas::closeEvent(QCloseEvent *e)
{
    if (picedit->closing) {
        e->accept();
        return;
    }
    picedit->closing = true;
    if (isTopLevel()) {
        picedit->close();
        e->ignore();
    }
}

/*******************************************************/
Palette1::Palette1(QWidget *parent, const char *name, PicEdit *p)
    : QWidget(parent)
{
    left = right = -1;
    picedit = p;
}

//*****************************************
void Palette1::paintEvent(QPaintEvent *)
//draw palette with the currently selected colors marked 'V' (visual) and 'P' (priority)
{
    QPainter p(this);
    int w, h, x, y, dx, dy, i;

    w = this->width();
    h = this->height();
    dx = w / 9;
    dy = h / 2;
    w = dx * 9;
    h = dy * 2;

    for (y = 0, i = 0; y < h; y += dy) {
        for (x = 0; x < w - dx; x += dx, i++) {
            p.fillRect(x, y, dx, dy, egacolor[i]);
            if (i == left) {
                p.setPen(i < 10 ? egacolor[15] : egacolor[0]);
                p.drawText(x + dx / 4, y + dy / 2 + 2, "V");
            }
            if (i == right) {
                p.setPen(i < 10 ? egacolor[15] : egacolor[0]);
                p.drawText(x + dx * 2 / 3, y + dy / 2 + 2, "P");
            }
        }
    }
    p.fillRect(w - dx, 0, dx, h, QColor(0x90, 0x90, 0x90));
    p.setPen(egacolor[0]);
    x = w - dx;
    p.drawText(x + 2, h - dy / 3, "Off");
    if (left == -1)
        p.drawText(x + dx / 4, dy / 2 + 2, "V");
    if (right == -1)
        p.drawText(x + dx * 2 / 3, dy / 2 + 2, "P");
}

//*****************************************
void Palette1::mousePressEvent(QMouseEvent *event)
{
    int w, h, x, y, dx, dy, i;

    w = this->width();
    h = this->height();
    dx = w / 9;
    dy = h / 2;
    w = dx * 9;
    h = dy * 2;

    x = event->x() / dx;
    y = event->y() / dy;

    if (x >= 8) //choose "off"
        i = -1;
    else       //choose color
        i = y * 8 + x;

    if (event->button() & Qt::LeftButton) {
        if (left != i) {
            left = i;
            picedit->picture->choose_color(M_LEFT, i);
            repaint();
        }
    } else if (event->button() & Qt::RightButton) {
        if (right != i) {
            right = i;
            picedit->picture->choose_color(M_RIGHT, i);
            repaint();
        }
    }
}

//************************************************
ViewData::ViewData(QWidget *parent, const char *name, Picture *p)
    : QWidget(parent)
      //view picture codes
{
    picture = p;
    QBoxLayout *all = new QVBoxLayout(this);
    codes = new QTextEdit(this);
    codes->setMinimumSize(300, 200);
    codes->setReadOnly(true);
    all->addWidget(codes);

    QBoxLayout *b = new QHBoxLayout(this);
    all->addLayout(b);
    QBoxLayout *left = new QVBoxLayout(this);
    b->addLayout(left);
    comments = new QCheckBox("Show comments", this);
    connect(comments, SIGNAL(clicked()), SLOT(read()));
    left->addWidget(comments);
    wrap = new QCheckBox("Line wrap", this);
    connect(wrap, SIGNAL(clicked()), SLOT(read()));
    left->addWidget(wrap);

    QBoxLayout *right = new QVBoxLayout(this);
    b->addLayout(right);
    QPushButton *close = new QPushButton("Close", this);
    close->setMaximumSize(80, 60);
    connect(close, SIGNAL(clicked()), SLOT(close()));
    right->addWidget(close);

    data.lfree();
}

//************************************************
void ViewData::resizeEvent(QResizeEvent *)
{
    QString str = codes->toPlainText();
    getmaxcol();
    codes->setText(str);
}

//************************************************
void ViewData::getmaxcol()
{
    QFontMetrics f = fontMetrics();
    maxcol = codes->width() / f.width('a');
}

//************************************************
void ViewData::read()
{
    char *str;
    bool comm = comments->isChecked();
    bool wr = wrap->isChecked();
    int c, cc, i, k, len;
    char *ptr, *ptr0;
    bool first;

    codes->setUpdatesEnabled(false);  //to speed up repaint

    picture->viewData(&data);
    getmaxcol();

    codes->clear();
    for (i = 0; i < data.num; i++) {
        std::string str2 = data.at(i);
        str = (char *)str2.c_str();
        if (wr) {   //wrap long lines
            k = 0;
            tmp[0] = 0;
            first = true;
            for (ptr = ptr0 = str; *ptr; ptr += 3) {
                if ((first && k + 3 >= maxcol) || k + 6 >= maxcol) {
                    len = (int)(ptr - ptr0);
                    strncat(tmp, ptr0, len);
                    tmp[len] = 0;
                    codes->insertPlainText(tmp);
                    strcpy(tmp, "   ");
                    first = false;
                    ptr0 = ptr;
                    k = 0;
                } else
                    k += 3;
            }
            strcat(tmp, ptr0);
            if (comm) { //add comments (action and color when applicable)
                sscanf(str, "%x %x", &c, &cc);
                strcat(tmp, " //");
                strcat(tmp, comment[c - 0xf0]);
                if (c == 0xf0 || c == 0xf2) {
                    strcat(tmp, " ");
                    strcat(tmp, colname[cc]);
                }
            }
            codes->insertPlainText(tmp);
        } else {
            if (comm) { //add comments (action and color when applicable)
                sscanf(str, "%x %x", &c, &cc);
                sprintf(tmp, "%s //%s", str, comment[c - 0xf0]);
                if (c == 0xf0 || c == 0xf2) {
                    strcat(tmp, " ");
                    strcat(tmp, colname[cc]);
                }
                codes->insertPlainText(tmp);
            } else
                codes->insertPlainText(str);
        }
    }
    codes->setUpdatesEnabled(true);
    codes->update();

    show();
}

//************************************************
