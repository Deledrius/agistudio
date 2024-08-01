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


#include <QDialog>
#include <QString>
#include <QWidget>

#include "ui/ui_wordsfind.h"
#include "ui/ui_wordsedit.h"


class ResourcesWin;
class WordList;
class WordsFind;

//******************************************************
class WordsEdit : public QMainWindow, private Ui::WordsEdit
{
    Q_OBJECT
public:
    explicit WordsEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0, ResourcesWin *res = 0);
    ResourcesWin *resources_win;
    WordList *wordlist;
    void open();
protected:
    void new_file();
    void open_file();
    void merge_file();
    void save_file();
    void save_as_file();

    void add_group_cb();
    void delete_group_cb();
    void change_group_number_cb();
    void add_word_cb(void);
    void do_add_word(void);
    void delete_word_cb();
    void find_cb();
    void count_groups_cb(void);
    void count_words_cb(void);

    void open(const QString &);
    void save(const QString &);
    void deinit();

    void select_group(int);
    void update_group(int);
    void select_group();
    void select_word();

    int winnum;
    WordsFind *wordsfind;
    bool changed;
    QString resource_filename;
    size_t FindLastGroup, FindLastWord;
    int find_down(const QString &word);
    int find_up(const QString &word);
    const QString format_group(int) const;
    void closeEvent(QCloseEvent *e);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    void update_all();

    friend WordsFind;
};

//******************************************************
class WordsFind : public QMainWindow, private Ui::WordsFind
{
    Q_OBJECT
public:
    explicit WordsFind(QWidget *parent = 0, const char *name = 0, WordsEdit *w = 0);
    bool first;
protected:
    void find_next_cb();
    WordsEdit *wordsedit;
    WordList *wordlist;
    size_t FindLastGroup, FindLastWord;
    int find_down(QString *word);
    int find_up(QString *word);
    void find_first();

    friend WordsEdit;
};

//******************************************************

#endif
