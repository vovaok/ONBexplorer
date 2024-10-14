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

CONFIG += c++14

# Uncomment supported ONB interfaces
DEFINES += ONB_SERIAL
#DEFINES += ONB_USBHID
DEFINES += ONB_UDP
DEFINES += ONB_VIRTUAL

COMPONENTS = $$PWD/../components5
include($$COMPONENTS/onb/onb.pri)
include($$COMPONENTS/commlib/commlib.pri)
#include($$COMPONENTS/panel3d/panel3d.pri)
include($$COMPONENTS/megaWidgets/megaWidgets.pri)

win32: {
    CONFIG(release,debug|release) {
        QMAKE_POST_LINK += windeployqt --no-translations $$DESTDIR
    }
}

SOURCES += main.cpp\
    ../components5/commlib/serialframe.cpp \
    ../components5/commlib/serialportwidget.cpp \
    ../components5/megaWidgets/graphwidget.cpp \
    ../components5/onb/serialonbinterface.cpp \
    ../components5/onb/udponbinterface.cpp \
    analyzerwidget.cpp \
    mainwindow.cpp \
    objlogger.cpp \
    upgradewidget.cpp \
    plotwidget.cpp \
    objtable.cpp

HEADERS  += mainwindow.h \
    ../components5/commlib/serialframe.h \
    ../components5/commlib/serialportwidget.h \
    ../components5/onb/serialonbinterface.h \
    analyzerwidget.h \
    objlogger.h \
    plotwidget.h \
    upgradewidget.h \
    objtable.h

FORMS    += mainwindow.ui

RC_ICONS += icon.ico
