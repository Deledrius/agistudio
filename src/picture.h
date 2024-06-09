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

#ifndef PICTURE_H
#define PICTURE_H

#include <QImage>
#include <QColor>

#include <list>

#define MAX_W 320
#define MAX_H 200
#define MAX_HH 168
//the actual height of the picture is 168 (to leave space for menu and text input)

typedef unsigned short int word;
typedef uint8_t byte;

#define M_LEFT 0
#define M_RIGHT 2

#define CIRCLE  0
#define SQUARE  1
#define SPRAY   0
#define SOLID   1

#define T_LINE 0
#define T_PEN  1
#define T_STEP 2
#define T_FILL 3
#define T_BRUSH 4

#define QUMAX 4000
#define EMPTY 0xFF

enum draw_code {
    SetPicColor     = 0xF0,
    EnablePriority  = 0xF1,
    SetPriColor     = 0xF2,
    EnableVisual    = 0xF3,
    YCorner         = 0xF4,
    XCorner         = 0xF5,
    AbsoluteLine    = 0xF6,
    RelativeLine    = 0xF7,
    Fill            = 0xF8,
    SetPattern      = 0xF9,
    Brush           = 0xFA,

    ActionEnd       = 0xFF
};

constexpr uint8_t action_codes_start = draw_code::SetPicColor;
constexpr uint8_t action_codes_end = draw_code::ActionEnd;


typedef struct {
    short x, y;
    byte c;
    QColor cc;
} Point;


typedef struct {
    int n;
    Point p[370];
} Points;


//bitmap picture - for preview
class BPicture
{
public:
    BPicture();
    byte **picture;
    byte **priority;
    void show(byte *, int);
protected:

    byte buf[QUMAX + 1];
    word rpos, spos;
    bool picDrawEnabled, priDrawEnabled;
    byte picColour, priColour, patCode, patNum;

    void qstore(byte q);
    byte qretrieve();
    void picPSet(word x, word y);
    void priPSet(word x, word y);
    void pset(word x, word y);
    byte picGetPixel(word x, word y) const;
    byte priGetPixel(word x, word y) const;
    int round(float aNumber, float dirn);
    void drawline(word x1, word y1, word x2, word y2);
    bool okToFill(byte x, byte y);
    void agiFill(word x, word y);
    void xCorner(byte **data);
    void yCorner(byte **data);
    void relativeDraw(byte **data);
    void fill(byte **data);
    void absoluteLine(byte **data);
    void plotPattern(byte x, byte y);
    void plotBrush(byte **data);
};

typedef std::list<byte> actionList;
typedef std::list<byte>::iterator actionListIter;

//'list' format picture - for edit
class Picture
{
public:
    Picture();
    byte picture[MAX_W * MAX_H];
    byte priority[MAX_W * MAX_H];
    byte *pptr;
    bool bg_on;
    QImage *bgpix;
    Points points, points0, points1;
    Points *curp, *newp;
    void load(byte *, int);
    int open(char *filename);
    int open(int ResNum);
    void save();
    int save(char *filename);
    int save(int ResNum);
    void newpic();
    void draw();
    void init();
    void set_mode(int);
    void clear_tools();
    void init_tool();
    void home_proc();
    void end_proc();
    void left_proc();
    void right_proc();
    void del_proc();
    void wipe_proc();
    void tool_proc(int);
    void choose_color(int button, int color);
    void set_brush(int mode, int val);
    int button_action(int newX, int newY);
    int move_action(int newX, int newY);
    const QString showPos(byte *code, byte *val) const;
    const size_t getPos() const;
    int setBufPos(int);
    void viewData(QStringList *data);
    void status(int mode);
    void refill(actionListIter pos_fill_start, actionListIter pos_fill_end, int mode);
    int drawing_mode;
    int tool;
    int brushSize, brushShape, brushTexture;
    byte picColour, priColour, patCode, patNum, curcol;
    bool picDrawEnabled, priDrawEnabled;
    bool refill_pic, refill_pri;
    int refill_x, refill_y;
protected:
    actionList picCodes;
    actionListIter picPos;

    byte buf[QUMAX + 1];
    word rpos, spos;
    bool add_pic, add_pri;
    int code_pic, col_pic, code_pri, col_pri;

    void dldelete();
    void removeAction();
    void wipeAction();
    void moveBackAction();
    void moveForwardAction();

    byte getCode(actionListIter *pos) const;
    byte testCode(actionListIter *pos) const;

    void qstore(byte q);
    byte qretrieve();
    void picPSet(word x, word y);
    void priPSet(word x, word y);
    void pset(word x, word y);
    byte picGetPixel(word x, word y) const;
    byte priGetPixel(word x, word y) const;
    int round(float aNumber, float dirn);
    void drawline(word x1, word y1, word x2, word y2);
    bool okToFill(byte x, byte y);
    void agiFill(word x, word y);
    void xCorner(actionListIter *pos);
    void yCorner(actionListIter *pos);
    void relativeDraw(actionListIter *pos);
    void fill(actionListIter *pos);
    void absoluteLine(actionListIter *pos);
    void plotPattern(byte x, byte y);
    void plotBrush(actionListIter *pos);
    void addCode(byte code);
    void replaceCode(byte code);
    void addPatCode();
    void adjustDisp(int *dX, int *dY);
    int  clickX, clickY, dX, dY;
    int stepClicks, numClicks;
    bool firstClick;
    void normline2(int x1, int y1, int x2, int y2);
    void putpix2(int x, int y);
};

extern char tmp[];

#endif
