/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  Almost all of this code was adapted from the Windows AGI Studio
 *  developed by Peter Kelly.
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

#include "logic.h"
#include "game.h"
#include "logedit.h"
#include "menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

extern TStringList InputLines;  //temporary -
//input text from the editor window or file

static bool UseTypeChecking = true;
static int ResPos, LogicSize;
static TStringList EditLines, IncludeFilenames;
static std::string DefineNames[MaxDefines];
static std::string DefineValues[MaxDefines];
static int NumDefines;
static int  RealLineNum[65535], LineFile[65535];
static int DefineNameLength[MaxDefines];
static std::string Messages[MaxMessages];
static bool MessageExists[MaxMessages];

typedef struct {
    std::string Name;
    int Loc;
} TLogicLabel;
static TLogicLabel Labels[MaxLabels + 1];
static int NumLabels;

static bool ErrorOccured;
static int CurLine;
static std::string LowerCaseLine, ArgText, LowerCaseArgText;
static std::string::size_type LinePos, LineLength, ArgTextLength, ArgTextPos;
static bool FinishedReading;
static int CommandNameStartPos;
static std::string CommandName;
static int CommandNum;
static bool NOTOn;

char empty_tmp[] = {0};

extern const char EncryptionKey[];
static int EncryptionStart;

//*************************************************
static void WriteByte(byte b)
{
    if (ResPos < ResourceData.Size) {
        ResourceData.Data[ResPos++] = b;
        if (ResPos > LogicSize)
            LogicSize = ResPos;
    }
}

static void WriteByteAtLoc(byte b, int Loc)
{
    if (Loc < ResourceData.Size) {
        ResourceData.Data[Loc] = b;
        if (Loc > LogicSize)
            LogicSize = Loc;
    }

}

static void WriteLSMSWord(short word)
{
    WriteByte(word % 256);
    WriteByte(word / 256);
}

//*************************************************
void Logic::ShowError(int Line, std::string ErrorMsg)
{
    int LineNum = RealLineNum[Line];
    if (LineFile[Line] == 0 || Line > EditLines.num) {
        // error is in logic in editor window
        sprintf(tmp, "Line %d: %s\n", RealLineNum[Line], ErrorMsg.c_str());
    } else { //error in include file
        if (LineFile[Line] > IncludeFilenames.num)
            sprintf(tmp, "[unknown include file] Line ???: %s\n", ErrorMsg.c_str());
        else
            sprintf(tmp, "File %s Line %d: %s\n", IncludeFilenames.at(LineFile[Line] - 1).c_str(), LineNum, ErrorMsg.c_str());
    }

    ErrorList.append(tmp);
    ErrorOccured = true;
}

//***************************************************
std::string Logic::ReadString(std::string::size_type *pos, std::string &str)
//returns string without quotes, starting from pos1
//pos is set to the 1st char after string
{
    std::string::size_type pos1 = *pos;
    std::string::size_type pos2 = pos1;

    //  printf ("ReadString: str=%s pos=%d\n",str.c_str(),*pos);

    do {
        pos2 = str.find_first_of("\"", pos2 + 1);
        if (pos2 == std::string::npos) {
            ShowError(CurLine, "\" required at end of string.");
            printf("string: *%s*\n", str.c_str());
            return "";
        }
    } while (str[pos2 - 1] == '\\');

    *pos = pos2 + 1;
    if (pos2 == pos1 + 1)
        return "";

    return str.substr(pos1 + 1, pos2 - pos1 - 1);
}

//***************************************************
int Logic::RemoveComments(TStringList Lines)
{
    int CommentDepth = 0;
    for (CurLine = 0; CurLine < Lines.num; CurLine++) {
        std::string Line = Lines.at(CurLine);
        std::string NewLine;
        bool InQuotes = false;
        for (unsigned i = 0; i < Line.size(); ++i) {
            if (!InQuotes) {
                if (CommentDepth == 0 && Line[i] == '[')
                    break;
                if (i < Line.size() - 1) {
                    if (CommentDepth == 0 && Line.substr(i, 2) == "//")
                        break;
                    else if (Line.substr(i, 2) == "/*") {
                        ++CommentDepth;
                        ++i;
                        continue;
                    }
                } else if (CommentDepth > 0 && Line.substr(i, 2) == "*/") {
                    --CommentDepth;
                    ++i;
                    continue;
                }
            }
            if (CommentDepth == 0) {
                if (Line[i] == '\"' && (i == 0 || Line[i - 1] != '\\'))
                    InQuotes = !InQuotes;
                NewLine += Line[i];
            }
        }
        Lines.replace(CurLine, NewLine);
    }
    return 0;
}

//***************************************************
int Logic::AddIncludes()
{
    TStringList IncludeStrings, IncludeLines;
    int  CurInputLine, CurIncludeLine;
    std::string filename;
    int err = 0;
    std::string::size_type pos1, pos2;
    int CurLine;
    char *ptr;

    IncludeFilenames = TStringList();
    IncludeStrings = TStringList();
    EditLines = TStringList();
    IncludeLines = TStringList();
    CurLine = 0;
    for (CurInputLine = 0; CurInputLine < InputLines.num; CurInputLine++) {
        EditLines.add(InputLines.at(CurInputLine));
        CurLine = EditLines.num - 1;
        RealLineNum[CurLine] = CurInputLine;
        LineFile[CurLine] = 0;
#ifdef _WIN32
        if (_strnicmp(InputLines.at(CurInputLine).c_str(), "#include", 8)) {
#else
        if (strncasecmp(InputLines.at(CurInputLine).c_str(), "#include", 8)) {
#endif
            continue;
        }
        std::string str = InputLines.at(CurInputLine).substr(8);
        if (str.length() < 4) {
            ShowError(CurLine, "Missing include filename !");
            err = 1;
            continue;
        }
        if (str[0] != ' ') {
            ShowError(CurLine, "' ' expected after #include.");
            err = 1;
            continue;
        }
        pos1 = str.find_first_of("\"", 1);
        pos2 = str.find_first_of("\"", pos1 + 1);
        if (pos1 == std::string::npos || pos2 == std::string::npos) {
            ShowError(CurLine, "Include filenames need quote marks around them.");
            err = 1;
            continue;
        }
        filename = str.substr(pos1 + 1, pos2 - pos1 - 1);
        if (filename.find_first_of("/") != std::string::npos) {
            ShowError(CurLine, "Only files in the src directory can be included.");
            err = 1;
            continue;
        }
        sprintf(tmp, "%s/src/%s", game->dir.c_str(), filename.c_str());
        FILE *fptr = fopen(tmp, "rb");
        if (fptr == NULL) {
            sprintf(tmp, "Can't open include file: %s/src/%s", game->dir.c_str(), filename.c_str());
            ShowError(CurLine, tmp);
            err = 1;
            continue;
        }
        IncludeLines.lfree();

        while (fgets(tmp, MAX_TMP, fptr) != NULL) {
            if ((ptr = strchr(tmp, 0x0a)))
                * ptr = 0;
            if ((ptr = strchr(tmp, 0x0d)))
                * ptr = 0;
            IncludeLines.add(tmp);
        }
        fclose(fptr);
        if (IncludeLines.num == 0)
            continue;
        IncludeFilenames.add(filename);
        RemoveComments(IncludeLines);
        EditLines.replace(CurLine, empty_tmp);
        for (CurIncludeLine = 0; CurIncludeLine < IncludeLines.num; CurIncludeLine++) {
            EditLines.add(IncludeLines.at(CurIncludeLine));
            CurLine = EditLines.num - 1;
            RealLineNum[CurLine] = CurIncludeLine;
            LineFile[CurLine] = IncludeFilenames.num;
        }
    }

    IncludeLines.lfree();
    InputLines.lfree();
    return err;
}

//***************************************************
int Logic::ReadDefines()
{
    int err = 0, i;
    std::string::size_type pos1, pos2;
    std::string ThisDefineName, ThisDefineValue;
    int CurLine;

    NumDefines = 0;
    for (CurLine = 0; CurLine < EditLines.num; CurLine++) {
#ifdef _WIN32
        if (_strnicmp(EditLines.at(CurLine).c_str(), "#define", 7)) {
#else
        if (strncasecmp(EditLines.at(CurLine).c_str(), "#define", 7)) {
#endif
            continue;
        }
        std::string str = EditLines.at(CurLine).substr(7);
        toLower(&str);
        if (str.length() < 4) {
            ShowError(CurLine, "Missing define name !");
            err = 1;
            continue;
        }
        if (str[0] != ' ') {
            ShowError(CurLine, "' ' expected after #define.");
            err = 1;
            continue;
        }
        if (NumDefines >= MaxDefines) {
            ShowError(CurLine, "Too many defines (max " + IntToStr(MaxDefines) + ")");
            err = 1;
            continue;
        }
        pos1 = str.find_first_not_of(" ", 1);
        pos2 = str.find_first_of(" ", pos1);
        if (pos1 == std::string::npos || pos2 == std::string::npos) {
            ShowError(CurLine, "Missing define name !");
            err = 1;
            continue;
        }
        ThisDefineName = str.substr(pos1, pos2 - 1);
        if (ThisDefineName.find_first_not_of("qwertyuiopasdfghjklzxcvbnm1234567890._") != std::string::npos) {
            ShowError(CurLine, "Define name can contain only characters from [a-z],'.' and '_'.");
            err = 1;
            continue;
        }
        for (i = 0; i < NumDefines; i++) {
            if (ThisDefineName == DefineNames[i]) {
                ShowError(CurLine, ThisDefineName + " already defined !");
                err = 1;
                break;
            }
        }
        if (err)
            continue;

        for (i = 0; i <= NumAGICommands; i++) {
            if (ThisDefineName == AGICommand[i].Name) {
                ShowError(CurLine, "Define name can not be a command name.");
                err = 1;
                break;
            }
        }
        if (err)
            continue;

        for (i = 1; i <= NumTestCommands; i++) {
            if (ThisDefineName == TestCommand[i].Name) {
                ShowError(CurLine, "Define name can not be a command name.");
                err = 1;
                break;
            }
        }
        if (err)
            continue;

        if (ThisDefineName == "if" || ThisDefineName == "else" || ThisDefineName == "goto") {
            ShowError(CurLine, "Invalid define name (" + ThisDefineName + ")");
            err = 1;
            continue;
        }

        pos1 = str.find_first_not_of(" ", pos2 + 1);
        if (pos1 == std::string::npos) {
            ShowError(CurLine, "Missing define value !");
            err = 1;
            continue;
        }
        if (str[pos1] == '"') {
            ThisDefineValue = "\"" + ReadString(&pos1, str) + "\"";
            if (ErrorOccured)
                continue;
            if (str.find_first_not_of(" ", pos1) != std::string::npos) {
                ShowError(CurLine, "Nothing allowed on line after define value.");
                err = 1;
                continue;
            }
        } else {
            pos2 = str.find_first_of(" ", pos1 + 1);
            if (pos2 == std::string::npos)
                ThisDefineValue = str.substr(pos1);
            else {
                ThisDefineValue = str.substr(pos1, pos2 - pos1);
                if (str.find_first_not_of(" ", pos2) != std::string::npos) {
                    ShowError(CurLine, "Nothing allowed on line after define value.");
                    err = 1;
                    continue;
                }
            }
            if (ThisDefineValue.find_first_not_of("qwertyuiopasdfghjklzxcvbnm1234567890._") != std::string::npos) {
                ShowError(CurLine, "Non-string define value can contain only characters from [a-z],'.' and '_'.");
                err = 1;
                continue;
            }
        }

        DefineNames[NumDefines] = ThisDefineName;
        DefineValues[NumDefines] = ThisDefineValue;
        DefineNameLength[NumDefines] = ThisDefineName.length();
        NumDefines++;
        EditLines.replace(CurLine, empty_tmp);
    }

    return err;
}

//***************************************************
int Logic::ReadPredefinedMessages()
{
    int err = 0, i;
    std::string::size_type pos1;
    int MessageNum;

    for (i = 0; i < MaxMessages; i++) {
        Messages[i] = "";
        MessageExists[i] = false;
    }
    for (CurLine = 0; CurLine < EditLines.num; CurLine++) {
#ifdef _WIN32
        if (_strnicmp(EditLines.at(CurLine).c_str(), "#message", 8)) {
#else
        if (strncasecmp(EditLines.at(CurLine).c_str(), "#message", 8)) {
#endif
            continue;
        }
        std::string str = EditLines.at(CurLine).substr(8);
        if (str[0] != ' ') {
            ShowError(CurLine, "' ' expected after #message.");
            err = 1;
            continue;
        }
        MessageNum = atoi(str.c_str());
        if (MessageNum == 0) {
            ShowError(CurLine, "Invalid message number (must be 1-255).");
            err = 1;
            continue;
        }
        pos1 = str.find_first_of("\"");
        if (pos1 == std::string::npos) {
            ShowError(CurLine, "\" required at start of string.");
            err = 1;
            continue;
        }
        Messages[MessageNum] = ReadString(&pos1, str);
        if (ErrorOccured)
            continue;
        if (Messages[MessageNum].find_first_not_of(" ", pos1) != std::string::npos) {
            sprintf(tmp, "Nothing allowed on line after message. ");
            ShowError(CurLine, tmp);
            err = 1;
            continue;
        }
        MessageExists[MessageNum] = true;
        EditLines.replace(CurLine, empty_tmp);
    }

    return err;

}

//***************************************************
int Logic::ReadLabels()
{
    int err = 0, i;
    std::string::size_type pos1, pos2;
    std::string LabelName;
    int CurLine;

    NumLabels = 0;
    for (CurLine = 0; CurLine < EditLines.num; CurLine++) {
        std::string str = EditLines.at(CurLine);
        toLower(&str);
        pos1 = str.find_first_not_of(" ");
        if (pos1 == std::string::npos)
            continue;
        pos2 = str.find_first_not_of("qwertyuiopasdfghjklzxcvbnm1234567890._", pos1);
        if (pos2 == std::string::npos)
            continue;
        if ((pos1 == pos2) || (str[pos2] != ':'))
            continue;
        LabelName = str.substr(pos1, pos2 - pos1);
        for (i = 1; i <= NumLabels; i++) {
            if (LabelName == Labels[i].Name) {
                ShowError(CurLine, "Label " + LabelName + " already defined.");
                err = 1;
                break;
            }
        }
        if (err)
            continue;
        if (NumLabels > MaxLabels) {
            ShowError(CurLine, "Too many labels (max " + IntToStr(MaxLabels) + ")");
            err = 1;
            continue;
        }
        if (LabelName == "if" || LabelName == "else" || LabelName == "goto") {
            ShowError(CurLine, "Invalid label name (" + LabelName + ")");
            err = 1;
            continue;
        }
        for (i = 0; i < NumDefines; i++) {
            if ((LabelName == DefineNames[i]) || (LabelName + ":" == DefineNames[i])) {
                ShowError(CurLine, "Can't have a label with the same name a a define.");
                err = 1;
                break;
            }
        }
        if (err)
            continue;
        NumLabels++;
        Labels[NumLabels].Name = LabelName;
        Labels[NumLabels].Loc = 0;
    }

    return err;
}

//***************************************************
void Logic::NextLine()
{
    int NumLines = EditLines.num;

    CurLine++;
    if (CurLine > NumLines) {
        FinishedReading = true;
        return;
    }
    do {
        LowerCaseLine = EditLines.at(CurLine);
        if (LowerCaseLine == empty_tmp || (LinePos = LowerCaseLine.find_first_not_of(" ")) == std::string::npos) {
            CurLine++;
            continue;
        }
        //printf("Line %d: %s\n",CurLine,LowerCaseLine.c_str());
        toLower(&LowerCaseLine);
        LineLength = LowerCaseLine.length();
        return;
    } while (CurLine < NumLines);
    FinishedReading = true;
}

//***************************************************
void Logic::SkipSpaces()
{
    LinePos = LowerCaseLine.find_first_not_of(" ", LinePos);
    if (LinePos == std::string::npos)
        NextLine();
}

//***************************************************
byte Logic::MessageNum(std::string TheMessage)
{
    for (int i = 1; i < MaxMessages; i++) {
        if (MessageExists[i] && Messages[i] == TheMessage)
            return i;
    }
    return 0;
}

//***************************************************
byte Logic::AddMessage(std::string TheMessage)
{
    // Adds a message to the message list regardles of whether or not it
    // already exists. Returns the number of the added message, or 0 if
    // there are already 255 messages.

    for (int i = 1; i < MaxMessages; i++) {
        if (!MessageExists[i]) {
            MessageExists[i] = true;
            Messages[i] = TheMessage;
            return i;
        }
    }
    return 0;
}

//***************************************************
static std::string TrimEndWhitespaces(const std::string &str)
{
    int i = str.length();
    while (i > 0 && (str[i - 1] == ' ' || str[i - 1] == '\t'))
        --i;
    return str.substr(0, i);
}

//***************************************************
std::string Logic::ReplaceDefine(std::string InText)
{
    std::string str = InText;
    toLower(&str);
    for (int i = 0; i < NumDefines; i++) {
        if (str == DefineNames[i])
            return DefineValues[i];
    }
    return InText;
}

//***************************************************
void Logic::ReadArgText()
// do not use for string - does not take quotes into account
{
    std::string::size_type pos1, pos2;

    SkipSpaces();
    pos1 = pos2 = LinePos;

    if (LowerCaseLine[pos1] == '"') {
        ArgText = "\"" + ReadString(&pos2, LowerCaseLine) + "\"";
        pos2 = LowerCaseLine.find_first_of(",)", pos2);
    } else
        pos2 = LowerCaseLine.find_first_of(",)", pos1);
    if (pos2 == std::string::npos) {
        LinePos = LineLength;
        ArgText = ReplaceDefine(TrimEndWhitespaces(
                                    EditLines.at(CurLine).substr(pos1)));
    } else {
        LinePos = pos2;
        ArgText = ReplaceDefine(TrimEndWhitespaces(
                                    EditLines.at(CurLine).substr(pos1, pos2 - pos1)));
    }

    LowerCaseArgText = ArgText;
    toLower(&LowerCaseArgText);
    ArgTextLength = ArgText.length();
    ArgTextPos = 0;
}

//***************************************************
int Logic::ReadArgValue()
{
    char *ptr;
    std::string str2 = ArgText.substr(ArgTextPos);
    const char *str = str2.c_str();
    long val = strtol(str, &ptr, 10);
    ArgTextPos += (int)(ptr - str);
    if ((val == 0 && ptr == str) || val == LONG_MIN || val == LONG_MAX)
        return -1;
    return val;
}

//***************************************************
int Logic::Val(std::string str)
{
    char *ptr;
    const char *s = str.c_str();
    long val = strtol(s, &ptr, 10);
    if (val == LONG_MIN || val == LONG_MAX)
        return -1;
    if (val == 0) {
        if (ptr == s)
            return -1;
        else
            return val;
    }
    return val;
}

//***************************************************
void Logic::ReadArgs(bool CommandIsIf, byte CmdNum)
{
    char *ThisArgTypePrefix;
    bool FinishedReadingSaidArgs = false;
    int ArgValue, NumSaidArgs;
    std::string ThisWord;
#define MaxSaidArgs 40
    int SaidArgs[MaxSaidArgs];
    int CurArg;
    CommandStruct ThisCommand;
    std::string ThisMessage;
    int ThisMessageNum;
    std::string ThisInvObjectName;
    int ThisInvObjectNum;
    int i;

    SkipSpaces();
    if (LinePos >= LineLength || EditLines.at(CurLine)[LinePos] != '(') {
        ShowError(CurLine, "'(' expected.");
        return;
    }
    LinePos++;
    if (CmdNum == 14 && CommandIsIf) { //said test command
        NumSaidArgs = -1;
        FinishedReadingSaidArgs = false;
        do {
            ReadArgText();
            NumSaidArgs++;
            if (ArgText[0] == '"') {
                ArgValue = 0;
                ArgTextPos = 0;
                ThisWord = ReadString(&ArgTextPos, ArgText);
                if (ErrorOccured)
                    ShowError(CurLine, "\" required at end of word.");
                else {
                    //find word group number
                    bool found = false;
                    for (int k = 0; k < wordlist->NumGroups; k++) {
                        for (int i = 0; i < wordlist->WordGroup[k].Words.num; i++) {
                            if (wordlist->WordGroup[k].Words.at(i) == ThisWord) {
                                ArgValue = wordlist->WordGroup[k].GroupNum;
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                    if (!found) {
                        ShowError(CurLine, "Unknown word " + ThisWord + ".");
                        return;
                    }
                }
            } else
                ArgValue = ReadArgValue();
            SaidArgs[NumSaidArgs] = ArgValue;
            if (SaidArgs[NumSaidArgs] < 0 || SaidArgs[NumSaidArgs] > 65535) {
                ShowError(CurLine, "Invalid word number for argument " + IntToStr(NumSaidArgs) + " (must be 0-65535).");
                SaidArgs[NumSaidArgs] = 0;
            }
            if ((LinePos < LineLength) & (LowerCaseLine[LinePos] == ',')) {
                if (NumSaidArgs > MaxSaidArgs) {
                    ShowError(CurLine, "Too many arguments for said command.");
                    FinishedReadingSaidArgs = true;
                }
            } else if (LinePos < LineLength && LowerCaseLine[LinePos] == ')')
                FinishedReadingSaidArgs = true;
            else
                ShowError(CurLine, "',' or ')' expected after argument " + IntToStr(NumSaidArgs) + ".");
            LinePos++;
        } while (!FinishedReadingSaidArgs || ErrorOccured);
        WriteByte(NumSaidArgs + 1);
        for (int i = 0; i <= NumSaidArgs; i++) {
            WriteByte(SaidArgs[i] % 256);
            WriteByte(SaidArgs[i] / 256);
        }
    }//if said test command
    else { //other command
        if (CommandIsIf)
            ThisCommand = TestCommand[CmdNum];
        else
            ThisCommand = AGICommand[CmdNum];
        for (CurArg = 0; CurArg < ThisCommand.NumArgs; CurArg++) {
            SkipSpaces();
            ReadArgText();
            if (ThisCommand.argTypes[CurArg] == atMsg && ArgTextLength >= 1 && ArgText[0] == '"') {
                // argument is message and given as string
                ArgTextPos = 0;
                ThisMessage = "";
                //splitting the message into lines if it doesn't fit the screen
                do {
                    if (ThisMessage != "" && ThisMessage[ThisMessage.length() - 1] != ' ')
                        ThisMessage += " ";
                    ThisMessage += ReadString(&ArgTextPos, ArgText);
                    if (LinePos + 1 >= LineLength || LowerCaseLine.find_first_not_of(" ", LinePos + 1) == std::string::npos) {

                        NextLine();
                        SkipSpaces();
                        ReadArgText();
                    } else
                        break;
                } while (true);
                ThisMessageNum = MessageNum(ThisMessage);
                if (ThisMessageNum > 0)
                    WriteByte(ThisMessageNum);
                else {
                    ThisMessageNum = AddMessage(ThisMessage);
                    if (ThisMessageNum == 0)
                        ShowError(CurLine, "Too many messages (max 255).");
                    else
                        WriteByte(ThisMessageNum);
                }
            }//argument is message and given as string
            else if (ThisCommand.argTypes[CurArg] == atIObj && ArgTextLength >= 1 && ArgText[0] == '"') {
                // argument is inventory object and given as string
                ArgTextPos = 0;
                ThisInvObjectName = ReadString(&ArgTextPos, ArgText);
                if (ThisInvObjectName == "")
                    ShowError(CurLine, "Object name must be at least one character.");
                else {
                    for (i = 0; i < objlist->ItemNames.num; i++) {
                        if (objlist->ItemNames.at(i) == ThisInvObjectName) {
                            ThisInvObjectNum = i;
                            WriteByte(i);
                            break;
                        }
                    }
                    if (i >= objlist->ItemNames.num)
                        ShowError(CurLine, "Unknown inventory object " + ThisInvObjectName);
                }
            }// argument is inventory object and given as string
            else { //normal argument
                ThisArgTypePrefix = (char *)ArgTypePrefix[(int)ThisCommand.argTypes[CurArg]];
                if (UseTypeChecking && (strcmp(LowerCaseArgText.substr(0, strlen(ThisArgTypePrefix)).c_str(), ThisArgTypePrefix)))
                    ShowError(CurLine, "Invalid or unknown argument type for argument " + IntToStr(CurArg) + " (should be a " + ArgTypeName[(int)ThisCommand.argTypes[CurArg]] + ").");
                else {
                    if (UseTypeChecking)
                        ArgTextPos += strlen(ThisArgTypePrefix);
                    else
                        while (ArgTextPos < ArgTextLength && !(LowerCaseArgText[ArgTextPos] >= 'a' && LowerCaseArgText[ArgTextPos] <= 'z'))
                            ArgTextPos++;
                    ArgValue = ReadArgValue();
                    if (ArgValue < 0 || ArgValue > 255)
                        ShowError(CurLine, "Invalid or missing value for argument " + IntToStr(CurArg) + " (must be 0-255)");
                    else
                        WriteByte(ArgValue);
                }
            }//normal argument
            if (CurArg < ThisCommand.NumArgs - 1) {
                if (ArgTextPos < ArgTextLength)
                    ShowError(CurLine, "',' expected after argument " + IntToStr(CurArg) + ".");
                else if (LinePos >= LineLength || LowerCaseLine[LinePos] != ',')
                    ShowError(CurLine, "',' expected after argument " + IntToStr(CurArg) + ".");
                else
                    LinePos++;
            } else if (ArgTextPos < ArgTextLength) {
                ShowError(CurLine, "(1) ')' expected after argument " + IntToStr(CurArg) + ".");
                printf("Line %s argtextpos=%d arglen=%d\n", LowerCaseLine.c_str(), (int)ArgTextPos, (int)ArgTextLength);
            }
        }
        SkipSpaces();
        if (LinePos >= LineLength || LowerCaseLine[LinePos] != ')') {

            if (ThisCommand.NumArgs > 0) {
                ShowError(CurLine, "(2) ')' expected after argument " + IntToStr(ThisCommand.NumArgs) + ".");
                printf("Line %s argtextpos=%d arglen=%d\n", LowerCaseLine.c_str(), (int)ArgTextPos, (int)ArgTextLength);
            } else
                ShowError(CurLine, "')' expected.");
        } else
            LinePos++;
    }
}

//***************************************************
std::string Logic::ReadText()
{
    int p = LinePos;
    std::string::size_type pos = LowerCaseLine.find_first_of("( ,):", LinePos);
    if (pos == std::string::npos) {
        LinePos = LineLength;
        return LowerCaseLine.substr(p);
    } else {
        LinePos = pos;
        return LowerCaseLine.substr(p, pos - p);
    }
}

//***************************************************
std::string Logic::ReadPlainText()
{
    int p = LinePos;
    std::string::size_type pos = LowerCaseLine.find_first_not_of("qwertyuiopasdfghjklzxcvbnm1234567890._", LinePos);

    if (pos == std::string::npos) {
        LinePos = LineLength;
        return LowerCaseLine.substr(p);
    } else {
        LinePos = pos;
        return LowerCaseLine.substr(p, pos - p);
    }
}

//***************************************************
std::string Logic::ReadExprText()
{
    int p = LinePos;
    std::string::size_type pos = LowerCaseLine.find_first_not_of("=+-*/><!", LinePos);
    if (pos == std::string::npos) {
        LinePos = LineLength;
        return LowerCaseLine.substr(p);
    } else {
        LinePos = pos;
        return LowerCaseLine.substr(p, pos - p);
    }
}

//***************************************************
void Logic::ReadCommandName()
{
    SkipSpaces();
    CommandNameStartPos = LinePos;
    CommandName = ReadText();
}

//***************************************************
byte Logic::FindCommandNum(bool CommandIsIf, std::string CmdName)
{
    int i;
    const char *s;

    s = CmdName.c_str();

    if (CommandIsIf) {
        for (i = 1; i <= NumTestCommands; i++) {
            if (!strcmp(s, TestCommand[i].Name))
                return i;
        }
        return 255;
    } else {
        for (i = 0; i <= NumAGICommands; i++) {
            if (!strcmp(s, AGICommand[i].Name))
                return i;
        }
        return 255;
    }
}

//***************************************************
bool Logic::AddSpecialIFSyntax()
{
    int OldLinePos;
    int arg1, arg2;
    bool arg2isvar, AddNOT;
    std::string ArgText, expr;

    OldLinePos = LinePos;
    LinePos -= CommandName.length();
    ArgText = ReplaceDefine(ReadPlainText());

    if (ArgText[0] == 'v') {
        if (NOTOn)
            ShowError(CurLine, "'!' not allowed before var.");
        arg1 = Val(ArgText.substr(1));
        if (arg1 < 0 || arg1 > 255)
            ShowError(CurLine, "Invalid number given or error in expression syntax.");
        else {
            SkipSpaces();
            expr = ReadExprText();
            SkipSpaces();
            ArgText = ReplaceDefine(ReadPlainText());
            arg2isvar = (ArgText[0] == 'v');
            if (arg2isvar)
                arg2 = Val(ArgText.substr(1));
            else
                arg2 = Val(ArgText);
            if (arg2 < 0 || arg2 > 255)
                ShowError(CurLine, "Invalid number given or error in expression syntax.");
            else {
                CommandNum = 0;
                AddNOT = false;
                if (expr == "==")
                    CommandNum = 0x01;  //equal
                else if (expr == "<")
                    CommandNum = 0x03;  //less
                else if (expr == ">")
                    CommandNum = 0x05;  //greater
                else if (expr == "!=") {
                    CommandNum = 0x01;     //!equal
                    AddNOT = true;
                } else if (expr == ">=") {
                    CommandNum = 0x03;     //!less
                    AddNOT = true;
                } else if (expr == "<=") {
                    CommandNum = 0x05;     //!greater
                    AddNOT = true;
                } else
                    ShowError(CurLine, "Expression syntax error");
                if (CommandNum > 0) {
                    if (arg2isvar)
                        CommandNum++;
                    if (AddNOT)
                        WriteByte(0xFD);
                    WriteByte(CommandNum);
                    WriteByte(arg1);
                    WriteByte(arg2);
                    return true;
                }
            }
        }
    }//if(ArgText[0]=='v')
    else if (ArgText[0] == 'f') {
        arg1 = Val(ArgText.substr(1));
        if (arg1 < 0 || arg1 > 255)
            ShowError(CurLine, "Invalid number given or error in expression syntax..");
        else {
            WriteByte(0x07);  // isset
            WriteByte(arg1);
            return true;
        }
    }//if(ArgText[0]=='f')
    else
        LinePos = OldLinePos;
    return false;
}

//***************************************************
bool Logic::AddSpecialSyntax()
{
    int arg1, arg2, arg3;
    bool arg2isvar = false, arg3isvar = false, arg2isstar = false;
    std::string ArgText = "", expr, expr2;
    int OldLinePos;

    OldLinePos = LinePos;
    LinePos -= CommandName.length();
    if (CommandName[0] == '*') {
        LinePos++;
        ArgText = "*" + ReplaceDefine(ReadPlainText());
    } else
        ArgText = ReplaceDefine(ReadPlainText());

    if (ArgText[0] == 'v') {

        arg1 = Val(ArgText.substr(1));
        if (arg1 < 0 || arg1 > 255)
            ShowError(CurLine, "Invalid number given or error in expression syntax.");
        else {
            SkipSpaces();
            expr = ReadExprText();
            if (expr == "++") {
                WriteByte(0x01); // increment
                WriteByte(arg1);
                return true;
            } else if (expr == "--") {
                WriteByte(0x02); // decrement
                WriteByte(arg1);
                return true;
            } else {
                if (expr[0] == '*') {
                    expr = expr.substr(1);
                    LinePos++;
                }
                SkipSpaces();
                arg2isstar = false;
                ArgText = ReadPlainText();
                if (ReadPlainText() == "" && LowerCaseLine[LinePos - ArgText.length()] == '*') {
                    LinePos++;
                    ArgText = "*" + ReplaceDefine(ReadPlainText());
                } else
                    ArgText = ReplaceDefine(ArgText);

                if (ArgText[0] == 'v' && !arg2isstar)
                    arg2isvar = true;
                else if (ArgText.substr(0, 2) == "*v" && !arg2isstar)
                    arg2isstar = true;

                if (arg2isvar)
                    arg2 = Val(ArgText.substr(1));
                else if (arg2isstar)
                    arg2 = Val(ArgText.substr(2));
                else
                    arg2 = Val(ArgText);

                if (arg2 < 0 || arg2 > 255)
                    ShowError(CurLine, "Invalid number given or error in expression syntax.");
                else {
                    if (expr == "+=" && !arg2isstar) {
                        if (arg2isvar)
                            WriteByte(0x06);  //addv
                        else
                            WriteByte(0x05);          //addn
                        WriteByte(arg1);
                        WriteByte(arg2);
                        return true;
                    } else if (expr == "-=" && !arg2isstar) {
                        if (arg2isvar)
                            WriteByte(0x08);  //subv
                        else
                            WriteByte(0x07);          //subn
                        WriteByte(arg1);
                        WriteByte(arg2);
                        return true;
                    } else if (expr == "*=" && !arg2isstar) {
                        if (arg2isvar)
                            WriteByte(0xa6);  //mul.v
                        else
                            WriteByte(0xa5);          //mul.n
                        WriteByte(arg1);
                        WriteByte(arg2);
                        return true;
                    } else if (expr == "/=" && !arg2isstar) {
                        if (arg2isvar)
                            WriteByte(0xa8);  //div.v
                        else
                            WriteByte(0xa7);          //div.n
                        WriteByte(arg1);
                        WriteByte(arg2);
                        return true;
                    } else if (expr == "=") {
                        if (LinePos < LineLength && EditLines.at(CurLine)[LinePos] == ';') {
                            //must be assignn, assignv or rindirect
                            if (arg2isvar)
                                WriteByte(0x04); // assignv
                            else if (arg2isstar)
                                WriteByte(0x0A); // rindirect
                            else
                                WriteByte(0x03); // assignv
                            WriteByte(arg1);
                            WriteByte(arg2);
                            return true;
                        } else if (arg2 != arg1)
                            ShowError(CurLine, "Expression syntax error");
                        else {
                            SkipSpaces();
                            expr2 = ReadExprText();
                            SkipSpaces();
                            ArgText = ReplaceDefine(ReadPlainText());
                            arg3isvar = (ArgText[0] == 'v');
                            if (arg3isvar)
                                arg3 = Val(ArgText.substr(1));
                            else
                                arg3 = Val(ArgText);
                            if (arg3 < 0 || arg3 > 255)
                                ShowError(CurLine, "Invalid number given or error in expression syntax.");
                            else {
                                if (expr2 == "+") {
                                    if (arg3isvar)
                                        WriteByte(0x06);  //addv
                                    else
                                        WriteByte(0x05); //addn
                                    WriteByte(arg1);
                                    WriteByte(arg3);
                                    return true;
                                } else if (expr2 == "-") {
                                    if (arg3isvar)
                                        WriteByte(0x08);  //subv
                                    else
                                        WriteByte(0x07); //subn
                                    WriteByte(arg1);
                                    WriteByte(arg3);
                                    return true;
                                } else if (expr2 == "*") {
                                    if (arg3isvar)
                                        WriteByte(0xa6);  //mul.v
                                    else
                                        WriteByte(0xa5); //mul.n
                                    WriteByte(arg1);
                                    WriteByte(arg3);
                                    return true;
                                } else if (expr2 == "/") {
                                    if (arg3isvar)
                                        WriteByte(0xa8);  //div.v
                                    else
                                        WriteByte(0xa7); //div.n
                                    WriteByte(arg1);
                                    WriteByte(arg3);
                                    return true;
                                } else
                                    ShowError(CurLine, "Expression syntax error");
                            }
                        }
                    }//if(expr == "=")
                    else
                        ShowError(CurLine, "Expression syntax error");
                }
            }//if (expr != "--" && expr != "++")
        }//if(arg1<0 || arg1>255)
    }//if(ArgText[0]=='v')
    else if (ArgText.substr(0, 2) == "*v") {
        LinePos -= (CommandName.length() - 1);
        ArgText = ReplaceDefine(ReadPlainText());
        arg1 = Val(ArgText.substr(1));
        if (arg1 < 0 || arg1 > 255)
            ShowError(CurLine, "Invalid number given or error in expression syntax.");
        else {
            SkipSpaces();
            expr = ReadExprText();
            if (expr != "=")
                ShowError(CurLine, "Expression syntax error");
            else {
                SkipSpaces();
                ArgText = ReplaceDefine(ReadPlainText());
                arg2isvar = (ArgText[0] == 'v');
                if (arg2isvar)
                    arg2 = Val(ArgText.substr(1));
                else
                    arg2 = Val(ArgText);
                if (arg2 < 0 || arg2 > 255)
                    ShowError(CurLine, "Invalid number given or error in expression syntax.");
                else {
                    if (arg2isvar)
                        WriteByte(0x09); //lindirectv
                    else
                        WriteByte(0x0b); //lindirectn
                    WriteByte(arg1);
                    WriteByte(arg2);
                    return true;
                }
            }
        }
    }//if(ArgText.substr(0,2)=="*v")
    else
        LinePos = OldLinePos;
    return false;
}

//***************************************************
int Logic::LabelNum(std::string LabelName)
{
    for (int i = 1; i <= NumLabels; i++) {
        if (Labels[i].Name == LabelName)
            return i;
    }
    return 0;
}

//***************************************************
bool Logic::LabelAtStartOfLine(std::string LabelName)
{
    std::string::size_type pos = LinePos - LabelName.length() - 1;
    if (LowerCaseLine.find_first_not_of(" ") < pos)
        return false;
    return true;
}

//***************************************************
void WriteEncByte(byte TheByte)
{
    WriteByte(TheByte ^ EncryptionKey[(ResPos - EncryptionStart) % 11]) ;
}

void Logic::WriteMessageSection()
{
    int MessageSectionStart, MessageSectionEnd;
    int MessageLoc[MaxMessages];
    int CurMessage, NumMessages, ThisMessageLength;

    MessageSectionStart = ResPos;
    NumMessages = 0;
    for (CurMessage = 255; CurMessage >= 0; CurMessage--) {
        if (MessageExists[CurMessage])
            break;
    }
    NumMessages = CurMessage;
    WriteByte(NumMessages);
    ResPos = MessageSectionStart + 3 + NumMessages * 2;
    EncryptionStart = ResPos;
    for (CurMessage = 1; CurMessage <= NumMessages; CurMessage++) {
        if (!MessageExists[CurMessage]) {
            MessageLoc[CurMessage] = 0;
            continue;
        }
        ThisMessageLength = Messages[CurMessage].length();
        MessageLoc[CurMessage] = ResPos - MessageSectionStart - 1;
        for (int i = 0; i < ThisMessageLength; i++) {
            if (Messages[CurMessage][i] == '\\' && i < ThisMessageLength - 1) {
                if (Messages[CurMessage][i + 1] == 'n') {
                    WriteEncByte(0x0a);
                    i++;
                } else if (Messages[CurMessage][i + 1] == '"') {
                    WriteEncByte(0x22);
                    i++;
                } else if (Messages[CurMessage][i + 1] == '\\') {
                    WriteEncByte(0x5c);
                    i++;
                } else
                    WriteEncByte(0x5c);
            } else
                WriteEncByte(Messages[CurMessage][i]);
        }
        WriteEncByte(0x00);
    }
    MessageSectionEnd = ResPos - MessageSectionStart - 1;
    ResPos = MessageSectionStart + 1;
    WriteLSMSWord(MessageSectionEnd);
    for (CurMessage = 1; CurMessage <= NumMessages; CurMessage++)
        WriteLSMSWord(MessageLoc[CurMessage]);
    ResPos = 0;
    WriteLSMSWord(MessageSectionStart - 2); // in the file, the message section start is relative to start of actual code
}

//***************************************************
int Logic::CompileCommands()
{
    int err = 0;
    short BlockDepth;
    short BlockStartDataLoc[MaxBlockDepth + 1];
    short BlockLength[MaxBlockDepth + 1];
    bool BlockIsIf[MaxBlockDepth + 1];
    bool InIf, LastCommandWasReturn, InIfBrackets = false, AwaitingNextTestCommand = false, EncounteredLabel;
    int NumCommandsInIfStatement = 0, NumCommandsInIfBrackets = 0;

    typedef struct {
        byte LabelNum;
        int DataLoc;
    } TLogicGoto;
    TLogicGoto Gotos[MaxGotos + 1];
    short NumGotos, GotoData, CurGoto;

    memset(BlockIsIf, 0, sizeof(BlockIsIf));
    InIf = false;
    BlockDepth = 0;
    NumGotos = 0;
    FinishedReading = false;
    CurLine = -1;
    NextLine();
    if (FinishedReading) {
        ShowError(CurLine, "Nothing to compile !");
        return 1;
    }
    do {
        LastCommandWasReturn = false;
        if (!InIf) {
            if (LinePos < LineLength && LowerCaseLine[LinePos] == '}') {
                LinePos++;
                if (BlockDepth == 0)
                    ShowError(CurLine, "'}' not at end of any command blocks.");
                else {
//          if (ResPos == BlockStartDataLoc[BlockDepth] + 2)
//                ShowError(CurLine,"Command block must contain at least one command.");
                    BlockLength[BlockDepth] = ResPos - BlockStartDataLoc[BlockDepth] - 2;
                    WriteByteAtLoc(BlockLength[BlockDepth] & 0xff, BlockStartDataLoc[BlockDepth]);
                    WriteByteAtLoc((BlockLength[BlockDepth] >> 8) & 0xff, BlockStartDataLoc[BlockDepth] + 1);
                    BlockDepth--;
                    SkipSpaces();
                    if (LinePos >= LineLength && CurLine < EditLines.num - 1)
                        NextLine();
                    if (LowerCaseLine.substr(LinePos, 4) == "else") {
                        LinePos += 4;
                        SkipSpaces();
                        if (! BlockIsIf[BlockDepth + 1])
                            ShowError(CurLine, "'else' not allowed after command blocks that start with 'else'.");

                        else if (LinePos >= LineLength || LowerCaseLine[LinePos] != '{')
                            ShowError(CurLine, "'{' expected after else.");

                        else {
                            LinePos++;
                            BlockDepth++;
                            BlockLength[BlockDepth] += 3;
                            WriteByteAtLoc(BlockLength[BlockDepth] & 0xff, BlockStartDataLoc[BlockDepth]);
                            WriteByteAtLoc((BlockLength[BlockDepth] >> 8) & 0xff, BlockStartDataLoc[BlockDepth] + 1);
                            BlockIsIf[BlockDepth] = true;
                            WriteByte(0xfe);
                            BlockStartDataLoc[BlockDepth] = ResPos;
                            WriteByte(0x00);  // block length filled in later.
                            WriteByte(0x00);
                        }
                    }//if(LowerCaseLine.substr(LinePos,4) == "else"
                }//if BlockDepth > 0
            }//if LowerCaseLine[LinePos] == '}'
            else {
                ReadCommandName();
                if (CommandName == "if") {
                    WriteByte(0xFF);
                    InIf = true;
                    SkipSpaces();
                    if (LinePos >= LineLength || EditLines.at(CurLine)[LinePos] != '(')
                        ShowError(CurLine, "'(' expected at start of if statement.");

                    LinePos++;
                    InIfBrackets = false;
                    NumCommandsInIfStatement = 0;
                    AwaitingNextTestCommand = true;
                } else if (CommandName == "else")
                    ShowError(CurLine, "'}' required before 'else'.");

                else if (CommandName == "goto") {
                    if (LinePos >= LineLength || LowerCaseLine[LinePos] != '(')
                        ShowError(CurLine, "'(' expected.");
                    else {
                        LinePos++;
                        ReadCommandName();
                        CommandName = ReplaceDefine(CommandName);
                        if (LabelNum(CommandName) == 0)
                            ShowError(CurLine, "Unknown label " + CommandName + ".");
                        else if (NumGotos >= MaxGotos)
                            ShowError(CurLine, "Too many labels (max " + IntToStr(MaxLabels) + ").");
                        else {
                            NumGotos++;
                            Gotos[NumGotos].LabelNum = LabelNum(CommandName);
                            WriteByte(0xFE);
                            Gotos[NumGotos].DataLoc = ResPos;
                            WriteByte(0x00);
                            WriteByte(0x00);
                            if (LinePos >= LineLength || LowerCaseLine[LinePos] != ')')
                                ShowError(CurLine, "')' expected after label name.");

                            LinePos++;
                            if (LinePos >= LineLength || LowerCaseLine[LinePos] != ';')
                                ShowError(CurLine, "';' expected after goto command.");
                            LinePos++;
                        }
                    }
                } else {
                    CommandNum = FindCommandNum(false, CommandName);
                    EncounteredLabel = (LabelNum(CommandName) > 0);
                    if (EncounteredLabel && LinePos < LineLength && LowerCaseLine[LinePos] == ':')
                        LinePos++;
                    else
                        EncounteredLabel = false;
                    EncounteredLabel = (EncounteredLabel && LabelAtStartOfLine(CommandName));
                    if (EncounteredLabel)
                        Labels[LabelNum(CommandName)].Loc = ResPos;
                    else {
                        if (CommandNum == 255) { // not found
                            if (!AddSpecialSyntax())
                                ShowError(CurLine, "Unknown action command " + EditLines.at(CurLine).substr(CommandNameStartPos, CommandName.length()) + ".");
                        } else {
                            WriteByte(CommandNum);
                            ReadArgs(false, CommandNum);
                            if (CommandNum == 0)
                                LastCommandWasReturn = true;
                        }
                        if (LinePos >= LineLength || EditLines.at(CurLine)[LinePos] != ';')
                            ShowError(CurLine, "';' expected after command.");

                        LinePos++;
                    }//if we found a label
                }//command
            }//if LowerCaseLine[LinePos] != '}'
        }//(!InIf)
        if (InIf) {
            LastCommandWasReturn = false;
            if (AwaitingNextTestCommand) {
                if (LowerCaseLine[LinePos] == '(') {
                    if (InIfBrackets)
                        ShowError(CurLine, "Brackets too deep in if statement.");
                    InIfBrackets = true;
                    WriteByte(0xFC);
                    NumCommandsInIfBrackets = 0;
                    LinePos++;
                }// if LowerCaseLine[LinePos] = '('
                else if (LowerCaseLine[LinePos] == ')') {
                    if (NumCommandsInIfStatement == 0)
                        ShowError(CurLine, "If statement must contain at least one command.");
                    else if (InIfBrackets && (NumCommandsInIfBrackets == 0))
                        ShowError(CurLine, "Brackets must contain at least one command.");
                    else
                        ShowError(CurLine, "Expected statement but found closing bracket.");
                    LinePos++;
                } else {
                    NOTOn = false;
                    if (LowerCaseLine[LinePos] == '!') {
                        NOTOn = true;
                        LinePos++;
                    }
                    SkipSpaces();
                    ReadCommandName();
                    CommandNum = FindCommandNum(true, CommandName);
                    if (NOTOn)
                        WriteByte(0xFD);
                    if (CommandNum == 255) { // not found
                        if (!AddSpecialIFSyntax())
                            ShowError(CurLine, "Unknown test command " + EditLines.at(CurLine).substr(CommandNameStartPos, CommandName.length()) + ".");
                    } else {
                        WriteByte(CommandNum);
                        ReadArgs(true, CommandNum);
                    }
                    NumCommandsInIfStatement++;
                    if (InIfBrackets)
                        NumCommandsInIfBrackets++;
                    AwaitingNextTestCommand = false;
                }
            }  // if AwaitingNextTestCommand
            else if (LinePos < LineLength) {
                if (LowerCaseLine[LinePos] == ')') {
                    LinePos++;
                    if (InIfBrackets) {
                        if (NumCommandsInIfBrackets == 0)
                            ShowError(CurLine, "Brackets must contain at least one command.");
                        else
                            InIfBrackets = false;
                        WriteByte(0xFC);
                    } else {
                        if (NumCommandsInIfStatement == 0)
                            ShowError(CurLine, "If statement must contain at least one command.");
                        else {
                            SkipSpaces();
                            if (LinePos > LineLength || EditLines.at(CurLine)[LinePos] != '{')
                                ShowError(CurLine, "'{' expected after if statement.");
                            LinePos++;
                            WriteByte(0xFF);
                            if (BlockDepth > MaxBlockDepth)
                                ShowError(CurLine, "Too many nested blocks (max " + IntToStr(MaxBlockDepth) + ").");
                            else {
                                BlockDepth++;
                                BlockStartDataLoc[BlockDepth] = ResPos;
                                BlockIsIf[BlockDepth] = true;
                                WriteByte(0x00);   // block length filled in later.
                                WriteByte(0x00);
                            }
                            InIf = false;
                        }
                    }
                } // else if LowerCaseLine[LinePos] == ')'
                else if (LowerCaseLine[LinePos] == '!') {
                    ShowError(CurLine, "'!' can only be placed directly in front of a command.");
                    LinePos++;
                } else if (LowerCaseLine.substr(LinePos, 2) == "&&") {
                    if (InIfBrackets)
                        ShowError(CurLine, "'&&' not allowed within brackets.");
                    AwaitingNextTestCommand = true;
                    LinePos += 2;
                } else if (LowerCaseLine.substr(LinePos, 2) == "||") {
                    if (!InIfBrackets)
                        ShowError(CurLine, "Commands to be ORred together must be placed within brackets.");
                    AwaitingNextTestCommand = true;
                    LinePos += 2;
                } else {
                    if (InIfBrackets)
                        ShowError(CurLine, "Expected '||' or end of if statement.");
                    else
                        ShowError(CurLine, "Expected '&&' or end of if statement.");
                }
            }// if (not AwaitingNextTestCommand) and (LinePos < LineLength)
        }//if InIf
        SkipSpaces();
        if (ErrorOccured)
            FinishedReading = true;
        else if (LinePos >= LineLength)
            NextLine();
    } while (!FinishedReading);
    if (!LastCommandWasReturn)
        ShowError(CurLine, "return command expected.");
    if (InIf) {
        if (AwaitingNextTestCommand) {
            if (NumCommandsInIfStatement == 0)
                ShowError(CurLine, "Expected test command.");
            else
                ShowError(CurLine, "Expected another test command or end of if statement.");
        } else {
            if (InIfBrackets)
                ShowError(CurLine, "Expected '||' or end of if statement.");
            else
                ShowError(CurLine, "Expected '&&' or end of if statement.");
        }
    } else if (BlockDepth > 0)
        ShowError(CurLine, "'}' expected.");
    for (CurGoto = 1; CurGoto <= NumGotos; CurGoto++) {
        GotoData = Labels[Gotos[CurGoto].LabelNum].Loc - Gotos[CurGoto].DataLoc - 2;
        WriteByteAtLoc((GotoData & 0xff), Gotos[CurGoto].DataLoc);
        WriteByteAtLoc((GotoData >> 8) & 0xff, Gotos[CurGoto].DataLoc + 1);
    }

    return err;
}

//***************************************************
int Logic::compile()
{
    int ret, i, j;

    sprintf(tmp, "%s/words.tok", game->dir.c_str());
    ret = wordlist->read(tmp);
    if (ret)
        return 1;

    sprintf(tmp, "%s/object", game->dir.c_str());
    ret = objlist->read(tmp, false);
    if (ret)
        return 1;

    objlist->ItemNames.toLower();
    // words already in lower case in file so we don't need to convert them
    for (i = 0; i < objlist->ItemNames.num; i++) {
        if (objlist->ItemNames.at(i).find_first_of("\"") == std::string::npos)
            continue;
        //replace " with \"
        char *ptr = (char *)objlist->ItemNames.at(i).c_str();
        for (j = 0; *ptr; ptr++) {
            if (*ptr == '"') {
                tmp[j++] = '\\';
                tmp[j++] = '"';
            } else
                tmp[j++] = *ptr;
        }
        tmp[j] = 0;
        objlist->ItemNames.replace(i, tmp);
    }

    ResourceData.Size = MaxResourceSize;
    LogicSize = 0;
    ResPos = 2;
    ErrorOccured = false;
    NumDefines = 0;
    ErrorList = "";

    if (RemoveComments(InputLines))
        return 1;
    if (AddIncludes())
        return 1;
    if (ReadDefines())
        return 1;
    if (ReadPredefinedMessages())
        return 1;
    if (ReadLabels())
        return 1;
    if (CompileCommands())
        return 1;

    WriteMessageSection();

    EditLines.lfree();

    if (ErrorOccured)
        return 1;
    //   printf("\n************* SUCCESS !!! ***********\n");
    ResourceData.Size = LogicSize;
    return 0;
}
