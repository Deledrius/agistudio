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

#ifndef VIEW_H
#define VIEW_H

#include "util.h"

#define MaxLoops 16
#define MaxCels  32

class Cel
{
public:
    Cel(int w, int h, int c, bool m);
    Cel();
    int width, height, transcol;
    bool mirror;
    byte *data;
    void setW(int w);
    void setH(int h);
    void clear();
    void setTrans(int i);
    void right();
    void left();
    void up();
    void down();
    void mirrorh();
    void mirrorv();
    void copy(Cel c);
    void fill(int xn, int yn, byte c);
    void deinit();
protected:
    int setc(int xn, int yn, byte c0, byte c);
    void fill1(int xn, int yn, byte c0, byte c);
};


class Loop
{
public:
    Loop();
    int NumCels;
    int LoopLoc;
    Cel *cels;
    int *CelLoc;
    int mirror;  //whom I mirror
    int mirror1; //who is a mirror of me
    void insertCel_before(int n);
    void insertCel_after(int n);
    void appendCel();
    void deleteCel(int n);
    void clear();
    void numcels(int n);
    void cel(int n, int w, int h, int c, bool m);
};

class View
{
public:
    View();
    int NumLoops, CurLoop, CurCel;
    bool opened;
    std::string Description;
    Loop *loops;
    void open();
    void init();
    int open(int resnum);
    int open(char *filename);
    void newView();
    void ReadViewInfo();
    void LoadCel(int loopno, int celno);
    void insertLoop_before();
    void insertLoop_after();
    void appendLoop();
    void deleteLoop();
    void setMirror(int n, int k);
    void unsetMirror(int n);
    void fixmirror();
    void printmirror();
    void save();
    int save(char *filename);
    int save(int resnum);
};


#endif
