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


#include <QAbstractButton>
#include <QFileDialog>
#include <QSettings>

#include "options.h"
#include "game.h"


//***********************************************
Options::Options(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    connect(pushButtonSrcDirBrowse, &QPushButton::clicked, this, &Options::browse_abs);
    connect(pushButtonTemplateDirBrowse, &QPushButton::clicked, this, &Options::browse_template);
    connect(pushButtonHelpDirBrowse, &QPushButton::clicked, this, &Options::browse_help);
    connect(pushButtonInterpPathBrowse, &QPushButton::clicked, this, &Options::browse_interpreter);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &Options::on_accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &Options::on_reject);
    connect(buttonBox, &QDialogButtonBox::clicked, this, &Options::on_buttonBox_clicked);

    connect(radioButtonGameDir, &QRadioButton::clicked, this, &Options::on_update_directories);
    connect(radioButtonFullPath, &QRadioButton::clicked, this, &Options::on_update_directories);

    populate_settings();
}

//***********************************************
void Options::apply_settings()
{
    game->settings->setValue("DefaultResourceType", comboBoxDefaultResource->currentIndex());
    game->settings->setValue("PictureEditorStyle", comboBoxPicEditStyle->currentIndex());
    game->settings->setValue("ExtractLogicAsText", radioButtonExtractAsText->isChecked());

    game->settings->setValue("UseRelativeSrcDir", radioButtonGameDir->isChecked());
    game->settings->setValue("RelativeSrcDir", lineEditGameDirSrc->text());
    game->settings->setValue("AbsoluteSrcDir", lineEditFullPathSrc->text());

    game->settings->setValue("TemplateDir", lineEditTemplateDir->text());
    game->settings->setValue("HelpDir", lineEditHelpDir->text());

    game->settings->setValue("LogicEditor/ShowAllMessages", checkBoxShowAllMessages->isChecked());
    game->settings->setValue("LogicEditor/ShowElsesAsGotos", checkBoxShowElseAsGoto->isChecked());
    game->settings->setValue("LogicEditor/ShowSpecialSyntax", checkBoxShowSpecialSyntax->isChecked());

    game->settings->setValue("Interpreter", comboBoxInterpreter->currentIndex());
    game->settings->setValue("InterpreterPath", lineEditInterpPath->text());
    game->settings->setValue("InterpreterArgs", lineEditInterpArgs->text());
}

//***********************************************
void Options::reset_to_defaults()
{
    game->reset_settings();
    populate_settings();
}

//***********************************************
void Options::populate_settings()
{
    comboBoxDefaultResource->setCurrentIndex(game->settings->value("DefaultResourceType").toInt());
    comboBoxPicEditStyle->setCurrentIndex(game->settings->value("PictureEditorStyle").toInt());
    radioButtonExtractAsText->setChecked(game->settings->value("ExtractLogicAsText").toBool());
    radioButtonExtractAsBin->setChecked(!game->settings->value("ExtractLogicAsText").toBool());

    radioButtonGameDir->setChecked(game->settings->value("UseRelativeSrcDir").toBool());
    radioButtonFullPath->setChecked(!game->settings->value("UseRelativeSrcDir").toBool());
    lineEditGameDirSrc->setText(game->settings->value("RelativeSrcDir").toString());
    lineEditFullPathSrc->setText(game->settings->value("AbsoluteSrcDir").toString());

    lineEditTemplateDir->setText(game->settings->value("TemplateDir").toString());
    lineEditHelpDir->setText(game->settings->value("HelpDir").toString());

    checkBoxShowAllMessages->setChecked(game->settings->value("LogicEditor/ShowAllMessages").toBool());
    checkBoxShowElseAsGoto->setChecked(game->settings->value("LogicEditor/ShowElsesAsGotos").toBool());
    checkBoxShowSpecialSyntax->setChecked(game->settings->value("LogicEditor/ShowSpecialSyntax").toBool());

    comboBoxInterpreter->setCurrentIndex(game->settings->value("Interpreter").toInt());
    lineEditInterpPath->setText(game->settings->value("InterpreterPath").toString());
    lineEditInterpArgs->setText(game->settings->value("InterpreterArgs").toString());

    on_update_directories();
}

//***********************************************
void Options::browse_abs()
{
    QString path = QFileDialog::getExistingDirectory();
    if (!path.isNull())
        lineEditFullPathSrc->setText(path);
}

//***********************************************
void Options::browse_template()
{
    QString path = QFileDialog::getExistingDirectory();
    if (!path.isNull())
        lineEditTemplateDir->setText(path);
}

//***********************************************
void Options::browse_help()
{
    QString path = QFileDialog::getExistingDirectory();
    if (!path.isNull())
        lineEditHelpDir->setText(path);
}

//***********************************************
void Options::browse_interpreter()
{
    QString path = QFileDialog::getOpenFileName();
    if (!path.isNull())
        lineEditInterpPath->setText(path);
}

//***********************************************
void Options::on_update_directories()
{
    bool toggle = radioButtonGameDir->isChecked();

    lineEditGameDirSrc->setEnabled(toggle);
    lineEditFullPathSrc->setEnabled(!toggle);
    pushButtonSrcDirBrowse->setEnabled(!toggle);
}

//***********************************************
void Options::on_accept()
{
    apply_settings();
    close();
}

//***********************************************
void Options::on_reject()
{
    populate_settings();
    close();
}

//***********************************************
void Options::on_buttonBox_clicked(QAbstractButton *button)
{
    switch (buttonBox->buttonRole(button)) {
        case QDialogButtonBox::ApplyRole:
            apply_settings();
            break;
        case QDialogButtonBox::ResetRole:
            reset_to_defaults();
            break;
        default:
            break;
    }
}

//***********************************************
