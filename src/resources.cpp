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

#include "resources.h"
#include "game.h"
#include "logedit.h"
#include "viewedit.h"
#include "preview.h"
#include "menu.h"
#include "midi.h"
#include "bmp2agipic.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QHideEvent>
#include <QVBoxLayout>
#include <QBoxLayout>
#include <QShowEvent>
#include <QCloseEvent>
#include <QGroupBox>

#include <stdio.h>
#ifdef _WIN32
#define strncasecmp(a, b, l) _stricmp(a, b)
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>


//**********************************************************
ResourcesWin::ResourcesWin(QWidget *parent, const char  *name, int win_num):
    QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Resources");

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    winnum = win_num;

    QMenu *window = new QMenu(this);
    Q_CHECK_PTR(window);
    window->setTitle("&Window");
    window->addAction("&New", this, SLOT(new_resource_window()));
    window->addSeparator();
    window->addAction("&Close", this, SLOT(close()));

    resourceMenu = new QMenu(this);
    Q_CHECK_PTR(resourceMenu);
    resourceMenu->setTitle("&Resource");
    resourceMenu->addAction("&Add", this, SLOT(add_resource()));
    resourceMenu->addAction("&Extract", this, SLOT(extract_resource()));
    resourceMenu->addAction("&Delete", this, SLOT(delete_resource()));
    resourceMenu->addAction("&Renumber", this, SLOT(renumber_resource()));
    importMenuItemAction = resourceMenu->addAction("Import &bitmap", this, SLOT(import_resource()));
    importMenuItemAction->setEnabled(false);
    resourceMenu->addAction("E&xtract all", this, SLOT(extract_all_resource()));

    QMenuBar *resourceMenubar = new QMenuBar(this);
    Q_CHECK_PTR(resourceMenubar);
    resourceMenubar->addMenu(window);
    resourceMenubar->addMenu(resourceMenu);
    resourceMenubar->addSeparator();

    QBoxLayout *hbox =  new QHBoxLayout(this);
    hbox->setMenuBar(resourceMenubar);

    QBoxLayout *resbox =  new QVBoxLayout(this);
    hbox->addLayout(resbox);

    type = new QComboBox(this);
    type->addItem("LOGIC");
    type->addItem("PICTURE");
    type->addItem("VIEW");
    type->addItem("SOUND");
    connect(type, SIGNAL(activated(int)), SLOT(select_resource_type(int)));
    type->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    resbox->addWidget(type);

    list = new QListWidget();
    list->setMinimumSize(200, 300);
    connect(list, SIGNAL(itemSelectionChanged()), this, SLOT(highlight_resource()));
    connect(list, SIGNAL(itemActivated(QListWidgetItem *)), this, SLOT(select_resource(QListWidgetItem *)));
    list->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));

    selected = game->res_default;
    resbox->addWidget(list);

    msg = new QLabel(this);
    msg->setAlignment(Qt::AlignLeft);
    resbox->addWidget(msg);

    addmenu = NULL;
    preview = NULL;
    closing = false;

    QBoxLayout *prevbox =  new QVBoxLayout();
    prevbox->addLayout(hbox);

    QGroupBox *group = new QGroupBox("Preview", this);
    group->setMinimumSize(340 + 10 * 2, 280 + 10 * 2);
    group->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    previewPane = group;
    hbox->addWidget(previewPane);

    adjustSize();
}

//********************************************************
void ResourcesWin::select_resource_type(int ResType)
{
    QString str;
    int i, k;

    type->setCurrentIndex(ResType);
    selected = ResType;

    // enable import item if resource type supports it
    importMenuItemAction->setEnabled(selected == PICTURE);

    // Show a resource list
    list->hide();
    list->clear();
    for (i = 0, k = 0; i < 256; i++) {
        if (game->ResourceInfo[ResType][i].Exists) {
            str.sprintf("%s.%03d", ResTypeName[ResType], i);
            list->addItem(str);
            ResourceIndex[k++] = i;
        }
    }
    ResourceNum = k;
    list->show();
    first = true;
}

//********************************************************
void ResourcesWin::highlight_resource()
{
    if (!list->selectedItems().isEmpty())
        highlight_resource(list->currentRow());
}

//********************************************************
void ResourcesWin::highlight_resource(int k)
{
    int i = ResourceIndex[k];
    int size = game->GetResourceSize(selected, i);

    QString str;
    str.sprintf("%d bytes", size);
    msg->setText(str);

    if (preview == NULL)
        preview = new Preview(previewPane, 0, this);

    preview->open(i, selected);
    preview->show();
}

//********************************************************
void ResourcesWin::select_resource(QListWidgetItem *item)
{
    if (!list->selectedItems().isEmpty())
        select_resource(list->currentRow());
}

//********************************************************
void ResourcesWin::select_resource(int k)
{
    int i = ResourceIndex[k];
    int size = game->GetResourceSize(selected, i);
    int n;
    extern void play_sound(int ResNum);

    QString str;
    str.sprintf("%d bytes", size);
    msg->setText(str);

    if ((n = get_win()) == -1)
        return;
    switch (selected) {
        case LOGIC:
            winlist[n].w.l = new LogEdit(NULL, NULL, n);
            winlist[n].type = LOGIC;
            winlist[n].w.l->open(i);
            break;
        case PICTURE:
            winlist[n].w.p = new PicEdit(NULL, NULL, n);
            winlist[n].type = PICTURE;
            winlist[n].w.p->open(i);
            break;
        case VIEW:
            winlist[n].w.v = new ViewEdit(NULL, NULL, n);
            winlist[n].type = VIEW;
            winlist[n].w.v->open(i);
            break;
        case SOUND:
            play_sound(i);
            break;
    }
}

//********************************************************
void ResourcesWin::set_current(int n)
{
    for (int i = 0; i < ResourceNum; i++) {
        if (ResourceIndex[i] == n) {
            list->setCurrentRow(i);
            break;
        }
    }
}
//*********************************************
void ResourcesWin::deinit()
{
    closing = true;
    if (preview) {
        preview->close();
        preview = NULL;
    }
    for (int i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == -1)
            continue;
        switch (winlist[i].type) {
            case LOGIC:
                if (winlist[i].w.l->resources_win == this)
                    winlist[i].w.l->resources_win = NULL;
                break;
            case VIEW:
                if (winlist[i].w.v->resources_win == this)
                    winlist[i].w.v->resources_win = NULL;
                break;
            case PICTURE:
                if (winlist[i].w.p->resources_win == this)
                    winlist[i].w.p->resources_win = NULL;
                break;
        }
    }

    winlist[winnum].type = -1;
    menu->dec_res();
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void ResourcesWin::closeEvent(QCloseEvent *e)
{
    deinit();
    e->accept();
}

//*********************************************
void ResourcesWin::hideEvent(QHideEvent *)
{
    if (preview && preview->isVisible())
        preview->showMinimized();
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void ResourcesWin::showEvent(QShowEvent *)
{
    if (preview && !preview->isVisible())
        preview->showNormal();
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//**********************************************
void ResourcesWin::new_resource_window()
{
    menu->new_resource_window();
}

//**********************************************
void ResourcesWin::delete_resource()
{
    int restype = selected;
    int k = list->currentRow();
    int resnum = ResourceIndex[k];

    sprintf(tmp, "Really delete %s.%03d?", ResTypeName[restype], resnum);
    switch (QMessageBox::warning(this, "Delete resource", tmp,
                                 "Delete", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            game->DeleteResource(restype, resnum);
            select_resource_type(restype);
            if (k > 0)
                list->setCurrentRow(k - 1);
            else
                list->setCurrentRow(0);
            break;
        case 1:
            break;
    }
}

//**********************************************
void ResourcesWin::renumber_resource()
{
    int k = list->currentRow();
    if (k == -1)
        return;

    AskNumber *ask_number = new AskNumber(0, 0, "Resource number",
                                          "Enter new resource number [0-255]: ");

    if (!ask_number->exec())
        return;

    QString str = ask_number->num->text();
    int newnum = str.toInt();
    int restype = selected;
    int resnum = ResourceIndex[k];
    if (game->ResourceInfo[restype][newnum].Exists)
        sprintf(tmp, "Resource %s.%03d already exists. Replace it ?", ResTypeName[restype], newnum);
    switch (QMessageBox::warning(this, "Renumber resource", tmp,
                                 "Replace", "Cancel",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            game->ReadResource(restype, resnum);
            game->DeleteResource(restype, resnum);
            game->AddResource(restype, newnum);
            select_resource_type(restype);
            break;
        case 1:
            break;
    }
}

static QImage openBitmap(const char *title)
{
    QImage pic;
    QString fileName = QFileDialog::getOpenFileName(nullptr, title, game->srcdir.c_str(), "All Files (*)");
    if (!fileName.isNull()) {
        pic = QImage(fileName);
        if (pic.isNull())
            menu->errmes("Error loading bitmap.");
        else {
            if ((pic.width() != 320 && pic.width() != 160) || pic.height() < 168)
                menu->errmes("Bitmap size must be 320x168 or 160x168.\nHeight can be more but will be cropped to 168.");
        }
    }
    return pic;
}

//**********************************************
void ResourcesWin::import_resource()
{
    QImage vis, pri;

    vis = openBitmap("Open visible bitmap");
    if (vis.isNull())
        return;
    pri = openBitmap("Open PRIORITY bitmap (or press cancel)");

    AskNumber *ask_number = new AskNumber(0, 0, "Resource number",
                                          "Enter new resource number [0-255]: ");
    if (!ask_number->exec())
        return;

    QString str = ask_number->num->text();
    int newnum = str.toInt();
    int restype = selected;

    int replace = 0;
    if (game->ResourceInfo[restype][newnum].Exists) {
        sprintf(tmp, "Resource %s.%03d already exists. Replace it ?", ResTypeName[restype], newnum);
        replace = QMessageBox::warning(this, "Overwrite resource", tmp, "Replace", "Cancel", 0, 1);
    }
    switch (replace) {
        case 0: {
            if (game->ResourceInfo[restype][newnum].Exists)
                game->DeleteResource(restype, newnum);

            QByteArray res;
            const char *err = bitmapToAGIPicture(vis, pri, &res);
            if (err) {
                menu->errmes(err);
                return;
            }
            ResourceData.Size = res.size();
            memcpy(ResourceData.Data, res.data(), res.size());

            game->AddResource(restype, newnum); // uses data from ResourceData
            select_resource_type(restype);

            for (int k = 0; k < 255; ++k)
                if (ResourceIndex[k] == newnum)
                    list->setCurrentRow(k);
        }
        break;
        case 1:
            break;
    }
}

//**********************************************
static void extract(char *filename, int restype, int resnum)
{
    if (game->ReadResource(restype, resnum))
        return ;

    FILE *fptr = fopen(filename, "wb");
    if (fptr == NULL) {
        menu->errmes("Can't open file %s ! ", filename);
        return ;
    }
    if (restype == LOGIC && game->save_logic_as_text) {
        Logic *logic = new Logic();
        int err = logic->decode(resnum);
        if (!err)
            fprintf(fptr, "%s", logic->OutputText.c_str());
        delete logic;
    } else
        fwrite(ResourceData.Data, ResourceData.Size, 1, fptr);
    fclose(fptr);
}

//**********************************************
void ResourcesWin::extract_resource()
{
    int restype = selected;
    int resnum = ResourceIndex[list->currentRow()];

    auto defaultfile = QString("%1.%2").arg(ResTypeName[restype]).arg(resnum, 3, 10, QLatin1Char('0'));
    auto defaultpath = QDir::cleanPath(QString(game->srcdir.c_str()) + QDir::separator() + defaultfile);

    QString fileName = QFileDialog::getSaveFileName(this, tr("Extract Resource"), defaultpath, tr("All Files (*)"));
    if (!fileName.isNull())
        extract(fileName.toLatin1().data(), restype, resnum);
}

//**********************************************
void ResourcesWin::extract_all_resource()
{
    char filename[256];

    int restype = selected;
    sprintf(tmp, "Do you really want to extract all %s resources ?", ResTypeName[restype]);
    switch (QMessageBox::warning(this, "Extract all", tmp,
                                 "Yes", "No",
                                 0,      // Enter == button 0
                                 1)) {   // Escape == button 1
        case 0:
            break;
        case 1:
            return;
    }


    for (int resnum = 0; resnum < 256; resnum++) {
        if (game->ResourceInfo[restype][resnum].Exists) {
            sprintf(filename, "%s/%s.%03d", game->srcdir.c_str(), ResTypeName[restype], resnum);
            extract(filename, restype, resnum);
        }
    }
}

//**********************************************
void ResourcesWin::add_resource()
{
    QStringList types = { "logic*.*", "picture*.*", "view*.*", "sound*.*", "All files (*)" };
    QString filters = types.join(";;");
    QString preferredfilter(types[selected]);

    QString fileName = QFileDialog::getOpenFileName(this, tr("Add Resource"), game->srcdir.c_str(), filters, &preferredfilter);
    if (!fileName.isNull()) {
        if (addmenu == NULL)
            addmenu = new AddResource(0, 0, this);
        addmenu->open(fileName.toLatin1().data());
    }
}

//**********************************************
void ResourcesWin::export_resource()
{
    int k = list->currentRow();
    if (k < 0)
        return;
    int i = ResourceIndex[k];

    switch (selected) {
        case SOUND:
            if (game->ReadResource(SOUND, i))
                menu->errmes("Couldn't read sound resource ! ");
            else
                showSaveAsMidi(this, ResourceData.Data);
            break;
        default:
            qWarning("BUG in code. Export not supported for this resource type!");
            break;
    }
}

//**********************************************
AddResource::AddResource(QWidget *parent, const char *name, ResourcesWin *res)
    : QWidget(parent)
{
    resources_win = res;
    setWindowTitle("Add resource");

    QBoxLayout *box = new QVBoxLayout(this);

    filename = new QLabel("Filename:", this);
    box->addWidget(filename);

    QBoxLayout *box0 = new QHBoxLayout(this);
    box->addLayout(box0);

    QLabel *lname = new QLabel("Name of resource:", this);
    box0->addWidget(lname);

    resname = new QLabel("", this);
    box0->addWidget(resname);

    type = new QButtonGroup(this);
    type->setExclusive(true);
    QRadioButton *logic = new QRadioButton(tr("LOGIC"), this);
    QRadioButton *picture = new QRadioButton(tr("PICTURE"), this);
    QRadioButton *view = new QRadioButton(tr("VIEW"), this);
    QRadioButton *sound = new QRadioButton(tr("SOUND"), this);
    logic->setChecked(true);
    restype = 0;

    connect(type, SIGNAL(clicked(int)), SLOT(select_type(int)));

    type->addButton(logic);
    type->addButton(picture);
    type->addButton(view);
    type->addButton(sound);
    box->addWidget(logic);
    box->addWidget(picture);
    box->addWidget(view);
    box->addWidget(sound);

    QBoxLayout *box1 = new QHBoxLayout(this);
    box->addLayout(box1);

    QLabel *lnumber = new QLabel("Number: [0-255]", this);
    box1->addWidget(lnumber);

    number = new QLineEdit(this);
    number->setMinimumWidth(60);
    connect(number, SIGNAL(textChanged(const QString &)), SLOT(edit_cb(const QString &)));
    box1->addWidget(number);

    QBoxLayout *box2 = new QHBoxLayout(this);
    box->addLayout(box2);

    QPushButton *ok = new QPushButton("OK", this);
    connect(ok, SIGNAL(clicked()), SLOT(ok_cb()));
    box2->addWidget(ok);

    QPushButton *cancel = new QPushButton("Cancel", this);
    connect(cancel, SIGNAL(clicked()), SLOT(cancel_cb()));
    box2->addWidget(cancel);
}

//**********************************************
void AddResource::open(char *file_name)
{
    file = std::string(file_name);
    auto f = QFileInfo(file_name).fileName();

    filename->setText("Filename: " + f);
    if (f.startsWith("logic"))
        restype = LOGIC;
    else if (f.startsWith("picture"))
        restype = PICTURE;
    else if (f.startsWith("view"))
        restype = VIEW;
    else if (f.startsWith("sound"))
        restype = SOUND;

    type->buttons()[restype]->setChecked(true);
    select_type(restype);
    show();
}

//**********************************************
void AddResource::edit_cb(const QString &str)
{
    int num = str.toInt();
    sprintf(tmp, "%s.%03d", ResTypeName[restype], num);
    resname->setText(tmp);
}

//**********************************************
static int load_resource(const char *filename, int restype)
{
    extern TStringList InputLines;
    char *ptr;
    byte b;
    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        menu->errmes("Can't open file %s ! ", filename);
        return 1;
    }
    struct stat buf;
    fstat(fileno(fptr), &buf);
    int size = buf.st_size;
    if (size >= MaxResourceSize) {
        menu->errmes("File %s is too big ! ", filename);
        fclose(fptr);
        return 1;
    }

    if (restype == LOGIC) {
        //check if file is binary or ascii
        fread(ResourceData.Data, MIN(size, 64), 1, fptr);
        for (int i = 0; i < MIN(size, 64); i++) {
            b = ResourceData.Data[i];
            if (b > 0x80 || (b < 0x20 && b != 0x0a && b != 0x0d && b != 0x09)) { //file is binary
                fseek(fptr, 0, SEEK_SET);
                ResourceData.Size = size;
                fread(ResourceData.Data, ResourceData.Size, 1, fptr);
                fclose(fptr);
                return 0;
            }
        }
        //file is ascii - trying to compile
        fseek(fptr, 0, SEEK_SET);
        Logic *logic = new Logic();
        InputLines.lfree();
        while (fgets(tmp, 1024, fptr) != NULL) {
            if ((ptr = strchr(tmp, 0x0a)))
                * ptr = 0;
            if ((ptr = strchr(tmp, 0x0d)))
                * ptr = 0;
            InputLines.add(tmp);
        }
        fclose(fptr);
        int err = logic->compile();
        delete logic;
        if (err)
            return 1;
    } else {
        ResourceData.Size = size;
        fread(ResourceData.Data, ResourceData.Size, 1, fptr);
        fclose(fptr);
    }
    return 0;
}

//**********************************************
void AddResource::ok_cb()
{
    QString str = number->text();
    int num = str.toInt();

    if (num < 0 || num > 255) {
        menu->errmes("Resource number must be between 0 and 255 !");
        return ;
    }

    if (game->ResourceInfo[restype][num].Exists) {
        sprintf(tmp, "Resource %s.%03d already exists. Replace it ?", ResTypeName[restype], num);

        switch (QMessageBox::warning(this, "Add resource", tmp,
                                     "Replace", "Cancel",
                                     0,      // Enter == button 0
                                     1)) {   // Escape == button 1
            case 0:
                if (!load_resource(file.c_str(), restype))
                    game->AddResource(restype, num);
                break;
            case 1:
                break;
        }
    } else {
        if (!load_resource(file.c_str(), restype)) {
            game->AddResource(restype, num);
            if (resources_win->selected == restype)
                resources_win->select_resource_type(restype);
        }
    }
    hide();
}

//**********************************************
void AddResource::cancel_cb()
{
    hide();
}

//**********************************************
void AddResource::select_type(int type)
{
    restype = type;

    QString str = number->text();
    int num = str.toInt();

    sprintf(tmp, "%s.%03d", ResTypeName[restype], num);
    resname->setText(tmp);
}
