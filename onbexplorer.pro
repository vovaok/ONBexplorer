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
}

CONFIG += c++11

COMPONENTS = d:/projects/qt/components5
include($$COMPONENTS/onb/onb.pri)
include ($$COMPONENTS/panel3d/qpanel3d.pri)

win32: {
    CONFIG(release,debug|release) {
        QMAKE_POST_LINK += windeployqt --no-translations $$DESTDIR
    }
}

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
