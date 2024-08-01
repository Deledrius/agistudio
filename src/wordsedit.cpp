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


#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "game.h"
#include "words.h"
#include "menu.h"
#include "wordsedit.h"


WordsEdit::WordsEdit(QWidget *parent, const char *name, int win_num, ResourcesWin *res)
    : QMainWindow(parent), winnum(win_num), resources_win(res),
      wordsfind(nullptr), wordlist(new WordList), changed(false), resource_filename(),
      FindLastGroup(-1), FindLastWord(-1)
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    // Menu signal handlers
    connect(actionNew, &QAction::triggered, this, &WordsEdit::new_file);
    connect(actionOpen, &QAction::triggered, this, &WordsEdit::open_file);
    connect(actionMerge, &QAction::triggered, this, &WordsEdit::merge_file);
    connect(actionSave, &QAction::triggered, this, &WordsEdit::save_file);
    connect(actionSaveAs, &QAction::triggered, this, &WordsEdit::save_as_file);
    connect(actionClose, &QAction::triggered, this, &WordsEdit::close);

    connect(actionAddWordGroup, &QAction::triggered, this, &WordsEdit::add_group_cb);
    connect(actionRemoveWordGroup, &QAction::triggered, this, &WordsEdit::delete_group_cb);
    connect(actionChangeGroupNumber, &QAction::triggered, this, &WordsEdit::change_group_number_cb);
    connect(actionAddWord, &QAction::triggered, this, &WordsEdit::add_word_cb);
    connect(actionRemoveWord, &QAction::triggered, this, &WordsEdit::delete_word_cb);
    connect(actionFind, &QAction::triggered, this, &WordsEdit::find_cb);
    connect(actionCountWordGroups, &QAction::triggered, this, &WordsEdit::count_groups_cb);
    connect(actionCountWords, &QAction::triggered, this, &WordsEdit::count_words_cb);

    // Event signal handlers
    connect(listGroups, &QListWidget::itemSelectionChanged, this, qOverload<>(&WordsEdit::select_group));
    connect(listWords, &QListWidget::itemSelectionChanged, this, &WordsEdit::select_word);

    // Button signal handlers
    connect(pushButtonAddGroup, &QPushButton::pressed, this, &WordsEdit::add_group_cb);
    connect(pushButtonRemoveGroup, &QPushButton::pressed, this, &WordsEdit::delete_group_cb);
    connect(pushButtonChangeGroupNum, &QPushButton::pressed, this, &WordsEdit::change_group_number_cb);

    connect(lineWord, &QLineEdit::returnPressed, this, &WordsEdit::do_add_word);
    connect(pushButtonAddWord, &QPushButton::pressed, this, &WordsEdit::add_word_cb);
    connect(pushButtonRemoveWord, &QPushButton::pressed, this, &WordsEdit::delete_word_cb);

    connect(pushButtonFind, &QPushButton::pressed, this, &WordsEdit::find_cb);
}

//********************************************************
void WordsEdit::deinit()
{
    delete wordlist;
    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void WordsEdit::hideEvent(QHideEvent *)
{
    if (wordsfind) {
        wordsfind->close();
        wordsfind = nullptr;
    }
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void WordsEdit::showEvent(QShowEvent *)
{
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//********************************************************
void WordsEdit::closeEvent(QCloseEvent *e)
{
    if (changed) {
        switch (QMessageBox::warning(this, tr("Word Tokens Editor"), tr("Save changes to WORDS.TOK?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel)) {
            case QMessageBox::Save:
                save_file();
                deinit();
                e->accept();
                break;
            case QMessageBox::Discard:
                deinit();
                e->accept();
                break;
            default: // Cancel
                e->ignore();
                break;
        }
    } else {
        deinit();
        e->accept();
    }
}

//********************************************************
void WordsEdit::update_all()
{
    listGroups->clear();
    listWords->clear();

    for (const auto &group_num : wordlist->GetWordGroupNumbers()) {
        auto entry = QString("%1. ").arg(group_num);

        for (bool isFirst(true); const auto &word : wordlist->GetGroupWords(group_num)) {
            if (!isFirst)
                entry += " | ";
            entry += word.c_str();
            isFirst = false;
        }

        listGroups->insertItem(listGroups->count(), entry);
    }

    show();
    changed = false;

    if (listGroups->count() > 0)
        listGroups->setCurrentRow(0);

    if (listWords->count() > 0)
        listWords->setCurrentRow(0);
}

//********************************************************
void WordsEdit::open(const QString &fname)
{
    if (wordlist->read(fname.toStdString()))
        return;
    resource_filename = fname;
    update_all();
}

//********************************************************
void WordsEdit::open()
{
    open(QString("%1/WORDS.TOK").arg(game->dir.c_str()));
}

//********************************************************
void WordsEdit::add_group_cb(void)
{
    bool ok;
    int num = QInputDialog::getInt(this, tr("Add Group"), tr("Enter group number [0-65535]:"), 0, 0, 65535, 1, &ok);
    if (!ok)
        return;

    int i;
    if ((i = wordlist->add_group(num)) == -1)
        return;

    listGroups->insertItem(i, QString("%1. ").arg(QString::number(num)));
    listGroups->setCurrentRow(i);
    changed = true;
}

//********************************************************
void WordsEdit::delete_group_cb(void)
{
    int rm = listGroups->currentRow();
    if (rm != -1) {
        auto group_num = wordlist->GetWordGroupNumberByIndex(rm);
        wordlist->delete_group(group_num);
        auto item = listGroups->takeItem(rm);
        delete item;
        if (wordlist->GetNumWordGroups() > 0) {
            listGroups->setCurrentRow(rm);
            select_group(rm);
        } else
            listWords->clear();
        changed = true;
    }
}

//********************************************************
const QString WordsEdit::format_group(int curgroupidx) const
{
    auto group_num = wordlist->GetWordGroupNumberByIndex(curgroupidx);
    auto group_words = QString("%1. ").arg(group_num);

    for (bool isFirst(true); const auto &word : wordlist->GetGroupWords(group_num)) {
        if (!isFirst)
            group_words += " | ";
        group_words += word.c_str();
        isFirst = false;
    }

    return group_words;
}

//********************************************************
void WordsEdit::update_group(int curgroupidx)
{
    auto item = listGroups->item(curgroupidx);
    item->setText(format_group(curgroupidx));
    listGroups->editItem(item);
}

//********************************************************
void WordsEdit::delete_word_cb(void)
{
    QString str = lineWord->text();
    auto group_num = wordlist->GetWordGroupNumberByIndex(listGroups->currentRow());
    int k = wordlist->delete_word(str.toStdString(), group_num);
    if (k != -1) {
        lineWord->clear();
        auto item = listWords->takeItem(k);
        delete item;
        update_group(listGroups->currentRow());
        select_word();
        changed = true;
        return;
    }
}

//********************************************************
void WordsEdit::change_group_number_cb(void)
{
    bool ok;
    int num = QInputDialog::getInt(this, tr("Change Group Number"), tr("Enter group number [0-65535]:"), 0, 0, 65535, 1, &ok);
    if (!ok)
        return;

    int i;
    int currentgroup = listGroups->currentRow();
    if ((i = wordlist->change_number(currentgroup, num)) == -1)
        return;

    auto item = listGroups->takeItem(currentgroup);
    delete item;
    listGroups->insertItem(i, "");
    update_group(i);
    changed = true;
}

//********************************************************
void WordsEdit::find_cb(void)
{
    if (wordsfind == nullptr)
        wordsfind = new WordsFind(nullptr, nullptr, this);
    wordsfind->show();
    wordsfind->lineFind->setFocus();
}

//********************************************************
int WordsEdit::find_down(const QString &word)
{
    for (auto i = FindLastGroup; i < wordlist->GetNumWordGroups(); i++) {
        auto group_num = wordlist->GetWordGroupNumberByIndex(i);
        for (auto k = FindLastWord; k < wordlist->GetGroupWords(group_num).size(); k++) {
            if (!QString::compare(wordlist->GetGroupWords(group_num).at(k).c_str(), word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        FindLastWord = 0;
    }
    FindLastGroup = wordlist->GetNumWordGroups() - 1;
    auto group_num = wordlist->GetWordGroupNumberByIndex(FindLastGroup);
    FindLastWord = wordlist->GetGroupWords(group_num).size() - 1;
    return 0;
}

//********************************************************
int WordsEdit::find_up(const QString &word)
{
    for (auto i = FindLastGroup; i >= 0; i--) {
        auto group_num = wordlist->GetWordGroupNumberByIndex(i);
        for (auto k = FindLastWord; k >= 0; k--) {
            if (!QString::compare(wordlist->GetGroupWords(group_num).at(k).c_str(), word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        group_num = wordlist->GetWordGroupNumberByIndex(i - 1);
        if (i > 0)
            FindLastWord = wordlist->GetGroupWords(group_num).size() - 1;
    }
    FindLastWord = 0;
    FindLastGroup = 0;
    return 0;
}

//********************************************************
void WordsEdit::select_group()
{
    listGroups->currentRow();
    if (listGroups->selectedItems().isEmpty())
        listWords->clear();
    else
        select_group(listGroups->currentRow());
}

//********************************************************
void WordsEdit::select_group(int groupidx)
{
    int group_num = wordlist->GetWordGroupNumberByIndex(groupidx);
    labelWord->setText(QString("Word Group #%1").arg(QString::number(group_num)));

    listWords->clear();
    for (const auto &word : wordlist->GetGroupWords(group_num))
        listWords->insertItem(listWords->count(), word.c_str());
    pushButtonRemoveWord->setEnabled(false);
    lineWord->clear();
    lineWord->setEnabled(false);
    listWords->show();
}

//********************************************************
void WordsEdit::select_word()
{
    if (!listWords->selectedItems().isEmpty()) {
        auto group_num = wordlist->GetWordGroupNumberByIndex(listGroups->currentRow());
        lineWord->setText(wordlist->GetGroupWords(group_num).at(listWords->currentRow()).c_str());
        pushButtonRemoveWord->setEnabled(true);
    }
}

//********************************************************
void WordsEdit::add_word_cb(void)
{
    lineWord->setEnabled(true);
    lineWord->setText("new word");
    lineWord->selectAll();
    lineWord->setFocus();
}

//********************************************************
void WordsEdit::do_add_word(void)
{
    QString word = lineWord->text();

    FindLastWord = 0;
    FindLastGroup = 0;
    int curgroup_index = listGroups->currentRow();
    auto group_num = wordlist->GetWordGroupNumberByIndex(listGroups->currentRow());
    if (find_down(word)) {
        group_num = wordlist->GetWordGroupNumberByIndex(FindLastGroup);
        auto prompt = tr("This word already exists (in group %1).\nDo you wish to remove this occurance and add it to this group?").arg(group_num);

        switch (QMessageBox::warning(this, tr("Remove duplicate word?"),
                                     prompt,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No)) {
            case QMessageBox::Yes:
                wordlist->delete_word(word.toStdString(), group_num);
                update_group(static_cast<int>(FindLastGroup));
                changed = true;
                break;
            default:
                return;
        }
    }

    wordlist->add_word(word.toStdString(), group_num);
    changed = true;
    select_group(curgroup_index);
    update_group(curgroup_index);
}

//********************************************************
void WordsEdit::count_groups_cb(void)
{
    QMessageBox::information(this, tr("AGI studio"), tr("There are %1 word groups.").arg(QString::number(wordlist->GetNumWordGroups())));
}

//********************************************************
void WordsEdit::count_words_cb(void)
{
    size_t n = wordlist->GetTotalWordCount();
    QMessageBox::information(this, tr("AGI studio"), tr("There are %1 words.").arg(QString::number(n)));
}

//********************************************************
void WordsEdit::open_file()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Tokens File"), game->dir.c_str(), tr("Tokens List Files (*.tok);;All Files (*)"));
    if (!fileName.isNull())
        open(fileName);
}

//********************************************************
void WordsEdit::save_as_file()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Tokens File"), game->dir.c_str(), tr("Tokens List Files(*.tok);;All Files (*)"));
    if (!fileName.isNull())
        save(fileName);
}

//********************************************************
void WordsEdit::new_file()
{
    wordlist->clear();
    listGroups->clear();
    listWords->clear();
    for (int i = 0; i < 3; i++)
        listGroups->insertItem(listGroups->count(), format_group(i));
    resource_filename = "";
    update_all();
}

//********************************************************
void WordsEdit::save(const QString &fname)
{
    if (wordlist->GetNumWordGroups() == 0) {
        menu->errmes("Error: Could not save the file as there are no word groups.");
        return;
    }
    if (!wordlist->save(fname.toStdString()))
        changed = false;
}

//********************************************************
void WordsEdit::save_file()
{
    if (!resource_filename.isEmpty())
        save(resource_filename);
    else
        save_as_file();
}

//********************************************************
void WordsEdit::merge_file()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Word Tokens File"), game->dir.c_str(), tr("Tokens List Files (*.tok);;All Files (*)"));

    if (!fileName.isNull()) {
        WordList *w = new WordList();
        if (w->read(fileName.toStdString()))
            return;
        wordlist->merge(*w);
        update_all();
    }
}


//********************************************************
//
WordsFind::WordsFind(QWidget *parent, const char *name, WordsEdit *w)
    : QMainWindow(parent), wordsedit(w), wordlist(w->wordlist),
      FindLastWord(-1), FindLastGroup(-1), first(false)
{
    setupUi(this);

    connect(buttonFindNext, &QPushButton::pressed, this, &WordsFind::find_next_cb);

    radioButtonMatchCase->setVisible(false);
}

//********************************************************
int WordsFind::find_down(QString *word)
{
    bool sub = radioButtonMatchSubstr->isChecked();

    for (auto i = FindLastGroup; i < wordlist->GetNumWordGroups(); i++) {
        auto group_num = wordlist->GetWordGroupNumberByIndex(i);
        for (auto k = FindLastWord; k < wordlist->GetGroupWords(group_num).size(); k++) {
            if ((sub && QString(wordlist->GetGroupWords(group_num).at(k).c_str()).contains(*word)) ||
                    !QString::compare(wordlist->GetGroupWords(group_num).at(k).c_str(), *word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        FindLastWord = 0;
    }
    FindLastGroup = wordlist->GetNumWordGroups() - 1;
    auto group_num = wordlist->GetWordGroupNumberByIndex(FindLastGroup);
    FindLastWord = wordlist->GetGroupWords(group_num).size() - 1;
    return 0;
}

//********************************************************
int WordsFind::find_up(QString *word)
{
    bool sub = radioButtonMatchSubstr->isChecked();

    for (auto i = FindLastGroup; i >= 0; i--) {
        auto group_num = wordlist->GetWordGroupNumberByIndex(i);
        for (auto k = FindLastWord; k >= 0; k--) {
            if ((sub && QString(wordlist->GetGroupWords(group_num).at(k).c_str()).contains(*word)) ||
                    !QString::compare(wordlist->GetGroupWords(group_num).at(k).c_str(), *word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        group_num = wordlist->GetWordGroupNumberByIndex(i - 1);
        if (i > 0)
            FindLastWord = wordlist->GetGroupWords(group_num).size() - 1;
    }
    FindLastWord = 0;
    FindLastGroup = 0;
    return 0;
}

//********************************************************
void WordsFind::find_first()
{
    int ret;
    QString word = lineFind->text();

    if (radioButtonDirDown->isChecked()) {
        if (radioButtonFromStart->isChecked()) {
            FindLastGroup = 0;
            FindLastWord = 0;
        } else {
            if (wordsedit->listGroups->currentRow() != -1)
                FindLastGroup = wordsedit->listGroups->currentRow();
            else
                FindLastGroup = 0;
            if (wordsedit->listWords->currentRow() != -1)
                FindLastWord = wordsedit->listWords->currentRow();
            else
                FindLastWord = 0;
        }
        ret = find_down(&word);
    } else {
        if (radioButtonFromStart->isChecked()) {
            FindLastGroup = wordlist->GetNumWordGroups() - 1;
            auto group_num = wordlist->GetWordGroupNumberByIndex(FindLastGroup);
            FindLastWord = wordlist->GetGroupWords(group_num).size() - 1;
        } else {
            if (wordsedit->listGroups->currentRow() != -1)
                FindLastGroup = wordsedit->listGroups->currentRow();
            else
                FindLastGroup = wordlist->GetNumWordGroups() - 1;

            auto group_num = wordlist->GetWordGroupNumberByIndex(FindLastGroup);
            if (wordsedit->listWords->currentRow() != -1)
                FindLastWord = wordsedit->listWords->currentRow();
            else
                FindLastWord = wordlist->GetGroupWords(group_num).size() - 1;
        }
        ret = find_up(&word);
    }

    if (ret) {
        wordsedit->listGroups->setCurrentRow(static_cast<int>(FindLastGroup));
        wordsedit->listWords->setCurrentRow(static_cast<int>(FindLastWord));
    } else
        statusBar()->showMessage(tr("Search term '%1' not found!").arg(word));
}

//********************************************************
void WordsFind::find_next_cb()
{
    int ret;
    QString word = lineFind->text();

    statusBar()->clearMessage();

    if (FindLastGroup == -1 || FindLastWord == -1) {
        find_first();
        return;
    }

    if (radioButtonDirDown->isChecked()) {
        auto group_num = wordlist->GetWordGroupNumberByIndex(FindLastGroup);
        if (FindLastWord + 1 >= wordlist->GetGroupWords(group_num).size()) {
            if (FindLastGroup + 1 >= wordlist->GetNumWordGroups()) {
                statusBar()->showMessage(tr("Search term '%1' not found!").arg(word));
                return ;
            } else {
                FindLastWord = 0;
                FindLastGroup++;
            }
        } else
            FindLastWord++;
        ret = find_down(&word);
    } else {
        if (FindLastWord - 1 < 0) {
            if (FindLastGroup - 1 < 0) {
                statusBar()->showMessage(tr("Search term '%1' not found!").arg(word));
                return;
            } else {
                FindLastGroup--;
                auto group_num = wordlist->GetWordGroupNumberByIndex(FindLastGroup);
                FindLastWord = wordlist->GetGroupWords(group_num).size() - 1;
            }
        } else
            FindLastWord--;
        ret = find_up(&word);
    }

    if (ret) {
        wordsedit->listGroups->setCurrentRow(static_cast<int>(FindLastGroup));
        wordsedit->listWords->setCurrentRow(static_cast<int>(FindLastWord));
    } else
        statusBar()->showMessage(tr("Search term '%1' not found!").arg(word));
}

//************************************************
