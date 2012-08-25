TEMPLATE = app
CONFIG	 = qt warn_on release thread
#CONFIG  = qt warn_on debug thread
# DEFINES += QT_DLL QT_THREAD_SUPPORT # win32
QMAKE_CXXFLAGS += -Wno-unused-result
HEADERS	 = agicommands.h \
		dir.h \
		game.h \
		global.h \
		helpwindow.h \
		linklist.h \
		logedit.h \
		logic.h \
		menu.h \
		midi.h \
		object.h \
		objedit.h \
		options.h \
		picedit.h \
		picture.h \
		preview.h \
		resources.h \
		roomgen.h \
		util.h \
		view.h \
		viewedit.h \
		words.h \
		wordsedit.h \
		wutil.h \
		bmp2agipic.h
SOURCES  = agicommands.cpp \
		agiplay.cpp \
		bpicture.cpp \
		dir.cpp \
		game.cpp \
		helpwindow.cpp \
		linklist.cpp \
		logcompile.cpp \
		logdecode.cpp \
		logedit.cpp \
		logic.cpp \
		main.cpp \
		menu.cpp \
		midi.cpp \
		object.cpp \
		objedit.cpp \
		options.cpp \
		picedit.cpp \
		picture.cpp \
		preview.cpp \
		resources.cpp \
		roomgen.cpp \
		util.cpp \
		view.cpp \
		viewedit.cpp \
		words.cpp \
		wordsedit.cpp \
		wutil.cpp \
		bmp2agipic.cpp
TARGET   = agistudio

#The following line was inserted by qt3to4
QT +=  qt3support 
