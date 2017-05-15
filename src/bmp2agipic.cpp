/*
 *  Bitmap to (Sierra) AGI picture resource converter
 *  Copyright (C) 2012 Jarno Elonen
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

#include "bmp2agipic.h"

#include <QColor>

#include <cmath>
#include <queue>
#include <vector>

#include <cstdio>
#include <cassert>

static const int AGI_WIDTH = 160;
static const int AGI_HEIGHT = 168;
static const int N_COLORS = 16;
static const unsigned char COLOR_NONE = 255;

const QColor ega[] = {
    QColor(0x00, 0x00, 0x00),        // black
    QColor(0x00, 0x00, 0xA0),        // blue
    QColor(0x00, 0xA0, 0x00),        // green
    QColor(0x00, 0xA0, 0xA0),        // cyan
    QColor(0xA0, 0x00, 0x00),        // red
    QColor(0xA0, 0x00, 0xA0),        // magenta
    QColor(0xA0, 0x50, 0x00),        // brown
    QColor(0xA0, 0xA0, 0xA0),        // light grey
    QColor(0x50, 0x50, 0x50),        // dark grey
    QColor(0x50, 0x50, 0xFF),        // light blue
    QColor(0x50, 0xFF, 0x50),        // light green
    QColor(0x50, 0xFF, 0xFF),        // light cyan
    QColor(0xFF, 0x50, 0x50),        // light red
    QColor(0xFF, 0x50, 0xFF),        // light magenta
    QColor(0xFF, 0xFF, 0x50),        // yellow
    QColor(0xFF, 0xFF, 0xFF)         // white
};

typedef unsigned char AGIPic[AGI_WIDTH][AGI_HEIGHT];

struct Coord {
    Coord(int _x, int _y) : x(_x), y(_y) {}
    int x, y;
};

struct CoordColor {
    CoordColor(int _x, int _y, unsigned char _c) : x(_x), y(_y), c(_c) {}
    int x, y;
    unsigned char c;
};



// Quantize given image to EGA palette and reshape to AGI native AGI_WIDTHxAGI_HEIGHT
void QuantizeAGI(const QImage &img, AGIPic &out)
{
#define SQUARE(x) ((x)*(x))

    assert(img.width() == AGI_WIDTH || img.width() == AGI_WIDTH * 2);
    assert(img.height() >= AGI_HEIGHT);

    int xstep = (img.width() == AGI_WIDTH) ? 1 : 2;
    for (int y = 0; y < AGI_HEIGHT; ++y) {
        for (int x = 0; x < img.width(); x += xstep) {
            float selErr = -1.f;
            unsigned char sel = 0;
            for (int c = 0; c < N_COLORS; ++c) {
                QColor pix(img.pixel(x, y));
                float err = sqrtf(
                                SQUARE(ega[c].redF() - pix.redF()) +
                                SQUARE(ega[c].greenF() - pix.greenF()) +
                                SQUARE(ega[c].blueF() - pix.blueF()));
                if (selErr < 0.f || err < selErr) {
                    sel = c;
                    selErr = err;
                }
            }
            out[x / xstep][y] = sel;
        }
    }
}

unsigned char &agiPix(AGIPic &pic, int x, int y)
{
    static unsigned char dummy = COLOR_NONE;
    assert(dummy == COLOR_NONE); // not been accidentally overwritten...
    if (x < 0 || x >= AGI_WIDTH || y < 0 || y >= AGI_HEIGHT)
        return dummy;
    else
        return pic[x][y];
}

// Returns true if neighborhood is solid, suitable for floodfill
bool isOnFloodFillArea(AGIPic &pic, int x, int y)
{
    if (agiPix(pic, x, y) == COLOR_NONE)
        return false;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (agiPix(pic, x, y) != agiPix(pic, x + dx, y + dy))
                return false;
    return true;
}

bool has4NeighborOfColor(AGIPic &pic, int x, int y, unsigned char c)
{
    return \
           (x < AGI_WIDTH - 1 && agiPix(pic, x + 1, y + 0) == c) ||
           (x > 0   && agiPix(pic, x - 1, y + 0) == c) ||
           (y < AGI_HEIGHT - 1 && agiPix(pic, x + 0, y + 1) == c) ||
           (y > 0   && agiPix(pic, x + 0, y - 1) == c);
}

// Trace how long a line continues to given direction (excluding starting pixel)
int traceToDir(AGIPic &pic, int x, int y, int dx, int dy, unsigned char c, int max)
{
    assert(c != COLOR_NONE); // could cause infinite trace
    int cnt = -1;
    do {
        cnt++;
        if (cnt >= max)
            break;
        x += dx;
        y += dy;
    } while (agiPix(pic, x, y) == c);
    return cnt;
}

int count8NeighborOfColor(AGIPic &pic, int x, int y, unsigned char c)
{
    int res = 0;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0)
                if (agiPix(pic, x + dx, y + dy) == c)
                    res++;
    return res;
}


// Flood fill area with color COLOR_NONE, but
// leave border pixels of previous COLOR_NONE areas
// untouched so that we'll get proper fill borders later.
void floodFillEmpty(AGIPic &pic, int x, int y)
{
    std::queue<Coord> q;
    q.push(Coord(x, y));

    const unsigned char visited = COLOR_NONE - 1;
    const unsigned char src_color = pic[x][y];
    assert(src_color != COLOR_NONE);
    assert(src_color != visited);

    while (!q.empty()) {
        Coord c = q.front();
        q.pop();
        if (agiPix(pic, c.x, c.y) == src_color &&
                !has4NeighborOfColor(pic, c.x, c.y, COLOR_NONE)) {
            agiPix(pic, c.x, c.y) = visited;
            q.push(Coord(c.x + 1, c.y + 0));
            q.push(Coord(c.x - 1, c.y + 0));
            q.push(Coord(c.x + 0, c.y + 1));
            q.push(Coord(c.x + 0, c.y - 1));
        }
    }

    for (int y = 0; y < AGI_HEIGHT; ++y)
        for (int x = 0; x < AGI_WIDTH; ++x)
            if (agiPix(pic, x, y) == visited)
                agiPix(pic, x, y) = COLOR_NONE;
}

int findBestLineStartFromArea(AGIPic &pic, int x0, int y0, int w, int h, int color, std::vector<Coord> &singlePoints, Coord &minCoord)
{
    // Find pixel with the least neighbors of color i
    // (i.e. most probable end of line)
    int minCount = 0xFF;

    for (int y = y0; y < y0 + h; ++y)
        for (int x = x0; x < x0 + w; ++x) {
            if (agiPix(pic, x, y) == color) {
                int cnt = count8NeighborOfColor(pic, x, y, agiPix(pic, x, y));
                if (cnt == 0) { // lonely pixel, handle immediately
                    singlePoints.push_back(Coord(x, y));
                    agiPix(pic, x, y) = COLOR_NONE;
                } else if (cnt < minCount) { // line
                    minCount = cnt;
                    minCoord.x = x;
                    minCoord.y = y;
                }
            }
        }
    return minCount;
}

void replaceLines(AGIPic &pic, QByteArray *res, bool isPri)
{
    // Handle one color at a time
    for (int color = 0; color < N_COLORS; ++color) {
        // Check that there's at least one pixel of color i left
        for (int y = 0; y < AGI_HEIGHT; ++y)
            for (int x = 0; x < AGI_WIDTH; ++x)
                if (agiPix(pic, x, y) == color)
                    goto found;
        continue;
found:
        *res += char(isPri ? 0xF2 : 0xF0);   // Change color command
        *res += char(color);   // Color index

        std::vector<Coord> singlePoints;

        while (true) { // loop until no pixels of color i are found
            Coord minCoord(-1, -1);
            int minCount = findBestLineStartFromArea(pic, 0, 0, AGI_WIDTH, AGI_HEIGHT, color, singlePoints, minCoord);
            if (minCount == 0xFF)   // No more lines found
                break;

newTrace:
            // Start tracing the line
            int x = minCoord.x, y = minCoord.y;
            assert(agiPix(pic, x, y) == color);
            *res += char(0xF7);   // Relative line command
            *res += char(x);   // Starting x
            *res += char(y);   // Starting y
            agiPix(pic, x, y) = COLOR_NONE;

            // Trace a line
            while (true) {
                // Find out direction into which the line continues the longest
                int max = 0, maxdx = 0xFF, maxdy = 0xFF;
                for (int dx = -1; dx <= 1; ++dx)
                    for (int dy = -1; dy <= 1; ++dy)
                        if (dx != 0 || dy != 0) {
                            int len = traceToDir(pic, x, y, dx, dy, color, AGI_WIDTH);
                            if (len > max) {
                                max = len;
                                maxdx = dx;
                                maxdy = dy;
                            }
                        }

                if (max == 0) { // Nowhere to go, end the trace
                    // Just for aesthetic reasons, try to start the next
                    // line nearby the end of the last one
                    minCount = findBestLineStartFromArea(pic, x - 5, y - 5, 10, 10, color, singlePoints, minCoord);
                    if (minCount == 0xFF)
                        break;        // Not found, do a full pic scan
                    else
                        goto newTrace; // New line in the neighborhood, start from there
                }

                if (max > 6) // Clamp to maximum length of relative line
                    max = 6;

                int stepx = maxdx * max;
                int stepy = maxdy * max;

                assert(abs(stepx) <= 7);
                assert(abs(stepy) <= 7);

                // Write out the relative line stepping byte
                unsigned char chr =
                    (((stepx < 0) ? 1 : 0) << 7) | // x sign
                    (abs(stepx) << 4) |     // Xdisp
                    (((stepy < 0) ? 1 : 0) << 3) | // y sign
                    (abs(stepy) << 0);       // Ydisp

                assert((chr & 0xF0) != 0xF0);      // Must not be mistaken for a draw command

                *res += char(chr);

                // Clear the line
                while (max >= 0) {
                    agiPix(pic, x + maxdx * max, y + maxdy * max) = COLOR_NONE;
                    --max;
                }

                // Move pointer to the new end
                x += stepx;
                y += stepy;
            }
        }
        // When all continuous lines are drawn,
        // write out single points as a brush/plot/pen operation
        if (singlePoints.size() > 0) {
            *res += char(0xFA);   // Pen command
            for (int i = 0; i < (int)singlePoints.size(); ++i) {
                assert((unsigned char)singlePoints[i].x < 0xF0);
                assert((unsigned char)singlePoints[i].y < 0xF0);
                *res += char(singlePoints[i].x);
                *res += char(singlePoints[i].y);
            }
        }
    }
}

// Convert either a visual or a priority image and write to res
void oneChannelToAGIPicture(const QImage &chan, QByteArray *res, bool isPri)
{
    unsigned char pic[AGI_WIDTH][AGI_HEIGHT];
    QuantizeAGI(chan, pic);

    // We're doing one channel at a time, disable the other one first
    *res += char(isPri ? 0xF1 : 0xF3);

    // Clear the default color, no sense in filling/drawing with it:
    unsigned default_color = isPri ? 4 : 15; // 4=red for pri, 15=white for visual
    for (int y = 0; y < AGI_HEIGHT; ++y)
        for (int x = 0; x < AGI_WIDTH; ++x)
            if (agiPix(pic, x, y) == default_color)
                agiPix(pic, x, y) = COLOR_NONE;

    // Record flood fill areas and empty them
    // (color by color in reverse order to favor black outlines)
    std::vector<CoordColor> floodFills;
    for (int c = N_COLORS - 1; c >= 0; --c)
        for (int y = 0; y < AGI_HEIGHT; ++y)
            for (int x = 0; x < AGI_WIDTH; ++x)
                if (agiPix(pic, x, y) == c && isOnFloodFillArea(pic, x, y)) {
                    floodFills.push_back(CoordColor(x, y, agiPix(pic, x, y)));
                    floodFillEmpty(pic, x, y);
                }

    // Replace lines and single pixels with commands
    replaceLines(pic, res, isPri);

    // Write out flood fills
    unsigned char curColor = COLOR_NONE;
    for (int i = 0; i < (int)floodFills.size(); ++i) {
        CoordColor c = floodFills[i];
        assert(c.c != default_color);   // we shouldn't need to fill with default color
        floodFills.erase(floodFills.begin() + i);
        i--;
        if (curColor != c.c) {
            curColor = c.c;
            *res += char(isPri ? 0xF2 : 0xF0);   // Change color command
            *res += char(c.c);   // Color index
            *res += char(0xF8);   // Flood fill command
        }
        assert(curColor != COLOR_NONE);   // i.e: flood fill command written
        *res += char(c.x);
        *res += char(c.y);
    }
    assert(floodFills.empty());
}


// Converts bitmaps (pic and pri) into an AGI "picture" resrouce.
// Returns NULL if success, or error message otherwise.
const char *bitmapToAGIPicture(const QImage &pic, const QImage &pri, QByteArray *res)
{
    // Check given images & convert to EGA palette arrays
    if ((pic.width() != AGI_WIDTH * 2 && pic.width() != AGI_WIDTH) || pic.height() < AGI_HEIGHT)
        return ("Picture bitmap size must be 160x168 or 320x168.");
    if (!pri.isNull())
        if ((pri.width() != AGI_WIDTH * 2 && pri.width() != AGI_WIDTH) || pri.height() < AGI_HEIGHT)
            return ("Priority bitmap size must be 160x168 or 320x168.");

    // Set brush once at the beginning
    *res += char(0xF9);   // Change pen size & style
    *res += char(0x00);   // solid single pixel

    oneChannelToAGIPicture(pic, res, false);
    if (!pri.isNull())
        oneChannelToAGIPicture(pri, res, true);

    *res += char(0xFF);   // eof marker

    return NULL;
}
