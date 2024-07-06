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

#include <QClipboard>
#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QSettings>
#include <QTextEdit>
#include <QTimer>

#include "game.h"
#include "helpwindow.h"
#include "menu.h"
#include "preview.h"
#include "resources.h"
#include "viewedit.h"
#include "wutil.h"


//*********************************************
ViewEdit::ViewEdit(QWidget *parent, const char *name, int win_num, ResourcesWin *res)
    : QMainWindow(parent), winnum(win_num), resources_win(res), description(nullptr),
      animate(nullptr), changed(false), undo(false), ViewNum(0),
      canvas(nullptr), drawing_mode(V_DRAW), transcol(0)
{
    setupUi(this);
    undoCel = Cel();
    view = new View();

    setAttribute(Qt::WA_DeleteOnClose);

    canvas = new Canvas(this, nullptr, this);
    canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    canvas->setFocusPolicy(Qt::ClickFocus);
    setFocusProxy(canvas);
    viewColumnLayout->replaceWidget(PictureFrame, canvas);
    viewColumnLayout->removeWidget(PictureFrame);
    delete PictureFrame;
    
    palette = new Palette(this);
    palette->setMinimumSize(250, 40);
    palette->setMaximumSize(350, 80);
    viewColumnLayout->replaceWidget(PaletteWidget, palette);
    viewColumnLayout->removeWidget(PaletteWidget);
    delete PaletteWidget;

    transcolor->setPalette(QPalette(egacolor[0]));

    // Menu signal handlers
    connect(actionNew, &QAction::triggered, this, qOverload<>(&ViewEdit::open));
    connect(actionOpenfromFile, &QAction::triggered, this, &ViewEdit::open_file);
    connect(actionSavetoGame, &QAction::triggered, this, &ViewEdit::save_to_game);
    connect(actionSavetoGameAs, &QAction::triggered, this, &ViewEdit::save_to_game_as);
    connect(actionSavetoFile, &QAction::triggered, this, &ViewEdit::save_file);
    connect(actionDeleteView, &QAction::triggered, this, &ViewEdit::delete_view);
    connect(actionClose, &QAction::triggered, this, &ViewEdit::close);

    connect(actionUndo, &QAction::triggered, this, &ViewEdit::undo_cel);
    connect(actionCopyCel, &QAction::triggered, this, &ViewEdit::copy_to_clipboard);
    connect(actionPasteCel, &QAction::triggered, this, &ViewEdit::paste_from_clipboard);

    connect(actionNextLoop, &QAction::triggered, this, &ViewEdit::next_loop);
    connect(actionPreviousLoop, &QAction::triggered, this, &ViewEdit::previous_loop);
    connect(actionFirstLoop, &QAction::triggered, this, &ViewEdit::first_loop);
    connect(actionLastLoop, &QAction::triggered, this, &ViewEdit::last_loop);
    connect(actionInsertBeforeLoop, &QAction::triggered, this, &ViewEdit::insert_loop_before);
    connect(actionInsertAfterLoop, &QAction::triggered, this, &ViewEdit::insert_loop_after);
    connect(actionAppendLoop, &QAction::triggered, this, &ViewEdit::append_loop);
    connect(actionDeleteLoop, &QAction::triggered, this, &ViewEdit::delete_loop);
    connect(actionClear, &QAction::triggered, this, &ViewEdit::clear_loop);

    connect(actionNextCel, &QAction::triggered, this, &ViewEdit::next_cel);
    connect(actionPreviousCel, &QAction::triggered, this, &ViewEdit::previous_cel);
    connect(actionFirstCel, &QAction::triggered, this, &ViewEdit::first_cel);
    connect(actionLastCel, &QAction::triggered, this, &ViewEdit::last_cel);
    connect(actionInsertBeforeCel, &QAction::triggered, this, &ViewEdit::insert_cel_before);
    connect(actionInsertAfterCel, &QAction::triggered, this, &ViewEdit::insert_cel_after);
    connect(actionAppendCel, &QAction::triggered, this, &ViewEdit::append_cel);
    connect(actionDeleteCel, &QAction::triggered, this, &ViewEdit::delete_cel);
    connect(actionClearCel, &QAction::triggered, this, &ViewEdit::clear_cel);
    connect(actionFlipCelHorizontally, &QAction::triggered, this, &ViewEdit::fliph_cel);
    connect(actionFlipCelVertically, &QAction::triggered, this, &ViewEdit::flipv_cel);

    connect(actionAnimationOptions, &QAction::triggered, this, &ViewEdit::animate_cb);
    connect(actionEditorHelp, &QAction::triggered, this, &ViewEdit::show_help);

    // Tool signal handlers
    connect(pushButtonLoopLeft, &QPushButton::clicked, this, &ViewEdit::previous_loop);
    connect(pushButtonLoopRight, &QPushButton::clicked, this, &ViewEdit::next_loop);
    connect(pushButtonCelLeft, &QPushButton::clicked, this, &ViewEdit::previous_cel);
    connect(pushButtonCelRight, &QPushButton::clicked, this, &ViewEdit::next_cel);
    connect(pushButtonWidthleft, &QPushButton::clicked, this, &ViewEdit::dec_width);
    connect(pushButtonWidthRight, &QPushButton::clicked, this, &ViewEdit::inc_width);
    connect(pushButtonHeightLeft, &QPushButton::clicked, this, &ViewEdit::dec_height);
    connect(pushButtonHeightRight, &QPushButton::clicked, this, &ViewEdit::inc_height);

    connect(checkBoxDescription, &QPushButton::clicked, this, &ViewEdit::is_descriptor_cb);
    connect(pushButtonEditDesc, &QPushButton::clicked, this, &ViewEdit::show_description);
    connect(comboBoxMirrorLoop, &QComboBox::activated, this, &ViewEdit::change_mirror);

    connect(zoomOutButton, &QPushButton::clicked, this, &ViewEdit::zoom_minus);
    connect(zoomInButton, &QPushButton::clicked, this, &ViewEdit::zoom_plus);
    connect(pushButtonUp, &QPushButton::clicked, this, &ViewEdit::shift_up);
    connect(pushButtonDown, &QPushButton::clicked, this, &ViewEdit::shift_down);
    connect(pushButtonLeft, &QPushButton::clicked, this, &ViewEdit::shift_left);
    connect(pushButtonRight, &QPushButton::clicked, this, &ViewEdit::shift_right);

    connect(buttonGroupDrawMode, &QButtonGroup::idClicked, this, &ViewEdit::change_mode);
    buttonGroupDrawMode->setId(radioButtonDraw, V_DRAW);
    buttonGroupDrawMode->setId(radioButtonFill, V_FILL);

    connect(pushButtonSetTrans, &QPushButton::clicked, this, qOverload<>(&ViewEdit::set_transcolor));

    canvas->setFocus();
}

//*********************************************
void ViewEdit::save(char *filename)
{
    view->save(filename);
    changed = false;
}

//*********************************************
void ViewEdit::display()
{
    showlooppar();
    showcelpar();

    if (view->Description != "") {
        checkBoxDescription->setChecked(true);
        pushButtonEditDesc->setEnabled(true);
    } else {
        checkBoxDescription->setChecked(false);
        pushButtonEditDesc->setEnabled(false);
    }
    if (description)
        description->hide();
    DisplayView();
    show();
}

//*********************************************
void ViewEdit::open(int ResNum)
{
    if (view->open(ResNum))
        return ;
    ViewNum = ResNum;
    setWindowTitle(tr("View Editor: view.%1").arg(QString::number(ViewNum), 3, '0'));
    changed = false;
    display();
}

//*********************************************
void ViewEdit::open(char *filename)
{
    if (view->open(filename))
        return;
    ViewNum = -1;
    setWindowTitle(tr("View Editor"));
    changed = false;
    display();
}

//*********************************************
void ViewEdit::DisplayView()
{
    int w, h;
    w = canvas->x0 + canvas->cur_w * canvas->pixsize * 2 + 10;
    h = canvas->y0 + canvas->cur_h * canvas->pixsize + 10;

    int i = view->loops[view->CurLoop].mirror;
    if (i != -1)
        canvas->DrawCel(view->loops[i].cels[view->CurCel].width, view->loops[i].cels[view->CurCel].height, view->loops[i].cels[view->CurCel].data, true);
    else
        canvas->DrawCel(view->loops[view->CurLoop].cels[view->CurCel].width, view->loops[view->CurLoop].cels[view->CurCel].height, view->loops[view->CurLoop].cels[view->CurCel].data, false);
    if (view->loops[view->CurLoop].cels[view->CurCel].transcol != transcol)
        set_transcolor(view->loops[view->CurLoop].cels[view->CurCel].transcol);
}

//*********************************************
void ViewEdit::DisplayView(int pixsize)
{
    int w, h;
    w = canvas->x0 + canvas->cur_w * pixsize * 2 + 10;
    h = canvas->y0 + canvas->cur_h * pixsize + 10;

    int i = view->loops[view->CurLoop].mirror;
    if (i != -1)
        canvas->DrawCel(view->loops[i].cels[view->CurCel].width, view->loops[i].cels[view->CurCel].height, view->loops[i].cels[view->CurCel].data, true, pixsize);
    else
        canvas->DrawCel(view->loops[view->CurLoop].cels[view->CurCel].width, view->loops[view->CurLoop].cels[view->CurCel].height, view->loops[view->CurLoop].cels[view->CurCel].data, false, pixsize);
    if (view->loops[view->CurLoop].cels[view->CurCel].transcol != transcol)
        set_transcolor(view->loops[view->CurLoop].cels[view->CurCel].transcol);
}

//*********************************************
void ViewEdit::showlooppar()
{
    labelLoopNum->setText(QString("%1/%2").arg(view->CurLoop).arg(view->NumLoops - 1));
    showmirror();
}

//*********************************************
void ViewEdit::showmirror()
{
    int m = view->loops[view->CurLoop].mirror;
    int m1 = view->loops[view->CurLoop].mirror1;

    comboBoxMirrorLoop->clear();
    comboBoxMirrorLoop->addItem("No other loop");
    comboBoxMirrorLoop->setCurrentIndex(0);
    int item = 1;
    for (int i = 0; i < view->NumLoops; i++) {
        if (i == view->CurLoop)
            continue;
        if ((view->loops[i].mirror == -1 && view->loops[i].mirror1 == -1) || i == m || i == m1) {
            comboBoxMirrorLoop->addItem(QString("Loop %1").arg(i), i);
            if (m == i)
                comboBoxMirrorLoop->setCurrentIndex(item);
            else if (m == -1 && m1 == i)
                comboBoxMirrorLoop->setCurrentIndex(item);
            item++;

        }
    }
}

//*********************************************
void ViewEdit::showcelpar()
{
    labelCelNum->setText(QString("%1/%2").arg(view->CurCel).arg(view->loops[view->CurLoop].NumCels - 1));
    lineEditWidth->setText(QString("%1").arg(view->loops[view->CurLoop].cels[view->CurCel].width));
    lineEditHeight->setText(QString("%1").arg(view->loops[view->CurLoop].cels[view->CurCel].height));
}

//*********************************************
void ViewEdit::deinit()
{
    if (description) {
        description->close();
        description = NULL;
    }
    if (animate)
        animate->closeall();
    delete view;
    winlist[winnum].type = -1;
    if (window_list && window_list->isVisible())
        window_list->draw();
}


//*********************************************
void ViewEdit::hideEvent(QHideEvent *)
{
    if (description) {
        description->close();
        description = NULL;
    }
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//*********************************************
void ViewEdit::showEvent(QShowEvent *)
{
    if (window_list && window_list->isVisible())
        window_list->draw();
}

//***********************************************
void ViewEdit::closeEvent(QCloseEvent *e)
{
    if (changed) {
        switch (QMessageBox::warning(this, tr("View Editor"), tr("Save changes to view?"),
                                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                     QMessageBox::Cancel)) {
            case QMessageBox::Save:
                save_to_game();
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

//*********************************************
void ViewEdit::open_file()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open View File"), game->srcdir.c_str(), tr("View Files (view*.*);;All Files (*)"));
    if (!fileName.isNull())
        open(fileName.toLatin1().data());
}

//*********************************************
void ViewEdit::open()
{
    setWindowTitle(tr("View Editor"));
    view->newView();
    ViewNum = -1;
    showlooppar();
    showcelpar();
    DisplayView();
    changed = false;
    show();
}

//*********************************************
void ViewEdit::save_file()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save View File"), game->srcdir.c_str(), tr("View Files (view*.*);;All Files (*)"));
    if (!fileName.isNull())
        save(fileName.toLatin1().data());
}

//*********************************************
void ViewEdit::save_to_game()
{
    if (ViewNum != -1) {
        view->save(ViewNum);
        if (resources_win) {
            if (resources_win->preview == NULL)
                resources_win->preview = new Preview();
            resources_win->preview->open(ViewNum, VIEW);
        }
        changed = false;
    } else
        save_to_game_as();
}

//*********************************************
void ViewEdit::save_to_game_as()
{
    bool ok;
    int num = QInputDialog::getInt(this, tr("View Number"), tr("Enter view number [0-255]:"), 0, 0, 255, 1, &ok);
    if (!ok)
        return;

    if (game->ResourceInfo[VIEW][num].Exists) {
        switch (QMessageBox::warning(this, tr("View Editor"), tr("Resource view.%1 already exists. Replace it?").arg(QString::number(num), 3, QChar('0')),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No)) {
            case QMessageBox::Yes:
                view->save(num);
                changed = false;
                ViewNum = num;
                if (resources_win) {
                    if (resources_win->preview == NULL)
                        resources_win->preview = new Preview();
                    resources_win->preview->open(ViewNum, VIEW);
                }
                break;
            default:
                break;
        }
    } else {
        view->save(num);
        changed = false;
        ViewNum = num;
        if (resources_win) {
            resources_win->select_resource_type(VIEW);
            resources_win->set_current(num);
        }
        open(num);
    }
}

//*********************************************
void ViewEdit::delete_view()
{
    int k;

    if (ViewNum == -1)
        return;
    switch (QMessageBox::warning(this, tr("View Editor"), tr("Really delete view.%1?").arg(QString::number(ViewNum), 3, QChar('0')),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)) {
        case QMessageBox::Yes:
            game->DeleteResource(VIEW, ViewNum);
            if (resources_win) {
                k = resources_win->list->currentRow();
                resources_win->select_resource_type(VIEW);
                resources_win->list->setCurrentRow(k);
            }
            break;
        default:
            break;
    }
}

//*********************************************
void ViewEdit::flipv_cel()
{
    view->loops[curIndex()].cels[view->CurCel].mirrorv();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::fliph_cel()
{
    view->loops[curIndex()].cels[view->CurCel].mirrorh();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::copy_to_clipboard()
{
    const auto &current_cel = view->loops[curIndex()].cels[view->CurCel];
    auto image = QImage(reinterpret_cast<unsigned char *>(current_cel.data), current_cel.width * 2, current_cel.height, current_cel.width * 2, QImage::Format_Indexed8);
    image.setColorTable(egaColorTable);
    image = image.convertToFormat(QImage::Format_ARGB32);
    QApplication::clipboard()->setImage(image);
}

//*********************************************
void ViewEdit::paste_from_clipboard()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();

    if (mimeData->hasImage()) {
        saveundo();
        auto image = qvariant_cast<QImage>(mimeData->imageData()).convertToFormat(QImage::Format_Indexed8, egaColorTable);

        Cel croppedcel(image.width() / 2, image.height(), image.pixelIndex(0,0), false);
        if (croppedcel.data) {
            for (int y = 0; y < croppedcel.height; y++) {
                for (int x = 0; x < croppedcel.width * 2; x++)
                    croppedcel.data[(croppedcel.width * 2 * y) + x] = image.pixelIndex(x, y);
            }
            view->loops[curIndex()].cels[view->CurCel].copy(croppedcel);
        }
    }
    showcelpar();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::next_loop()
{
    if (view->CurLoop < view->NumLoops - 1) {
        view->CurLoop++;
        view->CurCel = 0;
        showlooppar();
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::previous_loop()
{
    if (view->CurLoop > 0) {
        view->CurLoop--;
        view->CurCel = 0;
        showlooppar();
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::first_loop()
{
    view->CurLoop = 0;
    showlooppar();
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::last_loop()
{
    view->CurLoop = view->NumLoops - 1;
    showlooppar();
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::insert_loop_before()
{
    if (view->NumLoops < MaxLoops - 1) {
        view->insertLoop_before();
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 16 loops already reached.");
}

//*********************************************
void ViewEdit::insert_loop_after()
{
    if (view->NumLoops < MaxLoops - 1) {
        view->insertLoop_after();
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 16 loops already reached.");
}

//*********************************************
void ViewEdit::append_loop()
{
    if (view->NumLoops < MaxLoops - 1) {
        view->appendLoop();
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 16 loops already reached.");
}

//*********************************************

void ViewEdit::delete_loop()
{
    if (view->NumLoops > 1) {
        view->deleteLoop();
        if (view->CurLoop > view->NumLoops - 1)
            view->CurLoop--;
        showlooppar();
        showcelpar();
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::clear_loop()
{
    view->loops[view->CurLoop].clear();
    if (view->loops[view->CurLoop].mirror != -1)
        view->loops[view->loops[view->CurLoop].mirror].clear();
    showlooppar();
    showcelpar();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::change_mirror(int i)
{
    if (i == 0) {

        printf("unset mirror %d\n", view->CurLoop);

        if (view->loops[view->CurLoop].mirror != -1)
            view->unsetMirror(view->CurLoop);
        else if (view->loops[view->CurLoop].mirror1 != -1)
            view->unsetMirror(view->loops[view->CurLoop].mirror1);
    } else {
        int k = comboBoxMirrorLoop->currentData().toInt();
        printf("set %d to mirror %d\n", view->CurLoop, k);
        if (view->loops[view->CurLoop].mirror != k) {


            for (int j = 0; j < view->NumLoops; j++) {
                if (view->loops[j].mirror == view->CurLoop)
                    view->unsetMirror(j);
                if (view->loops[j].mirror1 == view->CurLoop)
                    view->loops[j].mirror1 = -1;
            }


            if (view->loops[k].mirror != -1)
                view->unsetMirror(k);
            view->setMirror(view->CurLoop, k);

        }
    }

    showlooppar();
    DisplayView();
    changed = true;
}


//**********************************************************
void ViewEdit::next_cel()
{
    if (view->CurCel < view->loops[view->CurLoop].NumCels - 1) {
        view->CurCel++;
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::next_cel_cycle()
{
    if (view->loops[view->CurLoop].NumCels <= 1)
        return;
    if (view->CurCel < view->loops[view->CurLoop].NumCels - 1)
        view->CurCel++;
    else
        view->CurCel = 0;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::previous_cel()
{
    if (view->CurCel > 0) {
        view->CurCel--;
        showcelpar();
        DisplayView();
    }
}

//*********************************************
void ViewEdit::prev_cel_cycle()
{
    if (view->loops[view->CurLoop].NumCels <= 1)
        return;
    if (view->CurCel > 0)
        view->CurCel--;
    else
        view->CurCel = view->loops[view->CurLoop].NumCels - 1;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::first_cel()
{
    view->CurCel = 0;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::last_cel()
{
    view->CurCel = view->loops[view->CurLoop].NumCels - 1;
    showcelpar();
    DisplayView();
}

//*********************************************
void ViewEdit::insert_cel_before()
{
    if (view->loops[view->CurLoop].NumCels < MaxCels - 1) {
        view->loops[view->CurLoop].insertCel_before(view->CurCel);
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].insertCel_before(view->CurCel);
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 32 cels already reached in this loop.");
}

//*********************************************
void ViewEdit::insert_cel_after()
{
    if (view->loops[view->CurLoop].NumCels < MaxCels - 1) {
        view->loops[view->CurLoop].insertCel_after(view->CurCel);
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].insertCel_after(view->CurCel);
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 32 cels already reached in this loop.");
}

//*********************************************
void ViewEdit::append_cel()
{
    if (view->loops[view->CurLoop].NumCels < MaxCels - 1) {
        view->loops[view->CurLoop].appendCel();
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].appendCel();
        showcelpar();
        DisplayView();
        changed = true;
    } else
        menu->errmes("Max of 32 cels already reached in this loop.");
}

//*********************************************
void ViewEdit::delete_cel()
{
    if (view->loops[view->CurLoop].NumCels > 1) {
        view->loops[view->CurLoop].deleteCel(view->CurCel);
        if (view->loops[view->CurLoop].mirror != -1)
            view->loops[view->loops[view->CurLoop].mirror].deleteCel(view->CurCel);
        if (view->CurCel >= view->loops[view->CurLoop].NumCels)
            view->CurCel--;
        showcelpar();
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::dec_width()
{
    int w;

    if ((w = view->loops[view->CurLoop].cels[view->CurCel].width) > 1) {
        w--;
        view->loops[view->CurLoop].cels[view->CurCel].setW(w);
        lineEditWidth->setText(QString::number(w));
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::inc_width()
{
    int w = view->loops[view->CurLoop].cels[view->CurCel].width + 1;
    if (w < 160) {
        view->loops[view->CurLoop].cels[view->CurCel].setW(w);
        lineEditWidth->setText(QString::number(w));
        DisplayView();
        changed = true;
    } else
        menu->errmes("Maximum width is 160.");
}

//*********************************************
void ViewEdit::dec_height()
{
    int h;
    if ((h = view->loops[view->CurLoop].cels[view->CurCel].height) > 1) {
        h--;
        view->loops[view->CurLoop].cels[view->CurCel].setH(h);
        lineEditHeight->setText(QString::number(h));
        DisplayView();
        changed = true;
    }
}

//*********************************************
void ViewEdit::inc_height()
{
    int h = view->loops[view->CurLoop].cels[view->CurCel].height + 1;

    if (h < 168) {
        view->loops[view->CurLoop].cels[view->CurCel].setH(h);
        lineEditHeight->setText(QString::number(h));
        DisplayView();
        changed = true;
    } else
        menu->errmes("Maximum height is 168.");
}

//*********************************************
void ViewEdit::change_width_height()
{
    QString str = lineEditWidth->text();
    int w = str.toInt();
    view->loops[view->CurLoop].cels[view->CurCel].setW(w);

    str = lineEditHeight->text();
    int h = str.toInt();
    view->loops[view->CurLoop].cels[view->CurCel].setH(h);

    DisplayView();
    changed = true;
    lineEditWidth->clearFocus();
    lineEditHeight->clearFocus();
    setFocus();
}

//*********************************************
void ViewEdit::shift_right()
{
    if (view->loops[view->CurLoop].mirror == -1)
        view->loops[view->CurLoop].cels[view->CurCel].right();
    else
        view->loops[curIndex()].cels[view->CurCel].left();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::shift_left()
{
    if (view->loops[view->CurLoop].mirror == -1)
        view->loops[view->CurLoop].cels[view->CurCel].left();
    else
        view->loops[curIndex()].cels[view->CurCel].right();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::shift_up()
{
    view->loops[curIndex()].cels[view->CurCel].up();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::shift_down()
{
    view->loops[curIndex()].cels[view->CurCel].down();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::fillCel(int x, int y, byte color)
{
    saveundo();
    view->loops[curIndex()].cels[view->CurCel].fill(x, y, color);
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::clear_cel()
{
    saveundo();
    view->loops[curIndex()].cels[view->CurCel].clear();
    DisplayView();
    changed = true;
}

//*********************************************
void ViewEdit::saveundo()
{
    undoCel.deinit();
    undoCel.copy(view->loops[curIndex()].cels[view->CurCel]);
    undo = true;
}

//*********************************************
void ViewEdit::undo_cel()
{
    if (undo) {
        view->loops[curIndex()].cels[view->CurCel].copy(undoCel);
        undo = false;
    }
    DisplayView();
}

/*************************************************/
int ViewEdit::curIndex() const
{
    int i = view->loops[view->CurLoop].mirror;
    if (i == -1)
        return view->CurLoop;
    else
        return i;
}

/*************************************************/
void ViewEdit::show_description()
{
    if (!description)
        description = new Description(0, 0, this);
    description->set();
    description->show();
}

/*******************************************************/
void ViewEdit::change_mode(int m)
{
    drawing_mode = m;
}

/*******************************************************/
void ViewEdit::change_mode1(int m)
{
    drawing_mode = m;
    if (m == V_DRAW)
        radioButtonDraw->setChecked(true);
    else
        radioButtonFill->setChecked(true);
}

/*******************************************************/
void ViewEdit::is_descriptor_cb()
{
    if (checkBoxDescription->isChecked())
        pushButtonEditDesc->setEnabled(true);
    else
        pushButtonEditDesc->setEnabled(false);
}

/*******************************************************/
void ViewEdit::set_transcolor()
{
    transcolor->setPalette(QPalette(egacolor[palette->left]));
    transcol = palette->left;
    view->loops[view->CurLoop].cels[view->CurCel].transcol = transcol;
}

/*******************************************************/
void ViewEdit::set_transcolor(int col)
{
    transcolor->setPalette(QPalette(egacolor[col]));
    transcol = col;
    view->loops[view->CurLoop].cels[view->CurCel].transcol = transcol;
}

/*******************************************************/
void ViewEdit::zoom_minus()
{
    if (canvas->pixsize > 1)
        DisplayView(canvas->pixsize - 1);
}

/*******************************************************/
void ViewEdit::zoom_plus()
{
    if (canvas->pixsize < 10)
        DisplayView(canvas->pixsize + 1);
}

/*******************************************************/
bool ViewEdit::focusNextPrevChild(bool)
{
    if (lineEditWidth->hasFocus())
        lineEditHeight->setFocus();
    else if (lineEditHeight->hasFocus())
        lineEditWidth->setFocus();
    else
        canvas->setFocus();
    return true;
}

/*******************************************************/
void ViewEdit::animate_cb()
{
    if (animate == nullptr)
        animate = new Animate(nullptr, nullptr, nullptr, this);
    animate->show();
}

/*******************************************************/
void ViewEdit::show_help()
{
    QString fullpath = QString("%1/view_editor_main.html").arg(game->settings->value("HelpDir").toString());

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

//*********************************************
void ViewEdit::keyPressEvent(QKeyEvent *k)
{
    switch (k->key()) {
        case Qt::Key_Q:
            previous_loop();
            break;
        case Qt::Key_W:
            next_loop();
            break;
        case Qt::Key_A:
            previous_cel();
            break;
        case Qt::Key_S:
            next_cel();
            break;
        case Qt::Key_Z:
            zoom_minus();
            break;
        case Qt::Key_X:
            zoom_plus();
            break;
        case Qt::Key_T:
            set_transcolor();
            break;
        case Qt::Key_D:
            change_mode1(V_DRAW);
            break;
        case Qt::Key_F:
            change_mode1(V_FILL);
            break;
        case Qt::Key_I:
            shift_up();
            break;
        case Qt::Key_K:
            shift_down();
            break;
        case Qt::Key_J:
            shift_left();
            break;
        case Qt::Key_L:
            shift_right();
            break;
        default:
            k->ignore();
            break;
    }
}


/*******************************************************/
Animate::Animate(QWidget *parent, const char *name, Preview *p, ViewEdit *v)
    : QWidget(parent), fwd(true), num(0), viewedit(v), preview(p)
{
    setWindowTitle("Animate");

    QBoxLayout *b = new QVBoxLayout(this);

    QHBoxLayout *b1 = new QHBoxLayout(this);
    b->addLayout(b1);
    QLabel *l = new QLabel("Delay (ms)", this);
    b1->addWidget(l);
    delay = new QLineEdit(this);
    delay->setText("200");
    delay->setMaximumWidth(100);
    b1->addWidget(delay);

    QButtonGroup *fb = new QButtonGroup(this);
    fb->setExclusive(true);
    forward = new QRadioButton("Forward");
    forward->setChecked(true);
    backward = new QRadioButton("Backward");
    connect(fb, SIGNAL(clicked(int)), SLOT(fb_cb()));
    fb->addButton(forward);
    fb->addButton(backward);
    b1->addWidget(forward);
    b1->addWidget(backward);

    QHBoxLayout *b2 = new QHBoxLayout(this);
    b->addLayout(b2);
    button = new QPushButton(this);
    button->setText("Start");
    b2->addWidget(button);
    connect(button, SIGNAL(clicked()), SLOT(start_stop()));
    QPushButton *close = new QPushButton(this);
    close->setText("Close");
    b2->addWidget(close);
    connect(close, SIGNAL(clicked()), SLOT(hide()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(next_cel()));
}

/*******************************************************/
void Animate::start_stop()
{
    if (timer->isActive()) {
        timer->stop();
        button->setText(tr("Start"));
    } else {
        QString str = delay->text();
        num = str.toInt();
        button->setText(tr("Stop"));
        fwd = forward->isChecked();
        timer->start(num);
    }
}

/*******************************************************/
void Animate::fb_cb()
{
    fwd = forward->isChecked();
}

/*******************************************************/
void Animate::next_cel()
{
    if (viewedit) {
        if (fwd)
            viewedit->next_cel_cycle();
        else
            viewedit->prev_cel_cycle();
    } else {
        if (fwd)
            preview->next_cel_cycle();
        else
            preview->prev_cel_cycle();
    }
}

/*******************************************************/
void Animate::closeall()
{
    if (timer->isActive())
        timer->stop();
    close();
}

/*******************************************************/
Description::Description(QWidget *parent, const char *name, ViewEdit *v)
    : QWidget(parent), viewedit(v)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("View description");

    QBoxLayout *d1 = new QVBoxLayout(this);
    d1->addSpacing(10);

    smallview = new ViewIcon(this, 0, viewedit);
    smallview->setMinimumSize(64, 64);

    d1->addWidget(smallview, 0, Qt::AlignCenter);

    desc = new QTextEdit(this);
    desc->setMinimumSize(300, 100);
    d1->addWidget(desc, 1);

    QBoxLayout *d2 = new QHBoxLayout(this);
    d1->addLayout(d2);
    d2->addSpacing(10);

    QPushButton *ok = new QPushButton(this);
    ok->setText("OK");
    ok->setMaximumWidth(80);
    connect(ok, SIGNAL(clicked()), SLOT(ok_cb()));
    d2->addWidget(ok);

    QPushButton *cancel = new QPushButton(this);
    cancel->setText("Cancel");
    cancel->setMaximumWidth(80);
    connect(cancel, SIGNAL(clicked()), SLOT(cancel_cb()));
    d2->addWidget(cancel);

    adjustSize();
    hide();
    getmaxcol();
}

//*********************************************
void Description::getmaxcol()
// Get maximum number of columns on screen (approx.)
// to wrap the long lines
{
    QFontMetrics f = fontMetrics();
    maxcol = desc->width() / f.averageCharWidth();
}

//*********************************************
void Description::resizeEvent(QResizeEvent *)
{
    getmaxcol();
    set();
}

//*********************************************
void Description::set()
{
    size_t n;

    desc->clear();

    if (viewedit->view->Description.empty())
        return;

    std::string ThisLine;
    std::string ThisMessage = viewedit->view->Description;

    do {
        if (ThisMessage.length() + ThisLine.length() > maxcol) {
            n = maxcol - ThisLine.length();
            do {
                n--;
            } while (!(n == 0 || ThisMessage[n] == ' '));
            if (n <= 0)
                n = maxcol - ThisLine.length();
            ThisLine += ThisMessage.substr(0, n);
            ThisMessage = (n < (int)ThisMessage.length()) ? ThisMessage.substr(n + 1) : "";
            desc->insertPlainText(ThisLine.c_str());
            ThisLine.clear();
        } else {
            ThisLine += ThisMessage;
            ThisMessage.clear();
        }
    } while (!ThisMessage.empty());

    if (!ThisLine.empty())
        desc->insertPlainText(ThisLine.c_str());
}

//*********************************************
void Description::ok_cb()
{
    std::string sanitized = desc->toPlainText().replace('\n', ' ').toStdString();

    if (sanitized != viewedit->view->Description) {
        viewedit->view->Description = sanitized;
        viewedit->changed = true;
    }
    hide();
}

//*********************************************
void Description::cancel_cb()
{
    hide();
}

//**********************************************
Canvas::Canvas(QWidget *parent, const char *name, ViewEdit *v)
    : QScrollArea(parent), CurColor(), data(), viewedit(v),
      x0(0), y0(0), pixsize(2), cur_mirror(false),
      cur_w(0), cur_h(0)
{
    pixmap = QPixmap();

    imagecontainer = new QLabel;
    this->setWidget(imagecontainer);
    imagecontainer->resize(cur_w, cur_h);
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void Canvas::setSize(int w, int h)
{
    if (cur_w != w || cur_h != h) {
        if (!pixmap)
            pixmap = QPixmap(w * pixsize * 2, h * pixsize);
        else
            pixmap = pixmap.scaled(w * pixsize * 2, h * pixsize);
        cur_w = w;
        cur_h = h;

        imagecontainer->resize(w * pixsize * 2, h * pixsize);
        imagecontainer->setPixmap(pixmap);
    }
}

//*********************************************
void Canvas::mousePressEvent(QMouseEvent *event)
{
    int x = event->pos().x(), y = event->pos().y();

    if (event->button() & Qt::LeftButton)
        CurColor = viewedit->palette->left;
    else if (event->button() & Qt::RightButton)
        CurColor = viewedit->palette->right;
    UpdateCel(x - x0, y - y0);
    viewedit->changed = true;
}

//*********************************************
void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->pos().x(), y = event->pos().y();
    UpdateCel(x - x0, y - y0);
}

//*********************************************
void Canvas::drawContents(QPainter *p, int, int, int, int) const
{
    if (cur_w == 0 || cur_h == 0)
        return;
    p->drawPixmap(x0, y0, pixmap);
}

//*********************************************
void Canvas::DrawCel(int w, int h, byte *celdata, bool mirror, int size)
{
    int x, y, ww, hh, w0, h0, ww0, hh0;

    w0 = cur_w;
    h0 = cur_h;
    ww0 = (x0 + w0) * 2 * pixsize;
    hh0 = (y0 + h0) * pixsize;
    pixsize = size;
    pixmap = pixmap.scaled(cur_w * pixsize * 2, cur_h * pixsize);
    ww = (x0 + w) * 2 * pixsize;
    hh = (y0 + h) * pixsize;

    QPainter p(&pixmap);

    data = celdata;

    if (mirror) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + w * 2 - 2 - x]]);
        }
    } else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + x]]);
        }
    }
    repaint(x0, y0, std::max(ww, ww0), std::max(hh, hh0));

    imagecontainer->resize(cur_w * pixsize * 2, cur_h * pixsize);
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void Canvas::DrawCel(int w, int h, byte *celdata, bool mirror)
{
    int x, y, ww, hh, w0, h0, ww0 = 0, hh0 = 0;
    bool changed;


    if (cur_w != w || cur_h != h) {
        changed = true;
        w0 = cur_w;
        h0 = cur_h;
        ww0 = (x0 + w0) * 2 * pixsize;
        hh0 = (y0 + h0) * pixsize;
        setSize(w, h);
    } else
        changed = false;

    ww = (x0 + w) * 2 * pixsize;
    hh = (y0 + h) * pixsize;

    QPainter p(&pixmap);

    cur_mirror = mirror;
    data = celdata;

    if (mirror) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + w * 2 - 2 - x]]);
        }
    } else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + x]]);
        }
    }

    if (changed)
        repaint(x0, y0, std::max(ww, ww0), std::max(hh, hh0));
    else
        repaint(x0, y0, ww, hh);

    imagecontainer->resize(cur_w * pixsize * 2, cur_h * pixsize);
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
void Canvas::UpdateCel(int x, int y)
{
    int xn = x / pixsize / 2;
    int yn = y / pixsize;

    if (xn >= 0 && xn < cur_w && yn >= 0 && yn < cur_h) {

        if (viewedit->drawing_mode == V_DRAW) {
            QPainter p(&pixmap);

            x = xn * 2 * pixsize;
            y = yn * pixsize;

            p.fillRect(x, y, pixsize * 2, pixsize, egacolor[CurColor]);
            repaint(x0 + x, y0 + y, pixsize * 2, pixsize);
            if (cur_mirror) {
                data[yn * cur_w * 2 + cur_w * 2 - 2 - xn * 2] = CurColor;
                data[yn * cur_w * 2 + cur_w * 2 - 2 - xn * 2 + 1] = CurColor;
            } else {
                data[yn * cur_w * 2 + xn * 2] = CurColor;
                data[yn * cur_w * 2 + xn * 2 + 1] = CurColor;
            }
        } else { //FILL
            if (cur_mirror)
                viewedit->fillCel(cur_w - 1 - xn, yn, CurColor);
            else
                viewedit->fillCel(xn, yn, CurColor);
        }
    }
    imagecontainer->setPixmap(pixmap);
}

//*********************************************
bool Canvas::focusNextPrevChild(bool)
{
    setFocus();
    return true;
}


//********************************************
ViewIcon::ViewIcon(QWidget *parent, const char *name, ViewEdit *v)
    : QWidget(parent), viewedit(v) { }

//*********************************************
void ViewIcon::paintEvent(QPaintEvent *)
{
    int x, y;

    QPainter p(this);

    int w = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].width;
    int h = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].height;
    bool mirror = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].mirror;
    byte *data = viewedit->view->loops[viewedit->view->CurLoop].cels[viewedit->view->CurCel].data;

    int pixsize = 2;

    int W = viewedit->description->width();
    setGeometry((W - pixsize * w * 2) / 2, 10, pixsize * w * 2, pixsize * h);

    if (pixsize * w * 2 > width() || pixsize * h > height())
        resize(pixsize * w * 2, pixsize * h);


    if (mirror) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + w * 2 - 2 - x]]);
        }
    } else {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w * 2; x += 2)
                p.fillRect(x * pixsize, y * pixsize, pixsize * 2, pixsize, egacolor[data[y * w * 2 + x]]);
        }
    }
}
