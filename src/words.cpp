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


#include <filesystem>
#include <fstream>
#include <ranges>

#include <QMessageBox>

#include "game.h"
#include "words.h"
#include "menu.h"


#define MaxGroupNum 65535
#define MaxWordGroups 10000

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
size_t WordList::GetNumWordGroups() const
{
    return WordGroup.size();
}

//***************************************************
const std::vector<uint16_t> WordList::GetWordGroupNumbers() const
{
    auto kv = std::views::keys(WordGroup);
    const std::vector<uint16_t> keys{ kv.begin(), kv.end() };
    return keys;
}

//***************************************************
int WordList::GetWordGroupNumberByIndex(size_t n) const
{
    if (n < 0 || n >= WordGroup.size())
        return -1;

    return (*std::next(WordGroup.begin(), n)).first;
}

//***************************************************
const std::vector<std::string> WordList::GetGroupWords(int groupnum) const
{
    if (WordGroup.contains(groupnum)) {
        const auto words = WordGroup.at(groupnum);
        return words;
    }

    return std::vector<std::string>();
}

//************************************************
int WordList::read(const std::string &filename)
{
    std::string CurWord, PrevWord;
    byte CurByte;
    size_t CharsFromPrevWord;
    int GroupNum;

    std::ifstream word_stream(filename, std::ios::binary);
    if (!word_stream.is_open()) {
        menu->errmes("Error opening file '%s'.", filename.c_str());
        return 1;
    }

    int size = std::filesystem::file_size(filename);
    if (size > MaxResourceSize) {
        menu->errmes("Error:  File '%s' is too big (>%d bytes).", filename.c_str(), MaxResourceSize);
        return 1;
    }

    EndOfFileReached = false;
    ResourceData.Size = size;
    word_stream.read(reinterpret_cast<char *>(ResourceData.Data), size);
    word_stream.close();

    WordGroup.clear();          // Empty the existing WordGroups before we begin

    ResPos = 0;
    ResPos = ReadMSLSWord();    // Start of words section
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
        // We must check for end of file, otherwise if the file is invalid, the
        // program may read indefinitely.
        if (EndOfFileReached) {
            menu->errmes("Error! Invalid WORDS.TOK file.");
            return 1;
        }
        CurWord += (0x7F ^ (CurByte - 0x80));
        GroupNum = ReadMSLSWord();
        if (CurWord != PrevWord) {  //this word different to previous, so add it
            //in this way, no duplicates are added
            if (WordGroup.contains(GroupNum)) {
                // Group exists
                WordGroup[GroupNum].emplace_back(CurWord);
            } else {
                // Group doesn't exist - create new one
                if (WordGroup.size() >= MaxWordGroups) {
                    menu->errmes("Error: Too many groups!");
                    return 1;
                }
                WordGroup[GroupNum] = { };
                WordGroup[GroupNum].emplace_back(CurWord);
            }
            PrevWord = CurWord;
        }
        CurByte = ReadByte();
        if (CurByte == 0 && ResPos >= ResourceData.Size - 1)
            EndOfFileReached = true;
        else
            ResPos--;
    } while (!EndOfFileReached);

    return 0;
}

//**************************************************
void WordList::clear()
{
    for (auto &group : WordGroup)
        group.second.clear();

    WordGroup = {
        {0,     {"a"}},
        {1,     {"anyword"}},
        {9999,  {"rol"}}
    };
}

//**************************************************
int WordList::save(const std::string &filename)
{
    int NumEmptyWordGroups = 0;
    int CurWord, CurChar;
    byte CurByte;
    int LetterLoc[28];
    std::string ThisWord, PrevWord;
    int CurFirstLetter;
    char FirstLetter;
    int ThisGroupNum;
    size_t CharsFromPrevWord;

    for (const auto &group : WordGroup) {
        if (group.second.size() == 0)
            NumEmptyWordGroups++;
    }
    if (NumEmptyWordGroups > 0)
        menu->warnmes("Warning: There are %d empty word groups.\nThese will not be saved.", NumEmptyWordGroups);

    auto word_stream = std::ofstream(filename, std::ios::binary);
    if (!word_stream.is_open()) {
        menu->errmes("Error opening file '%s'.", filename.c_str());
        return 1;
    }

    QStringList AllWords = QStringList();
    for (const auto &group : WordGroup) {
        if (group.second.size() > 0) {
            for (const auto &word : group.second) {
                auto s = word + " " + std::to_string(group.first);
                AllWords.append(s.c_str());
                AllWords.sort();
            }
        }
    }

    CurByte = 0;
    word_stream.write(reinterpret_cast<char *>(&CurByte), 51);
    for (CurFirstLetter = 1; CurFirstLetter <= 26; CurFirstLetter++)
        LetterLoc[CurFirstLetter] = 0;
    FirstLetter = 'a';
    LetterLoc[1] = word_stream.tellp();
    for (CurWord = 0; CurWord < AllWords.count(); CurWord++) {
        ThisWord = AllWords.at(CurWord).split(" ")[0].toStdString();
        if (ThisWord[0] != FirstLetter && ThisWord[0] >= 97 && ThisWord[0] <= 122) {
            FirstLetter = ThisWord[0];
            LetterLoc[FirstLetter - 96] = word_stream.tellp();
        }
        ThisGroupNum = AllWords.at(CurWord).split(" ")[1].toInt();
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
        word_stream.write(reinterpret_cast<char *>(&CharsFromPrevWord), 1);
        PrevWord = ThisWord;
        ThisWord = ThisWord.substr(CharsFromPrevWord, ThisWord.length() - CharsFromPrevWord);
        if (ThisWord.length() > 1) {
            for (CurChar = 0; CurChar < (int)ThisWord.length() - 1; CurChar++) {
                CurByte = 0x7f ^ ThisWord[CurChar];
                word_stream.write(reinterpret_cast<char *>(&CurByte), 1);
            }
        }
        CurByte = 0x80 + (0x7f ^ ThisWord[ThisWord.length() - 1]);
        word_stream.write(reinterpret_cast<char *>(&CurByte), 1);
        //write group number
        CurByte = ThisGroupNum / 256;
        word_stream.write(reinterpret_cast<char *>(&CurByte), 1);
        CurByte = ThisGroupNum % 256;
        word_stream.write(reinterpret_cast<char *>(&CurByte), 1);
    }
    CurByte = 0;
    word_stream.write(reinterpret_cast<char *>(&CurByte), 1);
    word_stream.seekp(0);
    for (CurFirstLetter = 1; CurFirstLetter <= 26; CurFirstLetter++) {
        CurByte = LetterLoc[CurFirstLetter] / 256;
        word_stream.write(reinterpret_cast<char *>(&CurByte), 1);
        CurByte = LetterLoc[CurFirstLetter] % 256;
        word_stream.write(reinterpret_cast<char *>(&CurByte), 1);
    }
    AllWords.clear();
    word_stream.close();
    return 0;
}

//**************************************************
int WordList::add_group(int num)
{
    if (WordGroup.contains(num)) {
        menu->errmes("Group %d already exists.", num);
        return -1;
    }

    if (WordGroup.size() >= MaxWordGroups) {
        menu->errmes("Error: Too many groups (max %d).", MaxWordGroups);
        return -1;
    }

    if (num >= MaxGroupNum) {
        menu->errmes("Error: Exceeds largest allowed group number (max %d).", MaxGroupNum);
        return -1;
    }

    WordGroup[num] = { };

    return std::distance(WordGroup.begin(), WordGroup.find(num));
}

//**************************************************
void WordList::delete_group(int num)
{
    WordGroup.erase(num);
}

//**************************************************
int WordList::delete_word(const std::string &word, int SelectedGroup)
{
    if (!WordGroup.contains(SelectedGroup))
        return -1;

    // Check that the word exists in the wordgroup, and remove it
    const auto &wpos = std::find(WordGroup[SelectedGroup].begin(), WordGroup[SelectedGroup].end(), word);
    if (wpos != WordGroup[SelectedGroup].end()) {
        int pos = std::distance(WordGroup[SelectedGroup].begin(), wpos);
        WordGroup[SelectedGroup].erase(wpos);
        return pos;
    }

    return -1;
}

//**************************************************
int WordList::add_word(const std::string &word, int SelectedGroup)
{
    if (!WordGroup.contains(SelectedGroup))
        return -1;

    // Only add a word if it isn't already in the wordgroup
    auto wpos = std::find(WordGroup[SelectedGroup].begin(), WordGroup[SelectedGroup].end(), word);
    if (wpos == WordGroup[SelectedGroup].end()) {
        WordGroup[SelectedGroup].emplace_back(word);
        std::sort(WordGroup[SelectedGroup].begin(), WordGroup[SelectedGroup].end());
        wpos = std::find(WordGroup[SelectedGroup].begin(), WordGroup[SelectedGroup].end(), word);
    }

    // Return the position of the inserted word, or the existing word (if found)
    return std::distance(WordGroup[SelectedGroup].begin(), wpos);
}

//**************************************************
int WordList::change_number(int oldnum, int newnum)
{
    if (WordGroup.contains(newnum)) {
        menu->errmes("Group %d already exists.", newnum);
        return -1;
    }

    auto wordlist = WordGroup.extract(oldnum);
    wordlist.key() = newnum;
    WordGroup.insert(std::move(wordlist));

    return newnum;
}

//************************************************************
bool WordList::GroupExists(int GroupNum) const
{
    return WordGroup.contains(GroupNum);
}

//************************************************************
size_t WordList::GetTotalWordCount() const
{
    size_t n = 0;
    for (const auto &group : WordGroup)
        n += group.second.size();
    return n;
}

//************************************************************
int WordList::GroupNumOfWord(const std::string &word) const
// Returns the group number of the group containing the specified word
{
    for (const auto &group : WordGroup) {
        for (const auto &word_entry : group.second) {
            if (word_entry == word)
                return group.first;
        }
    }

    return -1;
}

//************************************************************
void WordList::merge(const WordList &NewWordList)
{
    int NumWordsAdded = 0, GroupNumOfExistingWord = -1;
    std::string ThisWord;

    ReplaceMode WhatToDoWithExistingWords = ReplaceMode::Ask;

    if (NewWordList.GetNumWordGroups() == 0)
        return;

    for (const auto &newgroup : NewWordList.WordGroup) {
        auto newgroup_num = newgroup.first;
        auto &newgroup_words = newgroup.second;

        if (!GroupExists(newgroup_num))
            add_group(newgroup_num);

        for (const auto &newword : newgroup_words) {
            GroupNumOfExistingWord = GroupNumOfWord(newword);
            bool ok_to_replace = OKToReplaceWord(newword, GroupNumOfExistingWord, newgroup_num, WhatToDoWithExistingWords);
            if ((GroupNumOfExistingWord < 0) || ((GroupNumOfExistingWord >= 0) && (ok_to_replace))) {
                delete_word(newword, GroupNumOfExistingWord);
                add_word(newword, newgroup_num);
                NumWordsAdded++;
            }
        }
    }
}

//************************************************************
bool WordList::OKToReplaceWord(const std::string &TheWord, int OldGroupNum, int NewGroupNum, ReplaceMode &WhatToDoWithExistingWords)
{
    if (WhatToDoWithExistingWords == ReplaceMode::Always)
        return true;
    if (WhatToDoWithExistingWords == ReplaceMode::Never)
        return false;
    if (OldGroupNum == NewGroupNum)
        return true;

    QMessageBox ask;
    ask.setText(QString("The word '%1' already exists in group %2 of the currently open file.").arg(TheWord.c_str()).arg(QString::number(OldGroupNum)));
    ask.setInformativeText(QString("Do you wish to replace it with the occurrence in the merge file (group %1)?").arg(QString::number(NewGroupNum)));
    ask.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::NoToAll);
    int AskResult = ask.exec();

    if (AskResult == QMessageBox::YesToAll)
        WhatToDoWithExistingWords = ReplaceMode::Always;
    if (AskResult == QMessageBox::NoToAll)
        WhatToDoWithExistingWords = ReplaceMode::Never;
    if (AskResult == QMessageBox::Yes || AskResult == QMessageBox::YesToAll)
        return true;
    else
        return false;
}
//************************************************************
