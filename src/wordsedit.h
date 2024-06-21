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

#include "wutil.h"
#include "words.h"
#include "resources.h"

#include "ui/ui_wordsfind.h"
#include "ui/ui_wordsedit.h"


class WordsFind;

//******************************************************
class WordsEdit : public QMainWindow, private Ui::WordsEdit
{
    Q_OBJECT
public:
    WordsEdit(QWidget *parent = 0, const char *name = 0, int winnum = 0, ResourcesWin *res = 0);
    ResourcesWin *resources_win;
    WordList *wordlist;
    void open();
private slots:
    // Menu signal handlers
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionMerge_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionClose_triggered() { close(); }

    void on_actionAddWordGroup_triggered() { on_pushButtonAddGroup_pressed(); }
    void on_actionRemoveWordGroup_triggered() { on_pushButtonRemoveGroup_pressed(); }
    void on_actionChangeGroupNumber_triggered() { on_pushButtonChangeGroupNum_pressed(); }

    void on_actionAddWord_triggered() { on_pushButtonAddWord_pressed(); }
    void on_actionRemoveWord_triggered() { on_pushButtonRemoveWord_pressed(); }

    void on_actionFind_triggered() { on_pushButtonFind_pressed(); }

    void on_actionCountWordGroups_triggered(void);
    void on_actionCountWords_triggered(void);

    // Event signal handlers
    void on_listGroup_itemSelectionChanged();
    void on_listWords_itemSelectionChanged();

    // Button signal handlers
    void on_pushButtonAddGroup_pressed(void);
    void on_pushButtonRemoveGroup_pressed(void);
    void on_pushButtonChangeGroupNum_pressed(void);

    void on_lineWord_returnPressed(void);
    void on_pushButtonAddWord_pressed(void);
    void on_pushButtonRemoveWord_pressed(void);

    void on_pushButtonFind_pressed(void);
protected:
    void open(const QString&);
    void save(const QString&);
    void deinit();

    void select_group(int);
    void update_group(int);

    int winnum;
    WordsFind *wordsfind;
    bool changed;
    QString resource_filename;
    int FindLastGroup, FindLastWord;
    int SelectedGroup;
    int find_down(const QString& word);
    int find_up(const QString& word);
    const QString format_group(int curgroup) const;
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
    WordsFind(QWidget *parent = 0, const char *name = 0, WordsEdit *w = 0);
    bool first;
public slots:
    void on_buttonFindNext_pressed();
protected:
    WordsEdit* wordsedit;
    WordList* wordlist;
    int FindLastGroup, FindLastWord;
    int find_down(QString* word);
    int find_up(QString* word);
    void find_first();

    friend WordsEdit;
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
