
TEMPLATE = app
QT += core

CONFIG += console
#DEFINES += 
DESTDIR = $$PWD/bin



HEADERS += src/mongoose.h \
		src/window.h
			
SOURCES += src/mongoose.c \
		   src/main.cpp \
	           src/window.cpp
!win32{	SOURCES += $$PWD/src/sqlite3/sqlite3.c
}		   

win32{
LIBS += -L$$PWD/src/lib \
		-lsqlite3
} 		   
RESOURCES  = src/systray.qrc
RC_FILE = src/myapp.rc
