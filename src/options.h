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

#ifndef OPTIONS_H
#define OPTIONS_H


#include <QDialog>

#include "ui/ui_options.h"


class QAbstractButton;

class Options : public QDialog, private Ui::Settings
{
    Q_OBJECT
public:
    explicit Options(QWidget *parent = nullptr);

private:
    void populate_settings();
    void apply_settings();
    void reset_to_defaults();

    void browse_abs();
    void browse_template();
    void browse_help();
    void browse_interpreter();

    void on_update_directories();
    void on_accept();
    void on_reject();
    void on_buttonBox_clicked(QAbstractButton *);
};


#endif
