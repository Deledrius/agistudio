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


#include <algorithm>

#include <QBoxLayout>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>

#include "bmp2agipic.h"
#include "game.h"
#include "logedit.h"
#include "resources.h"
#include "menu.h"
#include "midi.h"
#include "preview.h"
#include "picedit.h"
#include "viewedit.h"


//**********************************************************
ResourcesWin::ResourcesWin(QWidget *parent, const char  *name, int win_num):
    QMainWindow(parent), ResourceIndex(), ResourceNum(), first(), winnum(win_num),
    addmenu(nullptr), preview(nullptr), closing(false)
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    connect(actionNew, &QAction::triggered, this, &ResourcesWin::new_resource_window);
    connect(actionClose, &QAction::triggered, this, &ResourcesWin::close);
    connect(actionAdd, &QAction::triggered, this, &ResourcesWin::add_resource);
    connect(actionExtract, &QAction::triggered, this, &ResourcesWin::extract_resource);
    connect(actionDelete, &QAction::triggered, this, &ResourcesWin::delete_resource);
    connect(actionRenumber, &QAction::triggered, this, &ResourcesWin::renumber_resource);
    connect(actionImportBitmap, &QAction::triggered, this, &ResourcesWin::import_resource);
    connect(actionExtract_All, &QAction::triggered, this, &ResourcesWin::extract_all_resource);

    connect(comboBoxResourceType, &QComboBox::activated, this, &ResourcesWin::select_resource_type);
    connect(listWidgetResources, &QListWidget::itemSelectionChanged, this, qOverload<>(&ResourcesWin::highlight_resource));
    connect(listWidgetResources, &QListWidget::itemActivated, this, qOverload<QListWidgetItem *>(&ResourcesWin::select_resource));

    list = listWidgetResources;
    selected = game->settings->value("DefaultResourceType").toInt();
}

//********************************************************
void ResourcesWin::select_resource_type(int ResType)
{
    QString str;
    uint i, k;

    comboBoxResourceType->setCurrentIndex(ResType);
    selected = ResType;

    // Enable import item if resource type supports it
    actionImportBitmap->setEnabled(selected == PICTURE);

    // Show a resource list
    listWidgetResources->hide();
    listWidgetResources->clear();
    for (i = 0, k = 0; i < 256; i++) {
        if (game->ResourceInfo[ResType][i].Exists) {
            str = QString("%1.%2").arg(ResTypeName[ResType]).arg(QString::number(i), 3, QChar('0'));
            listWidgetResources->addItem(str);
            ResourceIndex[k++] = i;
        }
    }
    ResourceNum = k;
    listWidgetResources->show();
    first = true;
}

//********************************************************
void ResourcesWin::highlight_resource()
{
    if (!listWidgetResources->selectedItems().isEmpty())
        highlight_resource(listWidgetResources->currentRow());
}

//********************************************************
void ResourcesWin::highlight_resource(int k)
{
    int i = ResourceIndex[k];
    int size = game->GetResourceSize(selected, i);

    statusBar()->showMessage(QString("%1 bytes").arg(QString::number(size)));

    if (preview == nullptr)
        preview = new Preview(groupBoxPreview, nullptr, this);

    preview->open(i, selected);
    preview->show();
}

//********************************************************
void ResourcesWin::select_resource(QListWidgetItem *item)
{
    if (!listWidgetResources->selectedItems().isEmpty())
        select_resource(list->currentRow());
}

//********************************************************
void ResourcesWin::select_resource(int k)
{
    int i = ResourceIndex[k];
    int size = game->GetResourceSize(selected, i);
    int n;
    extern void play_sound(int ResNum);

    statusBar()->showMessage(QString("%1 bytes").arg(QString::number(size)));

    if ((n = get_win()) == -1)
        return;
    switch (selected) {
        case LOGIC:
            winlist[n].w.l = new LogEdit(nullptr, nullptr, n);
            winlist[n].type = LOGIC;
            winlist[n].w.l->open(i);
            break;
        case PICTURE:
            winlist[n].w.p = new PicEdit(nullptr, nullptr, n);
            winlist[n].type = PICTURE;
            winlist[n].w.p->open(i);
            break;
        case VIEW:
            winlist[n].w.v = new ViewEdit(nullptr, nullptr, n);
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
            listWidgetResources->setCurrentRow(i);
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
        preview = nullptr;
    }
    for (int i = 0; i < MAXWIN; i++) {
        if (winlist[i].type == -1)
            continue;
        switch (winlist[i].type) {
            case LOGIC:
                if (winlist[i].w.l->resources_win == this)
                    winlist[i].w.l->resources_win = nullptr;
                break;
            case VIEW:
                if (winlist[i].w.v->resources_win == this)
                    winlist[i].w.v->resources_win = nullptr;
                break;
            case PICTURE:
                if (winlist[i].w.p->resources_win == this)
                    winlist[i].w.p->resources_win = nullptr;
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
    int k = listWidgetResources->currentRow();
    int resnum = ResourceIndex[k];

    switch (QMessageBox::warning(this, tr("Delete resource"),
                                 tr("Really delete %1.%2?").arg(ResTypeName[restype]).arg(QString::number(resnum), 3, QChar('0')),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            game->DeleteResource(restype, resnum);
            select_resource_type(restype);
            if (k > 0)
                listWidgetResources->setCurrentRow(k - 1);
            else
                listWidgetResources->setCurrentRow(0);
            break;
        default:
            break;
    }
}

//**********************************************
void ResourcesWin::renumber_resource()
{
    int k = listWidgetResources->currentRow();
    if (k == -1)
        return;

    bool ok;
    int newnum = QInputDialog::getInt(this, tr("Resource Number"), tr("Enter new resource number [0-255]:"), k, 0, 255, 1, &ok);
    if (!ok)
        return;

    int restype = selected;
    int resnum = ResourceIndex[k];
    if (!game->ResourceInfo[restype][newnum].Exists)
        return;
    switch (QMessageBox::warning(this, tr("Renumber resource"),
                                 tr("Resource %1.%2 already exists. Replace it?").arg(ResTypeName[restype]).arg(QString::number(newnum), 3, QChar('0')),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            game->ReadResource(restype, resnum);
            game->DeleteResource(restype, resnum);
            game->AddResource(restype, newnum);
            select_resource_type(restype);
            break;
        default:
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
            if ((pic.width() != 320 && pic.width() != 160) || pic.height() < 168) {
                menu->errmes("Bitmap size must be 320x168 or 160x168.\nHeight can be more but will be cropped to 168.");
                pic = QImage();
            }
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

    bool ok;
    int newnum = QInputDialog::getInt(this, tr("Resource Number"), tr("Enter new resource number [0-255]:"), 0, 0, 255, 1, &ok);
    if (!ok)
        return;

    int restype = selected;
    int replace = 0;
    if (game->ResourceInfo[restype][newnum].Exists) {
        replace = QMessageBox::warning(this, tr("Overwrite resource"),
                                       tr("Resource %1.%2 already exists. Replace it?").arg(ResTypeName[restype]).arg(QString::number(newnum), 3, QChar('0')),
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);
    }
    switch (replace) {
        case QMessageBox::Yes: {
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
                    listWidgetResources->setCurrentRow(k);
        }
        break;
        default:
            break;
    }
}

//**********************************************
static void extract(const std::string &filename, int restype, int resnum)
{
    if (game->ReadResource(restype, resnum))
        return;

    QFile outfile(filename.c_str());
    if (!outfile.open(QIODevice::WriteOnly)) {
        menu->errmes("Can't open file %s!", filename);
        return;
    }

    if (restype == LOGIC && game->settings->value("ExtractLogicAsText").toBool()) {
        Logic *logic = new Logic();
        int err = logic->decode(resnum);
        if (!err)
            outfile.write(logic->OutputText.c_str());
        delete logic;
    } else
        outfile.write(reinterpret_cast<char *>(ResourceData.Data), ResourceData.Size);
    outfile.close();
}

//**********************************************
void ResourcesWin::extract_resource()
{
    int restype = selected;
    int resnum = ResourceIndex[listWidgetResources->currentRow()];

    auto defaultfile = QString("%1.%2").arg(ResTypeName[restype]).arg(QString::number(resnum), 3, '0');
    auto defaultpath = QDir::cleanPath(QString(game->srcdir.c_str()) + QDir::separator() + defaultfile);

    QString fileName = QFileDialog::getSaveFileName(this, tr("Extract Resource"), defaultpath, tr("All Files (*)"));
    if (!fileName.isNull())
        extract(fileName.toStdString(), restype, resnum);
}

//**********************************************
void ResourcesWin::extract_all_resource()
{
    int restype = selected;
    switch (QMessageBox::warning(this, tr("Extract all"),
                                 tr("Do you really want to extract all %1 resources?").arg(ResTypeName[restype]),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            break;
        default:
            return;
    }

    QString filename;
    for (int resnum = 0; resnum < 256; resnum++) {
        if (game->ResourceInfo[restype][resnum].Exists) {
            filename = QString("%1/%2.%3").arg(game->srcdir.c_str()).arg(ResTypeName[restype]).arg(QString::number(resnum), 3, '0');
            extract(filename.toStdString(), restype, resnum);
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
        if (addmenu == nullptr)
            addmenu = new AddResource(nullptr, nullptr, this);
        addmenu->open(fileName.toStdString());
    }
}

//**********************************************
void ResourcesWin::export_resource()
{
    int k = listWidgetResources->currentRow();
    if (k < 0)
        return;
    int i = ResourceIndex[k];

    switch (selected) {
        case SOUND:
            if (game->ReadResource(SOUND, i))
                menu->errmes("Couldn't read sound resource! ");
            else
                showSaveAsMidi(this, ResourceData.Data);
            break;
        default:
            qWarning("Export not supported for this resource type!");
            break;
    }
}

//**********************************************
AddResource::AddResource(QWidget *parent, const char *name, ResourcesWin *res)
    : QWidget(parent), resources_win(res)
{
    setWindowTitle("Add Resource");

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
void AddResource::open(const std::string &file_name)
{
    file = file_name;
    auto f = QFileInfo(file_name.c_str()).fileName();

    filename->setText("Filename: " + f);
    if (f.toLower().startsWith("logic"))
        restype = LOGIC;
    else if (f.toLower().startsWith("picture"))
        restype = PICTURE;
    else if (f.toLower().startsWith("view"))
        restype = VIEW;
    else if (f.toLower().startsWith("sound"))
        restype = SOUND;

    type->buttons()[restype]->setChecked(true);
    select_type(restype);
    show();
}

//**********************************************
void AddResource::edit_cb(const QString &str)
{
    resname->setText(QString("%1.%2").arg(ResTypeName[restype]).arg(str, 3, '0'));
}

//**********************************************
static int load_resource(const std::string &filename, int restype)
{
    extern QStringList InputLines;

    QFile infile(filename.c_str());
    if (!infile.open(QIODevice::ReadOnly)) {
        menu->errmes("Can't open file %s!", filename);
        return 1;
    }

    int size = QFileInfo(infile).size();
    if (size >= MaxResourceSize) {
        menu->errmes("File %s is too big!", filename);
        infile.close();
        return 1;
    }

    if (restype == LOGIC) {
        int sample_size = std::min(size, 64);

        // Check if file contains non-text data
        infile.read(reinterpret_cast<char *>(ResourceData.Data), sample_size);
        for (int i = 0; i < sample_size; i++) {
            unsigned char b = ResourceData.Data[i];
            if (b > 0x80 || (b < 0x20 && b != 0x0a && b != 0x0d && b != 0x09)) {
                // File is binary
                infile.seek(0);
                ResourceData.Size = size;
                infile.read(reinterpret_cast<char *>(ResourceData.Data), ResourceData.Size);
                infile.close();
                return 0;
            }
        }

        // File appears to be text - trying to compile...
        infile.seek(0);
        Logic *logic = new Logic();
        InputLines.clear();
        QTextStream instream(&infile);
        QString line;
        while (instream.readLineInto(&line))
            InputLines.append(line);
        infile.close();
        int err = logic->compile();
        delete logic;
        if (err)
            return 1;
    } else {
        ResourceData.Size = size;
        infile.read(reinterpret_cast<char *>(ResourceData.Data), ResourceData.Size);
        infile.close();
    }
    return 0;
}

//**********************************************
void AddResource::ok_cb()
{
    int num = number->text().toInt();

    if (num < 0 || num > 255) {
        menu->errmes("Resource number must be between 0 and 255 !");
        return;
    }

    if (game->ResourceInfo[restype][num].Exists) {
        switch (QMessageBox::warning(this, "Add resource",
                                     tr("Resource %1.%2 already exists. Replace it?").arg(ResTypeName[restype]).arg(QString::number(num), 3, QChar('0')),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No)) {
            case QMessageBox::Yes:
                if (!load_resource(file, restype))
                    game->AddResource(restype, num);
                break;
            default:
                break;
        }
    } else {
        if (!load_resource(file, restype)) {
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
    resname->setText(QString("%1.%2").arg(ResTypeName[restype]).arg(number->text(), 3, '0'));
}
