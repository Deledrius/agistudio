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
      SelectedGroup(0), FindLastGroup(-1), FindLastWord(-1)
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
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
                on_actionSave_triggered();
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
    listGroup->clear();
    listWords->clear();
    QString entry;

    if (wordlist->NumGroups > 0) {
        for (int i = 0; i < wordlist->NumGroups; i++) {
            entry = QString("%1. ").arg(wordlist->WordGroup[i].GroupNum);
            for (int k = 0; k < wordlist->WordGroup[i].Words.count(); k++) {
                if (k > 0)
                    entry += " | ";
                entry += wordlist->WordGroup[i].Words.at(k);
            }
            listGroup->insertItem(i, entry);
        }
    }
    show();
    changed = false;

    if (listGroup->count() > 0)
        listGroup->setCurrentRow(0);

    if (listWords->count() > 0)
        listWords->setCurrentRow(0);
}

//********************************************************
void WordsEdit::open(const QString &fname)
{
    if (wordlist->read(fname.toStdString()))
        return ;
    resource_filename = fname;
    update_all();
}

//********************************************************
void WordsEdit::open()
{
    open(QString("%1/WORDS.TOK").arg(game->dir.c_str()));
}

//********************************************************
void WordsEdit::on_pushButtonAddGroup_pressed(void)
{
    bool ok;
    int num = QInputDialog::getInt(this, tr("Add Group"), tr("Enter group number [0-65535]:"), 0, 0, 65535, 1, &ok);
    if (!ok)
        return;

    int i;
    if ((i = wordlist->add_group(num)) == -1)
        return;

    listGroup->insertItem(i, QString("%1. ").arg(QString::number(num)));
    listGroup->setCurrentRow(i);
    changed = true;
}

//********************************************************
void WordsEdit::on_pushButtonRemoveGroup_pressed(void)
{
    int rm = listGroup->currentRow();
    if (rm != -1) {
        wordlist->delete_group(rm);
        auto item = listGroup->takeItem(rm);
        delete item;
        if (wordlist->NumGroups > 0) {
            listGroup->setCurrentRow(rm);
            select_group(rm);
        } else
            listWords->clear();
        changed = true;
    }
}

//********************************************************
const QString WordsEdit::format_group(int curgroup) const
{
    QString group_words = QString("%1. ").arg(wordlist->WordGroup[curgroup].GroupNum);

    for (qsizetype i = 0; i < wordlist->WordGroup[curgroup].Words.count(); i++) {
        if (i > 0)
            group_words += " | ";
        group_words += wordlist->WordGroup[curgroup].Words.at(i);
    }

    return group_words;
}

//********************************************************
void WordsEdit::update_group(int curgroup)
{
    auto item = listGroup->item(curgroup);
    item->setText(format_group(curgroup));
    listGroup->editItem(item);
}

//********************************************************
void WordsEdit::on_pushButtonRemoveWord_pressed(void)
{
    QString str = lineWord->text();
    int k = wordlist->delete_word(str.toStdString(), SelectedGroup);
    if (k != -1) {
        lineWord->clear();
        auto item = listWords->takeItem(k);
        delete item;
        update_group(SelectedGroup);
        on_listWords_itemSelectionChanged();
        changed = true;
        return;
    }
}

//********************************************************
void WordsEdit::on_pushButtonChangeGroupNum_pressed(void)
{
    bool ok;
    int num = QInputDialog::getInt(this, tr("Change Group Number"), tr("Enter group number [0-65535]:"), 0, 0, 65535, 1, &ok);
    if (!ok)
        return;

    int i;
    int currentgroup = listGroup->currentRow();
    if ((i = wordlist->change_number(currentgroup, num)) == -1)
        return;

    auto item = listGroup->takeItem(currentgroup);
    delete item;
    listGroup->insertItem(i, "");
    update_group(i);
    changed = true;
}

//********************************************************
void WordsEdit::on_pushButtonFind_pressed(void)
{
    if (wordsfind == nullptr)
        wordsfind = new WordsFind(nullptr, nullptr, this);
    wordsfind->show();
    wordsfind->lineFind->setFocus();
}

//********************************************************
int WordsEdit::find_down(const QString &word)
{
    for (int i = FindLastGroup; i < wordlist->NumGroups; i++) {
        for (int k = FindLastWord; k < wordlist->WordGroup[i].Words.count(); k++) {
            if (!QString::compare(wordlist->WordGroup[i].Words.at(k), word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        FindLastWord = 0;
    }
    FindLastGroup = wordlist->NumGroups - 1;
    FindLastWord = wordlist->WordGroup[FindLastGroup].Words.count() - 1;
    return 0;
}

//********************************************************
int WordsEdit::find_up(const QString &word)
{
    for (int i = FindLastGroup; i >= 0; i--) {
        for (int k = FindLastWord; k >= 0; k--) {
            if (!QString::compare(wordlist->WordGroup[i].Words.at(k), word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        if (i > 0)
            FindLastWord = wordlist->WordGroup[i - 1].Words.count() - 1;
    }
    FindLastWord = 0;
    FindLastGroup = 0;
    return 0;
}

//********************************************************
void WordsEdit::on_listGroup_itemSelectionChanged()
{
    listGroup->currentRow();
    if (listGroup->selectedItems().isEmpty())
        listWords->clear();
    else
        select_group(listGroup->currentRow());
}

//********************************************************
void WordsEdit::select_group(int num)
{
    labelWord->setText(QString("Word Group #%1").arg(QString::number(wordlist->WordGroup[num].GroupNum)));

    SelectedGroup = num;

    listWords->clear();
    for (qsizetype k = 0; k < wordlist->WordGroup[num].Words.count(); k++)
        listWords->insertItem(k, wordlist->WordGroup[num].Words.at(k));
    pushButtonRemoveWord->setEnabled(false);
    lineWord->clear();
    lineWord->setEnabled(false);
    listWords->show();
}

//********************************************************
void WordsEdit::on_listWords_itemSelectionChanged()
{
    if (!listWords->selectedItems().isEmpty()) {
        lineWord->setText(wordlist->WordGroup[SelectedGroup].Words.at(listWords->currentRow()));
        pushButtonRemoveWord->setEnabled(true);
    }
}

//********************************************************
void WordsEdit::on_pushButtonAddWord_pressed(void)
{
    lineWord ->setEnabled(true);
    lineWord->setText("new word");
    lineWord->selectAll();
    lineWord->setFocus();
}

//********************************************************
void WordsEdit::on_lineWord_returnPressed(void)
{
    QString word = lineWord->text();

    FindLastWord = 0;
    FindLastGroup = 0;
    int curgroup = listGroup->currentRow();
    if (find_down(word)) {
        auto prompt = tr("This word already exists (in group %1).\nDo you wish to remove this occurance and add it to this group?").arg(wordlist->WordGroup[FindLastGroup].GroupNum);

        switch (QMessageBox::warning(this, tr("Remove duplicate word?"),
                                     prompt,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No)) {
            case QMessageBox::Yes:
                wordlist->WordGroup[FindLastGroup].Words.removeAt(FindLastWord);
                update_group(FindLastGroup);
                changed = true;
                break;
            default:
                return;
        }
    }

    wordlist->WordGroup[curgroup].Words.append(word);
    wordlist->WordGroup[curgroup].Words.sort();
    changed = true;
    select_group(curgroup);
    update_group(curgroup);
}

//********************************************************
void WordsEdit::on_actionCountWordGroups_triggered(void)
{
    QMessageBox::information(this, tr("AGI studio"), tr("There are %1 word groups.").arg(QString::number(wordlist->NumGroups)));
}

//********************************************************
void WordsEdit::on_actionCountWords_triggered(void)
{
    int n = 0;
    for (int i = 0; i < wordlist->NumGroups; i++)
        n += wordlist->WordGroup[i].Words.count();
    QMessageBox::information(this, tr("AGI studio"), tr("There are %1 words.").arg(QString::number(n)));
}

//********************************************************
void WordsEdit::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Tokens File"), game->dir.c_str(), tr("Tokens List Files (*.tok);;All Files (*)"));
    if (!fileName.isNull())
        open(fileName);
}

//********************************************************
void WordsEdit::on_actionSaveAs_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Tokens File"), game->dir.c_str(), tr("Tokens List Files(*.tok);;All Files (*)"));
    if (!fileName.isNull())
        save(fileName);
}

//********************************************************
void WordsEdit::on_actionNew_triggered()
{
    wordlist->clear();
    listGroup->clear();
    listWords->clear();
    for (int i = 0; i < 3; i++)
        listGroup->insertItem(listGroup->count(), format_group(i));
    resource_filename = "";
    update_all();
}

//********************************************************
void WordsEdit::save(const QString &fname)
{
    if (wordlist->NumGroups == 0) {
        menu->errmes("Wordsedit", "Error: Could not save the file as there are no word groups.");
        return;
    }
    if (!wordlist->save(fname.toStdString()))
        changed = false;
}

//********************************************************
void WordsEdit::on_actionSave_triggered()
{
    if (!resource_filename.isEmpty())
        save(resource_filename);
    else
        on_actionSaveAs_triggered();
}

//********************************************************
void WordsEdit::on_actionMerge_triggered()
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

    radioButtonMatchCase->setVisible(false);
}

//********************************************************
int WordsFind::find_down(QString *word)
{
    bool sub = radioButtonMatchSubstr->isChecked();

    for (int i = FindLastGroup; i < wordlist->NumGroups; i++) {
        for (int k = FindLastWord; k < wordlist->WordGroup[i].Words.count(); k++) {
            if ((sub && wordlist->WordGroup[i].Words.at(k).contains(*word)) ||
                    !QString::compare(wordlist->WordGroup[i].Words.at(k), *word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        FindLastWord = 0;
    }
    FindLastGroup = wordlist->NumGroups - 1;
    FindLastWord = wordlist->WordGroup[FindLastGroup].Words.count() - 1;
    return 0;
}

//********************************************************
int WordsFind::find_up(QString *word)
{
    bool sub = radioButtonMatchSubstr->isChecked();

    for (int i = FindLastGroup; i >= 0; i--) {
        for (int k = FindLastWord; k >= 0; k--) {
            if ((sub && wordlist->WordGroup[i].Words.at(k).contains(*word)) ||
                    !QString::compare(wordlist->WordGroup[i].Words.at(k), *word)) {
                FindLastWord = k;
                FindLastGroup = i;
                return 1;
            }
        }
        if (i > 0)
            FindLastWord = wordlist->WordGroup[i - 1].Words.count() - 1;
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
            if (wordsedit->listGroup->currentRow() != -1)
                FindLastGroup = wordsedit->listGroup->currentRow();
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
            FindLastGroup = wordlist->NumGroups - 1;
            FindLastWord = wordlist->WordGroup[FindLastGroup].Words.count() - 1;
        } else {
            if (wordsedit->listGroup->currentRow() != -1)
                FindLastGroup = wordsedit->listGroup->currentRow();
            else
                FindLastGroup = wordlist->NumGroups - 1;
            if (wordsedit->listWords->currentRow() != -1)
                FindLastWord = wordsedit->listWords->currentRow();
            else
                FindLastWord = wordlist->WordGroup[FindLastGroup].Words.count() - 1;
        }
        ret = find_up(&word);
    }

    if (ret) {
        wordsedit->listGroup->setCurrentRow(FindLastGroup);
        wordsedit->listWords->setCurrentRow(FindLastWord);
    } else
        statusBar()->showMessage(tr("Search term '%1' not found!").arg(word));
}

//********************************************************
void WordsFind::on_buttonFindNext_pressed()
{
    int ret;
    QString word = lineFind->text();

    statusBar()->clearMessage();

    if (FindLastGroup == -1 || FindLastWord == -1) {
        find_first();
        return;
    }

    if (radioButtonDirDown->isChecked()) {
        if (FindLastWord + 1 >= wordlist->WordGroup[FindLastGroup].Words.count()) {
            if (FindLastGroup + 1 >= wordlist->NumGroups) {
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
                FindLastWord = wordlist->WordGroup[FindLastGroup].Words.count() - 1;
            }
        } else
            FindLastWord--;
        ret = find_up(&word);
    }

    if (ret) {
        wordsedit->listGroup->setCurrentRow(FindLastGroup);
        wordsedit->listWords->setCurrentRow(FindLastWord);
    } else
        statusBar()->showMessage(tr("Search term '%1' not found!").arg(word));
}

//************************************************
