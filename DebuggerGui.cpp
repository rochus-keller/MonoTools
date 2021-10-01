#include "DebuggerGui.h"
#include "MonoDebugger.h"
#include "MonoEngine.h"
#include <GuiTools/AutoMenu.h>
#include <QApplication>
#include <QtDebug>
#include <QThread>
#include <QShortcut>
using namespace Mono;

DebuggerGui::DebuggerGui():d_status(Idle),d_curThread(0),d_byteLevel(false),d_rootDomain(0)
{
    // when no mdb is present stepping with d_byteLevel=true still works; d_byteLevel=false is like continue

    d_dbg = new Debugger(this);
    const quint16 port = d_dbg->open();
    qDebug() << "debugger port" << port;

    d_eng = new Engine(this);

    connect( d_eng,SIGNAL(onError(QString)),this,SLOT(onError(QString)));
    connect(d_dbg,SIGNAL(sigEvent(DebuggerEvent)),this,SLOT(onEvent(DebuggerEvent)) );

#if 0
    QThread* t = new QThread(this);
    t->setObjectName("Mono Engine");
    d_eng->moveToThread(t);
    t->start();
#endif

    d_eng->setMonoDir("/home/me/Entwicklung/Modules/build-MonoDebuggerGui-Qt_5_4_2-Debug/mono");
    d_eng->setAssemblySearchPaths( QStringList() << "/home/me/Entwicklung/Modules/build-MonoDebuggerGui-Qt_5_4_2-Debug", true );
    d_eng->init(port);

    Gui::AutoMenu* m = new Gui::AutoMenu(tr("File"),this);
    m->addCommand(tr("Run"), this, SLOT(onRun()), tr("CTRL+R"));
    m->addCommand(tr("Suspend"), this, SLOT(onSuspend()) );
    m->addCommand(tr("Resume"), this, SLOT(onResume()));
    m->addCommand(tr("Step in"),this,SLOT(onStepIn()), tr("F11"));
    m->addCommand(tr("Step Over"),this,SLOT(onStepOver()), tr("F10"));
    m->addCommand(tr("Step Out"),this,SLOT(onStepOut()), tr("SHIFT+F11"));
    m->addCommand(tr("Break"), this, SLOT(onBreak()) );
    m->addCommand(tr("Exit"), this, SLOT(onExit()) );

    m = new Gui::AutoMenu(tr("Objects"),this);
    m->addCommand(tr("Threads"), this, SLOT(onGetThreads()) );
    m->addCommand(tr("Stack"), this, SLOT(onGetStack()) );
    m->addCommand(tr("Params and Locals"), this, SLOT(onLocals()) );

    new QShortcut(tr("CTRL+R"),this,SLOT(onRun()),0,Qt::WindowShortcut);
    new QShortcut(tr("F11"),this,SLOT(onStepIn()),0,Qt::WindowShortcut);
    new QShortcut(tr("SHIFT+F11"),this,SLOT(onStepOut()),0,Qt::WindowShortcut);
    new QShortcut(tr("F10"),this,SLOT(onStepOver()),0,Qt::WindowShortcut);

}

DebuggerGui::~DebuggerGui()
{
#if 0
    QThread* t = d_eng->thread();
    d_eng->deleteLater();
    t->quit();
    t->wait();
#endif
}

void DebuggerGui::onRun()
{
    ENABLED_IF(d_status == Idle);
    QStringList arg;
    arg << "Gugus" << "Alpha" << "Beta";
    d_eng->run("HelloWorld.exe", arg );
    d_status = Running;
}

void DebuggerGui::onSuspend()
{
    ENABLED_IF(d_status != Idle);
    d_dbg->suspend();
}

void DebuggerGui::onResume()
{
    ENABLED_IF(d_status != Idle);
    d_dbg->resume();
}

void DebuggerGui::onExit()
{
    ENABLED_IF(d_status != Idle);
    d_dbg->exit();
    //QMetaObject::invokeMethod( d_eng, "finish");
    d_status = Idle;
}

void DebuggerGui::onGetThreads()
{
    ENABLED_IF(d_status != Idle);
    d_threads = d_dbg->allThreads();
    qDebug() << "*** thread list";
    foreach( quint32 t, d_threads )
        qDebug() << t << d_dbg->getThreadName(t) << Debugger::s_state[d_dbg->getThreadState(t)];
}

void DebuggerGui::onGetStack()
{
    ENABLED_IF(d_status != Idle && d_curThread != 0);

    QList<Debugger::Frame> stack = d_dbg->getStack(d_curThread);
    qDebug() << "*** stack of thread" << d_curThread;
    for( int i = 0; i < stack.size(); i++ )
    {
        // QByteArray bytes = d_dbg->getMethodBody(stack[i].method).toHex();
        const quint32 type = d_dbg->getMethodOwner(stack[i].method);
        Debugger::TypeInfo info = d_dbg->getTypeInfo(type);
        Debugger::MethodDbgInfo meth = d_dbg->getMethodInfo(stack[i].method);
        qDebug() << i << info.fullName << d_dbg->getMethodName(stack[i].method) << stack[i].il_offset
                 << meth.codeSize << d_dbg->getMethodKind(stack[i].method) << d_dbg->isMethodStatic(stack[i].method)
                 << meth.sourceFile << meth.lines.size();
    }
}

void DebuggerGui::onStepIn()
{
    ENABLED_IF(d_status != Idle);
    d_dbg->stepIn(d_curThread,!d_byteLevel);
}

void DebuggerGui::onStepOver()
{
    ENABLED_IF(d_status != Idle);
    d_dbg->stepOver(d_curThread,!d_byteLevel);
}

void DebuggerGui::onStepOut()
{
    ENABLED_IF(d_status != Idle);
    d_dbg->stepOut(d_curThread,!d_byteLevel);
}

void DebuggerGui::onBreak()
{
    ENABLED_IF(d_status != Idle);
    d_dbg->callUserBreak(d_curThread);
}

static QString toString( const QVariant& var )
{
    if( var.canConvert<ObjectRef>() )
    {
        ObjectRef r = var.value<ObjectRef>();
        switch(r.type)
        {
        case ObjectRef::Nil:
            return "NIL";
        case ObjectRef::String:
            return "STRING";
        case ObjectRef::SzArray:
            return "VECTOR";
        case ObjectRef::Class:
            return "CLASS";
        case ObjectRef::Array:
            return "ARRAY";
        case ObjectRef::Object:
            return "OBJECT";
        case ObjectRef::Type:
            return "TYPE";
        default:
            return "???";
        }
    }else if( var.canConvert<IntPtr>() )
        return "IntPtr";
    else if( var.canConvert<ValueType>() )
    {
        ValueType v = var.value<ValueType>();
        QString str = "struct{ ";
        for( int i = 0; i < v.fields.size(); i++ )
        {
            if( i != 0 )
                str += ", ";
            str += toString(v.fields[i]);
        }
        str += " }";
        return str;
    }else if( var.isNull() )
        return "NULL";
    else
        return var.toString();
}

void DebuggerGui::onLocals()
{
    ENABLED_IF(d_status != Idle);

    QList<Debugger::Frame> stack = d_dbg->getStack(d_curThread);
    if( stack.isEmpty() )
        return;
    const bool isStatic = d_dbg->isMethodStatic(stack.first().method);
    const quint16 params = d_dbg->getParamCount(stack.first().method);
    const quint16 locals = d_dbg->getLocalsCount(stack.first().method);
    QVariantList vals = d_dbg->getParamValues(d_curThread,stack.first().id,!isStatic,params);
    qDebug() << "*** Params:";
    for( int i = 0; i < vals.size(); i++ )
        qDebug() << toString(vals[i]).toUtf8().constData();
    qDebug() << "*** Locals:";
    vals = d_dbg->getLocalValues(d_curThread,stack.first().id,locals);
    for( int i = 0; i < vals.size(); i++ )
        qDebug() << toString(vals[i]).toUtf8().constData();

}

void DebuggerGui::onError(const QString& msg)
{
    qCritical() << msg;
}

void DebuggerGui::onEvent(const DebuggerEvent& e)
{
    qDebug() << "event arrived" << DebuggerEvent::s_event[e.event] << e.thread << e.object;
    switch( e.event )
    {
    case DebuggerEvent::VM_START:
        d_curThread = e.thread;
        d_rootDomain = e.object;
        d_dbg->enableUserBreak();
        break;
    }
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DebuggerGui gui;
    gui.show();

    return a.exec();
}



