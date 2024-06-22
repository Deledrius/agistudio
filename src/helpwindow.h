/****************************************************************************
** $Id$
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef HELPWINDOW_H
#define HELPWINDOW_H


#include <QDir>
#include <QMainWindow>
#include <QMap>
#include <QString>
#include <QStringList>


class QAction;
class QComboBox;
class QMenu;
class QTextBrowser;

class HelpWindow : public QMainWindow
{
    Q_OBJECT
public:
    HelpWindow(const QString &home_,  const QString &path, QWidget *parent = 0, const char *name = 0);
    ~HelpWindow();
    void setSource(char *filename);
private slots:
    void setBackwardAvailable(bool);
    void setForwardAvailable(bool);

    void textChanged();
    void openFile();
    void newWindow();

    void pathSelected(const QString &);
    void histChosen(int);
    void bookmChosen(int);
    void addBookmark();
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);

private:
    bool eventFilter(QObject *o, QEvent *e);
    void readHistory();
    void readBookmarks();

    QTextBrowser *browser;
    QComboBox *pathCombo;
    QAction *backwardAction, *forwardAction;
    QString selectedURL;
    QDir path;
    QStringList fileList, history, bookmarks;
    QMap<int, QString> mHistory, mBookmarks;
    QMenu *hist, *bookm;
};


extern HelpWindow *helpwindow, *helpwindow1;


#endif

