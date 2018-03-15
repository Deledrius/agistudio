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
#include "viewedit.h"
#include "menu.h"
#include "wutil.h"
#include "preview.h"

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include <QApplication>
#include <QSplitter>
#include <QFrame>
#include <QMessageBox>
#include <QFileDialog>
#include <QStringList>
#include <QLayout>
#include <QPixmap>
#include <QPainter>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QGridLayout>
#include <QHideEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QBoxLayout>
#include <QShowEvent>
#include <QPaintEvent>
#include <QCloseEvent>

#include "zoom_minus_x.xpm"
#include "zoom_plus_x.xpm"
#include "right_x.xpm"
#include "left_x.xpm"
#include "rightarrow_x.xpm"
#include "leftarrow_x.xpm"
#include "uparrow_x.xpm"
#include "downarrow_x.xpm"

Cel saveCel = Cel();  //cel "clipboard"
bool cel_copied = false;

//*********************************************
ViewEdit::ViewEdit(QWidget *parent, const char *name, int win_num, ResourcesWin *res)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("View Editor");

    winnum = win_num;
    resources_win = res;
    view = new View();

    QMenu *file = new QMenu(this);
    Q_CHECK_PTR(file);
    file->setTitle("&File");
    file->addAction("&New", this, SLOT(open()));
    file->addAction("&Load from file", this, SLOT(open_file()));
    file->addAction("&Save to game", this, SLOT(save_to_game()));
    file->addAction("Save to game &as...", this, SLOT(save_to_game_as()));
    file->addAction("Save to file", this, SLOT(save_file()));
    file->addSeparator();
    file->addAction("&Delete", this, SLOT(delete_view()));
    file->addSeparator();
    file->addAction("&Close", this, SLOT(close()));

    QMenu *edit = new QMenu(this);
    Q_CHECK_PTR(edit);
    edit->setTitle("&Edit");
    edit->addAction("&Undo", this, SLOT(undo_cel()));
    edit->addSeparator();
    edit->addAction("&Copy cel", this, SLOT(copy_cel()), Qt::CTRL + Qt::Key_C);
    edit->addAction("&Paste cel", this, SLOT(paste_cel()), Qt::CTRL + Qt::Key_V);

    QMenu *loop = new QMenu(this);
    Q_CHECK_PTR(loop);
    loop->setTitle("&Loop");
    loop->addAction("&Next", this, SLOT(next_loop()));
    loop->addAction("&Previous", this, SLOT(previous_loop()));
    loop->addAction("&First", this, SLOT(first_loop()));
    loop->addAction("&Last", this, SLOT(last_loop()));
    loop->addSeparator();
    loop->addAction("Insert &before", this, SLOT(insert_loop_before()));
    loop->addAction("&Insert after", this, SLOT(insert_loop_after()));
    loop->addAction("&Append", this, SLOT(append_loop()));
    loop->addAction("&Delete", this, SLOT(delete_loop()));
    loop->addAction("&Clear", this, SLOT(clear_loop()));


    QMenu *cel = new QMenu(this);
    Q_CHECK_PTR(cel);
    cel->setTitle("&Cel");
    cel->addAction("&Next", this, SLOT(next_cel()));
    cel->addAction("&Previous", this, SLOT(previous_cel()));
    cel->addAction("&First", this, SLOT(first_cel()));
    cel->addAction("&Last", this, SLOT(last_cel()));
    cel->addSeparator();
    cel->addAction("Insert &before", this, SLOT(insert_cel_before()));
    cel->addAction("&Insert after", this, SLOT(insert_cel_after()));
    cel->addAction("&Append", this, SLOT(append_cel()));
    cel->addAction("&Delete", this, SLOT(delete_cel()));
    cel->addAction("&Clear", this, SLOT(clear_cel()));
    cel->addSeparator();
    cel->addAction("Flip &Horizontally", this, SLOT(fliph_cel()));
    cel->addAction("Flip &Vertically", this, SLOT(flipv_cel()));

    QMenuBar *menu = new QMenuBar(this);
    Q_CHECK_PTR(menu);
    menu->addMenu(file);
    menu->addMenu(edit);
    menu->addMenu(loop);
    menu->addMenu(cel);
    menu->addAction("&Animate", this, SLOT(animate_cb()));

    QBoxLayout *all =  new QHBoxLayout(this);
    all->setMenuBar(menu);

    QBoxLayout *left = new QVBoxLayout(this);
    all->addLayout(left);


    QPixmap pright = QPixmap(right_x);
    QPixmap pleft = QPixmap(left_x);


    QFrame *frame1 = new QFrame(this);
    frame1->setFrameStyle(QFrame::Box | QFrame::Sunken);
    frame1->setLineWidth(1);
    frame1->setMinimumSize(200, 180);
    frame1->setContentsMargins(10, 10, 10, 10);
    left->addWidget(frame1);


    int maxrow1 = 9, maxcol1 = 4;
    QGridLayout *grid1 = new QGridLayout(frame1);

    int i;

    for (i = 0; i < maxcol1; i++) {
        grid1->setColumnStretch(i, 1);
        grid1->addItem(new QSpacerItem(1, 0), 0, i);
    }

    for (i = 0; i < maxrow1; i++) {
        grid1->setRowStretch(i, 1);
        grid1->addItem(new QSpacerItem(0, 2), i, 0);
    }


    int row = 1;
    int col = 0;

    QLabel *looplabel = new QLabel("Loop:", frame1);
    grid1->addWidget(looplabel, row, col, Qt::AlignRight);
    col++;


    loopnum = new QLabel("0/0", frame1);
    grid1->addWidget(loopnum, row, col, Qt::AlignLeft);
    col++;


    QPushButton *loopleft = new QPushButton(frame1);
    loopleft->setIcon(pleft);
    connect(loopleft, SIGNAL(clicked()), SLOT(previous_loop()));
    grid1->addWidget(loopleft, row, col, Qt::AlignRight);
    col++;

    QPushButton *loopright = new QPushButton(frame1);
    loopright->setIcon(pright);
    connect(loopright, SIGNAL(clicked()), SLOT(next_loop()));
    grid1->addWidget(loopright, row, col, Qt::AlignLeft);
    col++;

    row++;
    col = 0;

    QLabel *cellabel = new QLabel("Cel:", frame1);
    grid1->addWidget(cellabel, row, col, Qt::AlignRight);
    col++;

    celnum = new QLabel("0/0", frame1);
    grid1->addWidget(celnum, row, col, Qt::AlignLeft);
    col++;

    QPushButton *celleft = new QPushButton(frame1);
    celleft->setIcon(pleft);
    connect(celleft, SIGNAL(clicked()), SLOT(previous_cel()));
    grid1->addWidget(celleft, row, col, Qt::AlignRight);
    col++;

    QPushButton *celright = new QPushButton(frame1);
    celright->setIcon(pright);
    connect(celright, SIGNAL(clicked()), SLOT(next_cel()));
    grid1->addWidget(celright, row, col, Qt::AlignLeft);
    col++;

    row++;
    col = 0;

    QLabel *lwidth = new QLabel("Width:", frame1);
    grid1->addWidget(lwidth, row, col, Qt::AlignRight);
    col++;

    width = new QLineEdit(frame1);
    width->setMinimumWidth(40);
    width->setMaximumWidth(60);
    connect(width, SIGNAL(returnPressed()), SLOT(change_width_height()));
    grid1->addWidget(width, row, col, Qt::AlignLeft);
    col++;

    QPushButton *widthleft = new QPushButton(frame1);
    widthleft->setIcon(pleft);
    connect(widthleft, SIGNAL(clicked()), SLOT(dec_width()));
    grid1->addWidget(widthleft, row, col, Qt::AlignRight);
    col++;

    QPushButton *widthright = new QPushButton(frame1);
    widthright->setIcon(pright);
    connect(widthright, SIGNAL(clicked()), SLOT(inc_width()));
    grid1->addWidget(widthright, row, col, Qt::AlignLeft);
    col++;

    row++;
    col = 0;

    QLabel *lheight = new QLabel("Height:", frame1);
    grid1->addWidget(lheight, row, col, Qt::AlignRight);
    col++;

    height = new QLineEdit(frame1);
    height->setMinimumWidth(40);
    height->setMaximumWidth(60);
    connect(height, SIGNAL(returnPressed()), SLOT(change_width_height()));
    grid1->addWidget(height, row, col, Qt::AlignLeft);
    col++;

    QPushButton *heightleft = new QPushButton(frame1);
    heightleft->setIcon(pleft);
    connect(heightleft, SIGNAL(clicked()), SLOT(dec_height()));
    grid1->addWidget(heightleft, row, col, Qt::AlignRight);
    col++;

    QPushButton *heightright = new QPushButton(frame1);
    heightright->setIcon(pright);
    connect(heightright, SIGNAL(clicked()), SLOT(inc_height()));
    grid1->addWidget(heightright, row, col, Qt::AlignLeft);
    col++;


    row++;
    col = 0;
    is_descriptor = new QCheckBox("Description", frame1);
    connect(is_descriptor, SIGNAL(clicked()), SLOT(is_descriptor_cb()));

    grid1->addWidget(is_descriptor, row, 0, 1, 2, Qt::AlignCenter);

    edit_descriptor = new QPushButton(frame1);
    edit_descriptor->setText("Edit");
    edit_descriptor->setMaximumHeight(20);
    edit_descriptor->setEnabled(false);
    connect(edit_descriptor, SIGNAL(clicked()), SLOT(show_description()));
    grid1->addWidget(edit_descriptor, row, (maxcol1 - 2), 1, -1, Qt::AlignCenter);


    row++;
    col = 0;


    QLabel *mirrorloop = new QLabel("This loop mirrors: ", frame1, 0);
    grid1->addWidget(mirrorloop, row, 0, 1, (maxcol1 - 1), Qt::AlignCenter);


    row++;
    col = 0;


    mirror_loop = new QComboBox(this);
    left->addWidget(mirror_loop);
    mirror_loop->insertItem(0, " no other loop ");
    mirror_loop->setMinimumSize(100, 20);
    connect(mirror_loop, SIGNAL(activated(int)), this, SLOT(change_mirror(int)));

    grid1->addWidget(mirror_loop, row, 0, 1, (maxcol1 - 1), Qt::AlignCenter);


    QFrame *frame2 = new QFrame(this);
    frame2->setFrameStyle(QFrame::Box | QFrame::Sunken);
    frame2->setLineWidth(1);
    frame2->setMinimumSize(180, 100);
    frame2->setContentsMargins(1, 1, 1, 1);
    left->addWidget(frame2);


    QBoxLayout *h_frame2 = new QHBoxLayout(frame2);


    QGridLayout *grid2 = new QGridLayout();
    h_frame2->addLayout(grid2);

    for (i = 0; i < 2; i++) {
        grid2->setColumnStretch(i, 1);
        grid2->addItem(new QSpacerItem(1, 0), 0, i);
    }
    for (i = 0; i < 3; i++) {
        grid2->setRowStretch(i, 1);
        grid2->addItem(new QSpacerItem(0, 2), i, 0);
    }


    QPushButton *zoom_minus = new QPushButton(frame2);
    zoom_minus->setIcon(QPixmap(zoom_minus_x));
    connect(zoom_minus, SIGNAL(clicked()), SLOT(zoom_minus()));
    grid2->addWidget(zoom_minus, 0, 0, Qt::AlignLeft);

    QPushButton *zoom_plus = new QPushButton(frame2);
    zoom_plus->setIcon(QPixmap(zoom_plus_x));
    connect(zoom_plus, SIGNAL(clicked()), SLOT(zoom_plus()));
    grid2->addWidget(zoom_plus, 0, 1, Qt::AlignRight);


    view_draw = new QRadioButton("Draw", frame2);
    view_draw->setChecked(true);
    drawing_mode = V_DRAW;
    grid2->addWidget(view_draw, 1, 0, 1, 1, Qt::AlignLeft);

    view_fill = new QRadioButton("Fill", frame2);
    grid2->addWidget(view_fill, 2, 0, 1, 1, Qt::AlignLeft);


    QButtonGroup *bg = new QButtonGroup(frame2);
    bg->setExclusive(true);
    bg->addButton(view_draw);
    bg->addButton(view_fill, 1);
    connect(bg, SIGNAL(buttonClicked(int)), SLOT(change_mode(int)));


    QGridLayout *grid3 = new QGridLayout();
    h_frame2->addLayout(grid3);

    for (i = 0; i < 3; i++) {
        grid3->setColumnStretch(i, 1);
        grid3->addItem(new QSpacerItem(0, 0), 0, i);
    }
    for (i = 0; i < 3; i++) {
        grid3->setRowStretch(i, 1);
        grid3->addItem(new QSpacerItem(0, 0), i, 0);
    }


    QPushButton *view_up = new QPushButton(frame2);
    view_up->setIcon(QPixmap(uparrow_x));
    connect(view_up, SIGNAL(clicked()), SLOT(shift_up()));
    grid3->addWidget(view_up, 0, 1, Qt::AlignBottom | Qt::AlignHCenter);

    QPushButton *view_left = new QPushButton(frame2);
    view_left->setIcon(QPixmap(leftarrow_x));
    connect(view_left, SIGNAL(clicked()), SLOT(shift_left()));
    grid3->addWidget(view_left, 1, 0, Qt::AlignRight | Qt::AlignVCenter);

    QPushButton *view_right = new QPushButton(frame2);
    view_right->setIcon(QPixmap(rightarrow_x));
    connect(view_right, SIGNAL(clicked()), SLOT(shift_right()));
    grid3->addWidget(view_right, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);

    QPushButton *view_down = new QPushButton(frame2);
    view_down->setIcon(QPixmap(downarrow_x));
    connect(view_down, SIGNAL(clicked()), SLOT(shift_down()));
    grid3->addWidget(view_down, 2, 1, Qt::AlignTop | Qt::AlignHCenter);



    QFrame *frame3 = new QFrame(this);
    frame3->setFrameStyle(QFrame::Box | QFrame::Sunken);
    frame3->setLineWidth(1);
    frame3->setMinimumSize(420, 300);
    frame3->setContentsMargins(4, 4, 4, 4);
    all->addWidget(frame3, 1);


    QBoxLayout *right = new QVBoxLayout(this);
    frame3->setLayout(right);

    canvas = new Canvas(frame3, 0, this);
    canvas->setMinimumSize(400, 200);
    right->addWidget(canvas, 1);
    canvas->setFocusPolicy(Qt::ClickFocus);
    setFocusProxy(canvas);

    QFrame *frame4 = new QFrame(frame3);
    frame4->setFrameStyle(QFrame::Box | QFrame::Sunken);
    frame4->setLineWidth(1);
    frame4->setMinimumSize(400, 80);
    frame4->setContentsMargins(10, 10, 10, 10);
    right->addWidget(frame4);


    int maxcol2 = 6;
    QGridLayout *grid4 = new QGridLayout(frame4);

    for (i = 0; i < maxcol2; i++) {
        grid4->setColumnStretch(i, 1);
        grid4->addItem(new QSpacerItem(4, 0), 0, i);
    }
    for (i = 0; i < 2; i++) {
        grid4->setRowStretch(i, 1);
        grid4->addItem(new QSpacerItem(0, 2), i, 0);
    }



    QLabel *trans_color = new QLabel("Transparency colour:", frame4);
    trans_color->setMaximumHeight(20);
    grid4->addWidget(trans_color, 0, 0, Qt::AlignLeft);

    transcolor = new QWidget(frame4);
    transcolor->setPalette(QPalette(egacolor[0]));
    transcolor->setMinimumSize(40, 16);
    transcolor->setMaximumSize(100, 30);
    grid4->addWidget(transcolor, 0, 1, Qt::AlignCenter);

    QPushButton *set_trans_color = new QPushButton(frame4);
    set_trans_color->setText("Set");
    set_trans_color->setMaximumHeight(20);
    connect(set_trans_color, SIGNAL(clicked()), SLOT(set_transcolor()));
    grid4->addWidget(set_trans_color, 0, 2, Qt::AlignRight);

    QWidget *dummy = new QWidget(frame4);
    grid4->addWidget(dummy, 0, 0, 3, (maxcol2 - 1), Qt::AlignCenter);

    palette = new Palette(frame4);
    palette->setMinimumSize(250, 40);
    palette->setMaximumSize(350, 80);
    grid4->addWidget(palette, 1, 0, 1, (maxcol2 - 2), Qt::AlignLeft);

    description = NULL;

    changed = false;
    undo = false;
    undoCel = Cel();
    animate = NULL;
    canvas->setFocus();

    adjustSize();
    hide();
}

//*********************************************
void ViewEdit::save(char *filename)
{
    view->save(filename);
    changed = false;
}

//*********************************************
void ViewEdit::display()
{
    showlooppar();
    showcelpar();

    if (view->Description != "") {
        is_descriptor->setChecked(true);
        edit_descriptor->setEnabled(true);
    } else {
        is_descriptor->setChecked(false);
        edit_descriptor->setEnabled(false);
    }
    if (description)
        description->hide();
    DisplayView();
    show();
}

//*********************************************
void ViewEdit::open(int ResNum)
{
    if (view->open(ResNum))
        return ;
    ViewNum = ResNum;
    sprintf(tmp, "View editor: view.%03d", ViewNum);
    setWindowTitle(tmp);
    changed = false;
    display();
}

//*********************************************
void ViewEdit::open(char *filename)
{
    if (view->open(filename))
        return;
    ViewNum = -1;
    sprintf(tmp, "View editor");
    setWindowTitle(tmp);
    changed = false;
    display();
}

//*********************************************
void ViewEdit::DisplayView()
{
    int w, h;
    w = canvas->x0 + canvas->cur_w * canvas->pixsize * 2 + 10;
    h = canvas->y0 + canvas->cur_h * canvas->pixsize + 10;

    int i = view->loops[view->CurLoop].mirror;
    if (i != -1)
        canvas->DrawCel(view->loops[i].cels[view->CurCel].width, view->loops[i].cels[view->CurCel].height, view->loops[i].cels[view->CurCel].data, true);
    else
        canvas->DrawCel(view->loops[view->CurLoop].cels[view->CurCel].width, view->loops[view->CurLoop].cels[view->CurCel].height, view->loops[view->CurLoop].cels[view->CurCel].data, false);
    if (view->loops[view->CurLoop].cels[view->CurCel].transcol != transcol)
        set_transcolor(view->loops[view->CurLoop].cels[view->CurCel].transcol);
}

//*********************************************
void ViewEdit::DisplayView(int pixsize)
{
    int w, h;
    w = canvas->x0 + canvas->cur_w * pixsize * 2 + 10;
    h = canvas->y0 + canvas->cur_h * pixsize + 10;

    if (canvas->width() < w || canvas->height() < h)
        canvas->resize(w, h);


    int i = view->loops[view->CurLoop].mirror;
    if (i != -1)
        canvas->DrawCel(view->loops[i].cels[view->CurCel].width, view->loops[i].cels[view->CurCel].height, view->loops[i].cels[view->CurCel].data, true, pixsize);
    else
        canvas->DrawCel(view->loops[view->CurLoop].cels[view->CurCel].width, view->loops[view->CurLoop].cels[view->CurCel].height, view->loops[view->CurLoop].cels[view->CurCel].data, false, pixsize);
    if (view->loops[view->CurLoop].cels[view->CurCel].transcol != transcol)
        set_transcolor(view->loops[view->CurLoop].cels[view->CurCel].transcol);
}

//*********************************************
void ViewEdit::showlooppar()
{
    sprintf(tmp, "%d/%d", view->CurLoop, view->NumLoops - 1);
    loopnum->setText(tmp);
    showmirror();
}

//*********************************************
void ViewEdit::showmirror()
{
    int m = view->loops[view->CurLoop].mirror;
    int m1 = view->loops[view->CurLoop].mirror1;

    mirror_loop->clear();
    mirror_loop->addItem("no other loop");
    mirror_loop->setCurrentIndex(0);
    int item = 1;
    for (int i = 0; i < view->NumLoops; i++) {
        if (i == view->CurLoop)
            continue;
        if ((view->loops[i].mirror == -1 && view->loops[i].mirror1 == -1) || i == m || i == m1) {

            sprintf(tmp, "Loop %d", i);
            mirror_loop->addItem(tmp);
            if (m == i)
                mirror_loop->setCurrentIndex(item);
            else if (m == -1 && m1 == i)
                mirror_loop->setCurrentIndex(item);
            item++;

        }
    }
}

//*********************************************
void ViewEdit::showcelpar()
{
    sprintf(tmp, "%d/%d", view->CurCel, view->loops[view->CurLoop].NumCels - 1);
    celnum->setText(tmp);
    sprintf(tmp, "%d", view->loops[view->CurLoop].cels[view->CurCel].width);
    width->setText(tmp);
    sprintf(tmp, "%d", view->loops[view->CurLoop].cels[view->CurCel].height);
    height->setText(tmp);
}

//*********************************************
void ViewEdit::deinit()
{
    if (description) {
        description->close();
        description = NULL;
    }
    if (animate)
        animate->closeall();
    delete view;
    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}


//*********************************************
void ViewEdit::hideEvent(QHideEvent *)
{
    if (description) {
        description->close();
        description = NULL;
    }
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void ViewEdit::showEvent(QShowEvent *)
{
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//***********************************************
void ViewEdit::closeEvent(QCloseEvent *e)
{
    if (changed) {
        if (ViewNum != -1)
            sprintf(tmp, "Save changes to view.%03d ?", ViewNum);
        else
            sprintf(tmp, "Save changes to view ?");
        strcat(tmp, "\n(view will be saved to game)");

        switch (QMessageBox::warning(this, "View editor",
                                     tmp,
                                     "Yes",
                                     "No",
                                     "Cancel",
                                     0, 2)) {
            case 0: // yes
                save_to_game();
                deinit();
                e->accept();

                //     else
                //e->ignore();
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
void ViewEdit::open_file()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open View File"), game->srcdir.c_str(), tr("View Files (view*.*);;All Files (*)"));
    if (!fileName.isNull())
        open(fileName.toLatin1().data());
}

//*********************************************
void ViewEdit::open()
{
    setWindowTitle("View editor");
    view->newView();
    ViewNum = -1;
    showlooppar();
    showcelpar();
    DisplayView();
    changed = false;
    show();
}

//*********************************************
void ViewEdit::save_file()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save View File"), game->srcdir.c_str(), tr("View Files (view*.*);;All Files (*)"));
    if (!fileName.isNull())
        save(fileName.toLatin1().data());
}

//*********************************************
void ViewEdit::save_to_game()
{
    if (ViewNum != -1) {
        view->save(ViewNum);
        if (resources_win) {
            if (resources_win->preview == NULL)
                resources_win->preview = new Preview();
            resources_win->preview->open(ViewNum, VIEW);
        }
        changed = false;
    } else
        save_to_game_as();
}

//*********************************************
void ViewEdit::save_to_game_as()
{
    AskNumber *view_number = new AskNumber(0, 0, "View number", "Enter view number: [0-255]");

    if (!view_number->exec())
        return;

    QString str = view_number->num->text();
    int num = str.toInt();

    if (num < 0 || num > 255) {
        menu->errmes("View number must be between 0 and 255 !");
        return ;
    }
    if (game->ResourceInfo[VIEW][num].Exists) {
        sprintf(tmp, "Resource view.%03d already exists. Replace it ?", num);

        switch (QMessageBox::warning(this, "View", tmp,
                                     "Replace", "Cancel",
                                     0,      // Enter == button 0
                                     1)) {   // Escape == button 1
            case 0:
                view->save(num);
                changed = false;
                ViewNum = num;
                if (resources_win) {
                    if (resources_win->preview == NULL)
                        resources_win->preview = new Preview();
                    resources_win->preview->open(ViewNum, VIEW);
                }
                break;
            case 1:
                break;
        }
    } else {
        view->save(num);
        changed = false;
        ViewNum = num;
        if (resources_win) {
            resources_win->select_resource_type(VIEW);
            resources_win->set_current(num);
        }
        open(num);
    }
}

//*********************************************
void ViewEdit::delete_view()
{
    int k;

    if (ViewNum == -1)
        return;

    sprintf(tmp, "Really delete view %03d ?", ViewNum);
    switch (QMessageBox::warning(this, "View", tmp,
                                 "Delete", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            game->DeleteResource(VIEW, ViewNum);
            if (resources_win) {
                k = resources_win->list->currentRow();
                resources_win->select_resource_type(VIEW);
                resources_win->list->setCurrentRow(k);
            }
            break;
        case 1:
            break;
    }
}

//*********************************************
void ViewEdit::flipv_cel()
{
    view->loops[curIndex()].cels[view->CurCel].mirrorv();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::fliph_cel()
{
    view->loops[curIndex()].cels[view->CurCel].mirrorh();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::copy_cel()
{
    saveCel.deinit();
    saveCel.copy(view->loops[curIndex()].cels[view->CurCel]);
    cel_copied = true;
}

//*********************************************
void ViewEdit::paste_cel()
{
    if (!cel_copied)
        return;
    saveundo();
    view->loops[curIndex()].cels[view->CurCel].copy(saveCel);
    showcelpar();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::next_loop()
{
    if (view->CurLoop < view->NumLoops - 1) {
        view->CurLoop++;
        view->CurCel = 0;
        showlooppar();
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::previous_loop()
{
    if (view->CurLoop > 0) {
        view->CurLoop--;
        view->CurCel = 0;
        showlooppar();
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::first_loop()
{
    view->CurLoop = 0;
    showlooppar();
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::last_loop()
{
    view->CurLoop = view->NumLoops - 1;
    showlooppar();
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::insert_loop_before()
{
    if (view->NumLoops < MaxLoops - 1) {
        view->insertLoop_before();
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 16 loops already reached.");
}

//*********************************************
void ViewEdit::insert_loop_after()
{
    if (view->NumLoops < MaxLoops - 1) {
        view->insertLoop_after();
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 16 loops already reached.");
}

//*********************************************
void ViewEdit::append_loop()
{
    if (view->NumLoops < MaxLoops - 1) {
        view->appendLoop();
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 16 loops already reached.");
}

//*********************************************

void ViewEdit::delete_loop()
{
    if (view->NumLoops > 1) {
        view->deleteLoop();
        if (view->CurLoop > view->NumLoops - 1)
            view->CurLoop--;
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::clear_loop()
{
    view->loops[view->CurLoop].clear();
    if (view->loops[view->CurLoop].mirror != -1)
        view->loops[view->loops[view->CurLoop].mirror].clear();
    showlooppar();
    showcelpar();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::change_mirror(int i)
{
    if (i == 0) {

        printf("unset mirror %d\n", view->CurLoop);

        if (view->loops[view->CurLoop].mirror != -1)
            view->unsetMirror(view->CurLoop);
        else if (view->loops[view->CurLoop].mirror1 != -1)
            view->unsetMirror(view->loops[view->CurLoop].mirror1);
    } else {
        QString str = mirror_loop->currentText();
        int k = str.toInt() + 5;
        printf("set %d to mirror %d\n", view->CurLoop, k);
        if (view->loops[view->CurLoop].mirror != k) {


            for (int j = 0; j < view->NumLoops; j++) {
                if (view->loops[j].mirror == view->CurLoop)
                    view->unsetMirror(j);
                if (view->loops[j].mirror1 == view->CurLoop)
                    view->loops[j].mirror1 = -1;
            }


            if (view->loops[k].mirror != -1)
                view->unsetMirror(k);
            view->setMirror(view->CurLoop, k);

        }
    }

    showlooppar();
    DisplayView();
    changed = true;
}


//**********************************************************
void ViewEdit::next_cel()
{
    if (view->CurCel < view->loops[view->CurLoop].NumCels - 1) {
        view->CurCel++;
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::next_cel_cycle()
{
    if (view->loops[view->CurLoop].NumCels <= 1)
        return;
    if (view->CurCel < view->loops[view->CurLoop].NumCels - 1)
        view->CurCel++;
    else
        view->CurCel = 0;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::previous_cel()
{
    if (view->CurCel > 0) {
        view->CurCel--;
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::prev_cel_cycle()
{
    if (view->loops[view->CurLoop].NumCels <= 1)
        return;
    if (view->CurCel > 0)
        view->CurCel--;
    else
        view->CurCel = view->loops[view->CurLoop].NumCels - 1;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::first_cel()
{
    view->CurCel = 0;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::last_cel()
{
    view->CurCel = view->loops[view->CurLoop].NumCels - 1;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::insert_cel_before()
{
    if (view->loops[view->CurLoop].NumCels < MaxCels - 1) {
        view->loops[view->CurLoop].insertCel_before(view->CurCel);
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].insertCel_before(view->CurCel);
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 32 cels already reached in this loop.");
}

//*********************************************
void ViewEdit::insert_cel_after()
{
    if (view->loops[view->CurLoop].NumCels < MaxCels - 1) {
        view->loops[view->CurLoop].insertCel_after(view->CurCel);
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].insertCel_after(view->CurCel);
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 32 cels already reached in this loop.");
}

//*********************************************
void ViewEdit::append_cel()
{
    if (view->loops[view->CurLoop].NumCels < MaxCels - 1) {
        view->loops[view->CurLoop].appendCel();
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].appendCel();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 32 cels already reached in this loop.");
}

//*********************************************
void ViewEdit::delete_cel()
{
    if (view->loops[view->CurLoop].NumCels > 1) {
        view->loops[view->CurLoop].deleteCel(view->CurCel);
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].deleteCel(view->CurCel);
        if (view->CurCel >= view->loops[view->CurLoop].NumCels)
            view->CurCel--;
        showcelpar();
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::dec_width()
{
    int w;

    if ((w = view->loops[view->CurLoop].cels[view->CurCel].width) > 1) {
        w--;
        view->loops[view->CurLoop].cels[view->CurCel].setW(w);
        sprintf(tmp, "%d", w);
        width->setText(tmp);
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::inc_width()
{
    int w = view->loops[view->CurLoop].cels[view->CurCel].width + 1;
    if (w < 160) {
        view->loops[view->CurLoop].cels[view->CurCel].setW(w);
        sprintf(tmp, "%d", w);
        width->setText(tmp);
        DisplayView();
        changed = true;
    } else
        menu->errmes("Maximum width is 160.");
}

//*********************************************
void ViewEdit::dec_height()
{
    int h;
    if ((h = view->loops[view->CurLoop].cels[view->CurCel].height) > 1) {
        h--;
        view->loops[view->CurLoop].cels[view->CurCel].setH(h);
        sprintf(tmp, "%d", h);
        height->setText(tmp);
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::inc_height()
{
    int h = view->loops[view->CurLoop].cels[view->CurCel].height + 1;

    if (h < 168) {
        view->loops[view->CurLoop].cels[view->CurCel].setH(h);
        sprintf(tmp, "%d", h);
        height->setText(tmp);
        DisplayView();
        changed = true;
    } else
        menu->errmes("Maximum height is 168.");
}

//*********************************************
void ViewEdit::change_width_height()
{
    QString str = width->text();
    int w = str.toInt();
    view->loops[view->CurLoop].cels[view->CurCel].setW(w);

    str = height->text();
    int h = str.toInt();
    view->loops[view->CurLoop].cels[view->CurCel].setH(h);

    DisplayView();
    changed = true;
    width->clearFocus();
    height->clearFocus();
    setFocus();
}

//*********************************************
void ViewEdit::shift_right()
{
    if (view->loops[view->CurLoop].mirror == -1)
        view->loops[view->CurLoop].cels[view->CurCel].right();
    else
        view->loops[curIndex()].cels[view->CurCel].left();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::shift_left()
{
    if (view->loops[view->CurLoop].mirror == -1)
        view->loops[view->CurLoop].cels[view->CurCel].left();
    else
        view->loops[curIndex()].cels[view->CurCel].right();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::shift_up()
{
    view->loops[curIndex()].cels[view->CurCel].up();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::shift_down()
{
    view->loops[curIndex()].cels[view->CurCel].down();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::fillCel(int x, int y, byte color)
{
    saveundo();
    view->loops[curIndex()].cels[view->CurCel].fill(x, y, color);
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::clear_cel()
{
    saveundo();
    view->loops[curIndex()].cels[view->CurCel].clear();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::saveundo()
{
    undoCel.deinit();
    undoCel.copy(view->loops[curIndex()].cels[view->CurCel]);
    undo = true;
}

//*********************************************
void ViewEdit::undo_cel()
{
    if (undo) {
        view->loops[curIndex()].cels[view->CurCel].copy(undoCel);
        undo = false;
    }
    DisplayView();
}

/*************************************************/
int ViewEdit::curIndex()
{
    int i = view->loops[view->CurLoop].mirror;
    if (i == -1)
        return view->CurLoop;
    else
        return i;
}

/*************************************************/
void ViewEdit::show_description()
{
    if (!description)
        description = new Description(0, 0, this);
    description->set();
    description->show();
}

/*******************************************************/
void ViewEdit::change_mode(int m)
{
    drawing_mode = m;
}

/*******************************************************/
void ViewEdit::change_mode1(int m)
{
    drawing_mode = m;
    if (m == V_DRAW)
        view_draw->setChecked(true);
    else
        view_fill->setChecked(true);
}

/*******************************************************/
void ViewEdit::is_descriptor_cb()
{
    if (is_descriptor->isChecked())
        edit_descriptor->setEnabled(true);
    else
        edit_descriptor->setEnabled(false);
}

/*******************************************************/
void ViewEdit::set_transcolor()
{
    transcolor->setPalette(QPalette(egacolor[palette->left]));
    transcol = palette->left;
    view->loops[view->CurLoop].cels[view->CurCel].transcol = transcol;
}

/*******************************************************/
void ViewEdit::set_transcolor(int col)
{
    transcolor->setPalette(QPalette(egacolor[col]));
    transcol = col;
    view->loops[view->CurLoop].cels[view->CurCel].transcol = transcol;
}

/*******************************************************/
void ViewEdit::zoom_minus()
//zoom_out
{
    if (canvas->pixsize > 1)
        DisplayView(canvas->pixsize - 1);
}

/*******************************************************/
void ViewEdit::zoom_plus()
//zoom_in
{
    if (canvas->pixsize < 10)
        DisplayView(canvas->pixsize + 1);
}

/*******************************************************/
bool ViewEdit::focusNextPrevChild(bool)
{
    if (width->hasFocus())
        height->setFocus();
    else if (height->hasFocus())
        width->setFocus();
    else
        canvas->setFocus();
    return true;
}

/*******************************************************/
void ViewEdit::animate_cb()
{
    if (animate == NULL)
        animate = new Animate(0, 0, 0, this);
    animate->show();
}

/*******************************************************/
Animate::Animate(QWidget *parent, const char *name, Preview *p, ViewEdit *v)
    : QWidget(parent)
{
    viewedit = v;
    preview = p;
    setWindowTitle("Animate");

    QBoxLayout *b = new QVBoxLayout(this);

    QHBoxLayout *b1 = new QHBoxLayout(this);
    b->addLayout(b1);
    QLabel *l = new QLabel("Delay (ms)", this);
    b1->addWidget(l);
    delay = new QLineEdit(this);
    delay->setText("200");
    delay->setMaximumWidth(100);
    b1->addWidget(delay);

    QButtonGroup *fb = new QButtonGroup(this);
    fb->setExclusive(true);
    forward = new QRadioButton("Forward");
    forward->setChecked(true);
    backward = new QRadioButton("Backward");
    connect(fb, SIGNAL(clicked(int)), SLOT(fb_cb()));
    fb->addButton(forward);
    fb->addButton(backward);
    b1->addWidget(forward);
    b1->addWidget(backward);

    QHBoxLayout *b2 = new QHBoxLayout(this);
    b->addLayout(b2);
    button = new QPushButton(this);
    button->setText("Start");
    b2->addWidget(button);
    connect(button, SIGNAL(clicked()), SLOT(start_stop()));
    QPushButton *close = new QPushButton(this);
    close->setText("Close");
    b2->addWidget(close);
    connect(close, SIGNAL(clicked()), SLOT(hide()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(next_cel()));
}

/*******************************************************/
void Animate::start_stop()
{
    if (timer->isActive()) {
        timer->stop();
        button->setText("Start");
    } else {
        QString str = delay->text();
        num = str.toInt();
        button->setText("Stop");
        fwd = forward->isChecked();
        timer->start(num);
    }
}

/*******************************************************/
void Animate::fb_cb()
{
    fwd = forward->isChecked();
}

/*******************************************************/
void Animate::next_cel()
{
    if (viewedit) {
        if (fwd)
            viewedit->next_cel_cycle();
        else
            viewedit->prev_cel_cycle();
    } else {
        if (fwd)
            preview->next_cel_cycle();
        else
            preview->prev_cel_cycle();
    }
}

/*******************************************************/
void Animate::closeall()
{
    if (timer->isActive())
        timer->stop();
    close();
}

/*******************************************************/
Description::Description(QWidget *parent, const char *name, ViewEdit *v)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("View description");
    viewedit = v;

    QBoxLayout *d1 = new QVBoxLayout(this);
    d1->addSpacing(10);

    smallview = new ViewIcon(this, 0, viewedit);
    smallview->setMinimumSize(64, 64);

    d1->addWidget(smallview, 0, Qt::AlignCenter);

    desc = new QTextEdit(this);
    desc->setMinimumSize(300, 100);
    d1->addWidget(desc, 1);

    QBoxLayout *d2 = new QHBoxLayout(this);
    d1->addLayout(d2);
    d2->addSpacing(10);

    QPushButton *ok = new QPushButton(this);
    ok->setText("OK");
    ok->setMaximumWidth(80);
    connect(ok, SIGNAL(clicked()), SLOT(ok_cb()));
    d2->addWidget(ok);

    QPushButton *cancel = new QPushButton(this);
    cancel->setText("Cancel");
    cancel->setMaximumWidth(80);
    connect(cancel, SIGNAL(clicked()), SLOT(cancel_cb()));
    d2->addWidget(cancel);

    adjustSize();
    hide();
    getmaxcol();
}

//*********************************************
void Description::getmaxcol()
//get maximum number of columns on screen (approx.)
//to wrap the long lines
{
    QFontMetrics f = fontMetrics();
    maxcol = desc->width() / f.width('a');
}

//*********************************************
void Description::resizeEvent(QResizeEvent *)
{
    getmaxcol();
    set();
}

//*********************************************
void Description::set()
{
    int n;

    desc->clear();

    if (viewedit->view->Description == "")
        return;

    std::string ThisLine = "";
    std::string ThisMessage = viewedit->view->Description;

    do {
        if (ThisMessage.length() + ThisLine.length() > maxcol) {
            n = maxcol - ThisLine.length();
            do {
                n--;
            } while (!(n == 0 || ThisMessage[n] == ' '));
            if (n <= 0)
                n = maxcol - ThisLine.length();
            ThisLine += ThisMessage.substr(0, n);
            ThisMessage = (n < (int)ThisMessage.length()) ? ThisMessage.substr(n + 1) : "";
            desc->insertPlainText(ThisLine.c_str());
            ThisLine = "";
        } else {
            ThisLine += ThisMessage;
            ThisMessage = "";
        }
    } while (ThisMessage != "");

    if (ThisLine != "")
        desc->insertPlainText(ThisLine.c_str());
}

//*********************************************
void Description::ok_cb()
{
    int i;

    QString str = desc->toPlainText();
    char *s = (char *)str.toLatin1().data();
    tmp[0] = 0;
    for (i = 0; *s; s++) {
        if (*s != '\n')
            tmp[i++] = *s;
        else if (i > 1 && tmp[i - 1] != ' ')
            tmp[i++] = ' ';
    }
    tmp[i] = 0;

    if (strcmp(viewedit->view->Description.c_str(), tmp)) {
        viewedit->view->Description = std::string(tmp);
        viewedit->changed = true;
    }
    hide();
}

//*********************************************
void Description::cancel_cb()
{
    hide();
}

//**********************************************
Canvas::Canvas(QWidget *parent, const char *name, ViewEdit *v)
    : QScrollArea(parent)
{
    viewedit = v;
    x0 = 10;
    y0 = 10;
    pixsize = 2;
    cur_mirror = false;
    pixmap = QPixmap();
    cur_w = cur_h = 0;

    imagecontainer = new QLabel;
    this->setWidget(imagecontainer);
    imagecontainer->resize(cur_w, cur_h);
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void Canvas::setSize(int w, int h)
{
    if (cur_w != w || cur_h != h) {
        if (!pixmap)
            pixmap = QPixmap(w * pixsize * 2, h * pixsize);
        else
            pixmap = pixmap.scaled(w * pixsize * 2, h * pixsize);
        cur_w = w;
        cur_h = h;

        imagecontainer->resize(w * pixsize * 2, h * pixsize);
        imagecontainer->setPixmap(pixmap);
    }
}

//*********************************************
void Canvas::mousePressEvent(QMouseEvent *event)
{
    int x = event->x(), y = event->y();

    if (event->button() & Qt::LeftButton)
        CurColor = viewedit->palette->left;
    else if (event->button() & Qt::RightButton)
        CurColor = viewedit->palette->right;
    UpdateCel(x - x0, y - y0);
    viewedit->changed = true;
}

//*********************************************
void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->x(), y = event->y();
    UpdateCel(x - x0, y - y0);
}

//*********************************************
void Canvas::drawContents(QPainter *p, int, int, int, int)
{
    if (cur_w == 0 || cur_h == 0)
        return;
    p->drawPixmap(x0, y0, pixmap);
}

//*********************************************
void Canvas::DrawCel(int w, int h, byte *celdata, bool mirror, int size)
{
    int x, y, ww, hh, w0, h0, ww0, hh0;

    w0 = cur_w;
    h0 = cur_h;
    ww0 = (x0 + w0) * 2 * pixsize;
    hh0 = (y0 + h0) * pixsize;
    pixsize = size;
    pixmap = pixmap.scaled(cur_w * pixsize * 2, cur_h * pixsize);
    ww = (x0 + w) * 2 * pixsize;
    hh = (y0 + h) * pixsize;

    QPainter p(&pixmap);

    data = celdata;

    if (mirror) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + w * 2 - 2 - x]]);
        }
    } else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + x]]);
        }
    }
    repaint(x0, y0, MAX(ww, ww0), MAX(hh, hh0));

    imagecontainer->resize(cur_w * pixsize * 2, cur_h * pixsize);
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void Canvas::DrawCel(int w, int h, byte *celdata, bool mirror)
{
    int x, y, ww, hh, w0, h0, ww0 = 0, hh0 = 0;
    bool changed;


    if (cur_w != w || cur_h != h) {
        changed = true;
        w0 = cur_w;
        h0 = cur_h;
        ww0 = (x0 + w0) * 2 * pixsize;
        hh0 = (y0 + h0) * pixsize;
        setSize(w, h);
    } else
        changed = false;

    ww = (x0 + w) * 2 * pixsize;
    hh = (y0 + h) * pixsize;

    QPainter p(&pixmap);

    cur_mirror = mirror;
    data = celdata;

    if (mirror) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + w * 2 - 2 - x]]);
        }
    } else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + x]]);
        }
    }

    if (changed)
        repaint(x0, y0, MAX(ww, ww0), MAX(hh, hh0));
    else
        repaint(x0, y0, ww, hh);

    imagecontainer->resize(cur_w * pixsize * 2, cur_h * pixsize);
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void Canvas::UpdateCel(int x, int y)
{
    int xn = x / pixsize / 2;
    int yn = y / pixsize;

    if (xn >= 0 && xn < cur_w && yn >= 0 && yn < cur_h) {

        QPainter p(&pixmap);
        if (viewedit->drawing_mode == V_DRAW) {

            x = xn * 2 * pixsize;
            y = yn * pixsize;

            p.fillRect(x, y, pixsize * 2, pixsize, egacolor[CurColor]);
            repaint(x0 + x, y0 + y, pixsize * 2, pixsize);
            if (cur_mirror) {
                data[yn * cur_w * 2 + cur_w * 2 - 2 - xn * 2] = CurColor;
                data[yn * cur_w * 2 + cur_w * 2 - 2 - xn * 2 + 1] = CurColor;
            } else {
                data[yn * cur_w * 2 + xn * 2] = CurColor;
                data[yn * cur_w * 2 + xn * 2 + 1] = CurColor;
            }
        } else { //FILL
            if (cur_mirror)
                viewedit->fillCel(cur_w - 1 - xn, yn, CurColor);
            else
                viewedit->fillCel(xn, yn, CurColor);
        }
    }
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void Canvas::keyPressEvent(QKeyEvent *k)
{
    //  printf("key ! %d\n",k->key());
    switch (k->key()) {
        case Qt::Key_Q:
            viewedit->previous_loop();
            break;
        case Qt::Key_W:
            viewedit->next_loop();
            break;
        case Qt::Key_A:
            viewedit->previous_cel();
            break;
        case Qt::Key_S:
            viewedit->next_cel();
            break;
        case Qt::Key_Z:
            viewedit->zoom_minus();
            break;
        case Qt::Key_X:
            viewedit->zoom_plus();
            break;
        case Qt::Key_T:
            viewedit->set_transcolor();
            break;
        case Qt::Key_D:
            viewedit->change_mode1(V_DRAW);
            break;
        case Qt::Key_F:
            viewedit->change_mode1(V_FILL);
            break;
        case Qt::Key_I:
            viewedit->shift_up();
            break;
        case Qt::Key_K:
            viewedit->shift_down();
            break;
        case Qt::Key_J:
            viewedit->shift_left();
            break;
        case Qt::Key_L:
            viewedit->shift_right();
            break;
        default:
            k->ignore();
            break;
    }
}

//*********************************************
bool Canvas::focusNextPrevChild(bool)
{
    setFocus();
    return true;
}


//********************************************
ViewIcon::ViewIcon(QWidget *parent, const char *name, ViewEdit *v)
    : QWidget(parent)
{
    viewedit = v;
}

//*********************************************
void ViewIcon::paintEvent(QPaintEvent *)
{
    int x, y;

    QPainter p(this);

    int w = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].width;
    int h = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].height;
    bool mirror = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].mirror;
    byte *data = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].data;

    int pixsize = 2;

    int W = viewedit->description->width();
    setGeometry((W - pixsize * w * 2) / 2, 10, pixsize * w * 2, pixsize * h);

    if (pixsize * w * 2 > width() || pixsize * h > height())
        resize(pixsize * w * 2, pixsize * h);


    if (mirror) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + w * 2 - 2 - x]]);
        }
    } else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + x]]);
        }
    }
}
