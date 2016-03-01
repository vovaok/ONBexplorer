#-------------------------------------------------
#
# Project created by QtCreator 2012-04-10T13:46:26
#
#-------------------------------------------------

QT       += core widgets network

TARGET = onb
TEMPLATE = app
DESTDIR = $$PWD/$$TARGET

CONFIG += c++11

COMPONENTS = d:/projects/qt/components5
include($$COMPONENTS/commlib/commlib.pri)
include($$COMPONENTS/usbhid/usbhid.pri)
include ($$COMPONENTS/panel3d/qpanel3d.pri)

PROJ_DIR = d:/projects
OBJNET_DIR = $$PROJ_DIR/iar/components/stm32++/src/objnet

INCLUDEPATH += $$OBJNET_DIR

#win32: {
#    CONFIG(release,debug|release) {
#        QMAKE_POST_LINK += windeployqt --no-translations $$DESTDIR
#    }
#}

SOURCES += main.cpp\
        mainwindow.cpp \
    $$OBJNET_DIR/objnetCommonNode.cpp \
    $$OBJNET_DIR/objnetmaster.cpp \
    $$OBJNET_DIR/objnetInterface.cpp \
    $$OBJNET_DIR/objnetdevice.cpp \
    serialcaninterface.cpp \
    objnetvirtualinterface.cpp \
    objnetvirtualserver.cpp \
    $$OBJNET_DIR/objnetnode.cpp \
    usbhidonbinterface.cpp \
    $$OBJNET_DIR/objectinfo.cpp

HEADERS  += mainwindow.h \
    $$OBJNET_DIR/objnetCommonNode.h \
    $$OBJNET_DIR/objnetmaster.h \
    $$OBJNET_DIR/objnetInterface.h \
    $$OBJNET_DIR/objnetmsg.h \
    $$OBJNET_DIR/objnetcommon.h \
    $$OBJNET_DIR/objnetdevice.h \
    serialcaninterface.h \
    objnetvirtualinterface.h \
    objnetvirtualserver.h \
    $$OBJNET_DIR/objnetnode.h \
    usbhidonbinterface.h \
    $$OBJNET_DIR/objectinfo.h

FORMS    += mainwindow.ui
