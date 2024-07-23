/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  A big part of this code was adapted from the Windows AGI Studio
 *  developed by Peter Kelly.
 *
 *  LZW decompression code belongs to Lance Ewing.
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


#include <fstream>

#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QStatusBar>

#include "menu.h"
#include "agicommands.h"
#include "game.h"
#include "logedit.h"


const char *ResTypeName[4] =  {"logic", "picture", "view", "sound"};
const char *ResTypeAbbrv[4] = {"log", "pic", "view", "snd"};
static const auto files =     {"VOL.0", "VIEWDIR", "LOGDIR", "SNDDIR", "PICDIR"};
Game *game;

static TResource CompressedResource;

/******************************* LZW variables ****************************/
#define MAXBITS 12
#define TABLE_SIZE  18041
#define START_BITS  9

static int BITS, MAX_VALUE, MAX_CODE;
static unsigned int *prefix_code;
static byte *append_character;
static byte decode_stack[4000];  /* Holds the decoded string */
static int input_bit_count = 0;  /* Number of bits in input bit buffer */
static unsigned long input_bit_buffer = 0L;
//*******************************************

const char EncryptionKey[] = "Avis Durgan";
TResourceInfo ResourceInfo[4][256];

TResource ResourceData;
/* global buffer used for all resource I/0 ! (for this reason it wouldn't be
 wise to run several I/O operations simultaneously, like clicking compile
 very fast in several logic editor windows ("compile all" is ok - it is
 sequential) - fortunately the program works too fast to allow the user
 to do it...) */

//*******************************************
Game::Game() : AGIVersionNumber(0), ResourceInfo(), isOpen(false), isV3(false)
{
    ResourceData.Data = (byte *)malloc(MaxResourceSize);

    settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "AGI Studio", "AGI Studio");
    if (settings->allKeys().empty())
        reset_settings();
}

//*******************************************
// Open the game found in 'gamepath'.
int Game::open(const std::string &gamepath)
{
    byte DirData[3080];
    int CurResType, CurResNum, NumDirEntries;
    long DirSize = -1, DirOffset = -1;
    byte byte1, byte2, byte3;
    bool ErrorOccured = false;

    dir = gamepath;

    for (CurResType = 0; CurResType <= 3; CurResType++) {
        for (CurResNum = 0; CurResNum <= 255; CurResNum++)
            ResourceInfo[CurResType][CurResNum].Exists = false;
    }

    ID = FindAGIV3GameID(dir);
    if (ID.empty())
        return 1;

    isV3 = (ID != "V2");
    if (!isV3) {
        // For v2 game: open 'logdir', 'picdir', 'viewdir', 'snddir'

        for (CurResType = 0; CurResType <= 3; CurResType++) {
            auto dir_filename = std::filesystem::path(dir) / (std::string(ResTypeAbbrv[CurResType]) + "DIR");
            auto dir_stream = std::ifstream(dir_filename, std::ios::binary);

            if (!dir_stream.is_open()) {
                menu->errmes("Error: can't open '%s'!", dir_filename.string().c_str());
                ErrorOccured = true;
                break;
            } else {
                int size = std::filesystem::file_size(dir_filename);
                if (size >  768) {
                    menu->errmes("Error: '%s' is too big (should not be more than 768 bytes)!", dir_filename.string().c_str());
                    ErrorOccured = true;
                    break;
                } else {
                    // Read resource info
                    dir_stream.read(reinterpret_cast<char *>(DirData), size);

                    for (CurResNum = 0; CurResNum <= size / 3 - 1; CurResNum++) {
                        byte1 = DirData[CurResNum * 3];
                        byte2 = DirData[CurResNum * 3 + 1];
                        byte3 = DirData[CurResNum * 3 + 2];
                        if (!(byte1 == 0xff && byte2 == 0xff && byte3 == 0xff)) {
                            ResourceInfo[CurResType][CurResNum].Loc = (byte1 % 16) * 0x10000 + byte2 * 0x100 + byte3;
                            sprintf(ResourceInfo[CurResType][CurResNum].Filename, "VOL.%d", byte1 / 16);
                            ResourceInfo[CurResType][CurResNum].Exists = true;
                        }
                    }
                }
                dir_stream.close();
            }
        }
    } else {
        // For v3 game: open [GAME_ID]dir (e.g. grdir, mhdir)
        auto dir_filename = std::filesystem::path(dir) / (ID + "DIR");
        auto dir_stream = std::ifstream(dir_filename, std::ios::binary);
        if (!dir_stream.is_open()) {
            menu->errmes("Error: can't open '%s'!", dir_filename.string().c_str());
            ErrorOccured = true;
        } else {
            int size = std::filesystem::file_size(dir_filename);
            if (size > 3080) {
                menu->errmes("Error: '%s' is too big (should not be more than 3080 bytes)!", dir_filename.string().c_str());
                ErrorOccured = true;
            } else {
                // Read resource info
                dir_stream.read(reinterpret_cast<char *>(DirData), size);
                for (CurResType = 0; CurResType <= 3; CurResType++) {
                    if (!ErrorOccured) {
                        switch (CurResType) {
                            case LOGIC:
                                DirOffset = DirData[0] + DirData[1] * 256;
                                DirSize = DirData[2] + DirData[3] * 256 - DirOffset;
                                break;
                            case PICTURE:
                                DirOffset = DirData[2] + DirData[3] * 256;
                                DirSize = DirData[4] + DirData[5] * 256 - DirOffset;
                                break;
                            case VIEW:
                                DirOffset = DirData[4] + DirData[5] * 256;
                                DirSize = DirData[6] + DirData[7] * 256 - DirOffset;
                                break;
                            case SOUND:
                                DirOffset = DirData[6] + DirData[7] * 256;
                                DirSize = size - DirOffset;
                                break;
                        }
                        if (DirOffset < 0 || DirSize < 0) {
                            menu->errmes("Error: DIR file is invalid.");
                            ErrorOccured = true;
                            break;
                        } else if (DirOffset + DirSize > size) {
                            menu->errmes("Error: Directory is beyond end of '%s' file!", dir_filename.string().c_str());
                            ErrorOccured = true;
                            break;
                        } else {
                            if (DirSize > 768)
                                NumDirEntries = 256;
                            else
                                NumDirEntries = DirSize / 3;
                            if (NumDirEntries > 0) {
                                for (CurResNum = 0; CurResNum < NumDirEntries; CurResNum++) {
                                    byte1 = DirData[DirOffset + CurResNum * 3];
                                    byte2 = DirData[DirOffset + CurResNum * 3 + 1];
                                    byte3 = DirData[DirOffset + CurResNum * 3 + 2];
                                    if (!(byte1 == 0xff && byte2 == 0xff && byte3 == 0xff)) {
                                        ResourceInfo[CurResType][CurResNum].Loc = (byte1 % 16) * 0x10000 + byte2 * 0x100 + byte3;
                                        sprintf(ResourceInfo[CurResType][CurResNum].Filename, "%sVOL.%d", ID.c_str(), byte1 / 16);
                                        ResourceInfo[CurResType][CurResNum].Exists = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            dir_stream.close();
        }
    }

    if (!ErrorOccured) {
        AGIVersionNumber = GetAGIVersionNumber();
        CorrectCommands(AGIVersionNumber);
        isOpen = true;
        make_source_dir();
        menu->showStatusMessage(dir.c_str());
        return 0;
    } else
        return 1;
}

//*******************************************
// Close current game
int Game::close()
{
    isOpen = false;
    return 0;
}

//*******************************************
// Create a new game (in 'path' folder) from template
int Game::from_template(const std::string &path)
{
    dir = path;
    make_source_dir();
    auto template_dir = game->settings->value("TemplateDir").toString().toStdString();

    // Check if template directory contains *dir files and vol.0
    for (const auto &file : files) {
        auto template_file = std::filesystem::path(template_dir) / file;
        if (!std::filesystem::exists(template_file)) {
            menu->errmes("Can't read '%s' in template directory '%s'!", file, template_dir.c_str());
            return 1;
        }
    }

    // Copy all template files to the new game location
    for (const auto &entry : std::filesystem::directory_iterator(template_dir)) {
        if (entry.is_regular_file()) {
            auto newfile = std::filesystem::path(path) / entry.path().filename();
            std::filesystem::copy_file(entry, newfile, std::filesystem::copy_options::overwrite_existing);
        }
    }

    // Copy all logic source files to the new game location
    std::error_code ec; // Prevents directory_iterator from throwing exceptions for an empty folder
    auto template_src_path = std::filesystem::path(template_dir) / "src";
    for (const auto &entry : std::filesystem::directory_iterator(template_src_path, ec)) {
        if (entry.is_regular_file()) {
            auto newfile = std::filesystem::path(srcdir) / entry.path().filename();
            std::filesystem::copy_file(entry, newfile, std::filesystem::copy_options::overwrite_existing);
        }
    }

    return open(path);
}

//*******************************************
// Create directory for game sources (extracting logic, etc.).
void Game::make_source_dir()
{
    if (game->settings->value("UseRelativeSrcDir").toBool())
        srcdir = dir + "/" + game->settings->value("RelativeSrcDir").toString().toStdString();  // srcdir is inside the game directory
    else
        srcdir = game->settings->value("AbsoluteSrcDir").toString().toStdString();              // srcdir can be anywhere

    if (!std::filesystem::exists(srcdir)) {
        if (std::filesystem::create_directory(srcdir) == false)
            menu->errmes("Can't create the source directory '%s'!\nLogic text files will not be saved.", srcdir.c_str());
    }
}

//*******************************************
// Creates a new empty game in folder "path".
int Game::newgame(const std::string &path)
{
    static byte BlankObjectFile[8] = {0x42, 0x76, 0x79, 0x70, 0x20, 0x44, 0x4A, 0x72};
    static byte BlankWordsFile [72] = {
        0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x9E, 0x00, 0x00, 0x01, 0x11, 0x06, 0x08, 0x10, 0x0D, 0x9B, 0x00,
        0x01, 0x00, 0x0D, 0x10, 0x93, 0x27, 0x0F, 0x00
    };
    dir = path;

    for (const auto &file : files)
        std::ofstream { std::filesystem::path(path) / file, std::ios::binary | std::ios::trunc };

    auto objfile = std::ofstream(std::filesystem::path(path) / "OBJECT", std::ios::binary | std::ios::trunc);
    objfile.write(reinterpret_cast<const char *>(BlankObjectFile), sizeof(BlankObjectFile));
    objfile.close();

    auto wordsfile = std::ofstream(std::filesystem::path(path) / "WORDS.TOK", std::ios::binary | std::ios::trunc);
    wordsfile.write(reinterpret_cast<const char *>(BlankWordsFile), sizeof(BlankWordsFile));
    wordsfile.close();

    make_source_dir();

    isOpen = true;
    isV3 = false;
    ID = "";
    AGIVersionNumber = 2917000;

    for (int i = 0; i <= 3; i++) {
        for (int j = 0; j <= 255; j++)
            ResourceInfo[i][j].Exists = false;
    }
    menu->showStatusMessage(dir.c_str());
    return 0;
}

//*******************************************
// Compare the filename prefix (if any) for vol.0 and dir;
// If they exist and are the same, it is a v3 game.
// Returns v3 game id, or 'V2' if a v3 Game is not found.
std::string Game::FindAGIV3GameID(const std::string &gamepath) const
{
    std::string ID1 = "V2";
    std::string dirString = "";
    std::string volString = "";

    for (const auto &entry : std::filesystem::directory_iterator(gamepath)) {
        const auto &filename = entry.path().filename().string();
        if (entry.is_regular_file()) {
            if (dirString.empty() && (filename.ends_with("DIR") || filename.ends_with("dir")))
                dirString = filename.substr(0, 3);
            else if (volString.empty() && (filename.ends_with("VOL.0") || filename.ends_with("vol.0")))
                volString = filename.substr(0, 3);
        }
    }

    if (!volString.empty() && (volString == dirString))
        ID1 = volString;

    return ID1;
}

//*******************************************************
long Game::GetAGIVersionNumber(void) const
{
    int ResPos, x;
    byte VerLen;
    bool ISVerNum;
    long VerNum;
    char VersionNumBuffer[] = "A_CDE_GHI";
    int ret = 2917000;

    // This is what we use if we can't find the version number.
    // Version 2.917 is the most common interpreter and
    // the one that all the "new" AGI games should be based on.

    ResourceData.Size = 0;
    auto data_file = std::filesystem::path(dir) / "AGIDATA.OVL";
    auto data_stream = std::ifstream(data_file, std::ios::binary);
    if (data_stream.is_open()) {
        int size = std::filesystem::file_size(data_file);

        if (size < MaxResourceSize) {
            ResourceData.Size = size;
            data_stream.read(reinterpret_cast<char *>(ResourceData.Data), ResourceData.Size);
        }
        data_stream.close();
    }

    if (ResourceData.Size > 0) {
        ResPos = 0;
        VerLen = 0;
        while (ResPos < ResourceData.Size && VerLen == 0) {
            memcpy(VersionNumBuffer, &ResourceData.Data[ResPos], 9);
            ResPos++;
            ISVerNum = true;
            if (VersionNumBuffer[1] == '.') {
                if (VersionNumBuffer[0] < '0' || VersionNumBuffer[2] > '9')
                    ISVerNum = false;
                for (x = 2; x <= 4; x++) {
                    if (VersionNumBuffer[x] < '0' || VersionNumBuffer[x] > '9')
                        ISVerNum = false;
                }
                if (ISVerNum && (VersionNumBuffer[0] <= '2'))
                    VerLen = 5; // 2.xxx format
                if (VersionNumBuffer[5] != '.')
                    ISVerNum = false;
                for (x = 6; x <= 8; x++) { // 3.xxx.xxx format
                    if (VersionNumBuffer[x] < '0' || VersionNumBuffer[x] > '9')
                        ISVerNum = false;
                }
                if (ISVerNum)
                    VerLen = 9;
            } else
                ISVerNum = false;
            if (VerLen > 0) {
                VerNum = (VersionNumBuffer[0] - '0') * 1000000;
                VerNum += (VersionNumBuffer[2] - '0') * 100000;
                VerNum += (VersionNumBuffer[3] - '0') * 10000;
                VerNum += (VersionNumBuffer[4] - '0') * 1000;
                if (VersionNumBuffer[5] == '.') {
                    VerNum += (VersionNumBuffer[6] - '0') * 100;
                    VerNum += (VersionNumBuffer[7] - '0') * 10;
                    VerNum += (VersionNumBuffer[8] - '0');
                }
                if (VerNum != 0) {
                    ret = VerNum;
                    break;
                }
            }
        }
    }
    return ret;
}

//***************************************
int Game::GetResourceSize(int ResType, int ResNum) const
{
    unsigned char lsbyte, msbyte;

    if (ResourceInfo[ResType][ResNum].Exists) {
        const auto res_file = std::filesystem::path(dir) / ResourceInfo[ResType][ResNum].Filename;

        auto res_stream = std::ifstream(res_file, std::ios::binary);
        if (res_stream.is_open()) {
            int size = std::filesystem::file_size(res_file);

            if (size >= ResourceInfo[ResType][ResNum].Loc + 5) {
                res_stream.seekg(ResourceInfo[ResType][ResNum].Loc);
                res_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
                res_stream.read(reinterpret_cast<char *>(&msbyte), 1);

                if (lsbyte == 0x12 && msbyte == 0x34) {
                    res_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
                    res_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
                    res_stream.read(reinterpret_cast<char *>(&msbyte), 1);
                    return (msbyte * 256 + lsbyte);
                }
            }
            res_stream.close();
        }
    }
    return -1;
}

//***************************************
// Reads resource 'ResNum' from the VOL file.
int Game::ReadResource(int ResType, int ResNum)
{
    unsigned char msbyte, lsbyte;

    if (isV3)
        return ReadV3Resource(ResType, ResNum);

    const auto res_file = std::filesystem::path(dir) / ResourceInfo[ResType][ResNum].Filename;
    auto res_stream = std::ifstream(res_file, std::ios::binary);
    if (!res_stream.is_open()) {
        menu->errmes("Error reading file '%s/%s'!", dir.c_str(), ResourceInfo[ResType][ResNum].Filename);
        return 1;
    }

    int size = std::filesystem::file_size(res_file);
    if (ResourceInfo[ResType][ResNum].Loc > size) {
        menu->errmes("Error reading '%s': Specified resource location is past end of file.", ResourceInfo[ResType][ResNum].Filename);
        return 1;
    }

    res_stream.seekg(ResourceInfo[ResType][ResNum].Loc);
    res_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
    res_stream.read(reinterpret_cast<char *>(&msbyte), 1);
    if (!(lsbyte == 0x12 && msbyte == 0x34)) {
        menu->errmes("Error reading '%s': Resource signature not found.", ResourceInfo[ResType][ResNum].Filename);
        return 1;
    }

    res_stream.seekg(ResourceInfo[ResType][ResNum].Loc + 3);
    res_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
    res_stream.read(reinterpret_cast<char *>(&msbyte), 1);
    ResourceData.Size = msbyte * 256 + lsbyte;
    if (ResourceData.Size == 0) {
        menu->errmes("Error reading '%s': Resource size 0!", ResourceInfo[ResType][ResNum].Filename);
        return 1;
    }
    res_stream.read(reinterpret_cast<char *>(ResourceData.Data), ResourceData.Size);
    res_stream.close();

    return 0;
}

//***************************************
std::unique_ptr<std::fstream> Game::OpenPatchVol(int PatchVol, int *filesize) const
{
    std::string patch_filename;
    if (isV3)
        patch_filename = ID + "vol." + std::to_string(PatchVol);
    else
        patch_filename = "vol." + std::to_string(PatchVol);
    const auto patch_path = std::filesystem::path(dir) / patch_filename;

    auto patch_stream = std::make_unique<std::fstream>(patch_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!patch_stream->is_open())
        return nullptr;

    *filesize = std::filesystem::file_size(patch_path);
    return patch_stream;
}

//***********************************************
// Rewrite DIR files in v3 games.
static int RewriteDirFile(std::fstream *dirstream, int dirsize)
{
    byte DirData[4][768];
    int CurResType;
    byte lsbyte, msbyte;
    int Offset[4], Size[4];
    byte OffsetArray[8];

    dirstream->seekp(0);
    for (CurResType = 3; CurResType >= 0; CurResType--) {
        dirstream->seekp(CurResType * 2);
        dirstream->read(reinterpret_cast<char *>(&lsbyte), 1);
        dirstream->read(reinterpret_cast<char *>(&msbyte), 1);
        Offset[CurResType] = (msbyte << 8) | lsbyte;
        if (CurResType == 3)
            Size[CurResType] = dirsize - Offset[CurResType];
        else
            Size[CurResType] = Offset[CurResType + 1] - Offset[CurResType];
        if (Offset[CurResType] > dirsize || Size[CurResType] > 768 || Offset[CurResType] + Size[CurResType] > dirsize) {
            menu->errmes("DIR file is invalid.");
            dirstream->close();
            return 1;
        }

    }
    for (CurResType = 0; CurResType <= 3; CurResType++) {
        dirstream->seekp(Offset[CurResType]);
        memset(DirData[CurResType], 0xff, 768);
        dirstream->read(reinterpret_cast<char *>(DirData[CurResType]), Size[CurResType]);
    }

    OffsetArray[0] = 8;
    OffsetArray[1] = 0;
    Offset[0] = 8;

    for (CurResType = 1; CurResType <= 3; CurResType++) {
        Offset[CurResType] = Offset[CurResType - 1] + 768;
        OffsetArray[CurResType * 2] = Offset[CurResType] % 256;
        OffsetArray[CurResType * 2 + 1] = Offset[CurResType] / 256;
    }
    dirstream->seekp(0);
    dirstream->write(reinterpret_cast<char *>(OffsetArray), 8);
    for (CurResType = 0; CurResType <= 3; CurResType++)
        dirstream->write(reinterpret_cast<char *>(DirData[CurResType]), 768);
    dirstream->flush();

    return 0;
}

//***********************************************
std::unique_ptr<std::fstream> Game::OpenDirUpdate(int *dirsize, int ResType)
{
    std::string dir_filename;
    if (isV3)
        dir_filename = (ID + "dir");
    else
        dir_filename = (std::string(ResTypeAbbrv[ResType]) + "dir");
    const auto dir_path = std::filesystem::path(dir) / dir_filename;

    dirname = dir_path.string();
    auto dir_stream = std::make_unique<std::fstream>(dir_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!dir_stream->is_open()) {
        menu->errmes("Error writing file '%s': %s!", dirname.c_str());
        return nullptr;
    }

    *dirsize = std::filesystem::file_size(dir_path);
    return dir_stream;
}

//***********************************************
int Game::AddResource(int ResType, int ResNum)
{
    std::unique_ptr<std::fstream> file_stream, dir_stream;
    int filesize, dirsize;
    int PatchVol;
    byte ResHeader[7], DirByte[3];
    int off;
    byte lsbyte, msbyte;

    if ((dir_stream = OpenDirUpdate(&dirsize, ResType)) == nullptr)
        return 1;

    PatchVol = 0;
    file_stream = OpenPatchVol(PatchVol, &filesize); //open vol.0
    if (file_stream == nullptr) {
        menu->errmes("Can't open vol.%d!", PatchVol);
        return 1;
    }

    do {
        if (filesize + ResourceData.Size > 1048000) {
            // Current volume is too big (to fit a diskette) - create the next one...
            file_stream.reset();
            PatchVol++;
            file_stream = OpenPatchVol(PatchVol, &filesize);
            if (file_stream == nullptr) {
                if (isV3)
                    menu->errmes("Can't open %svol.%d!", ID.c_str(), PatchVol);
                else
                    menu->errmes("Can't open vol.%d!", PatchVol);
                return 1;
            }
        }
    } while (filesize + ResourceData.Size > 1048000);

    //write the resource to the patch volume and update the DIR file
    if (isV3) {
        if (RewriteDirFile(dir_stream.get(), dirsize)) {
            dir_stream.reset();
            file_stream.reset();
            return 1;
        }
    } else {
        if (ResNum * 3 > dirsize) {
            DirByte[1] = 0xff;
            dir_stream->seekp(dirsize);
            do {
                dir_stream->write(reinterpret_cast<char *>(&DirByte[1]), 1);
            } while (dir_stream->tellp() != ResNum * 3);
        }
    }
    file_stream->seekp(filesize);
    off = file_stream->tellp();
    DirByte[0] = PatchVol * 0x10 + off / 0x10000;
    DirByte[1] = (off % 0x10000) / 0x100;
    DirByte[2] = off % 0x100;
    ResourceInfo[ResType][ResNum].Exists = true;
    ResourceInfo[ResType][ResNum].Loc = off;
    if (isV3)
        sprintf(ResourceInfo[ResType][ResNum].Filename, "%svol.%d", ID.c_str(), PatchVol);
    else
        sprintf(ResourceInfo[ResType][ResNum].Filename, "vol.%d", PatchVol);
    int n = 0;
    ResHeader[n++] = 0x12;
    ResHeader[n++] = 0x34;
    ResHeader[n++] = PatchVol;
    ResHeader[n++] = ResourceData.Size % 256;
    ResHeader[n++] = ResourceData.Size / 256;
    if (isV3) {
        ResHeader[n++] = ResourceData.Size % 256;  // no compression so compressed size
        ResHeader[n++] = ResourceData.Size / 256;  // and uncompressed size are the same
    }
    file_stream->write(reinterpret_cast<char *>(ResHeader), n);
    file_stream->write(reinterpret_cast<char *>(ResourceData.Data), ResourceData.Size);

    if (isV3) {
        dir_stream->seekp(ResType * 2);
        dir_stream->write(reinterpret_cast<char *>(&lsbyte), 1);
        dir_stream->write(reinterpret_cast<char *>(&msbyte), 1);
        off = (msbyte << 8) | lsbyte;
        dir_stream->seekp(off + ResNum * 3);
        dir_stream->write(reinterpret_cast<char *>(DirByte), 3);
        dir_stream.reset();
    } else {
        dir_stream->seekp(ResNum * 3);
        dir_stream->write(reinterpret_cast<char *>(DirByte), 3);
        dir_stream.reset();
    }
    file_stream->flush();

    return 0;
}

//************************************************
int Game::DeleteResource(int ResType, int ResNum)
{
    std::unique_ptr<std::fstream> dir_stream;
    int dirsize;

    if ((dir_stream = OpenDirUpdate(&dirsize, ResType)) == nullptr)
        return 1;

    if (isV3) {
        uint8_t lsbyte, msbyte;

        if (dirsize < 8) {
            menu->errmes("Error reading '%s': file invalid!", dirname.c_str());
            return 1;
        }
        dir_stream->seekp(ResType * 2);
        dir_stream->read(reinterpret_cast<char *>(&lsbyte), 1);
        dir_stream->read(reinterpret_cast<char *>(&msbyte), 1);
        int off = (msbyte << 8) | lsbyte;
        if (dirsize < off + ResNum * 3 + 2) {
            menu->errmes("Error reading '%s': file invalid!", dirname.c_str());
            return 1;
        }
        dir_stream->seekp(off + ResNum * 3);
    } else {
        if (dirsize < ResNum * 3 + 2) {
            menu->errmes("Error reading '%s': file invalid!", dirname.c_str());
            return 1;
        }
        dir_stream->seekp(ResNum * 3);
    }
    const char b = '\xff';
    dir_stream->write(&b, 1);
    dir_stream->write(&b, 1);
    dir_stream->write(&b, 1);
    ResourceInfo[ResType][ResNum].Exists = false;

    return 0;
}

//************************************************
int Game::RebuildVOLfiles()
{
    int step = 0, steps = 0;
    int ResType, ResNum, VolFileNum;
    std::ofstream vol_stream, dir_stream;
    byte b = 0xff, byte1, byte2, byte3;
    byte ResHeader[7];
    long off;
#define MaxVOLFileSize  1023*1024
    TResourceInfo NewResourceInfo[4][256];
    int ResourceNum[4];
    bool cancel = false;
    int DirOffset[4];
    std::string volname = "vol";

    if (isV3)
        volname = ID + "vol";

    for (ResType = 0; ResType <= 3; ResType++) {
        ResourceNum[ResType] = 0;
        for (ResNum = 0; ResNum < 256; ResNum++) {
            if (ResourceInfo[ResType][ResNum].Exists) {
                steps++;  //number of steps for the progress bar (if necessary)
                ResourceNum[ResType] = ResNum;
            }
        }
    }

    QProgressDialog progress("Rebuilding VOL files...", "Cancel", 0, 100, nullptr);  //shows up if the operation is taking more than 3 sec
    //(so it never shows up...)

    ResHeader[0] = 0x12;
    ResHeader[1] = 0x34;

    VolFileNum = 0;
    auto vol_path = std::filesystem::path(dir) / (volname + "." + std::to_string(VolFileNum) + ".new");
    vol_stream = std::ofstream(vol_path, std::ios::binary | std::ios::trunc);
    if (!vol_stream.is_open()) {
        menu->errmes("Error creating file '%s'!", vol_path.string().c_str());
        progress.cancel();
        return 1;
    }

    if (isV3) {
        auto dir_path = std::filesystem::path(dir) / (ID + "dir.new");
        dir_stream = std::ofstream(dir_path, std::ios::binary | std::ios::trunc);
        if (!dir_stream.is_open()) {
            menu->errmes("Error creating file '%s'!", dir_path.string().c_str());
            progress.cancel();
            return 1;
        }
        for (ResType = 0; ResType <= 3; ResType++) {
            DirOffset[ResType] = 8 + ResType * 0x300;
            byte1 = DirOffset[ResType] % 0x100;
            byte2 = DirOffset[ResType] / 0x100;
            dir_stream.write(reinterpret_cast<char *>(&byte1), 1);
            dir_stream.write(reinterpret_cast<char *>(&byte2), 1);
        }
        for (ResType = 0; ResType <= 3; ResType++) {
            for (ResNum = 0; ResNum < 256; ResNum++) {
                dir_stream.write(reinterpret_cast<char *>(&b), 1);
                dir_stream.write(reinterpret_cast<char *>(&b), 1);
                dir_stream.write(reinterpret_cast<char *>(&b), 1);
            }
        }
        dir_stream.flush();
        dir_stream.seekp(0);
    }

    for (ResType = 0; ResType <= 3; ResType++) {
        if (!isV3) {
            auto dir_path = std::filesystem::path(dir) / (std::string(ResTypeAbbrv[ResType]) + "dir.new");
            dir_stream = std::ofstream(dir_path, std::ios::binary | std::ios::trunc);
            if (!dir_stream.is_open()) {
                menu->errmes("Error creating file '%s'!", dir_path.string().c_str());
                progress.cancel();
                return 1;
            }
            for (ResNum = 0; ResNum < ResourceNum[ResType]; ResNum++) {
                dir_stream.write(reinterpret_cast<char *>(&b), 1);
                dir_stream.write(reinterpret_cast<char *>(&b), 1);
                dir_stream.write(reinterpret_cast<char *>(&b), 1);
            }
            dir_stream.flush();
            dir_stream.seekp(0);
        }

        for (ResNum = 0; ResNum < 256; ResNum++) {
            if (!ResourceInfo[ResType][ResNum].Exists) {
                NewResourceInfo[ResType][ResNum].Exists = false;
                continue;
            }
            if (ReadResource(ResType, ResNum)) {
                menu->errmes("Error saving '%s.%03d'!", ResTypeAbbrv[ResType], ResNum);
                progress.cancel();
                return 1;
            }
            off = vol_stream.tellp();
            if (off + ResourceData.Size + 5 > MaxVOLFileSize) {
                vol_stream.close();
                VolFileNum++;
                auto vol_path = std::filesystem::path(dir) / (volname + "." + std::to_string(VolFileNum) + ".new");
                vol_stream = std::ofstream(vol_path, std::ios::binary | std::ios::trunc);
                if (!vol_stream.is_open()) {
                    menu->errmes("Error creating file '%s'!", vol_path.string().c_str());
                    progress.cancel();
                    return 1;
                }
                off = vol_stream.tellp();
            }
            NewResourceInfo[ResType][ResNum].Exists = true;
            NewResourceInfo[ResType][ResNum].Loc = off;
            sprintf(NewResourceInfo[ResType][ResNum].Filename, "%s.%d", volname.c_str(), VolFileNum);
            byte1 = VolFileNum * 0x10 + off / 0x10000;
            byte2 = (off % 0x10000) / 0x100;
            byte3 = off % 0x100;
            ResHeader[2] = VolFileNum;
            ResHeader[3] = ResourceData.Size % 256;
            ResHeader[4] = ResourceData.Size / 256;
            if (isV3) {
                ResHeader[5] = ResourceData.Size % 256;
                ResHeader[6] = ResourceData.Size / 256;
                vol_stream.write(reinterpret_cast<char *>(ResHeader), 7);
            } else
                vol_stream.write(reinterpret_cast<char *>(ResHeader), 5);

            vol_stream.write(reinterpret_cast<char *>(ResourceData.Data), ResourceData.Size);
            if (isV3)
                dir_stream.seekp(DirOffset[ResType] + ResNum * 3);
            else
                dir_stream.seekp(ResNum * 3);
            dir_stream.write(reinterpret_cast<char *>(&byte1), 1);
            dir_stream.write(reinterpret_cast<char *>(&byte2), 1);
            dir_stream.write(reinterpret_cast<char *>(&byte3), 1);
            progress.setValue(step++);
            if (progress.wasCanceled()) {
                cancel = true;
                break;
            }
        }
        if (!isV3)
            dir_stream.close();

        if (progress.wasCanceled()) {
            cancel = true;
            break;
        }
    }
    progress.setValue(steps);

    vol_stream.close();
    if (isV3)
        dir_stream.close();

    if (cancel)
        return 1;

    //cleanup temporary files
    if (isV3) {
        auto oldname = std::filesystem::path(dir) / (ID + "dir.new");
        auto newname = std::filesystem::path(dir) / (ID + "dir");
        std::filesystem::rename(oldname, newname);
    } else {
        for (const std::string &res_abbr : ResTypeAbbrv) {
            auto oldname = std::filesystem::path(dir) / (res_abbr + "dir.new");
            auto newname = std::filesystem::path(dir) / (res_abbr + "dir");
            std::filesystem::rename(oldname, newname);
        }
    }

    QDir d(dir.c_str());
    // Delete old VOLs...
    QStringList list = d.entryList(QStringList() << QString(volname.c_str()) + ".?" << QString(volname.c_str()) + ".??");
    for (const auto &oldvol : list)
        d.remove(oldvol);

    // ...and replace them with the new ones:
    list = d.entryList(QStringList() << QString(volname.c_str()) + ".*.new");
    for (const auto &newvol : list) {
        QString new_name = newvol;
        new_name.replace(".new", "");
        d.rename(newvol, new_name);
    }
    memcpy(ResourceInfo, NewResourceInfo, sizeof(ResourceInfo));
    QMessageBox::information(menu, "AGI studio", "Rebuilding is complete!");
    return 0;
}

//***********************************************
// v3 decompression code from Qt AGI Utilities

static void initLZW()
{
    if (prefix_code == NULL) {
        prefix_code = (unsigned int *)malloc(TABLE_SIZE * sizeof(unsigned int));
        append_character = (byte *)malloc(TABLE_SIZE * sizeof(byte));
    }
}

static void resetLZW()
{
    input_bit_count = 0;
    input_bit_buffer = 0L;
}


/***************************************************************************
** setBITS
**
** Purpose: To adjust the number of bits used to store codes to the value
** passed in.
***************************************************************************/
static int setBITS(int value)
{
    if (value == MAXBITS)
        return true;

    BITS = value;
    MAX_VALUE = (1 << BITS) - 1;
    MAX_CODE = MAX_VALUE - 1;
    return false;
}

/***************************************************************************
** decode_string
**
** Purpose: To return the string that the code taken from the input buffer
** represents. The string is returned as a stack, i.e. the characters are
** in reverse order.
***************************************************************************/
static byte *decode_string(byte *buffer, unsigned int code)
{
    int i = 0;
    while (code > 255) {
        *buffer++ = append_character[code];
        code = prefix_code[code];
        if (i++ >= 4000) {
            menu->errmes("Fatal error during code expansion!");
            return nullptr;
        }
    }
    *buffer = code;
    return (buffer);
}

/***************************************************************************
** input_code
**
** Purpose: To return the next code from the input buffer.
***************************************************************************/
static unsigned int input_code(byte **input)
{
    unsigned int return_value;

    while (input_bit_count <= 24) {
        input_bit_buffer |= (unsigned long) * (*input)++ << input_bit_count;
        input_bit_count += 8;
    }

    return_value = (input_bit_buffer & 0x7FFF) % (1 << BITS);
    input_bit_buffer >>= BITS;
    input_bit_count -= BITS;
    return (return_value);
}

/***************************************************************************
** expand
**
** Purpose: To uncompress the data contained in the input buffer and store
** the result in the output buffer. The fileLength parameter says how
** many bytes to uncompress. The compression itself is a form of LZW that
** adjusts the number of bits that it represents its codes in as it fills
** up the available codes. Two codes have special meaning:
**
**  code 256 = start over
**  code 257 = end of data
***************************************************************************/
static void expand(byte *input, byte *output, int fileLength)
{
    int next_code, new_code, old_code;
    int character, /* counter=0, index, */ BITSFull /*, i */;
    byte *string, *endAddr;

    BITSFull = setBITS(START_BITS);  /* Starts at 9-bits */
    next_code = 257;                 /* Next available code to define */

    endAddr = (byte *)((long)output + (long)fileLength);

    old_code = input_code(&input);    /* Read in the first code */
    character = old_code;
    new_code = input_code(&input);

    while ((output < endAddr) && (new_code != 0x101)) {

        if (new_code == 0x100) {      /* Code to "start over" */
            next_code = 258;
            BITSFull = setBITS(START_BITS);
            old_code = input_code(&input);
            character = old_code;
            *output++ = (char)character;
            new_code = input_code(&input);
        } else {
            if (new_code >= next_code) { /* Handles special LZW scenario */
                *decode_stack = character;
                string = decode_string(decode_stack + 1, old_code);
            } else
                string = decode_string(decode_stack, new_code);

            /* Reverse order of decoded string and store in output buffer. */
            character = *string;
            while (string >= decode_stack)
                *output++ = *string--;

            if (next_code > MAX_CODE)
                BITSFull = setBITS(BITS + 1);

            prefix_code[next_code] = old_code;
            append_character[next_code] = character;
            next_code++;
            old_code = new_code;

            new_code = input_code(&input);
        }
    }
}

//***********************************************
static void convertLOG(byte *logBuf, int logLen)
{
    int startPos, endPos, i, avisPos = 0, numMessages;

    /* Find the start and end of the message section */
    startPos = *logBuf + (*(logBuf + 1)) * 256 + 2;
    numMessages = logBuf[startPos];
    endPos = logBuf[startPos + 1] + logBuf[startPos + 2] * 256;
    logBuf += (startPos + 3);
    startPos = (numMessages * 2) + 0;

    /* Encrypt the message section so that it compiles with AGIv2 */
    for (i = startPos; i < endPos && i < logLen; i++)
        logBuf[i] ^= EncryptionKey[avisPos++ % 11];
}

//***********************************************
static void DecompressPicture(byte *picBuf, byte *outBuf, int picLen, int *outLen)
{
#define  NORMAL     0
#define  ALTERNATE  1

    byte data, oldData = 0, outData;
    int mode = NORMAL, bufPos = 0;
    byte *out = outBuf;

    while (bufPos < picLen) {

        data = picBuf[bufPos++];

        if (mode == ALTERNATE)
            outData = ((data & 0xF0) >> 4) + ((oldData & 0x0F) << 4);
        else
            outData = data;

        if ((outData == 0xF0) || (outData == 0xF2)) {
            *out++ = outData;
            if (mode == NORMAL) {
                data = picBuf[bufPos++];
                *out++ = (data & 0xF0) >> 4;
                mode = ALTERNATE;
            } else {
                *out++  = (data & 0x0F);
                mode = NORMAL;
            }
        } else
            *out++ = outData;

        oldData = data;
    }

    *outLen = int(out - outBuf);
}

//***********************************************
int Game::ReadV3Resource(char ResourceType1_c, int ResourceID1)
{
    byte msbyte, lsbyte;
    bool ResourceIsPicture;
    byte VolNumByte;
    int ResourceType1 = (int)ResourceType1_c;

    if (CompressedResource.Data == nullptr)
        CompressedResource.Data = (byte *)malloc(MaxResourceSize);

    auto resource_path = std::filesystem::path(dir) / ResourceInfo[ResourceType1][ResourceID1].Filename;
    auto resource_stream = std::ifstream(resource_path, std::ios::binary);
    if (!resource_stream.is_open()) {
        menu->errmes("Error reading file '%s'.", resource_path.string().c_str());
        return 1;
    }

    int size = std::filesystem::file_size(resource_path);
    if (ResourceInfo[ResourceType1][ResourceID1].Loc > size) {
        menu->errmes("Error reading '%s': Specified resource location is past end of file.", ResourceInfo[ResourceType1][ResourceID1].Filename);
        return 1;
    }
    resource_stream.seekg(ResourceInfo[ResourceType1][ResourceID1].Loc);
    resource_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
    resource_stream.read(reinterpret_cast<char *>(&msbyte), 1);
    if (!(lsbyte == 0x12 && msbyte == 0x34)) {
        menu->errmes("Error reading '%s': Resource signature not found.", ResourceInfo[ResourceType1][ResourceID1].Filename);
        return 1;
    }

    resource_stream.read(reinterpret_cast<char *>(&VolNumByte), 1);
    ResourceIsPicture = ((VolNumByte & 0x80) == 0x80);
    resource_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
    resource_stream.read(reinterpret_cast<char *>(&msbyte), 1);
    ResourceData.Size = msbyte * 256 + lsbyte;
    resource_stream.read(reinterpret_cast<char *>(&lsbyte), 1);
    resource_stream.read(reinterpret_cast<char *>(&msbyte), 1);
    CompressedResource.Size = msbyte * 256 + lsbyte;
    resource_stream.read(reinterpret_cast<char *>(CompressedResource.Data), CompressedResource.Size);

    if (ResourceIsPicture)
        DecompressPicture(CompressedResource.Data, ResourceData.Data, CompressedResource.Size, &ResourceData.Size);
    else if (CompressedResource.Size != ResourceData.Size) {
        initLZW();
        resetLZW();
        expand(CompressedResource.Data, ResourceData.Data, ResourceData.Size);
        if (ResourceType1 == LOGIC)
            convertLOG(ResourceData.Data, ResourceData.Size);
    } else {
        ResourceData.Size = CompressedResource.Size;
        memcpy(ResourceData.Data, CompressedResource.Data, ResourceData.Size);
    }

    return 0;
}

//*********************************************************
void Game::reset_settings(void)
{
#ifdef _WIN32
    QString template_dir = QDir::cleanPath(QApplication::applicationDirPath() + "/template/");
    QString help_dir = QDir::cleanPath(QApplication::applicationDirPath() + "/help/");
#elif __APPLE__
    QString template_dir = QDir::cleanPath(QApplication::applicationDirPath() + "/../Resources/template/");
    QString help_dir = QDir::cleanPath(QApplication::applicationDirPath() + "/../Resources/help/");
#else
    QString template_dir = "/usr/share/agistudio/template";
    QString help_dir = "/usr/share/agistudio/help";
#endif

    settings->setValue("DefaultResourceType", VIEW);                // Default resource type in resources window at startup.
    settings->setValue("PictureEditorStyle", P_ONE);                // PicEdit Window style.
    settings->setValue("ExtractLogicAsText", true);                 // Default for 'extract' function.

    settings->setValue("LogicEditor/ShowAllMessages", true);        // Logic decompiler - show all messages at end, or just unused ones.
    settings->setValue("LogicEditor/ShowElsesAsGotos", false);      //
    settings->setValue("LogicEditor/ShowSpecialSyntax", true);      // v30=4 vs. assignn(v30,4).

    settings->setValue("UseRelativeSrcDir", true);                  // Will determine whether RelativeSrcDir or AbsoluteSrcDir is used:
    settings->setValue("RelativeSrcDir", "src");                    // - Path relative to the game dir used to store logic source.
    settings->setValue("AbsoluteSrcDir", "");                       // - Absolute path used to store logic source.

    settings->setValue("TemplateDir", template_dir);                // Template game directory
    settings->setValue("HelpDir", help_dir);                        // Help directory

    settings->setValue("Interpreter", 0);                           // Interpreter name.
    settings->setValue("InterpreterPath", "");                      // Interpreter executable.
    settings->setValue("InterpreterArgs", "");                      // Interpreter command-line arguments.
}

//************************************************
int Game::RecompileAll()
{
    int i, ResNum, err;
    Logic logic;
    extern QStringList InputLines;

    for (i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == TEXTRES) {
            if (winlist[i].w.t->filename != "") {
                winlist[i].w.t->save();
                winlist[i].w.t->statusBar()->showMessage("");
            }
        } else if (winlist[i].type == LOGIC) {
            if (winlist[i].w.l->filename != "") {
                winlist[i].w.l->save_logic();
                winlist[i].w.l->statusBar()->showMessage("");
            }
        }
    }

    int step = 0, steps = 0;
    for (ResNum = 0; ResNum < 256; ResNum++) {
        if (game->ResourceInfo[LOGIC][ResNum].Exists)
            steps++;
    }

    QProgressDialog progress("Recompiling all logics...", "Cancel", 0, 100, nullptr);
    progress.setMinimumDuration(0);

    for (ResNum = 0; ResNum < 256; ResNum++) {
        if (game->ResourceInfo[LOGIC][ResNum].Exists) {

            // Look for a source file first
            std::ifstream logic_stream;
            for (const auto &filename_fmt : {
                        "logic.%03d", "logic.%d", "logic%d.txt"
                    }) {
                auto logic_path = std::filesystem::path(game->srcdir) / QString::asprintf(filename_fmt, ResNum).toStdString();
                if (std::filesystem::exists(logic_path)) {
                    logic_stream = std::ifstream(logic_path);
                    break;
                }
            }

            if (logic_stream.is_open()) {
                InputLines.clear();
                std::string newline;
                while (std::getline(logic_stream, newline))
                    InputLines.append(newline.c_str());
                logic_stream.close();
            } else { //source file not found - reading from the game
                err = logic.decode(ResNum);
                if (err) {
                    menu->errmes("Errors in logic.%03d:\n%s", ResNum, logic.ErrorList.c_str());
                    continue;
                }
                InputLines.clear();
                std::string::size_type pos;
                std::string str = logic.OutputText;
                while ((pos = str.find_first_of("\n")) != std::string::npos) {
                    InputLines.append(str.substr(0, pos).c_str());
                    str = str.substr(pos + 1);
                }
                if (str != "")
                    InputLines.append(str.c_str());
            }
            err = logic.compile();
            if (!err)
                game->AddResource(LOGIC, ResNum);
            else {
                if (!logic.ErrorList.empty())
                    menu->errmes("Errors in logic.%03d:\n%s", ResNum, logic.ErrorList.c_str());
            }
            progress.setValue(step++);
            if (progress.wasCanceled())
                return 1;
        }
    }

    progress.setValue(steps);
    QMessageBox::information(menu, "AGI studio", "Recompilation is complete!");

    return 0;
}
