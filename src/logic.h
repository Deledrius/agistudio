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

#ifndef LOGIC_H
#define LOGIC_H

#include "words.h"
#include "object.h"
#include "agicommands.h"
#include "game.h"

#define MaxBlockDepth  12
#define MaxLabels  255
#define MaxDefines  255
#define MaxMessages 256
#define MaxGotos 255

//Logic class used both for decode and compile
class Logic
{
public:
    Logic();
    WordList *wordlist;
    ObjList *objlist;
    std::string OutputText;    //result of the decoding
    std::string ErrorList;     //compilation error messages
    unsigned int maxcol;  //max number of columns in window - used in formatting 'print' strings
    int compile();
    int decode(int resnum);

private:
    void ShowError(int Line, std::string ErrorMsg);
    void DisplayMessages();
    void ReadMessages();
    int FindLabels_ReadIfs();
    void AddBlockEnds();
    int FindLabels();
    void AddArg(byte Arg, byte ArgType);
    void AddSpecialSyntaxCommand();
    void AddSpecialIFSyntaxCommand();
    void ReadIfs();

    std::string ReadString(std::string::size_type *pos, std::string &str);
    int RemoveComments(TStringList Lines);
    int AddIncludes();
    int ReadDefines();
    int ReadPredefinedMessages();
    int ReadLabels();
    void NextLine();
    void SkipSpaces();
    byte MessageNum(std::string TheMessage);
    byte AddMessage(std::string TheMessage);
    std::string ReplaceDefine(std::string InText);
    void ReadArgText();
    int ReadArgValue();
    int Val(std::string str);
    void ReadArgs(bool CommandIsIf, byte CmdNum);
    std::string ReadText();
    std::string ReadPlainText();
    std::string ReadExprText();
    void ReadCommandName();
    byte FindCommandNum(bool CommandIsIf, std::string CmdName);
    bool AddSpecialIFSyntax();
    bool AddSpecialSyntax();
    int LabelNum(std::string LabelName);
    bool LabelAtStartOfLine(std::string LabelName);
    void WriteMessageSection();
    int CompileCommands();
};

extern char tmp[];

#endif
