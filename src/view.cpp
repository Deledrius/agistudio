/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  Part of this code is adapted from Peter Kelly's viewview.pas
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
#include "view.h"
#include "viewedit.h"
#include "menu.h"

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include <QApplication>

static int ResPos, DescPos, ResSize;
//**************************************************
View::View()
{
    Description = "";
    loops = NULL;
    opened = false;
}

//**************************************************
void View::init()
{
    ReadViewInfo();
    CurLoop = 0;
    CurCel = 0;
    opened = true;
}

//*************************************************
int View::open(char *filename)
{
    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        menu->errmes("Can't open file %s ! ", filename);
        return 1;
    }

    struct stat buf;
    fstat(fileno(fptr), &buf);
    ResourceData.Size = buf.st_size;
    fread(ResourceData.Data, ResourceData.Size, 1, fptr);
    fclose(fptr);

    init();
    return 0;
}

//**************************************************
int View::open(int ResNum)
{
    int err = game->ReadResource(VIEW, ResNum);
    if (err)
        return 1;
    init();
    return 0;
}

//*************************************************
static byte ReadByte(void)
{
    if (ResPos < ResourceData.Size)
        return ResourceData.Data[ResPos++];
    return 0;
}

//**************************************************
static int ReadLSMSWord(void)
{
    byte MSbyte, LSbyte;

    LSbyte = ReadByte();
    MSbyte = ReadByte();
    return (LSbyte + MSbyte * 256);
}

//**************************************************
static void SeekRes(int seekpos)
{
    if (seekpos >= 0 && seekpos <= ResourceData.Size - 1)
        ResPos = seekpos;
}

//*************************************************
void View::ReadViewInfo()
{
    int CurLoop, CurCel, NumCels, curbyte, i, cel_width, cel_height, cel_transcol;
    bool cel_mirror;

    NumLoops = 0;
    CurLoop = 0;
    CurCel = 0;
    SeekRes(2);
    NumLoops = ReadByte();

    loops = new Loop[NumLoops];
    Description = "";
    DescPos = ReadLSMSWord();
    if (DescPos > 0) {
        SeekRes(DescPos);
        curbyte = 1;
        while (curbyte != 0) {
            curbyte = ReadByte();
            if (curbyte != 0) {
                if (curbyte == 0x0a)
                    Description.append("\\n");
                else
                    Description.append(1, (char)curbyte);
            }
        }
    }

    SeekRes(5);

    for (CurLoop = 0; CurLoop < NumLoops; CurLoop++) {

        loops[CurLoop] = Loop();
        loops[CurLoop].LoopLoc = ReadLSMSWord();
        for (i = 0; i < CurLoop; i++) {
            if (loops[CurLoop].LoopLoc == loops[i].LoopLoc) { //if 2 loops point to the same place, then the 2nd is mirrored
                loops[CurLoop].mirror = i;
                break;
            }
        }
    }


    for (i = 0; i < NumLoops; i++) {
        for (int j = 0; j < NumLoops; j++) {
            if (i == j)
                continue;
            if (loops[i].mirror == j)
                loops[j].mirror1 = i;
        }
    }

    for (CurLoop = 0; CurLoop < NumLoops; CurLoop++) {
        SeekRes(loops[CurLoop].LoopLoc);
        NumCels = ReadByte();
        loops[CurLoop].numcels(NumCels);
        for (CurCel = 0; CurCel < NumCels; CurCel++)
            loops[CurLoop].CelLoc[CurCel] = ReadLSMSWord();
        for (CurCel = 0; CurCel < NumCels; CurCel++) {
            SeekRes(loops[CurLoop].LoopLoc + loops[CurLoop].CelLoc[CurCel]);
            cel_width = ReadByte();
            cel_height = ReadByte();
            curbyte = ReadByte();
            cel_transcol = (byte)(curbyte & 0x0f);

            if (curbyte >= 0x80)
                cel_mirror = true;
            else
                cel_mirror = false;

            loops[CurLoop].cel(CurCel, cel_width, cel_height, cel_transcol, cel_mirror);
            LoadCel(CurLoop, CurCel);
        }
    }
}

//*************************************************
void View::LoadCel(int loopno, int celno)
{
    int i0, i1, celX, celY, ChunkLength, ChunkCol, Width, Height, transcol;
    byte k, curbyte;
    bool mirror;

    Width = loops[loopno].cels[celno].width;
    Height = loops[loopno].cels[celno].height;
    transcol = loops[loopno].cels[celno].transcol;
    mirror = loops[loopno].cels[celno].mirror;
    SeekRes(loops[loopno].LoopLoc + loops[loopno].CelLoc[celno] + 3);
    celX = 0;
    celY = 0;
    while (celY != Height) {
        curbyte = ReadByte();
        if (curbyte > 0) {
            ChunkCol = (curbyte & 0xF0) / 0x10;
            ChunkLength = curbyte & 0x0F;
            i0 = celX;
            i1 = i0 + ChunkLength * 2 - 1;
            for (celX = i0; celX <= i1; celX++)
                loops[loopno].cels[celno].data[celY * Width * 2 + celX] = ChunkCol;
        } else {
            for (; celX < Width * 2; celX++)
                loops[loopno].cels[celno].data[celY * Width * 2 + celX] = transcol;
            if (mirror && (loops[loopno].mirror != -1)) {
                for (celX = 0; celX < Width; celX++) {
                    k = loops[loopno].cels[celno].data[celY * Width * 2 + (Width * 2 - 1 - celX)];
                    loops[loopno].cels[celno].data[celY * Width * 2 + (Width * 2 - 1 - celX)] = loops[loopno].cels[celno].data[celY * Width * 2 + celX];
                    loops[loopno].cels[celno].data[celY * Width * 2 + celX] = k;
                }
            }
            celY++;
            celX = 0;
        }
    }
}

//*************************************************
int View::save(char *filename)
{
    FILE *fptr = fopen(filename, "wb");

    if (fptr == NULL) {
        menu->errmes("Can't open file %s ! ", filename);
        return 1;
    }
    save();
    fwrite(ResourceData.Data, ResourceData.Size, 1, fptr);
    fclose(fptr);
    return 0;
}

//*************************************************
int View::save(int ResNum)
{
    save();
    return (game->AddResource(VIEW, ResNum));
}

//*************************************************
void WriteByte(byte b)
{
    if (ResPos >= MaxResourceSize) {
        menu->errmes("Resource too big !");
        return;
    }
    ResourceData.Data[ResPos++] = b;
    if (ResPos > ResSize)
        ResSize = ResPos;
}

//*************************************************
void View::save()
{
    byte c;
    int i, j, x, y, length, mirror, k, k1;
    int NumCels, CelSize, CelMirrorSize, DescLoc;
    bool ColDiff;
    short LoopLoc[MaxLoops];
    short CelLoc[MaxLoops][MaxCels];

    ResSize = 0;
    ResPos = 0;


    WriteByte(1);   //what do these two bytes do?
    WriteByte(1);
    WriteByte((byte)NumLoops);
    WriteByte(0);  //write description later, if it exists
    WriteByte(0);

    LoopLoc[0] = 5 + NumLoops * 2;

    //fix mirroring so (according to the AGI specs) the loops>=8 do not use it,
    //and the mirroring loop number is always higher than the mirrored loop
    for (i = 0; i < NumLoops; i++) {
        k = loops[i].mirror;
        k1 = loops[i].mirror1;
        if (i >= 8) {
            if (k != -1)
                unsetMirror(k);
            if (k1 != -1)
                unsetMirror(k1);
        } else if (k != -1) {
            if (k > i) {
                unsetMirror(i);
                setMirror(k, i);
            }
        }
    }

    for (i = 0; i < NumLoops; i++) {
        ResPos = LoopLoc[i];
        if (loops[i].mirror == -1) {
            NumCels = loops[i].NumCels;
            WriteByte((byte)NumCels);
            mirror = loops[i].mirror1;
            CelLoc[i][0] = 1 + NumCels * 2;
            for (int j = 0; j < NumCels; j++) {
                ResPos = LoopLoc[i] + CelLoc[i][j];
                int width = loops[i].cels[j].width;
                int height = loops[i].cels[j].height;
                byte transcol = loops[i].cels[j].transcol;
                WriteByte(width);
                WriteByte(height);
                if (mirror == -1)
                    WriteByte(transcol);
                else
                    WriteByte(((((byte)i) | 0x8) << 4) | (byte)loops[i].cels[j].transcol);
                CelSize = 3;
                CelMirrorSize = 3;
                for (y = 0; y < height; y++) {
                    x = 0;
                    do {
                        c = loops[i].cels[j].data[y * width * 2 + x * 2];
                        length = 0;
                        do {
                            length++;
                            ColDiff = (loops[i].cels[j].data[y * width * 2 + (x + length) * 2] != c);
                        } while (!(ColDiff || length >= 15 || x + length >= width));
                        if (x > 0 || (x == 0 && c != transcol))
                            CelMirrorSize++;
                        x += length;
                        if (x < width || (x >= width && c != transcol)) {
                            WriteByte((c << 4) | length);
                            CelSize++;
                        }
                    } while (x < width);
                    WriteByte(0); //end of line
                }
                if (mirror != -1) { //write extra few bytes so mirrored cel will fit
                    if (CelSize != CelMirrorSize) {
                        for (int t = 1; t <= abs(CelSize - CelMirrorSize); t++)
                            WriteByte(0);
                    }
                }
                if (j < NumCels - 1)
                    CelLoc[i][j + 1] = ResSize - LoopLoc[i];
            }
        }
        if (i < NumLoops - 1)
            LoopLoc[i + 1] = ResSize;
    }
    for (i = 0; i < NumLoops; i++) {
        if (loops[i].mirror != -1)
            LoopLoc[i] = LoopLoc[loops[i].mirror];
    }
    ResPos = ResSize;
    DescLoc = ResPos;

    //write description

    if (Description != "") {
        for (i = 0; i < (int)Description.length(); i++) {
            if (Description[i] == '\\' && (i < (int)Description.length() - 1 && Description[i + 1] == 'n')) {
                WriteByte(0x0a);
                i++;
            } else
                WriteByte(Description[i]);
        }
        WriteByte(0);
        ResPos = 3;
        WriteByte(DescLoc % 256);
        WriteByte(DescLoc / 256);
    }

    for (i = 0; i < NumLoops; i++) {
        ResPos = 5 + i * 2;
        WriteByte(LoopLoc[i] % 256);
        WriteByte(LoopLoc[i] / 256);
        if (loops[i].mirror == -1) {
            for (j = 0; j < loops[i].NumCels; j++) {
                ResPos = LoopLoc[i] + 1 + j * 2;
                WriteByte(CelLoc[i][j] % 256);
                WriteByte(CelLoc[i][j] / 256);
            }
        }
    }
    ResourceData.Size = ResSize;
}

//*************************************************
void View::newView()
{
    NumLoops = 1;
    CurLoop = 0;
    CurCel = 0;
    loops = new Loop[NumLoops];
    loops[CurLoop] = Loop();
    loops[CurLoop].numcels(1);
    loops[CurLoop].cel(CurCel, 1, 1, 0, false);
    Description = "";
}

//*************************************************
void View::fixmirror()
{
    int i;

    for (i = 0; i < NumLoops; i++)
        loops[i].mirror1 = -1;

    for (i = 0; i < NumLoops; i++) {
        for (int j = 0; j < NumLoops; j++) {
            if (i == j)
                continue;
            if (loops[i].mirror == j)
                loops[j].mirror1 = i;
        }
    }
}

//*************************************************
void View::printmirror()
{
    int i;

    for (i = 0; i < NumLoops; i++)
        printf("%d mirror=%d mirror1=%d\n", i, loops[i].mirror, loops[i].mirror1);
}

//*************************************************
void View::insertLoop_before()
//before current
{
    int w, h, c, i;

    Loop *loops1 = new Loop[NumLoops + 1];

    for (i = 0; i < NumLoops + 1; i++) {
        if (i < CurLoop)
            loops1[i] = loops[i];
        else if (i > CurLoop)
            loops1[i] = loops[i - 1];
        else {
            loops1[i] = Loop();
            loops1[i].numcels(1);
            if (i > 0) {
                w = loops[0].cels[0].width;
                h = loops[0].cels[0].height;
                c = loops[0].cels[0].transcol;
            } else {
                w = loops[NumLoops - 1].cels[0].width;
                h = loops[NumLoops - 1].cels[0].height;
                c = loops[NumLoops - 1].cels[0].transcol;
            }
            loops1[i].cel(0, w, h, c, false);
        }
    }
    delete  [] loops;
    loops = loops1;
    NumLoops++;;
    for (i = 0; i < NumLoops; i++) {
        if (loops[i].mirror >= CurLoop)
            loops[i].mirror++;
    }

    fixmirror();
}

//*************************************************
void View::insertLoop_after()
//after current
{
    int w, h, c, i;

    Loop *loops1 = new Loop[NumLoops + 1];

    for (i = 0; i < NumLoops + 1; i++) {
        if (i <= CurLoop)
            loops1[i] = loops[i];
        else if (i > CurLoop + 1)
            loops1[i] = loops[i - 1];
        else {
            loops1[i] = Loop();
            loops1[i].numcels(1);
            if (i > 0) {
                w = loops[0].cels[0].width;
                h = loops[0].cels[0].height;
                c = loops[0].cels[0].transcol;
            } else {
                w = loops[NumLoops - 1].cels[0].width;
                h = loops[NumLoops - 1].cels[0].height;
                c = loops[NumLoops - 1].cels[0].transcol;
            }
            loops1[i].cel(0, w, h, c, false);
        }
    }
    delete  [] loops;
    loops = loops1;
    NumLoops++;;
    for (i = 0; i < NumLoops; i++) {
        if (loops[i].mirror > CurLoop)
            loops[i].mirror++;
    }

    fixmirror();
}

//*************************************************
void View::appendLoop()
//append to end
{
    Loop *loops1 = new Loop[NumLoops + 1];

    for (int i = 0; i < NumLoops; i++)
        loops1[i] = loops[i];

    loops1[NumLoops] = Loop();
    loops1[NumLoops].numcels(1);
    int w = loops[NumLoops - 1].cels[0].width;
    int h = loops[NumLoops - 1].cels[0].height;
    int c = loops[NumLoops - 1].cels[0].transcol;
    loops1[NumLoops].cel(0, w, h, c, false);

    delete  [] loops;
    loops = loops1;
    NumLoops++;;
}

//*************************************************
void View::deleteLoop()
//delete CurLoop
{
    int i;

    for (i = 0; i < NumLoops; i++) {
        if (loops[i].mirror > CurLoop)
            loops[i].mirror--;
        else if (loops[i].mirror == CurLoop)
            unsetMirror(i);
    }
    for (i = CurLoop; i < NumLoops - 1; i++)
        loops[i] = loops[i + 1];
    NumLoops--;

    fixmirror();
}

//*************************************************
void View::unsetMirror(int n)
{
    int k = loops[n].mirror;

    if (k == -1)
        return;

    loops[n].mirror = -1;
    for (int i = 0; i < loops[n].NumCels; i++) {
        loops[n].cel(i, 1, 1, 0, false);
        loops[n].cels[i].copy(loops[k].cels[i]);
        loops[n].cels[i].mirrorh();
    }
    loops[k].mirror1 = -1;
}

//*************************************************
void View::setMirror(int n, int k)
{
    //set loops[n] to mirror k

    loops[n].mirror = k;
    loops[n].numcels(loops[k].NumCels);
    for (int i = 0; i < loops[n].NumCels; i++) {
        loops[n].cel(i, 1, 1, 0, false);
        loops[n].cels[i].copy(loops[k].cels[i]);
        loops[n].cels[i].mirrorh();
    }
    loops[k].mirror1 = n;
}

//*************************************************
Cel::Cel(int w, int h, int c, bool m)
{
    width = w;
    height = h;
    transcol = c;
    mirror = m;
    data = (byte *)malloc(width * 2 * height);
    clear();
}

//*************************************************
Cel::Cel()
{
    data = NULL;
    width = height = transcol = 0;
    mirror = false;
}

//*************************************************
void Cel::deinit()
{
    if (data)
        free(data);
    data = NULL;
    width = height = transcol = 0;
    mirror = false;
}

//*************************************************
void Cel::setW(int w)
{
    int x, y;

    if (w == width)
        return;

    byte *data1 = (byte *)malloc(w * 2 * height);
    for (y = 0; y < height; y++) {
        for (x = 0; x < w * 2; x++) {
            if (x < width * 2)
                data1[y * w * 2 + x] = data[y * width * 2 + x];
            else
                data1[y * w * 2 + x] = transcol;
        }
    }

    free(data);
    data = data1;
    width = w;
}

//*************************************************
void Cel::setH(int h)
{
    int x, y;

    if (h == height)
        return;

    byte *data1 = (byte *)malloc(width * 2 * h);

    for (y = 0; y < h; y++) {
        for (x = 0; x < width * 2; x++) {
            if (y < height)
                data1[y * width * 2 + x] = data[y * width * 2 + x];
            else
                data1[y * width * 2 + x] = transcol;
        }
    }

    free(data);
    data = data1;
    height = h;
}

//*************************************************
void Cel::clear()
{
    memset(data, transcol, width * 2 * height);
}

//*************************************************
void Cel::setTrans(int i)
{
    transcol = i;
}

//*************************************************
void Cel::right()
{
    int x, y;
    byte k0, k1;

    for (y = 0; y < height; y++) {
        k0 = data[y * width * 2 + width * 2 - 1];
        k1 = data[y * width * 2 + width * 2 - 2];
        for (x = width * 2 - 1; x > 1; x -= 2) {
            data[y * width * 2 + x] = data[y * width * 2 + x - 2];
            data[y * width * 2 + x - 1] = data[y * width * 2 + x - 3];
        }
        data[y * width * 2 + 1] = k0;
        data[y * width * 2] = k1;
    }
}

//*************************************************
void Cel::left()
{
    int x, y;
    byte k0, k1;

    for (y = 0; y < height; y++) {
        k0 = data[y * width * 2];
        k1 = data[y * width * 2 + 1];
        for (x = 0; x < width * 2 - 2; x += 2) {
            data[y * width * 2 + x] = data[y * width * 2 + x + 2];
            data[y * width * 2 + x + 1] = data[y * width * 2 + x + 3];
        }
        data[y * width * 2 + x] = k0;
        data[y * width * 2 + x + 1] = k1;
    }
}

//*************************************************
void Cel::up()
{
    int x, y;
    byte k0;

    for (x = 0; x < width * 2; x++) {
        k0 = data[x];
        for (y = 0; y < height - 1; y++)
            data[y * width * 2 + x] = data[(y + 1) * width * 2 + x];
        data[y * width * 2 + x] = k0;
    }
}

//*************************************************
void Cel::down()
{
    int x, y;
    byte k0;

    for (x = 0; x < width * 2; x++) {
        k0 = data[(height - 1) * width * 2 + x];
        for (y = height - 1; y > 0; y--)
            data[y * width * 2 + x] = data[(y - 1) * width * 2 + x];
        data[x] = k0;
    }
}

//*************************************************
void Cel::mirrorh()
{
    int x, y;
    byte k0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            k0 = data[y * width * 2 + x];
            data[y * width * 2 + x] = data[y * width * 2 + width * 2 - 1 - x];
            data[y * width * 2 + width * 2 - 1 - x] = k0;
        }
    }
}

//*************************************************
void Cel::mirrorv()
{
    int x, y;
    byte k0;

    for (x = 0; x < width * 2; x++) {
        for (y = 0; y < height / 2; y++) {
            k0 = data[y * width * 2 + x];
            data[y * width * 2 + x] = data[(height - 1 - y) * width * 2 + x];
            data[(height - 1 - y)*width * 2 + x] = k0;
        }
    }
}

//*************************************************
void Cel::copy(Cel c)
{
    width = c.width;
    height = c.height;
    transcol = c.transcol;
    mirror = c.mirror;
    if (data)
        free(data);
    data = (byte *)malloc(width * 2 * height);

    for (int i = 0; i < width * 2 * height; i++)
        data[i] = c.data[i];
}

//*************************************************
int Cel::setc(int xn, int yn, byte c0, byte c)
{
    if (xn < 0 || xn > width - 1 || yn < 0 || yn > height - 1)
        return 0;
    if (data[width * 2 * yn + xn * 2] == c)
        return 0;
    if (data[width * 2 * yn + xn * 2] == c0) {
        data[width * 2 * yn + xn * 2] = c;
        data[width * 2 * yn + xn * 2 + 1] = c;
        return 1;
    } else
        return 0;
}

//*************************************************
void Cel::fill1(int xn, int yn, byte c0, byte c)
{
    if (setc(xn, yn - 1, c0, c) == 1)
        fill1(xn, yn - 1, c0, c);
    if (setc(xn - 1, yn, c0, c) == 1)
        fill1(xn - 1, yn, c0, c);
    if (setc(xn, yn, c0, c) == 1)
        fill1(xn, yn, c0, c);
    if (setc(xn + 1, yn, c0, c) == 1)
        fill1(xn + 1, yn, c0, c);
    if (setc(xn, yn + 1, c0, c) == 1)
        fill1(xn, yn + 1, c0, c);
}

//*************************************************
void Cel::fill(int xn, int yn, byte c)
{
    byte c0;

    c0 = data[width * 2 * yn + xn * 2];
    fill1(xn, yn, c0, c);
}

//**************************************************
Loop::Loop()
{
    mirror = mirror1 = -1;
    cels = NULL;
    CelLoc = NULL;
}

//**************************************************
void Loop::numcels(int n)
{
    NumCels = n;
    if (cels)
        delete [] cels;
    if (CelLoc)
        delete [] CelLoc;
    cels = new Cel[NumCels];
    CelLoc = new int[NumCels];
}

void Loop::cel(int n, int w, int h, int c, bool m)
{
    cels[n] = Cel(w, h, c, m);
}

//**************************************************
void Loop::insertCel_before(int n)
//insert cell before 'n'
{
    Cel *cels1 = new Cel[NumCels + 1];
    for (int i = 0; i < NumCels + 1; i++) {
        if (i < n)
            cels1[i] = cels[i];
        else if (i > n)
            cels1[i] = cels[i - 1];
        else {
            if (i > 0)
                cels1[i] = Cel(cels[i - 1].width, cels[i - 1].height, cels[i - 1].transcol, cels[i - 1].mirror);
            else //can't be NumCels==0 !
                cels1[i] = Cel(cels[NumCels - 1].width, cels[NumCels - 1].height, cels[NumCels - 1].transcol, cels[NumCels - 1].mirror);
            cels1[i].clear();
        }
    }
    delete [] cels;
    cels = cels1;
    NumCels++;
}

//**************************************************
void Loop::insertCel_after(int n)
//insert cell after 'n'
{
    Cel *cels1 = new Cel[NumCels + 1];
    for (int i = 0; i < NumCels + 1; i++) {
        if (i <= n)
            cels1[i] = cels[i];
        else if (i > n + 1)
            cels1[i] = cels[i - 1];
        else {
            if (i > 0)
                cels1[i] = Cel(cels[i - 1].width, cels[i - 1].height, cels[i - 1].transcol, cels[i - 1].mirror);
            else  //can't be NumCels==0!
                cels1[i] = Cel(cels[NumCels - 1].width, cels[NumCels - 1].height, cels[NumCels - 1].transcol, cels[NumCels - 1].mirror);
            cels1[i].clear();
        }
    }
    delete [] cels;
    cels = cels1;
    NumCels++;
}

//**************************************************
void Loop::appendCel()
{
    Cel *cels1 = new Cel[NumCels + 1];
    for (int i = 0; i < NumCels; i++)
        cels1[i] = cels[i];

    cels1[NumCels] = Cel(cels[NumCels - 1].width, cels[NumCels - 1].height, cels[NumCels - 1].transcol, cels[NumCels - 1].mirror);
    cels1[NumCels].clear();

    delete [] cels;
    cels = cels1;
    NumCels++;
}

//**************************************************
void Loop::deleteCel(int n)
{
    for (int i = n; i < NumCels - 1; i++)
        cels[i] = cels[i + 1];
    NumCels--;
}

//**************************************************
void Loop::clear()
{
    NumCels = 1;
    cels[0].clear();
}
//**************************************************
