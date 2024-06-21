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

#ifndef WORDS_H
#define WORDS_H


#include <string>

#include <QStringList>


typedef struct {
    QStringList Words;
    int GroupNum;
} TWordGroup;

#define MaxGroupNum 65535
#define MaxWordGroups 10000

class WordList
{
public:
    WordList();
    TWordGroup WordGroup[MaxWordGroups];
    int NumGroups;
    char WhatToDoWithExistingWords;
    void clear();
    int read(const std::string &filename);
    int save(const std::string &filename);
    int add_group(int num);
    int delete_group(int num);
    int delete_word(const std::string &word, int SelectedGroup);
    int change_number(int oldnum, int newnum);
    int GetWordGroupIndex(int GroupNum) const;
    int GetNew_GroupIndex(int GroupNum) const;
    void merge(const WordList &w);
    int GroupIndexOfWord(const std::string &word) const;
    bool OKToReplaceWord(const std::string &TheWord, int OldGroupNum, int NewGroupNum);
    bool InsertWordGroup(int GroupNum);
    bool GroupExists(int GroupNum) const;
};


#define AskUser       0
#define AlwaysReplace 1
#define NeverReplace  2

#endif
