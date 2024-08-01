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


#include <map>
#include <string>
#include <vector>

#include <QStringList>


class WordList
{
public:
    WordList();

    void clear();
    int  read(const std::string &filename);
    int  save(const std::string &filename);
    int  add_group(int num);
    void delete_group(int num);
    int  add_word(const std::string &word, int SelectedGroup);
    int  delete_word(const std::string &word, int SelectedGroup);
    int  change_number(int oldnum, int newnum);

    size_t GetNumWordGroups() const;
    size_t GetTotalWordCount() const;
    int  GetWordGroupNumberByIndex(size_t) const;
    int  GroupNumOfWord(const std::string &word) const;
    bool GroupExists(int GroupNum) const;
    const std::vector<uint16_t> GetWordGroupNumbers() const;
    const std::vector<std::string> GetGroupWords(int) const;

    enum ReplaceMode {
        Ask,
        Always,
        Never
    };
    void merge(const WordList &w);
    bool OKToReplaceWord(const std::string &TheWord, int OldGroupNum, int NewGroupNum, ReplaceMode &WhatToDoWithExistingWords);

protected:
    std::map <uint16_t, std::vector<std::string>> WordGroup;
};


#endif
