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

#ifndef WUTIL_H
#define WUTIL_H


#include <vector>

#include <QWidget>


class QColor;
class QPaintEvent;
class QMouseEvent;

//*****************************************************
class Palette : public QWidget
{
    Q_OBJECT
public:
    Palette(QWidget *parent = 0, const char *name = 0);
    void paintEvent(class QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
    int left, right;
};

//*****************************************************
extern QColor egacolor[];
extern QList<QRgb> egaColorTable;
extern void make_egacolors(void);

#endif
