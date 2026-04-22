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

CONFIG(static, static|shared) {
    DESTDIR = $$DESTDIR/../bin
}

CONFIG += c++14

# Uncomment supported ONB interfaces
DEFINES += ONB_SERIAL
#DEFINES += ONB_USBHID
DEFINES += ONB_UDP
DEFINES += ONB_VIRTUAL

COMPONENTS = $$(COMPONENTS5)
message($$COMPONENTS)

include($$COMPONENTS/onb/onb.pri)
include($$COMPONENTS/commlib/commlib.pri)
#include($$COMPONENTS/panel3d/panel3d.pri)
include($$COMPONENTS/megaWidgets/megaWidgets.pri)

win32: {
  CONFIG(shared, static|shared) {
    CONFIG(release,debug|release) {
        QMAKE_POST_LINK += windeployqt --no-translations $$DESTDIR
    }
  }
}

SOURCES += main.cpp\
#    $$COMPONENTS/commlib/serialframe.cpp \
#    $$COMPONENTS/commlib/serialportwidget.cpp \
#    $$COMPONENTS/megaWidgets/graphwidget.cpp \
#    $$COMPONENTS/onb/serialonbinterface.cpp \
#    $$COMPONENTS/onb/udponbinterface.cpp \
    analyzerwidget.cpp \
    mainwindow.cpp \
    objlogger.cpp \
    upgradewidget.cpp \
    plotwidget.cpp \
    objtable.cpp \
    api/apicommon.cpp \
    api/apiserver.cpp \

HEADERS  += mainwindow.h \
#    $$COMPONENTS/commlib/serialframe.h \
#    $$COMPONENTS/commlib/serialportwidget.h \
#    $$COMPONENTS/onb/serialonbinterface.h \
    analyzerwidget.h \
    objlogger.h \
    plotwidget.h \
    upgradewidget.h \
    objtable.h \
    api/apicommon.h \
    api/apiserver.h

FORMS    += mainwindow.ui

RC_ICONS += icon.ico
