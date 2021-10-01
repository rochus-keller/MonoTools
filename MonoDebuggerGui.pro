QT       += core network gui widgets

TARGET = MonoDebugger
TEMPLATE = app

INCLUDEPATH += ..
#LIBS += -L/home/me/Programme/Mono3/lib/static -lmonosgen-2.0 -ldl -lrt
#LIBS += -L/home/me/Programme/Mono3/lib -lmonosgen-2.0
# we have to use a separate executable otherwise debugging doesn't work
DEFINES += _MONO_ENGINE_EXT_ HAVE_C99INCLUDES

SOURCES += DebuggerGui.cpp \
    MonoEngine.cpp \
    MonoDebugger.cpp

HEADERS += \
    MonoEngine.h \
    MonoDebugger.h \
    DebuggerGui.h \
    MonoDebuggerPrivate.h

include( ../GuiTools/Menu.pri )
