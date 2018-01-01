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


#ifndef WORDSEDIT_H
#define WORDSEDIT_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>

#include "util.h"
#include "wutil.h"
#include "words.h"
#include "resources.h"

class WordsFind;

//******************************************************
class WordsEdit : public QWidget
{
    Q_OBJECT
public:
    WordsEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0, ResourcesWin *res = 0);
    QListWidget *listgroup, *listwords;
    ResourcesWin *resources_win;
    WordList *wordlist;
    void open();
public slots:
    void add_group_cb(void);
    void delete_group_cb(void);
    void add_word_cb(void);
    void do_add_word(void);
    void delete_word_cb(void);
    void change_group_number_cb(void);
    void find_cb(void);
    void select_group();
    void select_group(int);
    void select_word();
    void update_group(int);
    void count_groups_cb(void);
    void count_words_cb(void);

    void new_file();
    void open_file();
    void merge_file();
    void save_file();
    void save_as_file();
protected:
    void open(char *);
    void save(char *);
    void deinit();
    int winnum;
    QLabel *labelword;
    QLineEdit *lineword;
    QPushButton *add_group, *delete_group, *add_word, *delete_word,
                *change_group_number, *find;
    WordsFind *wordsfind;
    bool changed;
    std::string filename;
    int FindLastGroup, FindLastWord;
    int SelectedGroup;
    int find_down(char *word);
    int find_up(char *word);
    void print_group(int curgroup);
    void add_group_ok_cb();
    void change_group_ok_cb();
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void update_all();
};

//******************************************************
class WordsFind : public QWidget
{
    Q_OBJECT
public:
    WordsFind(QWidget *parent = 0, const char *name = 0, WordsEdit *w = 0);
    void open();
    QLineEdit *find_field;
    bool first;
public slots:
    void find_next_cb();
    void find_first_cb();
    void cancel_cb();
protected:
    WordList *wordlist;
    WordsEdit *wordsedit;
    QPushButton *find_first, *find_next, *cancel;
    QRadioButton *up, *down;
    QRadioButton *start, *current;
    QRadioButton *exact, *substring;
    int FindLastGroup, FindLastWord;
    int find_down(char *word);
    int find_up(char *word);
};

//******************************************************
class ReplaceWord : public QDialog
{
    Q_OBJECT
public:
    ReplaceWord(std::string word = 0, int OldGroupNum = 0, int NewGroupNum = 0, QWidget *parent = 0, QString name = (const char *)0);
public slots:
    void yes();
    void yes_to_all();
    void no();
    void no_to_all();
};


#endif
