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

#ifndef UTIL_H
#define UTIL_H

#include <string>
using namespace std;

#include "global.h"

#define MAX(a,b)   ((a>b)?(a):(b))
#define MIN(a,b)   ((a<b)?(a):(b))

//*****************************************************
class TStringList
{
 public:
  TStringList();
  void add(const string&);
  void addsorted(const char *);
  void addsorted(const string&);
  void copy(const TStringList&);
  void lfree(void);
  void print(int);
  void print(void);
  void toLower(void);
  void toLower(int);
  void del(int);
  void replace(int n, const char *str);
  void replace(int n, const string& str);
  string at(int) const;
  int num;
  string *data;
  int max;
  int inc;

};

//*****************************************************
extern string MultStr(const char *str,int NumCopies);
extern string IntToStr(int n);
extern string IntToStr(byte n);
extern void toLower(string *str);
extern void toLower(char *str);
//*****************************************************
#endif
