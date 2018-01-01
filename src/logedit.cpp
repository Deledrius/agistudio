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

#include "logedit.h"
#include "game.h"
#include "menu.h"
#include "agicommands.h"

#include <string>
#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>

#include <QApplication>
#include <QRegExp>
#include <QLabel>
#include <QHideEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include <QSyntaxHighlighter>
#include <QTextStream>
#include <QFileDialog>
#include <QGroupBox>

TStringList InputLines;  //temporary buffer for reading the text from editor
//and sending to compilation

//***********************************************
// Syntax highlight

static QString operators = "!-+=<>/*&|^%";

static QColor normal_color =   QColor("black"),
              comment_color =  QColor("gray"),
              string_color =   QColor("firebrick"),
              number_color =   QColor("darkCyan"),
              command_color =  QColor("darkBlue"),
              keyword_color =  QColor("green"),
              test_color =     QColor("brown"),
              operator_color = QColor("steelBlue");

static QStringList keywords = {"if", "else", "goto"};

class LogicSyntaxHL : public QSyntaxHighlighter
{
public:
    LogicSyntaxHL(QTextEdit *parent)
        : QSyntaxHighlighter(parent)
    {
        HighlightingRule rule;

        operatorFormat.setForeground(operator_color);
        rule.pattern = QRegExp("[" + operators + "]");
        rule.format = operatorFormat;
        highlightingRules.append(rule);

        keywordFormat.setForeground(keyword_color);
        keywordFormat.setFontWeight(QFont::Bold);
        foreach (const QString &pattern, keywords) {
            rule.pattern = QRegExp("\\b" + pattern + "\\b");
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }
        rule.pattern = QRegExp("^#\\w+");
        rule.format = keywordFormat;
        highlightingRules.append(rule);

        testCmdFormat.setForeground(test_color);
        testCmdFormat.setFontItalic(true);
        for (size_t i = 0; i < NumTestCommands; i++) {
            auto cmd = QString(TestCommand[i].Name);
            if (!cmd.isEmpty()) {
                cmd = cmd.replace(".", "\\.");
                rule.pattern = QRegExp("\\b" + cmd + "\\b");
                rule.format = testCmdFormat;
                highlightingRules.append(rule);
            }
        }

        AGICmdFormat.setForeground(command_color);
        AGICmdFormat.setFontItalic(true);
        for (size_t i = 0; i < NumAGICommands; i++) {
            auto cmd = QString(AGICommand[i].Name);
            if (!cmd.isEmpty()) {
                cmd = cmd.replace(".", "\\.");
                rule.pattern = QRegExp("\\b" + cmd + "\\b");
                rule.format = AGICmdFormat;
                highlightingRules.append(rule);
            }
        }

        numberFormat.setForeground(number_color);
        rule.pattern = QRegExp("\\d+");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        stringFormat.setForeground(string_color);
        rule.pattern = QRegExp("\".*\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        singleLineCommentFormat.setForeground(comment_color);
        rule.pattern = QRegExp("//[^\n]*");
        rule.format = singleLineCommentFormat;
        highlightingRules.append(rule);

        multiLineCommentFormat.setForeground(comment_color);
        commentStartExpression = QRegExp("/\\*");
        commentEndExpression = QRegExp("\\*/");
    }

    void highlightBlock(const QString &text) override
    {
        foreach (const HighlightingRule &rule, highlightingRules) {
            QRegExp expression(rule.pattern);
            int index = expression.indexIn(text);
            while (index >= 0) {
                int length = expression.matchedLength();
                setFormat(index, length, rule.format);
                index = expression.indexIn(text, index + length);
            }
        }
        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1)
            startIndex = commentStartExpression.indexIn(text);

        while (startIndex >= 0) {
            int endIndex = commentEndExpression.indexIn(text, startIndex);
            int commentLength;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex
                                + commentEndExpression.matchedLength();
            }
            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
        }
    }

private:
    struct HighlightingRule {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegExp commentStartExpression;
    QRegExp commentEndExpression;

    QTextCharFormat operatorFormat;
    QTextCharFormat keywordFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat testCmdFormat;
    QTextCharFormat AGICmdFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
};

//***********************************************
LogEdit::LogEdit(QWidget *parent, const char *name, int win_num, ResourcesWin *res, bool readonly)
    : QWidget(parent)
    , findedit(NULL)
    , roomgen(NULL)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Logic editor");

    logic = new Logic();
    winnum = win_num;  //my window number
    resources_win = res; //resources window which called me

    editor = new QTextEdit(this);

    QFont font("Monospace");
    font.setPointSize(readonly ? 8 : 9);
    font.setStyleHint(QFont::TypeWriter);
    editor->setFont(font);

    editor->setSizePolicy(QSizePolicy(
                              QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    editor->setWordWrapMode(QTextOption::NoWrap);
    syntax_hl = new LogicSyntaxHL(editor);

    if (readonly)
        editor->setReadOnly(readonly);
    else
        editor->setMinimumSize(512, 600);

    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

    QBoxLayout *all = new QVBoxLayout(this);
    all->addWidget(editor);

    if (!readonly) {
        QMenu *file = new QMenu(this);
        Q_CHECK_PTR(file);
        file->setTitle("&File");
        file->addAction("&Read from file", this, SLOT(read_logic()));
        file->addAction("&Save", this, SLOT(save_logic()), Qt::CTRL + Qt::Key_S);
        file->addAction("Save &as", this, SLOT(save_as()));
        file->addAction("&Compile", this, SLOT(compile_logic()), Qt::Key_F9);
        file->addAction("Compile all", this, SLOT(compile_all_logic()));
        file->addAction("Compile all and run", this, SLOT(compile_and_run()), Qt::Key_F10);
        file->addAction("Change logic number", this, SLOT(change_logic_number()));
        file->addSeparator();
        file->addAction("&Delete", this, SLOT(delete_logic()));
        file->addAction("&New room", this, SLOT(new_room()));
        file->addSeparator();
        file->addAction("&Close", this, SLOT(close()));

        QMenu *edit = new QMenu(this);
        Q_CHECK_PTR(edit);
        edit->setTitle("&Edit");
        edit->addAction("Cu&t", editor, SLOT(cut()), Qt::CTRL + Qt::Key_X);
        edit->addAction("&Copy", editor, SLOT(copy()), Qt::CTRL + Qt::Key_C);
        edit->addAction("&Paste", editor, SLOT(paste()), Qt::CTRL + Qt::Key_V);
        edit->addSeparator();
        edit->addAction("Clear &all", this, SLOT(clear_all()));
        edit->addSeparator();
        edit->addAction("&Find", this, SLOT(find_cb()), Qt::CTRL + Qt::Key_F);
        edit->addAction("Fi&nd again", this, SLOT(find_again()), Qt::Key_F3);
        edit->addSeparator();
        edit->addAction("&Go to line", this, SLOT(goto_cb()), Qt::ALT + Qt::Key_G);

        QMenu *help = new QMenu(this);
        Q_CHECK_PTR(help);
        help->setTitle("&Help");
        help->addAction("&Context help", this, SLOT(context_help()), Qt::Key_F1);
        help->addAction("&All commands", this, SLOT(command_help()));

        QMenuBar *menu = new QMenuBar(this);
        Q_CHECK_PTR(menu);
        menu->addMenu(file);
        menu->addMenu(edit);
        menu->addMenu(help);

        all->setMenuBar(menu);

        status = new QStatusBar(this);
        QLabel *msg = new QLabel("");
        status->addWidget(msg, 4);
        all->addWidget(status);

        connect(editor, SIGNAL(cursorPositionChanged(int, int)),
                this, SLOT(update_line_num(int, int)));
    }

    getmaxcol();
    changed = false;
    filename = "";
    hide();
}

//***********************************************
void LogEdit::deinit()
{
    if (findedit) {
        findedit->close();
        findedit = NULL;
    }
    if (roomgen) {
        roomgen->close();
        roomgen = NULL;
    }
    delete logic;
    logic = 0;
    delete syntax_hl;
    syntax_hl = 0;

    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void LogEdit::hideEvent(QHideEvent *)
{
    if (findedit) {
        findedit->close();
        findedit = NULL;
    }
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void LogEdit::showEvent(QShowEvent *)
{
    if (window_list && window_list->isVisible())
        window_list->draw();
}
//***********************************************
void LogEdit::closeEvent(QCloseEvent *e)
{
    if (changed) {
        QString str = editor->toPlainText();
        if (!strcmp(str.toLatin1(), logic->OutputText.c_str())) { //not changed
            deinit();
            e->accept();
            return;
        }

        if (LogicNum != -1)
            sprintf(tmp, "Save changes to logic.%d ?", LogicNum);
        else
            sprintf(tmp, "Save changes to logic ?");
        switch (QMessageBox::warning(this, "Logic editor",
                                     tmp,
                                     "Yes",
                                     "No",
                                     "Cancel",
                                     0, 2)) {
            case 0: // yes
                save_logic();
                deinit();
                e->accept();
                break;
            case 1: // no
                deinit();
                e->accept();
                break;
            default: // cancel
                e->ignore();
                break;
        }

    } else {
        deinit();
        e->accept();
    }
}

//***********************************************
int LogEdit::open()
{
    getmaxcol();
    LogicNum = -1;
    show();
    changed = true;
    return 0;
}

//***********************************************
int LogEdit::open(char *fname)
{
    getmaxcol();

    FILE *fptr = fopen(fname, "rb");
    if (fptr != NULL) {
        filename = std::string(fname);
        editor->clear();
        char *ptr;
        QString filecont;
        while (fgets(tmp, MAX_TMP, fptr) != NULL) {
            if ((ptr = (char *)strchr(tmp, 0x0a)))
                * ptr = 0;
            if ((ptr = (char *)strchr(tmp, 0x0d)))
                * ptr = 0;
            filecont += QString(tmp) + "\n";
        }
        editor->setText(filecont);
        fclose(fptr);
        logic->OutputText = editor->toPlainText().toStdString();
        if ((ptr = (char *)strrchr(filename.c_str(), '/')))
            ptr++;
        else
            ptr = (char *)filename.c_str();
        if (LogicNum != -1)
            sprintf(tmp, "logic.%d (file %s)", LogicNum, ptr);
        else
            sprintf(tmp, "logic (file %s)", ptr);
        setWindowTitle(tmp);
        show();
        changed = true;

        return 0;
    } else {
        menu->errmes("Can't open file '%s'!", fname);
        return 1;
    }
}

//***********************************************
int LogEdit::open(int ResNum)
{
    int err = 0;
    QString str;
    FILE *fptr;
    getmaxcol();

    logic->maxcol = maxcol;

    //look for the source file first
    sprintf(tmp, "%s/logic.%03d", game->srcdir.c_str(), ResNum);
    fptr = fopen(tmp, "rb");
    if (fptr == NULL) {
        sprintf(tmp, "%s/logic.%d", game->srcdir.c_str(), ResNum);
        fptr = fopen(tmp, "rb");
    }
    if (fptr == NULL) {
        sprintf(tmp, "%s/logic%d.txt", game->srcdir.c_str(), ResNum);
        fptr = fopen(tmp, "rb");
    }
    if (fptr != NULL) {
        LogicNum = ResNum;
        err = open(tmp);
    } else { //source file not found - reading from the game
        err = logic->decode(ResNum);
        if (!err)
            editor->setText(logic->OutputText.c_str());
        else {
            sprintf(tmp, "logic.%d", ResNum);
            menu->errmes(tmp, "Errors:\n%s", logic->ErrorList.c_str());
        }
        str.sprintf("logic.%d", ResNum);
        setWindowTitle(str);
        LogicNum = ResNum;
        show();
        changed = true;
    }

    return err;
}

//***********************************************
void LogEdit::save(char *fname)
{
    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        menu->errmes("Can't save file '%s'!", fname);
        return;
    }
    file.write(editor->toPlainText().toLatin1());

    changed = false;
    filename = fname;
}

//***********************************************
void LogEdit::save_logic()
{
    if (LogicNum == -1)
        save_as();
    else if (filename != "") {
        save((char *)filename.c_str());
        char *ptr;
        if ((ptr = (char *)strrchr(filename.c_str(), '/')))
            ptr++;
        else
            ptr = (char *)filename.c_str();
        sprintf(tmp, "File %s", ptr);
        setWindowTitle(tmp);
    } else {
        sprintf(tmp, "%s/logic.%03d", game->srcdir.c_str(), LogicNum);
        save(tmp);
        sprintf(tmp, "Logic %d (file)", LogicNum);
        setWindowTitle(tmp);
    }
}

//***********************************************
void LogEdit::save_as()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Logic File"), filename.c_str(), tr("Logic Files (logic*.*);;All Files (*)"));
    if (!fileName.isNull())
        save(fileName.toLatin1().data());
}

//***********************************************
void LogEdit::read_logic()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Read Logic File"), game->srcdir.c_str(), tr("Logic Files (logic*.*);;All Files (*)"));
    if (!fileName.isNull())
        open(fileName.toLatin1().data());
}

//***********************************************
int LogEdit::compile_all_logic()
{
    int ret, err = 0;
    for (int i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == LOGIC) {
            ret = winlist[i].w.l->compile_logic();
            if (ret)
                err = 1;
        }
    }
    return err;
}

//***********************************************
int LogEdit::compile_logic()
{
    QString str;
    byte *s;
    int err, i;
    std::string filename;
    char name[128];
    char tmp1[16], *ptr, *ptr1;

    InputLines.lfree();
    for (i = 0; i < editor->document()->lineCount(); i++) {
        str = editor->document()->findBlockByLineNumber(i).text();
        if (!str.isNull() && str.length() > 0) {
            s = (byte *)str.toLatin1().data();
            if (s[0] < 0x80) //i'm getting \221\005 at the last line...
                InputLines.add((char *)str.toLatin1().data());
        } else
            InputLines.add("");
    }

    for (i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == TEXTRES) {
            if (winlist[i].w.t->filename != "") {
                winlist[i].w.t->save();
                winlist[i].w.t->status->showMessage("");
            }
        }
    }

    err = logic->compile();

    if (!err) {
        status->showMessage("Compiled OK !", 2000);
        if (LogicNum != -1) {
            game->AddResource(LOGIC, LogicNum);
            save_logic();
            changed = false;
        }
    } else {
        if (logic->ErrorList != "") {
            if (LogicNum != -1)
                sprintf(tmp1, "logic.%d", LogicNum);
            else
                sprintf(tmp1, "logic");
            strcpy(tmp, logic->ErrorList.c_str());

            if (!strncmp(tmp, "File ", 5)) {
                ptr = strstr(tmp, "Line ");
                strncpy(name, tmp + 5, (int)(ptr - tmp - 6));
                name[(int)(ptr - tmp - 6)] = 0;
                for (i = 0; i < MAXWIN; i++) {
                    if (winlist[i].type == TEXTRES) {
                        filename = winlist[i].w.t->filename;
                        char *ptr2;
                        if ((ptr2 = (char *)strrchr(filename.c_str(), '/')))
                            ptr2++;
                        else
                            ptr2 = (char *)filename.c_str();
                        if (!strcmp(ptr2, name)) {
                            int num = atoi(ptr + 5);
                            winlist[i].w.t->editor->textCursor().setPosition(num);
                            ptr1 = strchr(ptr, '\n');
                            *ptr1 = 0;
                            winlist[i].w.t->status->showMessage(ptr);
                            break;
                        }
                    }
                }
                if (i >= MAXWIN) {
                    char fullname[256];
                    std::string tmp1 = tmp;
                    sprintf(fullname, "%s/%s", game->srcdir.c_str(), name);
                    for (i = 0; i < MAXWIN; i++) {
                        if (winlist[i].type == -1) {
                            winlist[i].w.t = new TextEdit(NULL, NULL, i);
                            winlist[i].type = TEXTRES;
                            winlist[i].w.t->open(fullname);
                            ptr = (char *)strstr(tmp1.c_str(), "Line ");
                            int num = atoi(ptr + 5);
                            winlist[i].w.t->editor->textCursor().setPosition(num);
                            ptr1 = strchr(ptr, '\n');
                            *ptr1 = 0;
                            winlist[i].w.t->status->showMessage(ptr);
                            break;
                        }
                    }
                }
            } else {
                ptr = strstr(tmp, "Line ");
                int num = atoi(ptr + 5);
                editor->textCursor().setPosition(num);
                ptr1 = strchr(ptr, '\n');
                *ptr1 = 0;
                status->showMessage(tmp);
            }
            menu->errmes(tmp1, "Errors:\n%s", logic->ErrorList.c_str());
        }
    }

    return err;
}

//***********************************************
void LogEdit::compile_and_run()
{
    if (!compile_all_logic())
        menu->run_game();
}

//***********************************************
void LogEdit::change_logic_number()
{
    AskNumber *changelogic = new AskNumber(0, 0, "Logic number", "Enter logic number: [0-255]");

    if (!changelogic->exec())
        return;
    QString str = changelogic->num->text();
    int num = str.toInt();
    if (num < 0 || num > 255) {
        menu->errmes("Logic number must be between 0 and 255 !");
        return ;
    }
    if (game->ResourceInfo[LOGIC][num].Exists) {
        sprintf(tmp, "Resource logic.%d already exists. Replace it ?", num);
        switch (QMessageBox::warning(this, "Logic", tmp,
                                     "Replace", "Cancel",
                                     0,      // Enter == button 0
                                     1)) {   // Escape == button 1
            case 0:
                break;
            case 1:
                return;
        }
    }

    LogicNum = num;
    compile_logic();
    filename = "";
    save_logic();
    if (resources_win) {
        resources_win->select_resource_type(LOGIC);
        resources_win->set_current(num);
    }
    open(num);
}

//***********************************************
void LogEdit::delete_logic()
{
    int k;

    if (LogicNum == -1)
        return;

    sprintf(tmp, "Really delete logic %d ?", LogicNum);
    switch (QMessageBox::warning(this, "Logic", tmp,
                                 "Delete", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            game->DeleteResource(LOGIC, LogicNum);
            delete_file(LogicNum);
            if (resources_win) {
                k = resources_win->list->currentRow();
                resources_win->select_resource_type(LOGIC);
                resources_win->list->setCurrentRow(k);
            }
            changed = false;
            close();
            break;
        case 1:
            break;
    }
}

//***********************************************
void LogEdit::delete_file(int ResNum)
{
    struct stat buf;

    sprintf(tmp, "%s/logic.%03d", game->srcdir.c_str(), ResNum);
    if (stat(tmp, &buf) == 0)
        unlink(tmp);
    sprintf(tmp, "%s/logic.%d", game->srcdir.c_str(), ResNum);
    if (stat(tmp, &buf) == 0)
        unlink(tmp);
    sprintf(tmp, "%s/logic%d.txt", game->srcdir.c_str(), ResNum);
    if (stat(tmp, &buf) == 0)
        unlink(tmp);
}

//***********************************************
void LogEdit::clear_all()
{
    switch (QMessageBox::warning(this, "Logic", "Really clear all ?",
                                 "Clear", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            editor->clear();
            logic->OutputText = "";
            break;
        case 1:
            break;
    }
}

//***********************************************
void LogEdit::new_room()
{
    //  FILE *fptr;

    switch (QMessageBox::warning(this, "Logic", "Replace the editor contents\nwith the new room template ?",
                                 "Replace", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            /*
            sprintf(tmp,"%s/src/newroom.txt",game->templatedir.c_str());
            fptr = fopen(tmp,"rb");
            if(fptr==NULL){
              menu->errmes("Can't open "+string(tmp)+"!");
              return;
            }
            editor->clear();
            char *ptr;
            while(fgets(tmp,MAX_TMP,fptr)!=NULL){
              if((ptr=strchr(tmp,0x0a)))*ptr=0;
              if((ptr=strchr(tmp,0x0d)))*ptr=0;
              editor->insertPlainText(tmp);
            }
            fclose(fptr);
            */
            if (roomgen == NULL)
                roomgen = new RoomGen();
            if (roomgen->exec()) {
                editor->setText(roomgen->text.c_str());
                logic->OutputText = editor->toPlainText().toStdString();
                changed = true;
            }
            break;
        case 1:
            break;
    }

}
//***********************************************
void LogEdit::goto_cb()
{
    AskNumber *ask_number = new AskNumber(0, 0, "Go to line",
                                          "Go to line: ");

    if (!ask_number->exec())
        return;

    QString str = ask_number->num->text();
    int linenum = str.toInt();
    editor->textCursor().setPosition(linenum);
}

//***********************************************
void LogEdit::find_cb()
{
    if (findedit == NULL)
        findedit = new FindEdit(NULL, NULL, editor, status);
    findedit->show();
    findedit->find_field->setFocus();
}

//***********************************************
void LogEdit::find_again()
{
    if (findedit == NULL)
        find_cb();
    else
        findedit->find_next_cb();
}

//***********************************************
void LogEdit::getmaxcol()
//get maximum number of columns on screen (approx.)
//(for formatting the 'print' messages)
{
    // QFontMetrics f = fontMetrics();
    // maxcol = editor->width()/f.width('a');
    maxcol = 50;
}

//***********************************************
void LogEdit::resizeEvent(QResizeEvent *)
{
    QString str = editor->toPlainText();
    getmaxcol();
    editor->setText(str);
}

//***********************************************
void LogEdit::context_help()
{
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    auto word = cursor.selectedText().toLower();

    if (!menu->help_topic(word))
        status->showMessage("No help found for '" + word + "'", 2000);
}

//***********************************************
void LogEdit::command_help()
{
    menu->help_topic("commands_by_category");
}

//***********************************************
void LogEdit::update_line_num(int para, int pos)
{
    QString str;
    QTextStream(&str) << pos << ", " << para;
    status->showMessage(str);
}


//*******************************************************
TextEdit::TextEdit(QWidget *parent, const char *name, int win_num)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Text Editor");

    winnum = win_num;
    editor = new QTextEdit(this);
    editor->setMinimumSize(380, 380);

    QFont font;
    font.setPointSize(10);
    font.setStyleHint(QFont::TypeWriter);
    editor->setFont(font);
    editor->setWordWrapMode(QTextOption::NoWrap);

    QMenu *file = new QMenu(this);
    Q_CHECK_PTR(file);
    file->setTitle("&File");
    file->addAction("&New", this, SLOT(new_text()));
    file->addAction("&Open", this, SLOT(open()));
    file->addAction("&Save", this, SLOT(save()), Qt::CTRL + Qt::Key_S);
    file->addAction("Save &as", this, SLOT(save_as()));
    file->addSeparator();
    file->addAction("&Close", this, SLOT(close()));

    QMenu *edit = new QMenu(this);
    Q_CHECK_PTR(edit);
    edit->setTitle("&Edit");
    edit->addAction("Cu&t", editor, SLOT(cut()), Qt::CTRL + Qt::Key_X);
    edit->addAction("&Copy", editor, SLOT(copy()), Qt::CTRL + Qt::Key_C);
    edit->addAction("&Paste", editor, SLOT(paste()), Qt::CTRL + Qt::Key_V);
    edit->addSeparator();
    edit->addAction("Clea&r all", this, SLOT(clear_all()));
    edit->addSeparator();
    edit->addAction("&Find", this, SLOT(find_cb()), Qt::CTRL + Qt::Key_F);
    edit->addAction("Fi&nd again", this, SLOT(find_again()), Qt::Key_F3);

    QMenuBar *menu = new QMenuBar(this);
    Q_CHECK_PTR(menu);
    menu->addMenu(file);
    menu->addMenu(edit);
    setMinimumSize(640, 480);

    QBoxLayout *all = new QVBoxLayout(this);
    all->setMenuBar(menu);

    all->addWidget(editor);

    status = new QStatusBar(this);
    QLabel *msg = new QLabel("");
    status->addWidget(msg, 4);
    all->addWidget(status);

    changed = false;
    filename = "";
    findedit = NULL;
    OutputText = "";
}

//***********************************************
void TextEdit::deinit()
{
    if (findedit) {
        findedit->close();
        findedit = NULL;
    }
    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void TextEdit::hideEvent(QHideEvent *)
{
    if (findedit) {
        findedit->close();
        findedit = NULL;
    }
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void TextEdit::showEvent(QShowEvent *)
{
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//***********************************************
void TextEdit::closeEvent(QCloseEvent *e)
{
    if (changed) {
        QString str = editor->toPlainText();
        if (!strcmp(str.toLatin1(), OutputText.c_str())) { //not changed
            deinit();
            e->accept();
            return;
        }

        if (filename != "")
            sprintf(tmp, "Do you want to save changes to\n%s ?", filename.c_str());
        else
            strcpy(tmp, "Do you want to save changes ?");
        switch (QMessageBox::warning(this, "Text editor",
                                     tmp,
                                     "Yes",
                                     "No",
                                     "Cancel",
                                     0, 2)) {
            case 0: // yes
                save();
                deinit();
                e->accept();
                break;
            case 1: // no
                deinit();
                e->accept();
                break;
            default: // cancel
                e->ignore();
                break;
        }
    } else {
        deinit();
        e->accept();
    }
}

//***********************************************
void TextEdit::new_text()
{
    if (filename.length() > 0) {
        //if (changed)
        sprintf(tmp, "Do you want to save changes to\n%s ?", filename.c_str());
        switch (QMessageBox::warning(this, "Text editor",
                                     tmp,
                                     "Yes",
                                     "No",
                                     "Cancel",
                                     0, 2)) {
            case 0: // yes
                save(); //???????????????
                break;
            case 1: // no
                break;
            default: // cancel
                break;
        }
    }

    filename = "";
    setWindowTitle("New text");
    editor->clear();
    show();
    changed = true;
    OutputText = "";
}

//***********************************************
void TextEdit::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Text File"), game->srcdir.c_str(), tr("All Files (*)"));
    if (!fileName.isNull()) {
        open(fileName.toLatin1().data());
        show();
        changed = true;
    }
}

//***********************************************
int TextEdit::open(char *fname)
{
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        menu->errmes("Can't open file '%s'!", fname);
        return 1;
    }

    QByteArray contents = file.readAll();
    editor->setText(contents);
    OutputText = editor->toPlainText().toStdString();

    filename = fname;
    setWindowTitle(file.fileName());
    show();
    changed = true;
    return 0;
}

//***********************************************
void TextEdit::save()
{
    if (filename == "")
        save_as();
    else
        save(filename.c_str());
}

//***********************************************
void TextEdit::save_as()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Text File"), filename.c_str(), tr("All Files (*)"));
    if (!fileName.isNull())
        save(fileName.toLatin1());
}

//***********************************************
void TextEdit::save(const char *fname)
{
    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        menu->errmes("Can't save file '%s'!", fname);
        return;
    }
    file.write(editor->toPlainText().toLatin1());

    changed = false;
    filename = fname;
    setWindowTitle(file.fileName());
}

//***********************************************
void TextEdit::clear_all()
{
    switch (QMessageBox::warning(this, "Text", "Really clear all ?",
                                 "Clear", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            editor->clear();
            break;
        case 1:
            break;
    }
}

//***********************************************
void TextEdit::find_cb()
{
    if (findedit == NULL)
        findedit = new FindEdit(NULL, NULL, editor, status);
    findedit->show();
    findedit->find_field->setFocus();
}

//***********************************************
void TextEdit::find_again()
{
    if (findedit == NULL)
        find_cb();
    else
        findedit->find_next_cb();
}

//***********************************************
FindEdit::FindEdit(QWidget *parent, const char *name, QTextEdit *edit, QStatusBar *s)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Find");
    setMinimumSize(340, 140);

    status = s;
    editor = edit;

    QBoxLayout *all =  new QVBoxLayout(this);

    QBoxLayout *txt = new QHBoxLayout(this);
    all->addLayout(txt);

    QLabel *label = new QLabel("Find what:", this);
    txt->addWidget(label);

    find_field = new QLineEdit(this);
    find_field->setMinimumWidth(200);
    connect(find_field, SIGNAL(returnPressed()), SLOT(find_first_cb()));
    txt->addWidget(find_field);

    QBoxLayout *left1 =  new QHBoxLayout(this);
    all->addLayout(left1);

    QVBoxLayout *dirbox = new QVBoxLayout();
    QGroupBox *direction = new QGroupBox("Direction", this);
    up = new QRadioButton("Up", direction);
    down = new QRadioButton("Down", direction);
    down->setChecked(true);
    dirbox->addWidget(up);
    dirbox->addWidget(down);
    left1->addWidget(direction);
    direction->setLayout(dirbox);

    QVBoxLayout *frombox = new QVBoxLayout();
    QGroupBox *from = new QGroupBox("From", this);
    start = new QRadioButton("Start", from);
    current = new QRadioButton("Current", from);
    start->setChecked(true);
    frombox->addWidget(start);
    frombox->addWidget(current);
    left1->addWidget(from);
    from->setLayout(frombox);

    QVBoxLayout *typebox = new QVBoxLayout();
    QGroupBox *type = new QGroupBox("Match", this);
    match_whole = new QCheckBox("Match exact", type);
    match_case = new QCheckBox("Match case", type);
    typebox->addWidget(match_whole);
    typebox->addWidget(match_case);
    left1->addWidget(type);
    type->setLayout(typebox);


    QBoxLayout *right =  new QVBoxLayout(this);
    left1->addLayout(right);
    find_first = new QPushButton("Find", this);
    right->addWidget(find_first);
    connect(find_first, SIGNAL(clicked()), SLOT(find_first_cb()));
    find_next = new QPushButton("Find next", this);
    connect(find_next, SIGNAL(clicked()), SLOT(find_next_cb()));
    right->addWidget(find_next);
    cancel = new QPushButton("Cancel", this);
    connect(cancel, SIGNAL(clicked()), SLOT(cancel_cb()));
    right->addWidget(cancel);

    adjustSize();
    curline = 0;
}

//***********************************************
void FindEdit::find_first_cb()
{
    if (current->isChecked())
        curline = editor->textCursor().position();
    else if (down->isChecked())
        curline = 0;
    else
        curline = editor->toPlainText().length();

    find_next_cb();
}

//***********************************************
void FindEdit::find_next_cb()
{
    QTextDocument::FindFlags flags;
    if (match_whole->isChecked())
        flags |= QTextDocument::FindWholeWords;
    if (match_case->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (!down->isChecked())
        flags |= QTextDocument::FindBackward;

    if (!editor->find(find_field->text(), flags))
        menu->errmes("Find", "'%s' not found!", find_field->text().toStdString().c_str());
    status->showMessage("");
}

//***********************************************
void FindEdit::cancel_cb()
{
    hide();
    status->clearMessage();
}
//***********************************************
