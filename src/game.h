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

#ifndef GAME_H
#define GAME_H
#include <string>
#include <stdio.h>
#include "global.h"

using namespace std;

typedef struct {
  char Filename[12];    //[*]vol.*
  long Loc;             //location in vol file
  bool Exists;
}TResourceInfo ;

#define MaxResourceSize 65530

typedef struct {
  byte *Data;
  int Size;
}TResource;


class Game
{
 public:
  Game();
  int open(string name);
  int newgame(string name);
  int from_template(string name);
  int close();
  void save_settings();
  void read_settings();
  void defaults();
  void make_source_dir();
  int GetResourceSize(char ResType,int ResNum);
  int ReadResource(char ResourceType, int ResourceID);
  int AddResource(int ResType,int ResNum);
  int DeleteResource(int ResType,int ResNum);
  int RebuildVOLfiles();
  int RecompileAll();

  TResourceInfo ResourceInfo[4][256];  //logic, picture, view, sound
  string dir;  //game directory
  string ID;   //game ID for V3 games (always 'V2' for V2 games)
  string dirname;  //name of the 'directory' file
                   //(e.g. picdir, snddir for V2, [ID]dir for V3)

  string srcdir;   //dir for saving logic sources
  bool isOpen,isV3;

  //defaults; some GUI defauts are part of GAME object because it is the
  //only object which is guaranteed to exist at the beginning of the program
  int res_default;  //default resource type in resources window
  int picstyle;     //Picedit style
  bool save_logic_as_text;  //default for 'extract' function
  bool show_all_messages;   //logic decompile - show all messages at end
                            //or just unused ones
  bool show_elses_as_gotos;
  bool show_special_syntax; //v30=4 vs assignn(v30,4)
  bool reldir;  //if the source dir is relative to the game dir or absolute
  string command;  //interpreter command line
  string srcdirname;  //source dir as entered in options
       //(i.e. either relative or absolute; srcdir is always absolute)
  string templatedir;  //template game directory
  string helpdir;      //help directory
 private:
  double AGIVersionNumber;
  string FindAGIV3GameID(const char *name);
  double GetAGIVersionNumber(void);
  int ReadV3Resource(char ResourceType,int ResourceID);
  FILE *OpenPatchVol(int PatchVol,int *filesize);
  FILE *OpenDirUpdate(int *dirsize,int ResType);
};

extern Game *game;

extern const char *ResTypeName[4];
extern const char *ResTypeAbbrv[4];

extern TResource ResourceData;

extern const char EncryptionKey[];

#define MAX_TMP 2048
extern char tmp[];

//resource types
//(the numbers assigned to these defines are important !)
#define LOGIC   0
#define PICTURE 1
#define VIEW    2
#define SOUND   3

//additional resource types
#define OBJECT  4
#define WORDS   5
#define TEXT    6

#define RESOURCES 7
#define PREVIEW   8
#define HELPWIN   9

//Picedit styles (tools and picture in one window or in separate windows (for small displays))
#define P_ONE  0
#define P_TWO  1

#endif  //GAME_H
