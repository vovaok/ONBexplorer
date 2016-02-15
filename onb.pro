#-------------------------------------------------
#
# Project created by QtCreator 2012-04-10T13:46:26
#
#-------------------------------------------------

QT       += core widgets network

TARGET = onb
TEMPLATE = app

COMPONENTS = d:/projects/qt/components5
include($$COMPONENTS/commlib/commlib.pri)
include($$COMPONENTS/usbhid/usbhid.pri)

win32: {
    CONFIG(release,debug|release) {
        DESTDIR = "$$PWD/../$$TARGET"
        QMAKE_POST_LINK += windeployqt --no-translations $$DESTDIR
    }
}

SOURCES += main.cpp\
        mainwindow.cpp \
    objnetCommonNode.cpp \
    objnetmaster.cpp \
    objnetInterface.cpp \
    objnetdevice.cpp \
    serialcaninterface.cpp \
    objnetvirtualinterface.cpp \
    objnetvirtualserver.cpp \
    objnetnode.cpp \
    usbhidonbinterface.cpp

HEADERS  += mainwindow.h \
    objnetCommonNode.h \
    objnetmaster.h \
    objnetInterface.h \
    objnetmsg.h \
    objnetcommon.h \
    objnetdevice.h \
    serialcaninterface.h \
    objnetvirtualinterface.h \
    objnetvirtualserver.h \
    objnetnode.h \
    usbhidonbinterface.h

FORMS    += mainwindow.ui
