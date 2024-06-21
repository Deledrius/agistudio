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


#include <QFileDialog>
#include <QPainter>
#include <QKeyEvent>
#include <QPixmap>
#include <QRadioButton>
#include <QAbstractButton>
#include <QMouseEvent>
#include <QShowEvent>
#include <QInputDialog>

#include "game.h"
#include "menu.h"
#include "preview.h"
#include "picedit.h"


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
    : QMainWindow(parent), PicNum(-1), winnum(win_num), resources_win(res),
      closing(false), changed(false), viewdata(nullptr), pri_mode(0)
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    picture = new Picture();
    palette = new Palette1(this, nullptr, this);
    toolColumnLayout->replaceWidget(PaletteWidget, palette);

    // Set RadioButton IDs, as they can't be set in the designer
    toolButtonGroup->setId(lineButton, T_LINE);
    toolButtonGroup->setId(penButton, T_PEN);
    toolButtonGroup->setId(stepButton, T_STEP);
    toolButtonGroup->setId(fillButton, T_FILL);
    toolButtonGroup->setId(brushButton, T_BRUSH);

    toolShapeButtonGroup->setId(circleButton, 0);
    toolShapeButtonGroup->setId(squareButton, 1);

    toolTypeButtonGroup->setId(sprayButton, 0);
    toolTypeButtonGroup->setId(solidButton, 1);

    drawmodeButtonGroup->setId(visualLayerToggle, 0);
    drawmodeButtonGroup->setId(priorityLayerToggle, 1);

    // Create our custom statusbar display
    QLabel *msg = new QLabel(statusbar);
    statusbar->addPermanentWidget(msg);
    pricolor = new QFrame(statusbar);
    pricolor->setMinimumSize(8, 8);
    pricolor->setMaximumSize(8, 8);
    pricolor->setToolTip(tr("Priority 'color' required to mask Ego on this priority level"));
    pricolor->setAutoFillBackground(true);
    statusbar->addPermanentWidget(pricolor);

    // Set up our custom canvas
    if (game->picstyle == P_TWO) {
        canvas = new PCanvas(nullptr, 0, this);
        canvas->setMinimumSize(canvas->pixsize * MAX_W + canvas->x0 + 10, canvas->pixsize * MAX_HH + canvas->y0 + 10);
        canvas->resize(canvas->pixsize * MAX_W + canvas->x0, canvas->pixsize * MAX_HH + canvas->x0);

    } else {
        canvas = new PCanvas(this, 0, this);
        canvas->setMinimumSize(canvas->pixsize * MAX_W + canvas->x0 + 10, canvas->pixsize * MAX_HH + canvas->y0 + 10);
        canvas->resize(canvas->pixsize * MAX_W + canvas->x0, canvas->pixsize * MAX_HH + canvas->x0);
        canvas->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        canvasLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        canvasLayout->addWidget(canvas);
    }

    change_drawmode(drawmodeButtonGroup->checkedId());
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
    auto document_title = windowTitle() + QString(": picture.%1").arg(QString::number(PicNum), 3, '0');
    setWindowTitle(document_title);
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
    if (canvas->isWindow()) {
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
    if (canvas->isWindow() && !canvas->isVisible())
        canvas->showNormal();
    showing = false;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void PicEdit::hideEvent(QHideEvent *)
{
    hiding = true;
    if (isMinimized() && canvas->isWindow() && canvas->isVisible())
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
        switch (QMessageBox::warning(this, tr("Picture Editor"), tr("Save changes to picture?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel)) {
            case QMessageBox::Save:
                save_to_game();
                deinit();
                e->accept();
                break;
            case QMessageBox::Discard:
                deinit();
                e->accept();
                break;
            default: // Cancel
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
    bool ok;
    int num = QInputDialog::getInt(this, tr("Picture Number"), tr("Enter picture number [0-255]:"), PicNum, 0, 255, 1, &ok);
    if (!ok)
        return;

    if (game->ResourceInfo[PICTURE][num].Exists) {
        switch (QMessageBox::warning(this, tr("Picture Editor"),
                                     tr("Resource picture.%1 already exists. Replace it?").arg(QString::number(num), 3, QChar('0')),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No)) {
            case QMessageBox::Yes:
                picture->save(num);
                PicNum = num;
                if (resources_win) {
                    if (resources_win->preview == NULL)
                        resources_win->preview = new Preview();
                    resources_win->preview->open(PicNum, PICTURE);
                }
                changed = false;
                break;
            default:
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
    if (PicNum == -1)
        return;
    switch (QMessageBox::warning(this, tr("Picture Editor"),
                                 tr("Really delete picture %1?").arg(QString::number(PicNum), 3, QChar('0')),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            game->DeleteResource(PICTURE, PicNum);
            if (resources_win) {
                int k = resources_win->list->currentRow();
                resources_win->select_resource_type(PICTURE);
                resources_win->list->setCurrentRow(k);
            }
            break;
        default:
            break;
    }
}

//*********************************************
void PicEdit::open()
{
    picture->newpic();
    show_pos();
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
        viewdata = new ViewData(nullptr, nullptr, picture);
    if (PicNum != -1)
        viewdata->setWindowTitle(QString(tr("View data: picture.%1")).arg(QString::number(PicNum), 3, '0'));
    else
        viewdata->setWindowTitle(tr("View data: picture"));
    viewdata->read();
}

//*********************************************
void PicEdit::background()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Background Image"), game->srcdir.c_str(), tr("All Files (*)"));
    if (!fileName.isNull()) {
        canvas->load_bg(fileName);
        showBackground->setEnabled(true);
    }
}

//*********************************************
void PicEdit::on_zoomOutButton_clicked()
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
void PicEdit::on_zoomInButton_clicked()
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
            priorityLayerToggle->setChecked(false);
            visualLayerToggle->setChecked(true);
            break;
        case 1:  //draw priority
            pri_mode = true;
            picture->set_mode(1);
            visualLayerToggle->setChecked(false);
            priorityLayerToggle->setChecked(true);
            break;
    }
    canvas->linedraw = false;
    canvas->update();
}

//*********************************************
void PicEdit::toggle_bgmode(bool checked)
{
    canvas->bg_on = checked;
    if (canvas->bg_on && canvas->bg_loaded)
        picture->bg_on = true;
    else
        picture->bg_on = false;

    canvas->linedraw = false;
    canvas->update();
}

//*********************************************
void PicEdit::toggle_prilinemode(bool checked)
{
    canvas->pri_lines = checked;
    canvas->linedraw = false;
    canvas->update();
}

//*********************************************
void PicEdit::change_tool(int k)
{
    if (canvas->linedraw)
        canvas->line(false);
    picture->tool_proc(k);
    update_tools();
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
void PicEdit::deselect_tool()
{
    QAbstractButton *checked = toolButtonGroup->checkedButton();
    if (checked) {
        toolButtonGroup->setExclusive(false);
        checked->setChecked(false);
        toolButtonGroup->setExclusive(true);
    }
}

//*********************************************
void PicEdit::enable_background(bool mode)
{
    showBackground->setChecked(mode);
}

//*********************************************
bool PicEdit::background_enabled() const
{
    return showBackground->isChecked();
}

//*********************************************
void PicEdit::show_pos()
//show current picture buffer position
{
    unsigned char code = 0, val = -1;
    actionFrameDisplay->setValue(picture->getPos());
    actionCodeDisplay->setText(picture->showPos(&code, &val));

    if (code >= action_codes_start && code <= action_codes_end) {       // Action: can add comments
        if (code == SetPicColor || code == SetPriColor)                 // Add color name
            actionCodeDesc->setText(QString("%1 %2").arg(comment[code - action_codes_start]).arg(colname[val]));
        else
            actionCodeDesc->setText(comment[code - action_codes_start]);
    } else
        actionCodeDesc->clear();
}

//*********************************************
void PicEdit::on_startButton_clicked()
//set picture buffer position to start
{
    picture->home_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::on_endButton_clicked()
//set picture buffer position to end
{
    picture->end_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::on_prevButton_clicked()
//set picture buffer position to the previous action
{
    picture->left_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::on_nextButton_clicked()
//set picture buffer position to the next action
{
    picture->right_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::on_delButton_clicked()
//delete action from the picture buffer position
{
    picture->del_proc();
    canvas->update();
    show_pos();
    update_palette();
    update_tools();
}

//*********************************************
void PicEdit::on_wipeButton_clicked()
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
    int num = actionFrameDisplay->value();

    if (num != 0) {
        if (!picture->setBufPos(num))
            canvas->update();
    }
    show_pos();
    update_palette();
    update_tools();
    actionFrameDisplay->clearFocus();
    setFocus();
    return;
}

//*********************************************
void PicEdit::update_tools()
{
    shapeGroup->setEnabled(false);
    typeGroup->setEnabled(false);
    sizeGroup->setEnabled(false);

    switch (picture->tool) {
        case T_LINE:
            if (!lineButton->isChecked())
                lineButton->setChecked(true);
            break;
        case T_STEP:
            if (!stepButton->isChecked())
                stepButton->setChecked(true);
            break;
        case T_PEN:
            if (!penButton->isChecked())
                penButton->setChecked(true);
            break;
        case T_FILL:
            if (!fillButton->isChecked())
                fillButton->setChecked(true);
            break;
        case T_BRUSH:
            if (!brushButton->isChecked())
                brushButton->setChecked(true);
            shapeGroup->setEnabled(true);
            typeGroup->setEnabled(true);
            sizeGroup->setEnabled(true);
            break;
        default:
            QRadioButton *b = (QRadioButton *)toolButtonGroup->checkedButton();
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
//
PCanvas::PCanvas(QWidget *parent, const char *name, PicEdit *w)
    : QScrollArea(parent),
      picedit(w), pixsize(2), x0(0), y0(0), x1(0), y1(0),
      bg_loaded(false), bg_on(false), imagecontainer(),
      cur_w(MAX_W * pixsize), cur_h(MAX_HH * pixsize), pri_lines(false),
      bgpix(), pixmap(cur_w, cur_h), CurColor(), linedraw()
      // The area to draw picture
{
    picture = picedit->picture;

    imagecontainer = new QLabel(this);
    imagecontainer->resize(cur_w, cur_h);
    imagecontainer->setPixmap(pixmap);
    imagecontainer->setMouseTracking(true);

    this->setFrameStyle(QFrame::NoFrame);
    this->setWidget(imagecontainer);
    this->setMouseTracking(true);
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
    p.end();
    update();
    imagecontainer->resize(cur_w, cur_h);
}

//*********************************************
void PCanvas::mousePressEvent(QMouseEvent *event)
{
    int x = event->pos().x(), y = event->pos().y();
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
        picedit->deselect_tool();
        picture->clear_tools();
        picture->tool = -1;
        picture->init_tool();
    }
    update();
    picedit->changed = true;
}

//*********************************************
void PCanvas::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->pos().x(), y = event->pos().y();
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
        auto msg = picedit->statusbar->findChild<QLabel *>();
        msg->setText(QString("X=%1  Y=%2  Pri=%3").arg(x / 2).arg(y).arg(pri));
        auto pal = picedit->pricolor->palette();
        pal.setColor(QPalette::Window, egacolor[pri + 1]);
        picedit->pricolor->setPalette(pal);
    }
}

//*********************************************
void PCanvas::drawContents(QPainter *p, int, int, int, int) const
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
    imagecontainer->setPixmap(pixmap);
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
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void PCanvas::keyPressEvent(QKeyEvent *k)
{
    switch (k->key()) {
        case Qt::Key_L:
        case Qt::Key_F1:
            picedit->change_tool(T_LINE);
            break;
        case Qt::Key_P:
        case Qt::Key_F2:
            picedit->change_tool(T_PEN);
            break;
        case Qt::Key_S:
        case Qt::Key_F3:
            picedit->change_tool(T_STEP);
            break;
        case Qt::Key_F:
        case Qt::Key_F4:
            picedit->change_tool(T_FILL);
            break;
        case Qt::Key_B:
        case Qt::Key_F5:
            picedit->change_tool(T_BRUSH);
            break;
        case Qt::Key_Tab:
            picedit->change_drawmode(picedit->pri_mode ? 0 : 1);
            break;
        case Qt::Key_Home:
            picedit->on_startButton_clicked();
            break;
        case Qt::Key_End:
            picedit->on_endButton_clicked();
            break;
        case Qt::Key_Right:
            picedit->on_nextButton_clicked();
            break;
        case Qt::Key_Left:
            picedit->on_prevButton_clicked();
            break;
        case Qt::Key_Delete:
            picedit->on_delButton_clicked();
            break;
        case Qt::Key_F10:
            if (picedit->background_enabled())
                picedit->enable_background(false);
            else
                picedit->enable_background(true);
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
void PCanvas::load_bg(QString &filename)
{
    if (!bgpix.load(filename)) {
        menu->errmes(QString("Can't open file %s!").arg(filename).toStdString().c_str());
        return;
    }
    bg_loaded = true;

    picture->bgpix = &bgpix;
    picedit->enable_background(true);
    bg_on = true;
    picture->bg_on = true;
    update();
}

//*********************************************
void PCanvas::showEvent(QShowEvent *)
{
    if (isWindow() && !picedit->isVisible() && !picedit->showing)
        picedit->showNormal();
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void PCanvas::hideEvent(QHideEvent *)
{
    if (isWindow() && picedit->isVisible() && !picedit->hiding)
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
    if (isWindow()) {
        picedit->close();
        e->ignore();
    }
}

/*******************************************************/
//
Palette1::Palette1(QWidget *parent, const char *name, PicEdit *p)
    : QWidget(parent), left(-1), right(-1), picedit(p)
{
    this->setMinimumSize(250, 40);
    this->setMaximumSize(350, 80);
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

    x = event->pos().x() / dx;
    y = event->pos().y() / dy;

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
//
ViewData::ViewData(QWidget *parent, const char *name, Picture *p)
    : QWidget(parent), picture(p), maxcol(0)
      //view picture codes
{
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

    data.clear();
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
    maxcol = codes->width() / f.averageCharWidth();
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
    for (i = 0; i < data.count(); i++) {
        std::string str2 = data.at(i).toStdString();
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
                strcat(tmp, " //\n");
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
                sprintf(tmp, "%s\t// %s", str, comment[c - 0xf0]);
                if (c == 0xf0 || c == 0xf2) {
                    strcat(tmp, " ");
                    strcat(tmp, colname[cc]);
                }
                strcat(tmp, "\n");
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
