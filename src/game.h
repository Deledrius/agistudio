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
#include <iosfwd>


typedef unsigned char byte;

typedef struct {
    char Filename[15];    //[*]vol.*
    long Loc;             //location in vol file
    bool Exists;
} TResourceInfo ;

#define MaxResourceSize 65530

typedef struct {
    byte *Data;
    int Size;
} TResource;


class QSettings;

class Game
{
public:
    Game();
    int open(const std::string &name);
    int newgame(const std::string &name);
    int from_template(const std::string &name);
    void reset_settings();
    int close();
    void make_source_dir();
    int GetResourceSize(int ResType, int ResNum) const;
    int ReadResource(int ResourceType, int ResourceID);
    int AddResource(int ResType, int ResNum);
    int DeleteResource(int ResType, int ResNum);
    int RebuildVOLfiles();
    int RecompileAll();

    TResourceInfo ResourceInfo[4][256];  //logic, picture, view, sound
    std::string dir;  //game directory
    std::string ID;   //game ID for V3 games (always 'V2' for V2 games)
    std::string dirname;  //name of the 'directory' file
    //(e.g. picdir, snddir for V2, [ID]dir for V3)

    std::string srcdir;   // Directory for saving logic sources
    bool isOpen, isV3;

    // Keep our application settings here, accessible to other windows.
    //  Some GUI defaults are part of Game object because it is the
    //  only object which is guaranteed to exist at the start of the program.
    QSettings *settings;

private:
    long AGIVersionNumber;
    std::string FindAGIV3GameID(const std::string &gamepath) const;
    long GetAGIVersionNumber(void) const;
    int ReadV3Resource(char ResourceType, int ResourceID);
    std::unique_ptr<std::fstream> OpenPatchVol(int PatchVol, int *filesize) const;
    std::unique_ptr<std::fstream> OpenDirUpdate(int *dirsize, int ResType);
};

extern Game *game;

extern const char *ResTypeName[4];
extern const char *ResTypeAbbrv[4];

extern TResource ResourceData;

extern const char EncryptionKey[];

#define MAX_TMP 2048
extern char tmp[];

// Resource types
// (The numbers assigned to these defines are important!)
#define LOGIC   0
#define PICTURE 1
#define VIEW    2
#define SOUND   3

// Additional resource types
#define OBJECT  4
#define WORDS   5
#define TEXTRES 6

#define RESOURCES 7
#define PREVIEW   8
#define HELPWIN   9

// PicEdit styles (tools and picture in one window, or in separate windows for small displays)
#define P_ONE  0
#define P_TWO  1

#endif  //GAME_H
