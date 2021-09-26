QT       += core network gui widgets

TARGET = MonoDebugger
TEMPLATE = app

INCLUDEPATH += /home/me/Programme/Mono3/include/mono-2.0
#LIBS += -L/home/me/Programme/Mono3/lib/static -lmonosgen-2.0 -ldl -lrt
#LIBS += -L/home/me/Programme/Mono3/lib -lmonosgen-2.0
# we have to use a separate executable otherwise debugging doesn't work
DEFINES += _MONO_ENGINE_EXT_

SOURCES += DebuggerGui.cpp \
    MonoEngine.cpp \
    MonoDebugger.cpp

HEADERS += \
    MonoEngine.h \
    MonoDebugger.h \
    DebuggerGui.h \
    MonoDebuggerPrivate.h

include( ../GuiTools/Menu.pri )
