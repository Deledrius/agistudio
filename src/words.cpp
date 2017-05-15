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

#include "game.h"
#include "words.h"
#include "menu.h"
#include "wordsedit.h"

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>


static TWordGroup NewWordGroup[MaxWordGroups];
static int ResPos;
static bool EndOfFileReached;

//***************************************************
WordList::WordList() {}


//***************************************************
static byte ReadByte(void)
{
    byte ret;
    if (ResPos < ResourceData.Size) {
        ret = ResourceData.Data[ResPos];
        ResPos++;
    } else {
        ret = 0;
        EndOfFileReached = true;
    }
    return ret;
}

//***************************************************
static int ReadMSLSWord(void)
{
    byte MSbyte, LSbyte;

    MSbyte = ReadByte();
    LSbyte = ReadByte();
    return (MSbyte * 256 + LSbyte);

}

//***************************************************
int WordList::GetNew_GroupIndex(int GroupNum)
{
    int CurIndex;

    if (NumGroups > 0) {
        for (CurIndex = 0; CurIndex < NumGroups; CurIndex++) {
            if (NewWordGroup[CurIndex].GroupNum == GroupNum)
                return CurIndex;
        }
    }
    return -1;
}

//************************************************
int WordList::GetWordGroupIndex(int GroupNum)
{
    int CurIndex;

    if (NumGroups > 0) {
        for (CurIndex = 0; CurIndex < NumGroups; CurIndex++) {
            if (WordGroup[CurIndex].GroupNum == GroupNum)
                return CurIndex;
        }
    }
    return -1;
}

//************************************************
static std::string GetWord(std::string s)
//returns the part of a string before the last ' '
{
    int pos = s.find_last_of(' ');
    return s.substr(0, pos);

}

static int GetGroupNum(std::string s)
//returns the number after the last ' ' in a string
{
    int pos = s.find_last_of(' ');
    return atoi(s.substr(pos + 1).c_str());

}

//************************************************
int WordList::read(char *filename)
{
    std::string CurWord, PrevWord;
    byte CurByte, CharsFromPrevWord;
    int CurIndex, CurAddGroup, GroupIndex, GroupNum;
    int LowestRemainingGroup, SecondLowestRemainingGroup, SecondLowestRemainingGroupIndex = 0;
    int GroupAddOrder[MaxWordGroups];


    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        menu->errmes("Error opening file %s", filename);
        return 1;
    }

    struct stat buf;
    fstat(fileno(fptr), &buf);
    int size = buf.st_size;
    if (size > MaxResourceSize) {
        menu->errmes("Error:  File %s is too big (>%d bytes)", filename, MaxResourceSize);
        return 1;
    }

    EndOfFileReached = false;
    ResourceData.Size = size;
    fread(ResourceData.Data, size, 1, fptr);
    fclose(fptr);
    NumGroups = 0;
    ResPos = 0;
    ResPos = ReadMSLSWord(); //start of words section
    PrevWord = "";
    do {
        CurWord = "";
        CharsFromPrevWord = ReadByte();
        if (CharsFromPrevWord > PrevWord.length())
            CharsFromPrevWord = PrevWord.length();
        if (CharsFromPrevWord > 0)
            CurWord = PrevWord.substr(0, CharsFromPrevWord);
        do {
            CurByte = ReadByte();
            if (CurByte < 0x80)
                CurWord += (CurByte ^ 0x7f);
        } while (CurByte < 0x80 && !EndOfFileReached);
        //we must check for end of file, otherwise if the file is invalid, the
        //program may read indefinitely.
        if (EndOfFileReached) {
            menu->errmes("Error! Invalid WORDS.TOK file.");
            return 1;
        }
        CurWord += (0x7F ^ (CurByte - 0x80));
        GroupNum = ReadMSLSWord();
        if (CurWord != PrevWord) { //this word different to previous, so add it
            //in this way, no duplicates are added
            GroupIndex = GetNew_GroupIndex(GroupNum);
            if (GroupIndex >= 0)    //group exists
                NewWordGroup[GroupIndex].Words.add(CurWord);
            else {
                if (NumGroups >= MaxWordGroups) {
                    menu->errmes("Error ! Too many groups !");
                    return 1;
                }
                NumGroups++;
                NewWordGroup[NumGroups - 1].GroupNum = GroupNum;
                NewWordGroup[NumGroups - 1].Words = TStringList();
                NewWordGroup[NumGroups - 1].Words.add(CurWord);
            }//group doesn't exist - create new one
            PrevWord = CurWord;
        }
        CurByte = ReadByte();
        if (CurByte == 0 && ResPos >= ResourceData.Size - 1)
            EndOfFileReached = true;
        else
            ResPos--;
    } while (!EndOfFileReached);

    LowestRemainingGroup = -1;
    for (CurAddGroup = 0; CurAddGroup < NumGroups; CurAddGroup++) {
        SecondLowestRemainingGroup = 65536;
        for (CurIndex = 0; CurIndex < NumGroups; CurIndex++) {
            if (NewWordGroup[CurIndex].GroupNum < SecondLowestRemainingGroup &&
                    NewWordGroup[CurIndex].GroupNum > LowestRemainingGroup) {
                SecondLowestRemainingGroup = NewWordGroup[CurIndex].GroupNum;
                SecondLowestRemainingGroupIndex = CurIndex;
            }
        }
        GroupAddOrder[CurAddGroup] = SecondLowestRemainingGroupIndex;
        LowestRemainingGroup = SecondLowestRemainingGroup;
    }
    for (CurIndex = 0; CurIndex < NumGroups; CurIndex++) {
        WordGroup[CurIndex].GroupNum = NewWordGroup[GroupAddOrder[CurIndex]].GroupNum;
        WordGroup[CurIndex].Words.lfree();
        WordGroup[CurIndex].Words.copy(NewWordGroup[GroupAddOrder[CurIndex]].Words);
        NewWordGroup[GroupAddOrder[CurIndex]].Words.lfree();
    }
    return 0;
}

//**************************************************
void WordList::clear()
{
    for (int i = 0; i < NumGroups; i++)
        WordGroup[i].Words.lfree();
    NumGroups = 3;

    WordGroup[0].GroupNum = 0;
    WordGroup[0].Words = TStringList();
    WordGroup[0].Words.add("a");
    WordGroup[1].GroupNum = 1;
    WordGroup[1].Words = TStringList();
    WordGroup[1].Words.add("anyword");
    WordGroup[2].GroupNum = 9999;
    WordGroup[2].Words = TStringList();
    WordGroup[2].Words.add("rol");
}

//**************************************************
int WordList::save(char *filename)
{
    FILE *fptr;
    int CurGroupIndex, NumEmptyWordGroups;
    int CurWord, CurChar;
    byte CurByte;
    int LetterLoc[28];
    std::string ThisWord, PrevWord;
    int CurFirstLetter;
    char FirstLetter;
    int ThisGroupNum;
    size_t CharsFromPrevWord;
    std::string s;

    for (CurGroupIndex = 0, NumEmptyWordGroups = 0; CurGroupIndex < NumGroups; CurGroupIndex++) {
        if (WordGroup[CurGroupIndex].Words.num == 0)
            NumEmptyWordGroups++;
    }
    if (NumEmptyWordGroups > 0) {
        sprintf(tmp, "Warning: There are %d empty word groups.\nThese will not be saved.", NumEmptyWordGroups);
        menu->warnmes(tmp);
    }

    if ((fptr = fopen(filename, "wb")) == NULL) {
        menu->errmes("Error opening file %s", filename);
        return 1;
    }

    TStringList AllWords = TStringList();
    for (CurGroupIndex = 0; CurGroupIndex < NumGroups; CurGroupIndex++) {
        if (WordGroup[CurGroupIndex].Words.num > 0) {
            for (CurWord = 0; CurWord < WordGroup[CurGroupIndex].Words.num; CurWord++) {
                s = WordGroup[CurGroupIndex].Words.at(CurWord) + " " + IntToStr(WordGroup[CurGroupIndex].GroupNum);
                AllWords.addsorted((char *)s.c_str());
            }
        }
    }

    CurByte = 0;
    fwrite(&CurByte, 1, 51, fptr);
    for (CurFirstLetter = 1; CurFirstLetter <= 26; CurFirstLetter++)
        LetterLoc[CurFirstLetter] = 0;
    FirstLetter = 'a';
    LetterLoc[1] = ftell(fptr);
    for (CurWord = 0; CurWord < AllWords.num; CurWord++) {
        ThisWord = GetWord(AllWords.at(CurWord));
        if (ThisWord[0] != FirstLetter && ThisWord[0] >= 97 && ThisWord[0] <= 122) {
            FirstLetter = ThisWord[0];
            LetterLoc[FirstLetter - 96] = ftell(fptr);
        }
        ThisGroupNum = GetGroupNum(AllWords.at(CurWord));
        //work out # chars from prev word
        CharsFromPrevWord = 0;
        CurChar = 0;
        bool FinishedComparison = false;
        do {
            if (CurChar < (int)ThisWord.length() && (int)PrevWord.length() > CurChar && PrevWord[CurChar] == ThisWord[CurChar])
                CharsFromPrevWord++;
            else
                FinishedComparison = true;
            CurChar++;
        } while (! FinishedComparison);
        if (CharsFromPrevWord >= ThisWord.length())
            CharsFromPrevWord = ThisWord.length() - 1;
        //write # chars from prev word
        fwrite(&CharsFromPrevWord, 1, 1, fptr);
        PrevWord = ThisWord;
        ThisWord = ThisWord.substr(CharsFromPrevWord, ThisWord.length() - CharsFromPrevWord);
        if (ThisWord.length() > 1) {
            for (CurChar = 0; CurChar < (int)ThisWord.length() - 1; CurChar++) {
                CurByte = 0x7f ^ ThisWord[CurChar];
                fwrite(&CurByte, 1, 1, fptr);
            }
        }
        CurByte = 0x80 + (0x7f ^ ThisWord[ThisWord.length() - 1]);
        fwrite(&CurByte, 1, 1, fptr);
        //write group number
        CurByte = ThisGroupNum / 256;
        fwrite(&CurByte, 1, 1, fptr);
        CurByte = ThisGroupNum % 256;
        fwrite(&CurByte, 1, 1, fptr);
    }
    CurByte = 0;
    fwrite(&CurByte, 1, 1, fptr);
    fseek(fptr, 0, SEEK_SET);
    for (CurFirstLetter = 1; CurFirstLetter <= 26; CurFirstLetter++) {
        CurByte = LetterLoc[CurFirstLetter] / 256;
        fwrite(&CurByte, 1, 1, fptr);
        CurByte = LetterLoc[CurFirstLetter] % 256;
        fwrite(&CurByte, 1, 1, fptr);
    }
    AllWords.lfree();
    fclose(fptr);
    return 0;
}

//**************************************************
int WordList::add_group(int num)
{
    int i;

    for (i = 0; i < NumGroups; i++) {
        if (num == WordGroup[i].GroupNum) {
            menu->errmes("Group %d already exists.", num);
            return -1;
        }
        if (WordGroup[i].GroupNum > num)
            break;
    }

    NumGroups++;
    for (int k = NumGroups; k > i; k--)
        WordGroup[k] = WordGroup[k - 1];
    WordGroup[i].Words = TStringList();
    WordGroup[i].GroupNum = num;
    return i;
}

//**************************************************
int WordList::delete_group(int num)
{
    for (int i = num; i < NumGroups - 1; i++)
        WordGroup[i] = WordGroup[i + 1];
    NumGroups--;
    return 0;
}

//**************************************************
int WordList::delete_word(char *word, int SelectedGroup)
{
    if (SelectedGroup < 0)
        return -1;

    for (int k = 0; k < WordGroup[SelectedGroup].Words.num; k++) {
        if (!strcmp(word, WordGroup[SelectedGroup].Words.at(k).c_str())) {
            WordGroup[SelectedGroup].Words.del(k);
            return k;
        }
    }
    return -1;
}

//**************************************************
int WordList::change_number(int oldnum, int newnum)
{
    int i;

    for (i = 0; i < NumGroups; i++) {
        if (newnum == WordGroup[i].GroupNum) {
            menu->errmes("Group %d already exists.", newnum);
            return -1;
        }
        if (WordGroup[i].GroupNum > newnum)
            break;
    }


    TStringList cur_g = TStringList();
    cur_g.copy(WordGroup[oldnum].Words);
    if (oldnum < i) {
        i--;
        for (int k = oldnum; k < i; k++)
            WordGroup[k] = WordGroup[k + 1];
    } else {
        for (int k = oldnum; k > i; k--)
            WordGroup[k] = WordGroup[k - 1];
    }
    WordGroup[i].Words = cur_g;
    WordGroup[i].GroupNum = newnum;
    return i;
}

//************************************************************
bool WordList::GroupExists(int GroupNum)
{
    int CurGroupIndex;
    if (NumGroups > 0) {
        for (CurGroupIndex = 0; CurGroupIndex < NumGroups; CurGroupIndex++) {
            if (WordGroup[CurGroupIndex].GroupNum == GroupNum)
                return true;
        }
    }
    return false;
}

//************************************************************
bool WordList::InsertWordGroup(int GroupNum)
{
    int CurGroupIndex, InsertPosition = 0;
    bool InsertPositionFound;

    if (NumGroups >= MaxWordGroups) {
        menu->errmes("Error! Too many groups (max %d).", MaxWordGroups);
        return false;
    }

    CurGroupIndex = 0;
    InsertPositionFound = false;
    do {
        if (WordGroup[CurGroupIndex].GroupNum > GroupNum) {
            InsertPosition = CurGroupIndex;
            InsertPositionFound = true;
        } else
            CurGroupIndex++;
    } while (!InsertPositionFound && (CurGroupIndex < NumGroups));
    if (CurGroupIndex > NumGroups - 1)
        InsertPosition = NumGroups;
    WordGroup[NumGroups].Words = TStringList();
    if (NumGroups > 0) {
        for (CurGroupIndex = NumGroups - 1; CurGroupIndex >= InsertPosition; CurGroupIndex--) {
            WordGroup[CurGroupIndex + 1].GroupNum = WordGroup[CurGroupIndex].GroupNum;
            WordGroup[CurGroupIndex + 1].Words.lfree();
            WordGroup[CurGroupIndex + 1].Words.copy(WordGroup[CurGroupIndex].Words);
        }
    }
    NumGroups++;
    WordGroup[InsertPosition].GroupNum = GroupNum;
    WordGroup[InsertPosition].Words.lfree();
    return true;
}

//************************************************************
int WordList::GroupIndexOfWord(std::string word)
//returns the group index of the group containing the specified word}
{
    for (int i = 0; i < NumGroups; i++) {
        for (int k = 0; k < WordGroup[i].Words.num; k++) {
            if (WordGroup[i].Words.at(k) == word)
                return i;
        }
    }

    return -1;
}

//************************************************************
void WordList::merge(const WordList &NewWordList)
{
    int CurIndex, CurWord, GroupIndex, NumWordsAdded, GroupIndexOfExistingWord;
    std::string ThisWord;

    WhatToDoWithExistingWords = AskUser;

    if (NewWordList.NumGroups == 0)
        return;
    NumWordsAdded = 0;
    for (CurIndex = 0; CurIndex < NewWordList.NumGroups; CurIndex++) {
        if (!GroupExists(NewWordList.WordGroup[CurIndex].GroupNum))
            InsertWordGroup(NewWordList.WordGroup[CurIndex].GroupNum);
        GroupIndex = GetWordGroupIndex(NewWordList.WordGroup[CurIndex].GroupNum);
        if (GroupIndex >= 0 && NewWordList.WordGroup[CurIndex].Words.num > 0) {
            for (CurWord = 0; CurWord < NewWordList.WordGroup[CurIndex].Words.num; CurWord++) {
                GroupIndexOfExistingWord = GroupIndexOfWord(NewWordList.WordGroup[CurIndex].Words.at(CurWord));
                if ((GroupIndexOfExistingWord < 0) || ((GroupIndexOfExistingWord >= 0) && (OKToReplaceWord(NewWordList.WordGroup[CurIndex].Words.at(CurWord), WordGroup[GroupIndexOfExistingWord].GroupNum, WordGroup[GroupIndex].GroupNum)))) {

                    delete_word((char *)NewWordList.WordGroup[CurIndex].Words.at(CurWord).c_str(), GroupIndexOfExistingWord);
                    WordGroup[GroupIndex].Words.addsorted(NewWordList.WordGroup[CurIndex].Words.at(CurWord));
                    NumWordsAdded++;
                }
            }
        }//{if (GroupIndex >= 0) and (NewWordList.WordGroup[CurIndex].Words.Count > 0)}
    }
}

//************************************************************
bool WordList::OKToReplaceWord(std::string TheWord, int OldGroupNum, int NewGroupNum)
{
    if (WhatToDoWithExistingWords == AlwaysReplace)
        return true;
    if (WhatToDoWithExistingWords == NeverReplace)
        return false;
    if (OldGroupNum == NewGroupNum)
        return true;
    ReplaceWord *ask = new ReplaceWord(TheWord, OldGroupNum, NewGroupNum);
    int AskResult = ask->exec();

    if (AskResult == mrYesToAll)
        WhatToDoWithExistingWords = AlwaysReplace;
    if (AskResult == mrNoToAll)
        WhatToDoWithExistingWords = NeverReplace;
    if (AskResult == mrYes || AskResult == mrYesToAll)
        return true;
    else
        return false;
}
//************************************************************
