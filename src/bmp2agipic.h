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

#ifndef bmp2agipic_h
#define bmp2agipic_h

#include <QImage>
#include <QByteArray>

// Converts bitmaps (pic and pri) into an AGI "picture" resource.
// Returns NULL if success, or error message otherwise.
// Pri image may be null (QImage.isNull().
const char *bitmapToAGIPicture(const QImage &pic, const QImage &pri, QByteArray *res);

#endif
