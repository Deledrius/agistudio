/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  Almost all of the picture processing code is taken from showpic.c
 *  by Lance Ewing <lance.e@ihug.co.nz>
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
#include "picture.h"


#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

BPicture *ppicture;

//********************************************
//"bytemap" picture for preview - it is not going to be edited,
//so there is no need for the linked list and other things from the Picture class

BPicture::BPicture()
{
    picture = (byte **)malloc(MAX_H * sizeof(byte *));
    priority = (byte **)malloc(MAX_H * sizeof(byte *));
    for (int i = 0; i < MAX_H; i++) {
        picture[i] = (byte *)malloc(MAX_W);
        priority[i] = (byte *)malloc(MAX_W);
    }
}
//****************************************************

void BPicture::qstore(byte q)
{
    if (spos + 1 == rpos || (spos + 1 == QUMAX && !rpos)) {
        //nosound();
        return;
    }
    buf[spos] = q;
    spos++;
    if (spos == QUMAX)
        spos = 0;  /* loop back */
}
//********************************************
byte BPicture::qretrieve()
{
    if (rpos == QUMAX)
        rpos = 0; /* loop back */
    if (rpos == spos)
        return EMPTY;
    rpos++;
    return buf[rpos - 1];
}

/**************************************************************************
** picPSet
**
** Draws a pixel in the picture screen.
**************************************************************************/
void BPicture::picPSet(word x, word y)
{
    word vx, vy;

    vx = (x << 1);
    vy = y;
    if (vx > 319)
        return;
    if (vy > 199)
        return;
    picture[vy][vx] = picColour;
    picture[vy][vx + 1] = picColour;
}

/**************************************************************************
** priPSet
**
** Draws a pixel in the priority screen.
**************************************************************************/
void BPicture::priPSet(word x, word y)
{
    word vx, vy;

    vx = (x << 1);
    vy = y;
    if (vx > 319)
        return;
    if (vy > 199)
        return;
    priority[vy][vx] = priColour;
    priority[vy][vx + 1] = priColour;
}

/**************************************************************************
** pset
**
** Draws a pixel in each screen depending on whether drawing in that
** screen is enabled or not.
**************************************************************************/
void BPicture::pset(word x, word y)
{
    if (picDrawEnabled)
        picPSet(x, y);
    if (priDrawEnabled)
        priPSet(x, y);
}

/**************************************************************************
** picGetPixel
**
** Get colour at x,y on the picture page.
**************************************************************************/
byte BPicture::picGetPixel(word x, word y)
{
    word vx, vy;

    vx = (x << 1);
    vy = y;
    if (vx > 319)
        return (4);
    if (vy > 199)
        return (4);

    return (picture[vy][vx]);
}

/**************************************************************************
** priGetPixel
**
** Get colour at x,y on the priority page.
**************************************************************************/
byte BPicture::priGetPixel(word x, word y)
{
    word vx, vy;

    vx = (x << 1);
    vy = y;
    if (vx > 319)
        return (4);
    if (vy > 199)
        return (4);

    return (priority[vy][vx]);
}

/**************************************************************************
** round
**
** Rounds a float to the closest int. Takes into actions which direction
** the current line is being drawn when it has a 50:50 decision about
** where to put a pixel.
**************************************************************************/
int BPicture::round(float aNumber, float dirn)
{
    if (dirn < 0)
        return ((aNumber - floor(aNumber) <= 0.501) ? (int)floor(aNumber) : (int)ceil(aNumber));
    return ((aNumber - floor(aNumber) < 0.499) ? (int)floor(aNumber) : (int)ceil(aNumber));
}

/**************************************************************************
** drawline
**
** Draws an AGI line.
**************************************************************************/
void BPicture::drawline(word x1, word y1, word x2, word y2)
{
    int height, width;
    float x, y, addX, addY;

    height = (y2 - y1);
    width = (x2 - x1);
    addX = (height == 0 ? height : (float)width / abs(height));
    addY = (width == 0 ? width : (float)height / abs(width));

    if (abs(width) > abs(height)) {
        y = y1;
        addX = (width == 0 ? 0 : (width / abs(width)));
        for (x = x1; x != x2; x += addX) {
            pset(round(x, addX), round(y, addY));
            y += addY;
        }
        pset(x2, y2);
    } else {
        x = x1;
        addY = (height == 0 ? 0 : (height / abs(height)));
        for (y = y1; y != y2; y += addY) {
            pset(round(x, addX), round(y, addY));
            x += addX;
        }
        pset(x2, y2);
    }
}

/**************************************************************************
** okToFill
**************************************************************************/
bool BPicture::okToFill(byte x, byte y)
{
    if (!picDrawEnabled && !priDrawEnabled)
        return false;
    if (picColour == 15)
        return false;
    if (!priDrawEnabled)
        return (picGetPixel(x, y) == 15);
    if (priDrawEnabled && !picDrawEnabled)
        return (priGetPixel(x, y) == 4);
    return (picGetPixel(x, y) == 15);
}

/**************************************************************************
** agiFill
**************************************************************************/
void BPicture::agiFill(word x, word y)
{
    byte x1, y1;
    rpos = spos = 0;

    qstore(x);
    qstore(y);

    //  printf("fill %d %d\n",x,y);

    for (;;) {

        x1 = qretrieve();
        y1 = qretrieve();

        //      printf("x1=%d y1=%d\n");

        if ((x1 == EMPTY) || (y1 == EMPTY))
            break;
        else {

            if (okToFill(x1, y1)) {

                pset(x1, y1);

                if (okToFill(x1, y1 - 1) && (y1 != 0)) {
                    qstore(x1);
                    qstore(y1 - 1);
                }
                if (okToFill(x1 - 1, y1) && (x1 != 0)) {
                    qstore(x1 - 1);
                    qstore(y1);
                }
                if (okToFill(x1 + 1, y1) && (x1 != 159)) {
                    qstore(x1 + 1);
                    qstore(y1);
                }
                if (okToFill(x1, y1 + 1) && (y1 != 167)) {
                    qstore(x1);
                    qstore(y1 + 1);
                }

            }

        }

    }
}

/**************************************************************************
** xCorner
**
** Draws an xCorner  (drawing action 0xF5)
**************************************************************************/
void BPicture::xCorner(byte **data)
{
    byte x1, x2, y1, y2;

    x1 = *((*data)++);
    y1 = *((*data)++);

    pset(x1, y1);

    for (;;) {
        x2 = *((*data)++);
        if (x2 >= 0xF0)
            break;
        drawline(x1, y1, x2, y1);
        x1 = x2;
        y2 = *((*data)++);
        if (y2 >= 0xF0)
            break;
        drawline(x1, y1, x1, y2);
        y1 = y2;
    }

    (*data)--;
}

/**************************************************************************
** yCorner
**
** Draws an yCorner  (drawing action 0xF4)
**************************************************************************/
void BPicture::yCorner(byte **data)
{
    byte x1, x2, y1, y2;

    x1 = *((*data)++);
    y1 = *((*data)++);

    pset(x1, y1);

    for (;;) {
        y2 = *((*data)++);
        if (y2 >= 0xF0)
            break;
        drawline(x1, y1, x1, y2);
        y1 = y2;
        x2 = *((*data)++);
        if (x2 >= 0xF0)
            break;
        drawline(x1, y1, x2, y1);
        x1 = x2;
    }

    (*data)--;
}

/**************************************************************************
** relativeDraw
**
** Draws short lines relative to last position.  (drawing action 0xF7)
**************************************************************************/
void BPicture::relativeDraw(byte **data)
{
    byte x1, y1, disp;
    char dx, dy;

    x1 = *((*data)++);
    y1 = *((*data)++);

    pset(x1, y1);

    for (;;) {
        disp = *((*data)++);
        if (disp >= 0xF0)
            break;
        dx = ((disp & 0xF0) >> 4) & 0x0F;
        dy = (disp & 0x0F);
        if (dx & 0x08)
            dx = (-1) * (dx & 0x07);
        if (dy & 0x08)
            dy = (-1) * (dy & 0x07);
        drawline(x1, y1, x1 + dx, y1 + dy);
        x1 += dx;
        y1 += dy;
    }

    (*data)--;
}

/**************************************************************************
** fill
**
** Agi flood fill.  (drawing action 0xF8)
**************************************************************************/
void BPicture::fill(byte **data)
{
    byte x1, y1;

    for (;;) {
        if ((x1 = *((*data)++)) >= 0xF0)
            break;
        if ((y1 = *((*data)++)) >= 0xF0)
            break;
        agiFill(x1, y1);
    }

    (*data)--;
}

/**************************************************************************
** absoluteLine
**
** Draws long lines to actual locations (cf. relative) (drawing action 0xF6)
**************************************************************************/
void BPicture::absoluteLine(byte **data)
{
    byte x1, y1, x2, y2;

    x1 = *((*data)++);
    y1 = *((*data)++);

    pset(x1, y1);

    for (;;) {
        if ((x2 = *((*data)++)) >= 0xF0)
            break;
        if ((y2 = *((*data)++)) >= 0xF0)
            break;
        drawline(x1, y1, x2, y2);
        x1 = x2;
        y1 = y2;
    }

    (*data)--;
}


#define plotPatternPoint() \
   if (patCode & 0x20) { \
      if ((splatterMap[bitPos>>3] >> (7-(bitPos&7))) & 1) pset(x1, y1); \
      bitPos++; \
      if (bitPos == 0xff) bitPos=0; \
   } else pset(x1, y1)

/**************************************************************************
** plotPattern
**
** Draws pixels, circles, squares, or splatter brush patterns depending
** on the pattern code.
**************************************************************************/
void BPicture::plotPattern(byte x, byte y)
{
    static byte circles[][15] = { /* agi circle bitmaps */
        {0x80},
        {0xfc},
        {0x5f, 0xf4},
        {0x66, 0xff, 0xf6, 0x60},
        {0x23, 0xbf, 0xff, 0xff, 0xee, 0x20},
        {0x31, 0xe7, 0x9e, 0xff, 0xff, 0xde, 0x79, 0xe3, 0x00},
        {0x38, 0xf9, 0xf3, 0xef, 0xff, 0xff, 0xff, 0xfe, 0xf9, 0xf3, 0xe3, 0x80},
        {0x18, 0x3c, 0x7e, 0x7e, 0x7e, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x7e, 0x7e, 0x3c, 0x18}
    };

    static byte splatterMap[32] = { /* splatter brush bitmaps */
        0x20, 0x94, 0x02, 0x24, 0x90, 0x82, 0xa4, 0xa2,
        0x82, 0x09, 0x0a, 0x22, 0x12, 0x10, 0x42, 0x14,
        0x91, 0x4a, 0x91, 0x11, 0x08, 0x12, 0x25, 0x10,
        0x22, 0xa8, 0x14, 0x24, 0x00, 0x50, 0x24, 0x04
    };

    static byte splatterStart[128] = { /* starting bit position */
        0x00, 0x18, 0x30, 0xc4, 0xdc, 0x65, 0xeb, 0x48,
        0x60, 0xbd, 0x89, 0x05, 0x0a, 0xf4, 0x7d, 0x7d,
        0x85, 0xb0, 0x8e, 0x95, 0x1f, 0x22, 0x0d, 0xdf,
        0x2a, 0x78, 0xd5, 0x73, 0x1c, 0xb4, 0x40, 0xa1,
        0xb9, 0x3c, 0xca, 0x58, 0x92, 0x34, 0xcc, 0xce,
        0xd7, 0x42, 0x90, 0x0f, 0x8b, 0x7f, 0x32, 0xed,
        0x5c, 0x9d, 0xc8, 0x99, 0xad, 0x4e, 0x56, 0xa6,
        0xf7, 0x68, 0xb7, 0x25, 0x82, 0x37, 0x3a, 0x51,
        0x69, 0x26, 0x38, 0x52, 0x9e, 0x9a, 0x4f, 0xa7,
        0x43, 0x10, 0x80, 0xee, 0x3d, 0x59, 0x35, 0xcf,
        0x79, 0x74, 0xb5, 0xa2, 0xb1, 0x96, 0x23, 0xe0,
        0xbe, 0x05, 0xf5, 0x6e, 0x19, 0xc5, 0x66, 0x49,
        0xf0, 0xd1, 0x54, 0xa9, 0x70, 0x4b, 0xa4, 0xe2,
        0xe6, 0xe5, 0xab, 0xe4, 0xd2, 0xaa, 0x4c, 0xe3,
        0x06, 0x6f, 0xc6, 0x4a, 0xa4, 0x75, 0x97, 0xe1
    };

    int circlePos = 0;
    byte x1, y1, penSize, bitPos = splatterStart[patNum];

    penSize = (patCode & 7);

    if (x < ((penSize / 2) + 1))
        x = ((penSize / 2) + 1);
    else if (x > 160 - ((penSize / 2) + 1))
        x = 160 - ((penSize / 2) + 1);
    if (y < penSize)
        y = penSize;
    else if (y >= 168 - penSize)
        y = 167 - penSize;

    for (y1 = y - penSize; y1 <= y + penSize; y1++) {
        for (x1 = x - ((int)ceil((float)penSize / 2)); x1 <= x + ((int)floor((float)penSize / 2)); x1++) {
            if (patCode & 0x10)   /* Square */
                plotPatternPoint();
            else { /* Circle */
                if ((circles[patCode & 7][circlePos >> 3] >> (7 - (circlePos & 7))) & 1)
                    plotPatternPoint();
                circlePos++;
            }
        }
    }
}


/**************************************************************************
** plotBrush
**
** Plots points and various brush patterns.
**************************************************************************/
void BPicture::plotBrush(byte **data)
{
    byte x1, y1;

    for (;;) {
        if (patCode & 0x20) {
            if ((patNum = *((*data)++)) >= 0xF0)
                break;
            patNum = (patNum >> 1 & 0x7f);
        }
        if ((x1 = *((*data)++)) >= 0xF0)
            break;
        if ((y1 = *((*data)++)) >= 0xF0)
            break;
        plotPattern(x1, y1);
    }

    (*data)--;
}

//****************************************************
void BPicture::show(byte *picdata, int picsize)
{
    byte *data = picdata;
    bool stillDrawing = true;
    byte action;

    for (int i = 0; i < MAX_H; i++) {
        memset(picture[i], 15, MAX_W);
        memset(priority[i], 4, MAX_W);
    }
    rpos = QUMAX;
    spos = 0;
    picDrawEnabled = false;
    priDrawEnabled = false;
    picColour = priColour = 0;

    do {
        action = *(data++);
        switch (action) {
            case 0xFF:
                stillDrawing = 0;
                break;
            case 0xF0:
                picColour = *(data++);
                picDrawEnabled = true;
                break;
            case 0xF1:
                picDrawEnabled = false;
                break;
            case 0xF2:
                priColour = *(data++);
                priDrawEnabled = true;
                break;
            case 0xF3:
                priDrawEnabled = false;
                break;
            case 0xF4:
                yCorner(&data);
                break;
            case 0xF5:
                xCorner(&data);
                break;
            case 0xF6:
                absoluteLine(&data);
                break;
            case 0xF7:
                relativeDraw(&data);
                break;
            case 0xF8:
                fill(&data);
                break;
            case 0xF9:
                patCode = *(data++);
                break;
            case 0xFA:
                plotBrush(&data);
                break;
            default:
                printf("Unknown picture code : %X\n", action);
                break;
        }
    } while ((data < (data + picsize)) && stillDrawing);
}
//****************************************************
