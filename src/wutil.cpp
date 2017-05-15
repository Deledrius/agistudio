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

#include <stdio.h>

#include <QPainter>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>

#include "wutil.h"

QColor egacolor[16];

//**********************************************

AskNumber::AskNumber(QWidget *parent, const char *name, const char *caption, const char *prompt)
    : QDialog(parent)
{
    setWindowTitle(caption);

    QBoxLayout *all =  new QVBoxLayout(this);

    QBoxLayout *top = new QHBoxLayout(this);
    all->addLayout(top);
    QLabel *label = new QLabel(prompt, this);
    top->addWidget(label);
    num = new QLineEdit(this);
    num->setMinimumWidth(80);
    connect(num, SIGNAL(returnPressed()), SLOT(accept()));
    top->addWidget(num);

    QBoxLayout *bottom = new QHBoxLayout(this);
    all->addLayout(bottom);
    QPushButton *ok = new QPushButton(this);
    ok->setText("OK");
    connect(ok, SIGNAL(clicked()), SLOT(accept()));
    bottom->addWidget(ok);
    QPushButton *cancel = new QPushButton(this);
    cancel->setText("Cancel");
    connect(cancel, SIGNAL(clicked()), SLOT(reject()));
    bottom->addWidget(cancel);
}

AskText::AskText(QWidget *parent, const char *name, const char *caption, const char *prompt)
    : QDialog(parent)
{
    setWindowTitle(caption);

    QBoxLayout *all =  new QVBoxLayout(this);

    QLabel *label = new QLabel(prompt, this);
    all->addWidget(label);
    text = new QLineEdit(this);
    text->setMinimumWidth(120);
    connect(text, SIGNAL(returnPressed()), SLOT(accept()));
    all->addWidget(text);

    QBoxLayout *bottom = new QHBoxLayout(this);
    all->addLayout(bottom);
    QPushButton *ok = new QPushButton(this);
    ok->setText("OK");
    connect(ok, SIGNAL(clicked()), SLOT(accept()));
    bottom->addWidget(ok);
    QPushButton *cancel = new QPushButton(this);
    cancel->setText("Cancel");
    connect(cancel, SIGNAL(clicked()), SLOT(reject()));
    bottom->addWidget(cancel);
}


//**********************************************
void make_egacolors(void)
{
    static bool ok = false;

    if (ok)
        return;

    egacolor[0] = QColor(0, 0, 0);     //black
    egacolor[1] = QColor(0, 0, 0xa0);  //blue
    egacolor[2] = QColor(0, 0xa0, 0);  //green
    egacolor[3] = QColor(0, 0xa0, 0xa0); //cyan
    egacolor[4] = QColor(0xa0, 0, 0);  //red
    egacolor[5] = QColor(0xa0, 0, 0xa0); //magenta
    egacolor[6] = QColor(0xa0, 0x50, 0); //brown
    egacolor[7] = QColor(0xa0, 0xa0, 0xa0); //lightgray
    egacolor[8] = QColor(0x50, 0x50, 0x50); //gray
    egacolor[9] = QColor(0x50, 0x50, 0xff); //lightblue
    egacolor[10] = QColor(0x50, 0xff, 0x50); //lightgreen
    egacolor[11] = QColor(0x50, 0xff, 0xff); //lightcyan
    egacolor[12] = QColor(0xff, 0x50, 0x50); //lightred
    egacolor[13] = QColor(0xff, 0x50, 0xff); //lightmagenta
    egacolor[14] = QColor(0xff, 0xff, 0x50); //yellow
    egacolor[15] = QColor(0xff, 0xff, 0xff); //white

    ok = true;
}

//*********************************************

/*******************************************************/
Palette::Palette(QWidget *parent, const char *name)
    : QWidget(parent), left(0), right(0) {}


void Palette::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    int w, h, x, y, dx, dy, i;

    w = this->width();
    h = this->height();
    dx = w / 8;
    dy = h / 2;
    w = dx * 8;
    h = dy * 2;

    for (y = 0, i = 0; y < h; y += dy) {
        for (x = 0; x < w; x += dx, i++) {
            p.fillRect(x, y, dx, dy, egacolor[i]);
            if (i == left) {
                p.setPen(i < 10 ? egacolor[15] : egacolor[0]); //set font !
                p.drawText(x + dx / 4, y + dy / 2, "L");
            }
            if (i == right) {
                p.setPen(i < 10 ? egacolor[15] : egacolor[0]);
                p.drawText(x + dx * 2 / 3, y + dy / 2, "R");
            }
        }
    }
}

void Palette::mousePressEvent(QMouseEvent *event)
{
    int w, h, x, y, dx, dy, i;

    w = this->width();
    h = this->height();
    dx = w / 8;
    dy = h / 2;
    w = dx * 8;
    h = dy * 2;

    x = event->x() / dx;
    y = event->y() / dy;
    i = y * 8 + x;

    if (event->button() & Qt::LeftButton)
        left = i;
    else if (event->button() & Qt::RightButton)
        right = i;

    repaint();
}
