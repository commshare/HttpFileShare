
TEMPLATE = app
QT += core

CONFIG += console
#DEFINES += 
DESTDIR = ../bin

LIBS += -L$$PWD/lib \
		-lsqlite3
		
HEADERS += mongoose.h \
			window.h
			
SOURCES += mongoose.c \
		   main.cpp \
           window.cpp
		   
		   
RESOURCES  = systray.qrc
RC_FILE = myapp.rc
