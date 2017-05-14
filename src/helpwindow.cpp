/****************************************************************************
**
** This help window was modified from an example code by QT.
** Original license:
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/
//my modifications of the original source:
//included .xpm in the source, removed 'about' buttons, added setSource,
//removed tr() (didn't compile with Mandrake 7.0)
//added hideEvent,showEvent

#include "helpwindow.h"

#include <QStatusBar>
#include <QPixmap>
#include <QMenuBar>
#include <QToolButton>
#include <QIcon>
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QLineEdit>
#include <QObject>
#include <QFileInfo>
#include <QDataStream>
#include <QShowEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QToolBar>

#include <ctype.h>

#include "home.xpm"
#include "forward.xpm"
#include "back.xpm"

#include "menu.h"

HelpWindow *helpwindow,*helpwindow1;

HelpWindow::HelpWindow( const QString& home_, const QString& _path, QWidget* parent, const char *name )
    : QMainWindow(parent), pathCombo( 0 ), selectedURL(),
      path( QFileInfo( home_ ).path(), "*.htm*" )
{
    readHistory();
    readBookmarks();

    fileList = path.entryList();

    browser = new QTextBrowser( this );
    browser->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    connect( browser, SIGNAL( textChanged() ),
             this, SLOT( textChanged() ) );

    setCentralWidget( browser );
    browser->setOpenExternalLinks(true);

    if ( !home_.isEmpty() )
        browser->setSource("file:///" + home_);

    connect( browser, SIGNAL( highlighted( const QString&) ),
             statusBar(), SLOT( message( const QString&)) );

    resize(640, 700);

    QMenu* file = new QMenu( this );
    file->setTitle("&File");
    file->addAction("&New Window", this, SLOT( newWindow() ), Qt::ALT | Qt::Key_N);
    file->addAction("&Open File", this, SLOT( openFile() ), Qt::ALT | Qt::Key_O);
    file->addSeparator();
    file->addAction("&Close", this, SLOT( hide() ), Qt::ALT | Qt::Key_Q);

    QMenu* go = new QMenu( this );
    go->setTitle("&Go");
    backwardAction = go->addAction(QPixmap(back), "&Backward", browser, SLOT( backward()), Qt::ALT | Qt::Key_Left);
    forwardAction = go->addAction(QPixmap(forward), "&Forward", browser, SLOT( forward()), Qt::ALT | Qt::Key_Right);
    go->addAction(QPixmap(home), "&Home", browser, SLOT( home() ));

    hist = new QMenu( this );
    hist->setTitle("H&istory");
    QStringList::Iterator it = history.begin();
    for (; it != history.end(); ++it)
        hist->addAction(*it);
    connect(hist, SIGNAL( activated(int) ), this, SLOT( histChosen(int) ));

    bookm = new QMenu( this );
    bookm->setTitle("&Bookmarks");
    bookm->addAction( "Add Bookmark", this, SLOT( addBookmark() ) );
    bookm->addSeparator();

    QStringList::Iterator it2 = bookmarks.begin();
    for (; it2 != bookmarks.end(); ++it2)
        bookm->addAction(*it2);
    connect( bookm, SIGNAL( activated( int ) ),
             this, SLOT( bookmChosen( int ) ) );

    menuBar()->addMenu(file);
    menuBar()->addMenu(go);
    menuBar()->addMenu(hist);
    menuBar()->addMenu(bookm);

    forwardAction->setEnabled(false);
    backwardAction->setEnabled(false);
    connect( browser, SIGNAL( backwardAvailable( bool ) ),
             this, SLOT( setBackwardAvailable( bool ) ) );
    connect( browser, SIGNAL( forwardAvailable( bool ) ),
             this, SLOT( setForwardAvailable( bool ) ) );

    QToolBar* toolbar = new QToolBar("Navigation", this);
    addToolBar(toolbar);

    QAction* bButton;
    bButton = new QAction(QPixmap(back), "Backward", browser);
    connect(bButton, &QAction::triggered, browser, &QTextBrowser::backward);
    connect(browser, SIGNAL(backwardAvailable(bool)), bButton, SLOT(setEnabled(bool)));
    bButton->setEnabled( false );
    toolbar->addAction(bButton);

    QAction* fButton;
    fButton = new QAction(QPixmap(forward), "Forward", browser);
    connect(fButton, &QAction::triggered, browser, &QTextBrowser::forward);
    connect(browser, SIGNAL(forwardAvailable(bool)), fButton, SLOT(setEnabled(bool)));
    fButton->setEnabled( false );
    toolbar->addAction(fButton);

    QAction* homeButton;
    homeButton = new QAction(QPixmap(home), "Home", browser);
    connect(homeButton, &QAction::triggered, browser, &QTextBrowser::home);
    toolbar->addAction(homeButton);

    toolbar->addSeparator();

    pathCombo = new QComboBox(browser);
    toolbar->addWidget(pathCombo);
    connect( pathCombo, SIGNAL( activated( const QString & ) ),
             this, SLOT( pathSelected( const QString & ) ) );
    toolbar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    pathCombo->addItem( home_ );
    pathCombo->installEventFilter( this );

    auto l = findChildren<QLineEdit *>();
    if (!l.empty())
        l.at(0)->installEventFilter(this);

    browser->setFocus();
}


void HelpWindow::setBackwardAvailable( bool b)
{
    backwardAction->setEnabled(b);
}

void HelpWindow::setForwardAvailable( bool b)
{
    forwardAction->setEnabled(b);
}


void HelpWindow::textChanged()
{
    if ( browser->documentTitle().isNull() )
        setWindowTitle(browser->source().toString());
    else
        setWindowTitle( browser->documentTitle() ) ;

    selectedURL = windowTitle();
    if ( !selectedURL.isEmpty() && pathCombo ) {
        path = QDir( QFileInfo( selectedURL ).path(), "*.htm*" );
        fileList = path.entryList();
        bool exists = false;
        int i;
        for ( i = 0; i < pathCombo->count(); ++i ) {
            if ( pathCombo->itemText( i ) == selectedURL ) {
                exists = true;
                break;
            }
        }
        if ( !exists ) {
            pathCombo->addItem( selectedURL, 0 );
            pathCombo->setCurrentIndex( 0 );
            hist->addAction(selectedURL);
        } else
            pathCombo->setCurrentIndex( i );
        selectedURL = QString::null;
    }
}

HelpWindow::~HelpWindow()
{
    history.clear();
    QMap<int, QString>::Iterator it = mHistory.begin();
    for ( ; it != mHistory.end(); ++it )
        history.append( *it );

    QFile f( QDir::currentPath() + "/.history" );
    f.open( QIODevice::WriteOnly );
    QDataStream s( &f );
    s << history;
    f.close();

    bookmarks.clear();
    QMap<int, QString>::Iterator it2 = mBookmarks.begin();
    for ( ; it2 != mBookmarks.end(); ++it2 )
        bookmarks.append( *it2 );

    QFile f2( QDir::currentPath() + "/.bookmarks" );
    f2.open( QIODevice::WriteOnly );
    QDataStream s2( &f2 );
    s2 << bookmarks;
    f2.close();
}


void HelpWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open"), QDir::currentPath(), tr("HTML Files (*.htm *html)"));
    if (!filename.isNull())
        browser->setSource(filename);
}

void HelpWindow::setSource(char *filename)
{
    browser->setSource("file:///" + QString(filename));
}

void HelpWindow::newWindow()
{
    ( new HelpWindow(browser->source().toString(), "qbrowser") )->show();
}

void HelpWindow::pathSelected( const QString &_path )
{
    browser->setSource( _path );
    path = QDir( QFileInfo( _path ).path(), "*.htm*" );
    fileList = path.entryList();
    QMap<int, QString>::Iterator it = mHistory.begin();
    bool exists = false;
    for ( ; it != mHistory.end(); ++it ) {
        if ( *it == _path ) {
            exists = true;
            break;
        }
    }
    if ( !exists )
        hist->addAction( _path );
}

bool HelpWindow::eventFilter( QObject * o, QEvent * e )
{
    auto l = findChildren<QLineEdit *>();
    if (l.empty())
        return false;

    QLineEdit *lined = l.at(0);

    if ( ( o == pathCombo || o == lined ) &&
         e->type() == QEvent::KeyPress ) {

        if (!((QKeyEvent *)e)->text().isEmpty()) {
            if ( lined->hasSelectedText() )
                lined->del();
            QString nt( lined->text() );
            nt.remove( 0, nt.lastIndexOf( '/' ) + 1 );
            nt.truncate( lined->cursorPosition() );
            nt += (((QKeyEvent *)e)->text());

            QStringList::Iterator it = fileList.begin();
            while ( it != fileList.end() && (*it).left( nt.length() ) != nt )
                ++it;

            if ( !(*it).isEmpty() ) {
                nt = *it;
                int cp = lined->cursorPosition() + 1;
                int l = path.canonicalPath().length() + 1;
                lined->setText(path.canonicalPath() + "/" + nt);
                lined->setCursorPosition(cp);
                lined->setSelection(cp, l + nt.length());
                return true;
            }
        }
    }
    return false;
}

void HelpWindow::readHistory()
{
    if ( QFile::exists( QDir::currentPath() + "/.history" ) ) {
        QFile f( QDir::currentPath() + "/.history" );
        f.open( QIODevice::ReadOnly );
        QDataStream s( &f );
        s >> history;
        f.close();
        while ( history.count() > 20 )
            history.removeOne( *history.begin() );
    }
}

void HelpWindow::readBookmarks()
{
    if ( QFile::exists( QDir::currentPath() + "/.bookmarks" ) ) {
        QFile f( QDir::currentPath() + "/.bookmarks" );
        f.open( QIODevice::ReadOnly );
        QDataStream s( &f );
        s >> bookmarks;
        f.close();
    }
}

void HelpWindow::histChosen( int i )
{
    if ( mHistory.contains( i ) )
        browser->setSource( mHistory[ i ] );
}

void HelpWindow::bookmChosen( int i )
{
    if ( mBookmarks.contains( i ) )
        browser->setSource( mBookmarks[ i ] );
}

void HelpWindow::addBookmark()
{
    bookm->addAction(new QAction(windowTitle(), this));
}

//*********************************************
void HelpWindow::hideEvent( QHideEvent * )
{

  if(window_list && window_list->isVisible())window_list->draw();

}

//*********************************************
void HelpWindow::showEvent( QShowEvent * )
{
 
  if(window_list && window_list->isVisible())window_list->draw();

}
