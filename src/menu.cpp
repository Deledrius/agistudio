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


#include <cstdarg>
#include <vector>

#include <QActionGroup>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTextEdit>

#include "helpwindow.h"
#include "logedit.h"
#include "menu.h"
#include "objedit.h"
#include "options.h"
#include "picedit.h"
#include "preview.h"
#include "resources.h"
#include "viewedit.h"
#include "wordsedit.h"
#include "wutil.h"


Menu *menu;
WindowList *window_list;
static About *about;
WinList winlist[MAXWIN];  //list of all open resource editor windows

//*************************************************
Menu::Menu(QWidget *parent, const char *name)
    : QMainWindow(parent), resources_win(nullptr), num_res(0)
{
    setupUi(this);

    // Sadly, Action Groups are not exposed in QtDesigner...
    activeGameGroup = new QActionGroup(this);
    activeGameGroup->addAction(actionGameClose);
    activeGameGroup->addAction(actionGameRun);
    activeGameGroup->addAction(actionResNewWindow);
    activeGameGroup->addAction(actionResAdd);
    activeGameGroup->addAction(actionResExtract);
    activeGameGroup->addAction(actionResDelete);
    activeGameGroup->addAction(actionResRenumber);
    activeGameGroup->addAction(actionResRebuildVOLFiles);
    activeGameGroup->addAction(actionResRecompileAll);
    activeGameGroup->addAction(actionToolsViewEditor);
    activeGameGroup->addAction(actionToolsLogicEditor);
    activeGameGroup->addAction(actionToolsTextEditor);
    activeGameGroup->addAction(actionToolsObjectEditor);
    activeGameGroup->addAction(actionToolsWordTokensEditor);
    activeGameGroup->addAction(actionToolsPictureEditor);
    activeGameGroup->addAction(actionToolsSoundPlayer);

    activeGameGroup->addAction(actionCloseGameButton);
    activeGameGroup->addAction(actionRunGameButton);
    activeGameGroup->addAction(actionViewEditorButton);
    activeGameGroup->addAction(actionLogicEditorButton);
    activeGameGroup->addAction(actionObjectEditorButton);
    activeGameGroup->addAction(actionWordTokensEditorButton);
    activeGameGroup->addAction(actionPictureEditorButton);

    resActionGroup = new QActionGroup(this);
    resActionGroup->addAction(actionResAdd);
    resActionGroup->addAction(actionResExtract);
    resActionGroup->addAction(actionResDelete);
    resActionGroup->addAction(actionResRenumber);

    err = new QMessageBox(QMessageBox::Critical, tr("AGI Studio"), tr(""));
    warn = new QMessageBox(QMessageBox::Warning, tr("AGI Studio"), tr(""));

    // Menu signal handlers
    connect(actionNewBlank, &QAction::triggered, this, &Menu::blank);
    connect(actionNewFromTemplate, &QAction::triggered, this, &Menu::from_template);
    connect(actionGameOpen, &QAction::triggered, this, &Menu::open_game);
    connect(actionGameClose, &QAction::triggered, this, &Menu::close_game);
    connect(actionGameRun, &QAction::triggered, this, &Menu::run_game);
    connect(actionAppSettings, &QAction::triggered, this, &Menu::options);
    connect(actionAppQuit, &QAction::triggered, this, &Menu::close);

    connect(actionResNewWindow, &QAction::triggered, this, &Menu::new_resource_window);
    connect(actionResAdd, &QAction::triggered, this, &Menu::add_resource);
    connect(actionResExtract, &QAction::triggered, this, &Menu::extract_resource);
    connect(actionResDelete, &QAction::triggered, this, &Menu::delete_resource);
    connect(actionResRenumber, &QAction::triggered, this, &Menu::renumber_resource);
    connect(actionResRebuildVOLFiles, &QAction::triggered, this, &Menu::rebuild_vol);
    connect(actionResRecompileAll, &QAction::triggered, this, &Menu::recompile_all);

    connect(actionToolsViewEditor, &QAction::triggered, this, &Menu::view_editor);
    connect(actionToolsLogicEditor, &QAction::triggered, this, &Menu::logic_editor);
    connect(actionToolsTextEditor, &QAction::triggered, this, &Menu::text_editor);
    connect(actionToolsObjectEditor, &QAction::triggered, this, &Menu::object_editor);
    connect(actionToolsWordTokensEditor, &QAction::triggered, this, &Menu::words_editor);
    connect(actionToolsPictureEditor, &QAction::triggered, this, &Menu::picture_editor);
    connect(actionToolsSoundPlayer, &QAction::triggered, this, &Menu::sound_player);

    connect(actionWindowSaveAll, &QAction::triggered, this, &Menu::save_all);
    connect(actionWindowSaveAllandRun, &QAction::triggered, this, &Menu::save_and_run);
    connect(actionWindowList, &QAction::triggered, this, &Menu::window_list_cb);

    connect(actionHelpContents, &QAction::triggered, this, &Menu::help_contents);
    connect(actionHelpIndex, &QAction::triggered, this, &Menu::help_index);
    connect(actionHelpAboutQt, &QAction::triggered, this, &Menu::about_qt);
    connect(actionHelpAboutAGIStudio, &QAction::triggered, this, &Menu::about_it);

    // Toolbar signal handlers
    connect(actionOpenGameButton, &QAction::triggered, this, &Menu::open_game);
    connect(actionCloseGameButton, &QAction::triggered, this, &Menu::close_game);
    connect(actionRunGameButton, &QAction::triggered, this, &Menu::run_game);
    connect(actionViewEditorButton, &QAction::triggered, this, &Menu::view_editor);
    connect(actionLogicEditorButton, &QAction::triggered, this, &Menu::logic_editor);
    connect(actionTextEditorButton, &QAction::triggered, this, &Menu::text_editor);
    connect(actionObjectEditorButton, &QAction::triggered, this, &Menu::object_editor);
    connect(actionWordTokensEditorButton, &QAction::triggered, this, &Menu::words_editor);
    connect(actionPictureEditorButton, &QAction::triggered, this, &Menu::picture_editor);

    disable_game_actions();

    for (int i = 0; i < MAXWIN; i++)
        winlist[i].type = -1;
    make_egacolors();

    window_list = nullptr;
}

//**********************************************
int get_win()
{
    int n;

    for (n = 0; n < MAXWIN; n++) {
        if (winlist[n].type == -1)
            break;
    }
    if (n == MAXWIN) {
        menu->errmes("Too many open windows!");
        return -1;
    }
    return n;
}

//**********************************************
void Menu::disable_game_actions()
{
    activeGameGroup->setDisabled(true);
}

//**********************************************
void Menu::enable_game_actions()
{
    activeGameGroup->setEnabled(true);
}

//**********************************************
void Menu::open_game()
{
    OpenGameDir(nullptr, false);
}

//**********************************************
void Menu::show_resources()
{
    if (resources_win == nullptr) {
        int n;
        if ((n = get_win()) == -1)
            return;
        resources_win = new ResourcesWin(nullptr, "Resources", n);
        winlist[n].type = RESOURCES;
        winlist[n].w.r = resources_win;
        inc_res(resources_win);
    }
    resources_win->select_resource_type(game->settings->value("DefaultResourceType").toInt());
    resources_win->show();
    enable_game_actions();
}

//**********************************************
void Menu::inc_res(ResourcesWin *res)
{
    num_res++;
    if (num_res == 1) {
        resources_win = res;
        enable_resources();
    } else {
        resources_win = NULL;
        disable_resources();
    }
}

//**********************************************
void Menu::dec_res()
{
    num_res--;
    if (num_res == 1) {
        for (int i = 0; i < MAXWIN; i++) {
            if (winlist[i].type == RESOURCES) {
                resources_win = winlist[i].w.r;
                break;
            }
        }
        enable_resources();
    } else {
        resources_win = NULL;
        disable_resources();
    }
}

//**********************************************
void Menu::showStatusMessage(const QString &msg)
{
    statusBarMainMenu->showMessage(msg);
}

//**********************************************
void Menu::enable_resources()
{
    resActionGroup->setEnabled(true);
}

//**********************************************
void Menu::disable_resources()
{
    resActionGroup->setDisabled(true);
}

//**********************************************
void Menu::quit_cb()
{
    close();
}

//**********************************************
void Menu::closeEvent(QCloseEvent *e)
{
    if (game->isOpen) {
        close_game();
        if (game->isOpen) {
            e->ignore();
            return;
        }
    }
    e->accept();
}

//**********************************************
void Menu::close_game()
{
    int i;

    //close all open windows (they will ask to save if something was changed)
    for (i = 0; i < MAXWIN; i++) {
        if (winlist[i].type != -1) {
            switch (winlist[i].type) {
                case LOGIC:
                    winlist[i].w.l->close();
                    break;
                case PICTURE:
                    winlist[i].w.p->close();
                    break;
                case VIEW:
                    winlist[i].w.v->close();
                    break;
                case OBJECT:
                    winlist[i].w.o->close();
                    break;
                case WORDS:
                    winlist[i].w.w->close();
                    break;
                case TEXTRES:
                    winlist[i].w.t->close();
                    break;
            }
        }
    }

    int n;
    for (i = 0; i < MAXWIN; i++) {
        n = winlist[i].type;
        if (n >= LOGIC && n <= TEXTRES)
            return ;  //some window was not closed
    }


    //ok to close
    for (i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == RESOURCES)
            winlist[i].w.r->close();
    }

    if (window_list)
        window_list->hide();
    disable_game_actions();
    game->close();
}

//**********************************************
void Menu::run_game()
{
    QString program = game->settings->value("InterpreterPath").toString();
    if (program.isEmpty()) {
        this->errmes("Cannot Run Game.\nNo Interpreter has been configured.");
        return;
    }

    QStringList arguments = game->settings->value("InterpreterArgs").toString().split(' ');
    for (auto &arg : arguments) {
        if (arg.contains("%1"))
            arg = arg.arg(game->dir.c_str());
    }

    QProcess *myProcess = new QProcess(this);
    myProcess->start(program, arguments);
}

//**********************************************
int Menu::save_all()
{
    int ret, err = 0;
    for (int i = 0; i < MAXWIN; i++) {
        switch (winlist[i].type) {
            case LOGIC:
                ret = winlist[i].w.l->compile_logic();
                if (ret)
                    err = 1;
                break;
            case PICTURE:
                winlist[i].w.p->save_to_game();
                break;
            case VIEW:
                winlist[i].w.v->save_to_game();
                break;
        }
    }
    return err;
}

//**********************************************
void Menu::save_and_run()
{
    if (!save_all())
        run_game();
}

//**********************************************
void Menu::window_list_cb()
{
    if (window_list == nullptr)
        window_list = new WindowList();
    window_list->draw();
}

//**********************************************
void Menu::options()
{
    auto options = new Options();
    options->show();
}

//**********************************************
void Menu::from_template()
{
    menu->use_template = true;
    OpenGameDir(nullptr, true);
}

//**********************************************
void Menu::blank()
{
    menu->use_template = false;
    OpenGameDir(nullptr, true);
}

//**********************************************
void Menu::new_resource_window()
{
    int i, n;
    int sel[4] = {0, 0, 0, 0};

    for (i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == RESOURCES)
            sel[winlist[i].w.r->selected] = 1;
    }

    if ((n = get_win()) == -1)
        return;
    ResourcesWin *resources_win = new ResourcesWin(NULL, "Resources", n);
    winlist[n].type = RESOURCES;
    winlist[n].w.r = resources_win;
    int res = game->settings->value("DefaultResourceType").toInt();
    for (i = 0; i < 4; i++) {
        if (sel[i] == 0) {
            res = i;
            break;
        }
    }
    resources_win->select_resource_type(res);
    resources_win->show();
    num_res++;
    if (num_res > 1)
        disable_resources();
}

//**********************************************
void Menu::add_resource()
{
    resources_win->add_resource();
}

//**********************************************
void Menu::extract_resource()
{
    resources_win->extract_resource();
}

//**********************************************
void Menu::delete_resource()
{
    resources_win->delete_resource();
}

//**********************************************
void Menu::renumber_resource()
{
    resources_win->renumber_resource();
}

//**********************************************
void Menu::rebuild_vol()
{
    switch (QMessageBox::warning(this, tr("Rebuild VOL files"), tr("Are you sure?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            game->RebuildVOLfiles();
            break;
        default:
            break;
    }
}

//**********************************************
void Menu::recompile_all()
{
    switch (QMessageBox::warning(this, tr("Recompile all"), tr("Do you really want to recompile all logic scripts?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            game->RecompileAll();
            break;
        default:
            break;
    }
}

//**********************************************
void Menu::view_editor()
{
    int n;
    if ((n = get_win()) == -1)
        return;
    winlist[n].w.v = new ViewEdit(nullptr, nullptr, n, resources_win);
    winlist[n].type = VIEW;
    winlist[n].w.v->open();
}

//**********************************************
void Menu::logic_editor()
{
    int n;
    if ((n = get_win()) == -1)
        return;
    winlist[n].w.l = new LogEdit(nullptr, nullptr, n, resources_win);
    winlist[n].type = LOGIC;
    winlist[n].w.l->open();
}

//**********************************************
void Menu::text_editor()
{
    int n;
    if ((n = get_win()) == -1)
        return;
    winlist[n].w.t = new TextEdit(nullptr, nullptr, n);
    winlist[n].type = TEXTRES;
    winlist[n].w.t->new_text();
}

//**********************************************
void Menu::object_editor()
{
    int n;
    if ((n = get_win()) == -1)
        return;
    winlist[n].w.o = new ObjEdit(nullptr, nullptr, n);
    winlist[n].type = OBJECT;
    winlist[n].w.o->open();
}

//**********************************************
void Menu::words_editor()
{
    int n;
    if ((n = get_win()) == -1)
        return;
    winlist[n].w.w = new WordsEdit(nullptr, nullptr, n, resources_win);
    winlist[n].type = WORDS;
    winlist[n].w.w->open();
}

//**********************************************
void Menu::picture_editor()
{
    int n;
    if ((n = get_win()) == -1)
        return;
    winlist[n].w.p = new PicEdit(nullptr, nullptr, n, resources_win);
    winlist[n].type = PICTURE;
    winlist[n].w.p->open();
}

//**********************************************
void Menu::sound_player()
{
    extern void play_sound(const std::string &);

    QString fileName = QFileDialog::getOpenFileName(this, tr("Play Sound File"), game->srcdir.c_str(), tr("Sound Files (sound*.*);;All Files (*)"));
    if (!fileName.isNull())
        play_sound(fileName.toStdString());
}

//**********************************************
void Menu::help_contents()
{
    QString fullpath = QString("%1/index.html").arg(game->settings->value("HelpDir").toString());

    if (helpwindow == nullptr) {
        int n;
        if ((n = get_win()) == -1)
            return;
        helpwindow = new HelpWindow(fullpath, ".");
        winlist[n].type = HELPWIN;
        winlist[n].w.h = helpwindow;
    } else
        helpwindow->setSource(fullpath);
    helpwindow->show();
}

//**********************************************
bool Menu::help_topic(const QString &topic)
{
    QString fullpath = QString("%1/%2.html").arg(game->settings->value("HelpDir").toString()).arg(
                           QString(topic).replace(".", "_"));

    if (!QFile(fullpath).exists())
        return false;

    if (helpwindow1 == nullptr) {
        int n;
        if ((n = get_win()) == -1)
            return true;
        helpwindow1 = new HelpWindow(fullpath, ".");
        winlist[n].type = HELPWIN;
        winlist[n].w.h = helpwindow1;
    } else
        helpwindow1->setSource(fullpath);
    helpwindow1->show();
    helpwindow1->raise();
    return true;
}

//**********************************************
void Menu::help_index()
{
    QString fullpath = QString("%1/indexabc.html").arg(game->settings->value("HelpDir").toString());

    if (helpwindow1 == nullptr) {
        int n;
        if ((n = get_win()) == -1)
            return;
        helpwindow1 = new HelpWindow(fullpath, ".");
        winlist[n].type = HELPWIN;
        winlist[n].w.h = helpwindow1;
    } else
        helpwindow1->setSource(fullpath);
    helpwindow1->show();
}

//**********************************************
void Menu::about_it()
{
    if (about == nullptr)
        about = new About();
    about->show();
}

//**********************************************
void Menu::about_qt()
{
    QMessageBox::aboutQt(this, tr("AGI studio"));
}

//*************************************************
void Menu::errmes(const char *fmt, ...)
{
    std::va_list argp;

    va_start(argp, fmt);
    std::vector<char> buffer(std::vsnprintf(nullptr, 0, fmt, argp) + 1);
    vsnprintf(buffer.data(), buffer.size(), fmt, argp);
    err->setText(buffer.data());
    va_end(argp);

    err->setWindowTitle("AGI studio");
    err->show();
}

//**********************************************
void Menu::warnmes(const char *fmt, ...)
{
    std::va_list argp;

    va_start(argp, fmt);
    std::vector<char> buffer(std::vsnprintf(nullptr, 0, fmt, argp) + 1);
    vsnprintf(buffer.data(), buffer.size(), fmt, argp);
    warn->setText(buffer.data());
    va_end(argp);

    warn->setWindowTitle("AGI studio");
    warn->show();
}

//**********************************************

About::About(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setWindowTitle("About QT AGI Studio");

    QBoxLayout *all = new QVBoxLayout(this);

    QLabel *alogo = new QLabel(this);
    alogo->setPixmap(QPixmap(":/icons/logo.xpm"));
    alogo->setAlignment(Qt::AlignHCenter);
    alogo->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    all->addWidget(alogo);

    QTextEdit *about = new QTextEdit(this);
    about->setReadOnly(true);
    QString about_text =
        "<center><b>Qt AGI studio v.%1</b><br>"
        "https://github.com/Deledrius/agistudio<br>"
        "<br>"
        "<b>Authors:</b><br>"
        "Helen Zommer (helen@cc.huji.ac.il)<br>"
        "Jarno Elonen (elonen@iki.fi)<br>"
        "Joseph Davies (joseph.davies@zero-factorial.com)<br>"
        "<br>"
        "<b>and also:</b><br>"
        "Peter Kelly (pmk@post.com)<br>"
        "Lance Ewing (lance.e@ihug.co.nz)<br>"
        "Claudio Matsuoka (claudio@helllabs.org)<br>"
        "<br>"
        "<b>Windows port by:</b><br>"
        "Nat Budin (natb@brandeis.edu)"
        "<br><br></center>"
        "This program is free software; you can "
        "redistribute it and/or modify it under "
        "the terms of the GNU General Public "
        "License, version 2 or later, as published "
        "by the Free Software Foundation.";
    about->setText(about_text.arg(BUILD_VERSION));
    all->addWidget(about);

    QPushButton *ok = new QPushButton(this);
    ok->setText("OK");
    ok->setMaximumSize(80, 40);
    connect(ok, SIGNAL(clicked()), SLOT(hide()));
    all->addWidget(ok);
}

//**********************************************

WindowList::WindowList(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setWindowTitle("Window list");
    QBoxLayout *l =  new QVBoxLayout(this);

    win = new QListWidget(this);
    win->setMinimumSize(100, 200);
    connect(win, SIGNAL(selected(int)), SLOT(select_cb(int)));
    l->addWidget(win);

    QBoxLayout *l1 = new QHBoxLayout();
    l->addLayout(l1);

    QPushButton *select = new QPushButton(this);
    select->setText("Select");
    connect(select, SIGNAL(clicked()), SLOT(select_ok()));
    l1->addWidget(select);

    QPushButton *del = new QPushButton(this);
    del->setText("Delete");
    connect(del, SIGNAL(clicked()), SLOT(del_cb()));
    l1->addWidget(del);

    QPushButton *close = new QPushButton(this);
    close->setText("Close list");
    connect(close, SIGNAL(clicked()), SLOT(hide()));
    l1->addWidget(close);

    adjustSize();
}

//**********************************************

void WindowList::draw()
{
    QString caption;

    win->clear();
    for (int i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == -1)
            continue;
        switch (winlist[i].type) {
            case LOGIC:
                caption = QString("Logic editor: ").append(winlist[i].w.l->windowTitle());
                if (winlist[i].w.l->isMinimized())
                    caption.insert(0, "(.) ");
                break;
            case PICTURE:
                caption = winlist[i].w.p->windowTitle();
                if (winlist[i].w.p->isMinimized())
                    caption.insert(0, "(.) ");
                break;
            case VIEW:
                caption = winlist[i].w.v->windowTitle();
                if (winlist[i].w.v->isMinimized())
                    caption.insert(0, "(.) ");
                break;
            case OBJECT:
                caption = winlist[i].w.o->windowTitle();
                if (winlist[i].w.o->isMinimized())
                    caption.insert(0, "(.) ");
                break;
            case WORDS:
                caption = winlist[i].w.w->windowTitle();
                if (winlist[i].w.w->isMinimized())
                    caption.insert(0, "(.) ");
                break;
            case TEXTRES:
                caption = winlist[i].w.t->windowTitle();
                if (winlist[i].w.t->isMinimized())
                    caption.insert(0, "(.) ");
                break;
            case RESOURCES:
                caption = winlist[i].w.r->windowTitle();
                if (winlist[i].w.r->isMinimized())
                    caption.insert(0, "(.) ");
                break;
            case HELPWIN:
                caption = QString("Help");
                if (winlist[i].w.h->isMinimized())
                    caption.insert(0, "(.) ");
                else if (!winlist[i].w.h->isVisible())
                    caption.insert(0, "(~) ");
                break;
            case PREVIEW:
                caption = winlist[i].w.pr->windowTitle();
                if (winlist[i].w.pr->isMinimized())
                    caption.insert(0, "(.) ");
                break;
        }
        win->addItem(caption);
    }
    show();
}

//**********************************************

void WindowList::select_cb(int sel)
{
    QString caption;

    int n = 0;
    for (int i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == -1)
            continue;
        if (n == sel) {
            switch (winlist[i].type) {
                case LOGIC:
                    if (winlist[i].w.l->isMinimized()) {
                        winlist[i].w.l->showNormal();
                        caption = QString("Logic editor: ").append(winlist[i].w.l->windowTitle());
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.l->activateWindow();
                    winlist[i].w.l->raise();
                    break;
                case PICTURE:
                    if (winlist[i].w.p->isMinimized()) {
                        winlist[i].w.p->showNormal();
                        caption = winlist[i].w.p->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.p->activateWindow();
                    winlist[i].w.p->raise();
                    break;
                case VIEW:
                    if (winlist[i].w.v->isMinimized()) {
                        winlist[i].w.v->showNormal();
                        caption = winlist[i].w.v->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.v->activateWindow();
                    winlist[i].w.v->raise();
                    break;
                case TEXTRES:
                    if (winlist[i].w.t->isMinimized()) {
                        winlist[i].w.t->showNormal();
                        caption = winlist[i].w.t->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.t->activateWindow();
                    winlist[i].w.t->raise();
                    break;
                case WORDS:
                    if (winlist[i].w.w->isMinimized()) {
                        winlist[i].w.w->showNormal();
                        caption = winlist[i].w.w->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.w->activateWindow();
                    winlist[i].w.w->raise();
                    break;
                case OBJECT:
                    if (winlist[i].w.o->isMinimized()) {
                        winlist[i].w.o->showNormal();
                        caption = winlist[i].w.o->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.o->activateWindow();
                    winlist[i].w.o->raise();
                    break;
                case RESOURCES:
                    if (winlist[i].w.r->isMinimized()) {
                        winlist[i].w.r->showNormal();
                        caption = winlist[i].w.r->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.r->activateWindow();
                    winlist[i].w.r->raise();
                    break;
                case HELPWIN:
                    if (winlist[i].w.h->isMinimized()) {
                        winlist[i].w.h->showNormal();
                        caption = QString("Help");
                        win->setCurrentRow(sel);
                    } else if (!winlist[i].w.h->isVisible()) {
                        winlist[i].w.h->show();
                        caption = winlist[i].w.h->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.h->activateWindow();
                    winlist[i].w.h->raise();
                    break;
                case PREVIEW:
                    if (winlist[i].w.pr->isMinimized()) {
                        winlist[i].w.pr->showNormal();
                        caption = winlist[i].w.pr->windowTitle();
                        win->setCurrentRow(sel);
                    }
                    winlist[i].w.pr->activateWindow();
                    winlist[i].w.pr->raise();
                    break;
            }
            break;
        }
        n++;
    }
}

//**********************************************
void WindowList::select_ok()
{
    if (win->currentRow() != -1)
        select_cb(win->currentRow());
}

//**********************************************
void WindowList::del_cb()
{
    int sel = win->currentRow();
    if (sel == -1)
        return;

    int n = 0;
    for (int i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == -1)
            continue;
        if (n == sel) {
            switch (winlist[i].type) {
                case LOGIC:
                    winlist[i].w.l->close();
                    break;
                case VIEW:
                    winlist[i].w.v->close();
                    break;
                case PICTURE:
                    winlist[i].w.p->close();
                    break;
                case TEXTRES:
                    winlist[i].w.t->close();
                    break;
                case WORDS:
                    winlist[i].w.w->close();
                    break;
                case OBJECT:
                    winlist[i].w.o->close();
                    break;
                case RESOURCES:
                    winlist[i].w.r->close();
                    break;
                case HELPWIN:
                    winlist[i].w.h->hide();
                    break;
                case PREVIEW:
                    winlist[i].w.pr->close();
                    break;
            }
            break;
        }
        n++;
    }
    draw();
}


//*********************************************************
void OpenGameDir(QWidget *parent, bool newgame)
{
    QString title("Open game");
    if (newgame)
        title = "New game";
    QString dir = QFileDialog::getExistingDirectory(parent, title);
    if (dir.isNull())
        return;

    // Close the currently edited game.
    if (game->isOpen) {
        menu->close_game();
        if (game->isOpen)  // Closing was canceled.
            return;
    }

    if (newgame) {
        QDir dir_search(dir);
        if (!dir_search.entryList(QStringList("*vol.?"), QDir::Files).isEmpty()) {
            auto prompt = QString("The folder '%1' already contains an AGI game.  Continuing will erase the existing game.\n\nDo you wish to proceed?").arg(dir);
            switch (QMessageBox::warning(parent, "AGI Studio", prompt,
                                         QMessageBox::Yes | QMessageBox::No,
                                         QMessageBox::No)) {
                case QMessageBox::Yes:
                    break;
                default:
                    return;
            }
        }
    }

    int err = 0;
    if (newgame) {
        if (menu->use_template)
            err = game->from_template(dir.toStdString());
        else
            err = game->newgame(dir.toStdString());
    } else
        err = game->open(dir.toStdString());
    if (!err)
        menu->show_resources();
}
