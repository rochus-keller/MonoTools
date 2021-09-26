#-------------------------------------------------
#
# Project created by QtCreator 2021-04-11T12:09:12
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = MonoTools
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += /home/me/Programme/Mono3/include/mono-2.0
LIBS += -L/home/me/Programme/Mono3/lib/static -lmonosgen-2.0 -ldl -lrt
#LIBS += -L/home/me/Programme/Mono3/lib -lmonosgen-2.0

SOURCES += main.cpp \
    MonoEngine.cpp \
    Assembly.cpp \
    MonoDebugger.cpp

HEADERS += \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/appdomain.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/assembly.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/attrdefs.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/blob.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/class.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/debug-helpers.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/debug-mono-symfile.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/environment.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/exception.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/image.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/loader.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/metadata.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/mono-config.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/mono-debug.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/mono-gc.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/object.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/opcodes.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/profiler.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/reflection.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/row-indexes.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/sgen-bridge.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/threads.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/tokentype.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/metadata/verify.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/utils/mono-counters.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/utils/mono-dl-fallback.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/utils/mono-error.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/utils/mono-logger.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/utils/mono-publib.h \
    ../../../Programme/Mono3/include/mono-2.0/mono/jit/jit.h \
    MonoEngine.h \
    Assembly.h \
    MonoDebugger.h
