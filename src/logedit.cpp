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


#include <QFileDialog>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

#include "agicommands.h"
#include "game.h"
#include "logedit.h"
#include "menu.h"
#include "resources.h"
#include "roomgen.h"


QStringList InputLines;     // Temporary buffer for reading the text from editor
                            //  and sending to compilation.

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
        rule.pattern = QRegularExpression("[" + operators + "]");
        rule.format = operatorFormat;
        highlightingRules.append(rule);

        keywordFormat.setForeground(keyword_color);
        keywordFormat.setFontWeight(QFont::Bold);
        foreach (const QString &pattern, keywords) {
            rule.pattern = QRegularExpression("\\b" + pattern + "\\b");
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }
        rule.pattern = QRegularExpression("^#\\w+");
        rule.format = keywordFormat;
        highlightingRules.append(rule);

        testCmdFormat.setForeground(test_color);
        testCmdFormat.setFontItalic(true);
        for (size_t i = 0; i < NumTestCommands; i++) {
            auto cmd = QString(TestCommand[i].Name);
            if (!cmd.isEmpty()) {
                cmd = cmd.replace(".", "\\.");
                rule.pattern = QRegularExpression("\\b" + cmd + "\\b");
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
                rule.pattern = QRegularExpression("\\b" + cmd + "\\b");
                rule.format = AGICmdFormat;
                highlightingRules.append(rule);
            }
        }

        numberFormat.setForeground(number_color);
        rule.pattern = QRegularExpression("\\d+");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        stringFormat.setForeground(string_color);
        rule.pattern = QRegularExpression("\".*\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        singleLineCommentFormat.setForeground(comment_color);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = singleLineCommentFormat;
        highlightingRules.append(rule);

        multiLineCommentFormat.setForeground(comment_color);
        commentStartExpression = QRegularExpression("/\\*");
        commentEndExpression = QRegularExpression("\\*/");
    }

    void highlightBlock(const QString &text) override
    {
        foreach (const HighlightingRule &rule, highlightingRules) {
            QRegularExpression expression(rule.pattern);
            auto match = expression.match(text);
            int index = match.capturedStart();
            while (index >= 0) {
                int length = match.capturedLength();
                setFormat(index, length, rule.format);
                index = expression.match(text, index + length).capturedStart();
            }
        }
        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1)
            startIndex = commentStartExpression.match(text).capturedStart();

        while (startIndex >= 0) {
            auto match = commentEndExpression.match(text, startIndex);
            int endIndex = match.capturedStart();
            int commentLength;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex
                                + match.capturedLength();
            }
            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = commentStartExpression.match(text, startIndex + commentLength).capturedStart();
        }
    }

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

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
    : QMainWindow(parent), findedit(nullptr), roomgen(nullptr), winnum(win_num), resources_win(res),
      changed(false), filename(), LogicNum(0), maxcol(0)
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Logic Editor");
    setWindowFlags(Qt::Widget);

    logic = new Logic();

    QFont font("Monospace");
    font.setPointSize(readonly ? 8 : 9);
    font.setStyleHint(QFont::TypeWriter);
    textEditor->setFont(font);

    syntax_hl = new LogicSyntaxHL(textEditor);

    if (readonly) {
        textEditor->setReadOnly(readonly);
        menuBar()->setVisible(false);
        statusBar()->setVisible(false);
    } else {
        actionNew->setDisabled(true);

        connect(actionOpen, &QAction::triggered, this, &LogEdit::read_logic);
        connect(actionSave, &QAction::triggered, this, &LogEdit::save_logic);
        connect(actionSaveAs, &QAction::triggered, this, &LogEdit::save_as);
        connect(actionClose, &QAction::triggered, this, &LogEdit::close);

        connect(actionClear, &QAction::triggered, this, &LogEdit::clear_all);
        connect(actionFind, &QAction::triggered, this, &LogEdit::find_cb);
        connect(actionFindNext, &QAction::triggered, this, &LogEdit::find_again);
        connect(actionGotoLine, &QAction::triggered, this, &LogEdit::goto_cb);

        connect(actionCompile, &QAction::triggered, this, &LogEdit::compile_logic);
        connect(actionCompileAll, &QAction::triggered, this, &LogEdit::compile_all_logic);
        connect(actionCompileAllandRun, &QAction::triggered, this, &LogEdit::compile_and_run);
        connect(actionChangeLogicNumber, &QAction::triggered, this, &LogEdit::change_logic_number);
        connect(actionNewRoom, &QAction::triggered, this, &LogEdit::new_room);
        connect(actionDeleteRoom, &QAction::triggered, this, &LogEdit::delete_logic);

        connect(actionContextHelp, &QAction::triggered, this, &LogEdit::context_help);
        connect(actionAllCommands, &QAction::triggered, this, &LogEdit::command_help);
        
        connect(textEditor, &QTextEdit::cursorPositionChanged, this, &LogEdit::update_line_num);
    }

    getmaxcol();
    hide();
}

//***********************************************
void LogEdit::deinit()
{
    if (findedit) {
        findedit->close();
        findedit = nullptr;
    }
    if (roomgen) {
        roomgen->close();
        roomgen = nullptr;
    }
    delete logic;
    logic = nullptr;
    delete syntax_hl;
    syntax_hl = nullptr;

    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void LogEdit::hideEvent(QHideEvent *)
{
    if (findedit) {
        findedit->close();
        findedit = nullptr;
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
        if (textEditor->toPlainText().toStdString() == logic->OutputText) { // Not actually changed
            deinit();
            e->accept();
            return;
        }

        switch (QMessageBox::warning(this, tr("Logic Editor"), tr("Save changes to Logic script?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel)) {
            case QMessageBox::Save:
                save_logic();
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
int LogEdit::open(const std::string &filepath)
{
    getmaxcol();

    QFile file(filepath.c_str());
    if (file.open(QIODeviceBase::ReadOnly)) {
        filename = filepath;
        textEditor->clear();

        QTextStream in(&file);
        QString filecont;
        while (!in.atEnd())
            filecont += in.readLine() + '\n';
        textEditor->setText(filecont);
        file.close();

        logic->OutputText = textEditor->toPlainText().toStdString();
        
        QString fname = QFileInfo(filepath.c_str()).fileName();
        if (LogicNum != -1)
            setNewTitle(QString("logic.%1 (file %2)").arg(QString::number(LogicNum)).arg(fname));
        else
            setNewTitle(QString("logic (file %1)").arg(fname));
        show();
        changed = true;

        return 0;
    } else {
        menu->errmes("Can't open file '%s'!", filepath);
        return 1;
    }
}

//***********************************************
int LogEdit::open(int ResNum)
{
    int err = 0;
    getmaxcol();
    logic->maxcol = maxcol;

    // Look for a valid source file first
    QStringList test_paths = {
        QString("%1/logic.%2").arg(game->srcdir.c_str()).arg(QString::number(ResNum), 3, '0'),
        QString("%1/logic.%2").arg(game->srcdir.c_str()).arg(QString::number(ResNum)),
        QString("%1/logic%2.txt").arg(game->srcdir.c_str()).arg(QString::number(ResNum))
    };

    std::string source_file = "";
    for (auto & test_path : test_paths)
        if (QFile::exists(test_path))
            source_file = test_path.toStdString();

    if (!source_file.empty()) {
        LogicNum = ResNum;
        err = open(source_file);
    } else { // Source file not found - reading from the game
        err = logic->decode(ResNum);
        if (!err)
            textEditor->setText(logic->OutputText.c_str());
        else
            menu->errmes(QString("logic.%1 Errors:\n%2").arg(QString::number(ResNum), 3, '0').arg(logic->ErrorList.c_str()).toStdString().c_str());

        setNewTitle(QString("logic.%1").arg(QString::number(ResNum), 3, '0'));
        LogicNum = ResNum;
        show();
        changed = true;
    }

    return err;
}

//***********************************************
void LogEdit::save(const std::string &fname)
{
    QFile file(fname.c_str());
    if (!file.open(QIODevice::WriteOnly)) {
        menu->errmes("Can't save file '%s'!", fname);
        return;
    }
    file.write(textEditor->toPlainText().toStdString().c_str());

    changed = false;
    filename = fname;
}

//***********************************************
void LogEdit::save_logic()
{
    if (LogicNum == -1)
        save_as();
    else if (!filename.empty()) {
        save(filename);
        setNewTitle(QString("File %1").arg(QFileInfo(filename.c_str()).fileName()));
    } else {
        save(QString("%1/logic.%2").arg(game->srcdir.c_str()).arg(QString::number(LogicNum), 3, '0').toStdString());
        setNewTitle(QString("Logic %1 (file)").arg(QString::number(LogicNum), 3, '0'));
    }
}

//***********************************************
void LogEdit::save_as()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Logic File"), filename.c_str(), tr("Logic Files (logic*.*);;All Files (*)"));
    if (!fileName.isNull())
        save(fileName.toStdString());
}

//***********************************************
void LogEdit::read_logic()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Read Logic File"), game->srcdir.c_str(), tr("Logic Files (logic*.*);;All Files (*)"));
    if (!fileName.isNull())
        open(fileName.toStdString());
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
    int err, i;

    InputLines.clear();
    for (i = 0; i < textEditor->document()->lineCount(); i++) {
        QString str = textEditor->document()->findBlockByLineNumber(i).text();
        if (!str.isNull() && str.length() > 0) {
            if (str.at(0) < QChar(0x80)) //i'm getting \221\005 at the last line...
                InputLines.append(str);
        } else
            InputLines.append("");
    }

    for (i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == TEXTRES) {
            if (!winlist[i].w.t->filename.empty()) {
                winlist[i].w.t->save();
                winlist[i].w.t->statusBar()->clearMessage();
            }
        }
    }

    err = logic->compile();

    if (!err) {
        statusBar()->showMessage("Compiled OK!");
        if (LogicNum != -1) {
            game->AddResource(LOGIC, LogicNum);
            save_logic();
            changed = false;
        }
    } else {
        if (!logic->ErrorList.empty()) {
            std::string logic_name;
            if (LogicNum != -1)
                logic_name = std::format("logic.{:03}", LogicNum);
            else
                logic_name = "logic";

            if (logic->ErrorList.starts_with("File ")) {
                // Capture Filename, Line Number, and Error Message
                QRegularExpression re(R"(File ([\w\d.]+) Line ([\d]+): (.+))");
                QRegularExpressionMatch match = re.match(logic->ErrorList.c_str());

                std::string error_filename = match.capturedTexts()[0].toStdString();
                for (i = 0; i < MAXWIN; i++) {
                    if (winlist[i].type == TEXTRES) {
                        std::string window_file = QFileInfo(winlist[i].w.t->filename.c_str()).fileName().toStdString();
                        if (window_file == error_filename) {
                            winlist[i].w.t->setPosition(match.capturedTexts()[1].toInt());
                            winlist[i].w.t->statusBar()->showMessage(match.capturedTexts()[2]);
                            break;
                        }
                    }
                }
                if (i >= MAXWIN) {
                    auto fullname = game->srcdir + "/" + logic_name;
                    for (i = 0; i < MAXWIN; i++) {
                        if (winlist[i].type == -1) {
                            winlist[i].w.t = new TextEdit(nullptr, nullptr, i);
                            winlist[i].type = TEXTRES;
                            winlist[i].w.t->open(fullname);
                            winlist[i].w.t->setPosition(match.capturedTexts()[1].toInt());
                            winlist[i].w.t->statusBar()->showMessage(match.capturedTexts()[2]);
                            break;
                        }
                    }
                }
            } else {
                // Capture Line Number and Error Message
                QRegularExpression re(R"(Line ([\d]+): (.+))");
                QRegularExpressionMatch match = re.match(logic->ErrorList.c_str());
                textEditor->textCursor().setPosition(match.capturedTexts()[0].toInt());
                statusBar()->showMessage(match.capturedTexts()[1]);
            }
            QMessageBox::critical(this, logic_name.c_str(), std::format("Errors:\n\n{}", logic->ErrorList.c_str()).c_str());
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
    bool ok;
    int num = QInputDialog::getInt(this, tr("Logic Number"), tr("Enter logic number [0-255]:"), LogicNum, 0, 255, 1, &ok);
    if (!ok)
        return;

    if (game->ResourceInfo[LOGIC][num].Exists) {
        switch (QMessageBox::warning(this, tr("Logic Editor"),
                                     tr("Resource logic.%1 already exists. Replace it?").arg(QString::number(num), 3, '0'),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No)) {
            case QMessageBox::Yes:
                break;
            case QMessageBox::No:
                return;
        }
    }

    LogicNum = num;
    compile_logic();
    filename.clear();
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
    if (LogicNum == -1)
        return;

    switch (QMessageBox::warning(this, tr("Logic Editor"), tr("Really delete logic %1?").arg(QString::number(LogicNum), 3, '0'),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            game->DeleteResource(LOGIC, LogicNum);
            delete_file(LogicNum);
            if (resources_win) {
                int k = resources_win->list->currentRow();
                resources_win->select_resource_type(LOGIC);
                resources_win->list->setCurrentRow(k);
            }
            changed = false;
            close();
            break;
        default:
            break;
    }
}

//***********************************************
void LogEdit::delete_file(int ResNum)
{
    QStringList test_paths = {
        QString("%1/logic.%2").arg(game->srcdir.c_str()).arg(QString::number(ResNum), 3, '0'),
        QString("%1/logic.%2").arg(game->srcdir.c_str()).arg(QString::number(ResNum)),
        QString("%1/logic%2.txt").arg(game->srcdir.c_str()).arg(QString::number(ResNum))
    };

    for (auto &test_path : test_paths)
        if (QFile::exists(test_path))
            QFile::remove(test_path);
}

//***********************************************
void LogEdit::clear_all()
{
    switch (QMessageBox::warning(this, tr("Logic Editor"), tr("Really clear all?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            textEditor->clear();
            logic->OutputText = "";
            break;
        default:
            break;
    }
}

//***********************************************
void LogEdit::new_room()
{
    switch (QMessageBox::warning(this, tr("Logic Editor"), tr("Replace the editor contents\nwith the new room template?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            if (roomgen == nullptr)
                roomgen = new RoomGen();

            if (roomgen->exec()) {
                textEditor->setText(roomgen->text.c_str());
                logic->OutputText = textEditor->toPlainText().toStdString();
                changed = true;
            }
            break;
        default:
            break;
    }
}

//***********************************************
void LogEdit::goto_cb()
{
    auto cursor = textEditor->textCursor();

    bool ok;
    int linenum = QInputDialog::getInt(this, tr("Go to line"), tr("Go to line:"),
                                       cursor.blockNumber(), 1, textEditor->document()->blockCount(), ok = &ok);
    if (!ok)
        return;

    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, linenum);
    textEditor->setTextCursor(cursor);
}

//***********************************************
void LogEdit::find_cb()
{
    if (findedit == nullptr)
        findedit = new FindEdit(this, nullptr, textEditor);
    findedit->show();
    findedit->focusEditLine();
}

//***********************************************
void LogEdit::find_again()
{
    if (findedit == nullptr)
        find_cb();
    else
        findedit->find_next_cb();
}

//***********************************************
void LogEdit::getmaxcol()
// Get maximum number of columns on screen (approx.)
// (for formatting the 'print' messages)
{
    QFontMetrics f = fontMetrics();
    maxcol = textEditor->width() / f.horizontalAdvance('a');
}

//***********************************************
void LogEdit::resizeEvent(QResizeEvent *)
{
    QString str = textEditor->toPlainText();
    getmaxcol();
    textEditor->setText(str);
}

//***********************************************
void LogEdit::context_help()
{
    QTextCursor cursor = textEditor->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    auto word = cursor.selectedText().toLower();

    if (!menu->help_topic(word))
        statusBar()->showMessage("No help found for '" + word + "'", 2000);
}

//***********************************************
void LogEdit::command_help()
{
    menu->help_topic("commands_by_category");
}

//***********************************************
void LogEdit::update_line_num()
{
    QString str;
    QTextStream(&str) << textEditor->textCursor().positionInBlock() << ", " << textEditor->textCursor().blockNumber();
    statusBar()->showMessage(str);
}

//***********************************************
void LogEdit::setNewTitle(const QString &newtitle)
{
    setWindowTitle("Logic Editor - " + newtitle);
}


//*******************************************************
TextEdit::TextEdit(QWidget *parent, const char *name, int win_num)
    : QMainWindow(parent), findedit(nullptr), winnum(win_num),
    changed(false), filename(), OutputText()
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Text Editor");

    QFont font;
    font.setPointSize(10);
    font.setStyleHint(QFont::TypeWriter);
    textEditor->setFont(font);

    menuLogic->setDisabled(true);
    actionGotoLine->setDisabled(true);

    connect(actionNew, &QAction::triggered, this, &TextEdit::new_text);
    connect(actionOpen, &QAction::triggered, this, qOverload<>(&TextEdit::open));
    connect(actionSave, &QAction::triggered, this, qOverload<>(&TextEdit::save));
    connect(actionSaveAs, &QAction::triggered, this, &TextEdit::save_as);
    connect(actionClose, &QAction::triggered, this, &TextEdit::close);

    connect(actionClear, &QAction::triggered, this, &TextEdit::clear_all);
    connect(actionFind, &QAction::triggered, this, &TextEdit::find_cb);
    connect(actionFindNext, &QAction::triggered, this, &TextEdit::find_again);
}

//***********************************************
void TextEdit::deinit()
{
    if (findedit) {
        findedit->close();
        findedit = nullptr;
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
        findedit = nullptr;
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
        if (textEditor->toPlainText().toStdString() == OutputText) { // Not actually changed
            deinit();
            e->accept();
            return;
        }

        switch (QMessageBox::warning(this, tr("Text Editor"), tr("Do you want to save changes?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel)) {
            case QMessageBox::Save:
                save();
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

//***********************************************
void TextEdit::new_text()
{
    if (filename.length() > 0) {
        switch (QMessageBox::warning(this, tr("Text Editor"), tr("Do you want to save changes to\n%1?").arg(filename.c_str()),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel)) {
            case QMessageBox::Save:
                save();
                break;
            case QMessageBox::Discard:
                break;
            default: // Cancel
                break;
        }
    }

    filename.clear();
    setNewTitle(QString("(Untitled Text File)"));
    textEditor->clear();
    show();
    changed = true;
    OutputText.clear();
}

//***********************************************
void TextEdit::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Text File"), game->srcdir.c_str(), tr("All Files (*)"));
    if (!fileName.isNull()) {
        open(fileName.toStdString());
        show();
        changed = true;
    }
}

//***********************************************
int TextEdit::open(const std::string &fname)
{
    QFile file(fname.c_str());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        menu->errmes("Can't open file '%s'!", fname);
        return 1;
    }

    QByteArray contents = file.readAll();
    textEditor->setText(contents);
    OutputText = textEditor->toPlainText().toStdString();

    filename = fname;
    setNewTitle(file.fileName());
    show();
    changed = true;

    return 0;
}

//***********************************************
void TextEdit::save()
{
    if (filename.empty())
        save_as();
    else
        save(filename);
}

//***********************************************
void TextEdit::save_as()
{
    QString saveFile = QFileDialog::getSaveFileName(this, tr("Save Text File"), filename.c_str(), tr("All Files (*)"));
    if (!saveFile.isEmpty())
        save(saveFile.toStdString());
}

//***********************************************
void TextEdit::save(const std::string &fname)
{
    QFile file(fname.c_str());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        menu->errmes("Can't save file '%s'!", fname);
        return;
    }
    file.write(textEditor->toPlainText().toStdString().c_str());

    changed = false;
    filename = fname;
    setNewTitle(file.fileName());
}

//***********************************************
void TextEdit::setPosition(int newpos)
{
    textEditor->textCursor().setPosition(newpos);
}

//***********************************************
void TextEdit::clear_all()
{
    switch (QMessageBox::warning(this, tr("Text Editor"), tr("Really clear all?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            textEditor->clear();
            break;
        default:
            break;
    }
}

//***********************************************
void TextEdit::find_cb()
{
    if (findedit == nullptr)
        findedit = new FindEdit(this, nullptr, textEditor);
    findedit->show();
    findedit->focusEditLine();
}

//***********************************************
void TextEdit::find_again()
{
    if (findedit == nullptr)
        find_cb();
    else
        findedit->find_next_cb();
}

//***********************************************
void TextEdit::setNewTitle(const QString &newtitle)
{
    setWindowTitle("Text Editor - " + newtitle);
}


//***********************************************
FindEdit::FindEdit(QWidget *parent, const char *name, QTextEdit *edit)
    : QMainWindow(parent), editor(edit), curline(0)
{
    setupUi(this);

    setWindowTitle("Find");

    radioButtonMatchSubstr->setVisible(false);
    connect(lineFind, &QLineEdit::returnPressed, this, &FindEdit::find_first_cb);
    connect(buttonFindNext, &QPushButton::clicked, this, &FindEdit::find_next_cb);
}

//***********************************************
void FindEdit::find_first_cb()
{
    if (radioButtonFromCurrent->isChecked())
        curline = editor->textCursor().position();
    else if (radioButtonDirDown->isChecked())
        curline = 0;
    else
        curline = editor->toPlainText().length();

    find_next_cb();
}

//***********************************************
void FindEdit::find_next_cb()
{
    QTextDocument::FindFlags flags;
    if (radioButtonMatchExact->isChecked())
        flags |= QTextDocument::FindWholeWords;
    if (radioButtonMatchCase->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (!radioButtonDirDown->isChecked())
        flags |= QTextDocument::FindBackward;

    if (!editor->find(lineFind->text(), flags))
        statusBar()->showMessage(QString("'%1' not found!").arg(lineFind->text()));
    else
        statusBar()->clearMessage();
}

//***********************************************
void FindEdit::cancel_cb()
{
    hide();
    statusBar()->clearMessage();
}
//***********************************************
