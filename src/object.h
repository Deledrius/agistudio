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

#ifndef OBJECT_H
#define OBJECT_H

#include "util.h"

#define MaxItems   256

//the list of inventory objects
class ObjList
{
 public:
  ObjList();
  TStringList ItemNames;
  byte RoomNum[MaxItems];
  byte MaxScreenObjects; //what this is for ?
  void XORData();
  int read(char *filename,bool FileIsEncrypted);
  int save(char *filename,bool FileIsEncrypted);
  bool GetItems();
  void clear();
};


#endif
