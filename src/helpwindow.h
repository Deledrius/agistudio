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

#include <q3textbrowser.h>
#include <q3mainwindow.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdir.h>
//Added by qt3to4:
#include <Q3PopupMenu>
#include <QShowEvent>
#include <QHideEvent>
#include <QEvent>

class QComboBox;
class Q3PopupMenu;

class HelpWindow : public Q3MainWindow
{
    Q_OBJECT
public:
    HelpWindow( const QString& home_,  const QString& path, QWidget* parent = 0, const char *name=0 );
    ~HelpWindow();
    void setSource(char *filename);
private slots:
    void setBackwardAvailable( bool );
    void setForwardAvailable( bool );

    void textChanged();
    void openFile();
    void newWindow();

    void pathSelected( const QString & );
    void histChosen( int );
    void bookmChosen( int );
    void addBookmark();
    void showEvent(  QShowEvent * );
    void hideEvent(  QHideEvent * );  
    
private:
    bool eventFilter( QObject * o, QEvent * e );
    void readHistory();
    void readBookmarks();
    
    Q3TextBrowser* browser;
    QComboBox *pathCombo;
    int backwardId, forwardId;
    QString selectedURL;
    QDir path;
    QStringList fileList, history, bookmarks;
    QMap<int, QString> mHistory, mBookmarks;
    Q3PopupMenu *hist, *bookm;
};


extern HelpWindow *helpwindow,*helpwindow1;


#endif

