#-------------------------------------------------
#
# Project created by QtCreator 2012-04-10T13:46:26
#
#-------------------------------------------------

QT       += core widgets network

TARGET = onbExplorer
TEMPLATE = app

CONFIG(release,debug|release) {
    DESTDIR = $$PWD/$$TARGET
    win32-msvc* {
        DESTDIR = $$PWD/onbExplorer_msvc
    }
}

CONFIG += c++11

COMPONENTS = d:/projects/qt/components5
include($$COMPONENTS/onb/onb.pri)
include($$COMPONENTS/commlib/commlib.pri)
include($$COMPONENTS/panel3d/panel3d.pri)

win32: {
    CONFIG(release,debug|release) {
        QMAKE_POST_LINK += windeployqt --no-translations $$DESTDIR
    }
}

SOURCES += main.cpp\
        mainwindow.cpp \
    upgradewidget.cpp \
    graphwidget.cpp \
    objtable.cpp

HEADERS  += mainwindow.h \
    upgradewidget.h \
    graphwidget.h \
    objtable.h

FORMS    += mainwindow.ui

RC_ICONS += icon.ico
