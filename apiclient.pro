#-------------------------------------------------
#
# Project created by QtCreator 2012-04-10T13:46:26
#
#-------------------------------------------------

QT       += core widgets network

TARGET = onbApiClient
TEMPLATE = lib
CONFIG += staticlib c++14

OBJNET_DIR = $$(OBJNET_DIR)

INCLUDEPATH += $$OBJNET_DIR
INCLUDEPATH += $$OBJNET_DIR/..

SOURCES += \
    $$OBJNET_DIR/objnetCommonNode.cpp \
    $$OBJNET_DIR/objnetmaster.cpp \
    $$OBJNET_DIR/objnetInterface.cpp \
    $$OBJNET_DIR/objnetdevice.cpp \
    $$OBJNET_DIR/objnetnode.cpp \
    $$OBJNET_DIR/objectinfo.cpp \
    $$OBJNET_DIR/onbupgrader.cpp \
    $$OBJNET_DIR/objnetmsg.cpp \

HEADERS += \
    $$OBJNET_DIR/../core/ringbuffer.h \
    $$OBJNET_DIR/objnetCommonNode.h \
    $$OBJNET_DIR/objnetmaster.h \
    $$OBJNET_DIR/objnetInterface.h \
    $$OBJNET_DIR/objnetmsg.h \
    $$OBJNET_DIR/objnetcommon.h \
    $$OBJNET_DIR/objnetdevice.h \
    $$OBJNET_DIR/objnetnode.h \
    $$OBJNET_DIR/onbupgrader.h \
    $$OBJNET_DIR/objectinfo.h \
    $$OBJNET_DIR/../core/closure.h \
    $$OBJNET_DIR/../core/closure_impl.h \

SOURCES += api/apiclient.cpp api/apicommon.cpp
HEADERS += api/apiclient.h   api/apicommon.h
