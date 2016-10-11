TEMPLATE = lib

TARGET = XrCom

CONFIG -= console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += dll

QMAKE_CFLAGS_RELEASE  -= -Zc:strictStrings
QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings

SOURCES += \
    comport.cpp \
    dllmain.cpp \
    available_ports.cpp

HEADERS += \
    comport.h \
    available_ports.h

DEF_FILE +=\
    XrCom.def

OTHER_FILES +=

#LIBS += -lwinuser

DISTFILES += \
    XrCom.def
