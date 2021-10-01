#include "MonoDebugger.h"
#include "MonoDebuggerPrivate.h"
#include <QEventLoop>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtDebug>
/*
* Copyright 2021 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the MonoTools library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QCoreApplication>
#include <limits>
#include <QDateTime>
using namespace Mono;

const char* DebuggerEvent::s_event[] = {
    "VM_START",
    "VM_DEATH",
    "THREAD_START",
    "THREAD_DEATH",
    "APPDOMAIN_CREATE",
    "APPDOMAIN_UNLOAD",
    "METHOD_ENTRY",
    "METHOD_EXIT",
    "ASSEMBLY_LOAD",
    "ASSEMBLY_UNLOAD",
    "BREAKPOINT",
    "STEP",
    "TYPE_LOAD",
    "EXCEPTION",
    "KEEPALIVE",
    "USER_BREAK",
    "USER_LOG",
};

static inline const char* toString(int e)
{
    switch(e)
    {
    case ERR_NONE:
        return "";
    case ERR_INVALID_OBJECT:
        return "INVALID_OBJECT";
    case ERR_INVALID_FIELDID:
        return "INVALID_FIELDID";
    case ERR_INVALID_FRAMEID:
        return "INVALID_FRAMEID";
    case ERR_NOT_IMPLEMENTED:
        return "NOT_IMPLEMENTED";
    case ERR_NOT_SUSPENDED:
        return "NOT_SUSPENDED";
    case ERR_INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case ERR_UNLOADED:
        return "UNLOADED";
    case ERR_NO_INVOCATION:
        return "NO_INVOCATION";
    case ERR_ABSENT_INFORMATION:
        return "ABSENT_INFORMATION";
    case ERR_NO_SEQ_POINT_AT_IL_OFFSET:
        return "NO_SEQ_POINT_AT_IL_OFFSET";
    case ERR_LOADER_ERROR:
        return "LOADER_ERROR";
    default:
        return "<unknown error>";
    }
}

static inline quint64 readUint64( const char* buf )
{
    return (((quint8)buf[0]) << 56) | (((quint8)buf[1]) << 48) |
            (((quint8)buf[2]) << 40) | (((quint8)buf[3]) << 32) |
            (((quint8)buf[4]) << 24) | (((quint8)buf[5]) << 16) |
            (((quint8)buf[6]) << 8) | (((quint8)buf[7]) << 0);

}

static inline int readUint64( const QByteArray& buf, int off, quint64& res )
{
    if( buf.size() < off + 8 )
        throw 0;
    res = readUint64( buf.constData() + off );
    return 8;
}

static inline quint32 readUint32( const char* buf )
{
    return (((quint8)buf[0]) << 24) | (((quint8)buf[1]) << 16) |
            (((quint8)buf[2]) << 8) | (((quint8)buf[3]) << 0);

}

static int readUint32( const QByteArray& buf, int off, quint32& res )
{
    if( buf.size() < off + 4 )
        throw 0;
    res = readUint32( buf.constData() + off );
    return 4;
}

static quint16 readUint16( const char* buf )
{
    return (((quint8)buf[0]) << 8) | (((quint8)buf[1]) << 0);
}

static QByteArray readString( const char* buf )
{
    const quint32 len = readUint32(buf);
    QByteArray str( buf + 4, len );
    return str;
}

static int readString( const QByteArray& buf, int off, QByteArray& res )
{
    quint32 len = 0;
    off += readUint32( buf, off, len );
    if( buf.size() < off + len )
        throw 0;
    res = buf.mid(off,len);
    return 4 + len;
}

static inline void writeUint32( char* buf, quint32 val )
{
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = (val >> 0) & 0xff;
}

static inline void writeUint64( char* buf, quint64 val )
{
    buf[0] = (val >> 56) & 0xff;
    buf[1] = (val >> 48) & 0xff;
    buf[2] = (val >> 40) & 0xff;
    buf[3] = (val >> 32) & 0xff;
    buf[4] = (val >> 24) & 0xff;
    buf[5] = (val >> 16) & 0xff;
    buf[6] = (val >> 8) & 0xff;
    buf[7] = (val >> 0) & 0xff;
}

static QByteArray writeString( const QByteArray& str )
{
    QByteArray res(4,0);
    writeUint32(res.data(),str.size());
    res += str;
    return res;
}


Debugger::Debugger(QObject *parent) : QObject(parent),d_sock(0),d_status(WaitHandshake),d_modeReq(0),d_mode(FreeRun),
    d_breakMeth(0)
{
    d_srv = new QTcpServer(this);
    d_srv->setMaxPendingConnections(1);
    connect( d_srv, SIGNAL(newConnection()), this, SLOT(onNewConnection()) );
    connect( d_srv, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)) );
}

quint16 Debugger::open(quint16 port)
{
    if( d_srv->isListening() )
        return d_srv->serverPort();
    if( !d_srv->listen(QHostAddress::LocalHost,port) )
        return 0;

    return d_srv->serverPort();
}

bool Debugger::close()
{
    if( !d_srv->isListening() )
        return false;
    if( isOpen() )
    {
        exit();
        //if( d_sock )
        //    d_sock->close();
    }
    d_srv->close();
    return true;
}

bool Debugger::isOpen() const
{
    return d_sock && d_sock->isOpen();
}

bool Debugger::resume()
{
    if( !isOpen() )
        return false;
    if( d_mode == FreeRun )
        return sendReceive(CMD_SET_VM,CMD_VM_RESUME).isOk();
    // else
    if( !clearStep() )
        return false;
    return sendReceive(CMD_SET_VM,CMD_VM_RESUME).isOk();
}

bool Debugger::suspend()
{
    if( !isOpen() )
        return false;
    return sendReceive(CMD_SET_VM,CMD_VM_SUSPEND).isOk();
}

bool Debugger::exit()
{
    QByteArray code(4,0);
    writeUint32(code.data(), 0);
    return sendReceive(CMD_SET_VM,CMD_VM_EXIT,code).isOk(); // calls exit() in the VM, mono_jit_exec doesn't return
    // VM DISPOSE doesn't do anything useful
}

bool Debugger::step(quint32 threadId, Debugger::RunMode mode, bool nextLine)
{
    if( !isOpen() )
        return false;
    if( d_mode == mode )
        return sendReceive(CMD_SET_VM,CMD_VM_RESUME).isOk();

    // else
    if( !clearStep() )
        return false;
    QByteArray data(3 + 1 + 16,0);
    char* d = data.data();
    d[0] = DebuggerEvent::STEP;
    d[1] = SUSPEND_POLICY_ALL;
    d[2] = 1;
    d[3] = MOD_KIND_STEP;
    writeUint32(d+4,threadId);
    writeUint32(d+8, nextLine ? STEP_SIZE_LINE : STEP_SIZE_MIN );
    switch( mode )
    {
    case StepIn:
        writeUint32(d+12,STEP_DEPTH_INTO);
        break;
    case StepOut:
        writeUint32(d+12,STEP_DEPTH_OUT);
        break;
    case StepOver:
        writeUint32(d+12,STEP_DEPTH_OVER);
        break;
    }
    writeUint32(d+16,0); // TODO Filter
    Reply r = sendReceive(CMD_SET_EVENT_REQUEST,CMD_EVENT_REQUEST_SET, data);
    if( r.isOk() )
    {
        d_mode = mode;
        d_modeReq = readUint32(r.d_data);
        return sendReceive(CMD_SET_VM,CMD_VM_RESUME).isOk();
    }
    return false;
}

bool Debugger::clearStep()
{
    if( d_mode == FreeRun )
        return true;
    QByteArray code(5,0);
    code[0] = DebuggerEvent::STEP;
    writeUint32(code.data()+1, d_modeReq);
    if( !sendReceive(CMD_SET_EVENT_REQUEST,CMD_EVENT_REQUEST_CLEAR,code).isOk() )
        return false;
    d_modeReq = 0;
    d_mode = FreeRun;
    return true;
}

bool Debugger::stepIn(quint32 threadId, bool nextLine)
{
    return step(threadId,StepIn,nextLine);
}

bool Debugger::stepOver(quint32 threadId, bool nextLine)
{
    return step(threadId,StepOver,nextLine);
}

bool Debugger::stepOut(quint32 threadId, bool nextLine)
{
    return step(threadId,StepOut,nextLine);
}

bool Debugger::enableUserBreak()
{
    if(!clearStep())
        return false;

    // enables user breaks which are triggered by calling [mscorlib]System.Diagnostics.Debugger::Break() in the code
    // if not enabled nothing happens
    QByteArray data(3,0);
    char* d = data.data();
    d[0] = DebuggerEvent::USER_BREAK;
    d[1] = SUSPEND_POLICY_ALL;
    d[2] = 0;
    Reply r = sendReceive(CMD_SET_EVENT_REQUEST,CMD_EVENT_REQUEST_SET, data);
    return r.isOk();
}

bool Debugger::callUserBreak(quint32 threadId)
{
    if( d_breakMeth == 0 )
        return false;

    // Does not work because apparently CMD_VM_INVOKE_METHOD can only be called if suspended
    QByteArray data(12 + 1 + 4,0);
    writeUint32(data.data(),threadId);
    writeUint32(data.data()+4,0); // flags
    writeUint32(data.data()+8,d_breakMeth);
    data[12] = VALUE_TYPE_ID_NULL;
    writeUint32(data.data()+13,0); // args count
    Reply r = sendReceive(CMD_SET_VM,CMD_VM_INVOKE_METHOD, data);
    return r.isOk();
}

bool Debugger::addBreakpoint(quint32 methodId, quint32 iloffset)
{
    if( !isOpen() )
        return false;

    const QPair<quint32,quint32> key = qMakePair(methodId,iloffset);
    if( d_breakPoints.contains(key) )
        return true;
    QByteArray data(3 + 1 + 12,0);
    char* d = data.data();
    d[0] = DebuggerEvent::BREAKPOINT;
    d[1] = SUSPEND_POLICY_ALL;
    d[2] = 1;
    d[3] = MOD_KIND_LOCATION_ONLY;
    writeUint32(d+4,methodId);
    writeUint64(d+8, iloffset );
    Reply r = sendReceive(CMD_SET_EVENT_REQUEST,CMD_EVENT_REQUEST_SET, data);
    if( r.isOk() )
    {
        d_breakPoints.insert(key,readUint32(r.d_data));
        return true;
    }
    return false;
}

bool Debugger::removeBreakpoint(quint32 methodId, quint32 iloffset)
{
    if( !isOpen() )
        return false;

    const QPair<quint32,quint32> key = qMakePair(methodId,iloffset);
    if( !d_breakPoints.contains(key) )
        return true;

    QByteArray code(5,0);
    code[0] = DebuggerEvent::BREAKPOINT;
    writeUint32(code.data()+1, d_breakPoints.value(key));
    if( !sendReceive(CMD_SET_EVENT_REQUEST,CMD_EVENT_REQUEST_CLEAR,code).isOk() )
        return false;

    d_breakPoints.remove(key);
    return true;
}

bool Debugger::clearAllBreakpoints()
{
    Reply r = sendReceive(CMD_SET_EVENT_REQUEST,CMD_EVENT_REQUEST_CLEAR_ALL_BREAKPOINTS);
    return r.isOk();
}

QList<quint32> Debugger::allThreads()
{
    QList<quint32> res;
    if( !isOpen() )
        return res;
    Reply r = sendReceive(CMD_SET_VM,CMD_VM_ALL_THREADS);
    if( r.isOk() )
    {
        const char* data = r.d_data.constData();
        const int count = readUint32(r.d_data.constData());
        data += 4;
        for( int i = 0; i < count; i++ )
        {
            res << readUint32( data );
            data += 4;
        }
    }
    return res;
}

QByteArray Debugger::getThreadName(quint32 threadId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),threadId);
    Reply r = sendReceive(CMD_SET_THREAD, CMD_THREAD_GET_NAME,data);
    if( r.isOk() )
        return readString(r.d_data.constData());
    else
        return QByteArray();
}

const char* Debugger::s_state[] = {
    "invalid", "unstarted", "running", "suspended", "aborted", "stopped"
};

Debugger::ThreadState Debugger::getThreadState(quint32 threadId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),threadId);
    Reply r = sendReceive(CMD_SET_THREAD, CMD_THREAD_GET_STATE,data);
    if( r.isOk() )
    {
        const quint32 state = readUint32(r.d_data.constData());
        if( state & ThreadState_Unstarted )
            return Unstarted;
        if( state & ThreadState_Aborted )
            return Aborted;
        if( state & ThreadState_Stopped )
            return Stopped;
        if( state & ThreadState_Suspended )
            return Suspended;
        return Running;
    }else
        return Invalid;
}

quint32 Debugger::getCoreLib(quint32 domainId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),domainId);
    Reply r = sendReceive(CMD_SET_APPDOMAIN, CMD_APPDOMAIN_GET_CORLIB,data);
    if( r.isOk() )
        return readUint32(r.d_data.constData());
    return 0;
}

QList<quint32> Debugger::findType(const QByteArray& name)
{
    QByteArray data = writeString(name);
    data += '\0';
    Reply r = sendReceive(CMD_SET_VM, CMD_VM_GET_TYPES,data);
    QList<quint32> res;
    if( r.isOk() )
    {
        int off = 0;
        quint32 len;
        off += readUint32(r.d_data,off,len);
        for( int i = 0; i < len; i++ )
        {
            quint32 t;
            off += readUint32(r.d_data,off,t);
            res << t;
        }
    }
    return res;
}

quint32 Debugger::findType(const QByteArray& name, quint32 assemblyId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),assemblyId);
    data += writeString(name);
    data += '\0';
    Reply r = sendReceive(CMD_SET_ASSEMBLY, CMD_ASSEMBLY_GET_TYPE,data);
    if( r.isOk() )
        return readUint32(r.d_data.constData());
    else
        return 0;
}

QList<quint32> Debugger::getTypesOf(const QString& sourcePath)
{
    QByteArray data = writeString(sourcePath.toUtf8());
    data += '\0';
    Reply r = sendReceive(CMD_SET_VM, CMD_VM_GET_TYPES_FOR_SOURCE_FILE,data);
    QList<quint32> res;
    if( r.isOk() )
    {
        int off = 0;
        quint32 len;
        off += readUint32(r.d_data,off,len);
        for( int i = 0; i < len; i++ )
        {
            quint32 t;
            off += readUint32(r.d_data,off,t);
            res << t;
        }
    }
    return res;
}

QList<Debugger::Frame> Debugger::getStack(quint32 threadId)
{
    QList<Frame> res;
    if( !isOpen() )
        return res;
    QByteArray payload(12,0);
    writeUint32(payload.data(), threadId);
    writeUint32(payload.data()+4, 0);
    writeUint32(payload.data()+8, -1); // len is not implemented in Mono 3, expects -1

    Reply r = sendReceive(CMD_SET_THREAD,CMD_THREAD_GET_FRAME_INFO,payload);
    if( r.isOk() )
    {
        const char* data = r.d_data.constData();
        const int count = readUint32(r.d_data.constData());
        data += 4;
        for( int i = 0; i < count; i++ )
        {
            Frame f;
            f.id = readUint32( data );
            data += 4;
            f.method = readUint32( data );
            data += 4;
            f.il_offset = readUint32( data );
            data += 4;
            f.flags = *data;
            data += 1;
            res << f;
       }
    }

    return res;
}

static int readValue( const QByteArray& data, int off, QVariant& val )
{
    Q_ASSERT( !data.isEmpty() );
    quint32 i;
    quint64 l;
    quint8 type = (quint8)data[off];
    const char* d = data.constData()+1+off;
    switch( type )
    {
    case VT_Void:
        val = QVariant();
        return 1;
    case VT_Boolean:
        val = readUint32(d) != 0;
        return 5;
    case VT_Char:
        val = QChar(readUint32(d));
        return 5;
    case VT_I1:
        val = qint8(int(readUint32(d)));
        return 5;
    case VT_U1:
        val = quint8(readUint32(d));
        return 5;
    case VT_I2:
        val = qint16(int(readUint32(d)));
        return 5;
    case VT_U2:
        val = quint16(readUint32(d));
        return 5;
    case VT_I4:
        val = int(readUint32(d));
        return 5;
    case VT_U4:
        val = readUint32(d);
        return 5;
    case VT_I8:
        val = qint64(readUint64(d));
        return 9;
    case VT_U8:
        val = readUint64(d);
        return 9;
    case VT_R4:
        i = readUint32(d);
        val = *((float*)&i);
        return 5;
    case VT_R8:
        l = readUint64(d);
        val = *((double*)&l);
        return 9;
    case VT_Ptr:
        val = QVariant::fromValue(IntPtr(readUint64(d)));
        return 9;
    case VT_U:
        val = readUint64(d);
        return 9;
    case VT_I:
        val = qint64(readUint64(d));
        return 9;
    case VT_String:
        val = QVariant::fromValue(ObjectRef(ObjectRef::String,readUint32(d)));
        return 5;
    case VT_Class:
        val = QVariant::fromValue(ObjectRef(ObjectRef::Class,readUint32(d)));
        return 5;
    case VT_Array:
        val = QVariant::fromValue(ObjectRef(ObjectRef::Array,readUint32(d)));
        return 5;
    case VT_Object:
        val = QVariant::fromValue(ObjectRef(ObjectRef::Object,readUint32(d)));
        return 5;
    case VT_SzArray:
        val = QVariant::fromValue(ObjectRef(ObjectRef::SzArray,readUint32(d)));
        return 5;
    case VALUE_TYPE_ID_NULL:
        val = QVariant();
        return 1;
    case VALUE_TYPE_ID_TYPE:
        val = QVariant::fromValue(ObjectRef(ObjectRef::Type,readUint32(d)));
        return 5;
    case VALUE_TYPE_ID_PARENT_VTYPE:
        val = readUint32(d);
        return 5;
    case VT_ValueType:
        {
            int off = 1; // type above
            const bool isEnum = data[off++] != 0;
            ValueType vt;
            off += readUint32(data,off,vt.cls);
            quint32 len;
            off += readUint32(data,off,len);
            for(int i = 0; i < len; i++ )
            {
                QVariant val2;
                off += readValue(data,off, val2);
                vt.fields.append(val2);
            }
            return off;
        }
        break;
    case VT_ByRef:
    case VT_Var:
    case VT_GenericInst:
    case VT_TypedByRef:
    case VT_FnPtr:
    case VT_MVar:
    case VT_CModReqD:
    case VT_CModOpt:
    case VT_Internal:
    case VT_Modifier:
    case VT_Sentinel:
    case VT_Pinned:
    case VT_Type:
    case VT_Boxed:
    case VT_Enum:
        qWarning() << "readValue: unsupported type" << type;
        break;
    }
    return 0;
}

QVariantList Debugger::getParamValues(quint32 threadId, quint32 frameId, bool hasThis, quint16 numOfParams)
{
    QByteArray payload(8,0);
    writeUint32(payload.data(), threadId);
    writeUint32(payload.data()+4, frameId);
    QVariantList res;
    if( hasThis )
    {
        Reply r = sendReceive(CMD_SET_STACK_FRAME, CMD_STACK_FRAME_GET_THIS,payload);
        if( !r.isOk() )
            return res;
        if( quint8(r.d_data[0]) != VALUE_TYPE_ID_NULL )
        {
            QVariant val;
            readValue(r.d_data,0,val);
            res.append(val);
        }
    }
    if( numOfParams == 0 )
        return res;
    QByteArray data(4 + numOfParams * 4, 0 );
    writeUint32(data.data(), numOfParams);
    int off = 4;
    for( int i = 0; i < numOfParams; i++ )
    {
        writeUint32(data.data() + off, -i-1);
        off += 4;
    }
    Reply r = sendReceive(CMD_SET_STACK_FRAME, CMD_STACK_FRAME_GET_VALUES,payload+data);
    if( !r.isOk() )
        return res;
    off = 0;
    for( int i = 0; i < numOfParams; i++ )
    {
        QVariant val;
        off += readValue(r.d_data,off,val);
        res.append(val);
    }
    return res;
}

QVariantList Debugger::getLocalValues(quint32 threadId, quint32 frameId, quint16 numOfLocals)
{
    QByteArray payload(8,0);
    writeUint32(payload.data(), threadId);
    writeUint32(payload.data()+4, frameId);
    QVariantList res;
    if( numOfLocals == 0 )
        return res;
    QByteArray data(4 + numOfLocals * 4, 0 );
    writeUint32(data.data(), numOfLocals);
    int off = 4;
    for( int i = 0; i < numOfLocals; i++ )
    {
        writeUint32(data.data() + off, i);
        off += 4;
    }
    Reply r = sendReceive(CMD_SET_STACK_FRAME, CMD_STACK_FRAME_GET_VALUES,payload+data);
    if( !r.isOk() )
        return res;
    off = 0;
    for( int i = 0; i < numOfLocals; i++ )
    {
        QVariant val;
        off += readValue(r.d_data,off,val);
        res.append(val);
    }
    return res;
}

QString Debugger::getString(quint32 strId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),strId);
    Reply r = sendReceive(CMD_SET_STRING_REF, CMD_STRING_REF_GET_VALUE,data);
    if( r.isOk() )
        return QString::fromUtf8( readString(r.d_data.constData() ) );
    return QString();
}

quint32 Debugger::getArrayLength(quint32 arrId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),arrId);
    Reply r = sendReceive(CMD_SET_ARRAY_REF, CMD_ARRAY_REF_GET_LENGTH,data);
    if( r.isOk() )
        return readUint32(r.d_data.constData()+4); // just interested in the fisrt dim length
    return 0;
}

QVariantList Debugger::getArrayValues(quint32 arrId, quint32 len)
{
    QByteArray data(12,0);
    writeUint32(data.data(),arrId);
    writeUint32(data.data()+4,0); // index
    writeUint32(data.data()+8,len);
    Reply r = sendReceive(CMD_SET_ARRAY_REF, CMD_ARRAY_REF_GET_VALUES,data);
    if( r.isOk() )
    {
        quint32 off = 0;
        QVariantList res;
        for( int i = 0; i < len; i++ )
        {
            QVariant val;
            off += readValue(r.d_data,off,val);
            res.append(val);
        }
        return res;
    }
    return QVariantList();
}

Debugger::MethodDbgInfo Debugger::getMethodInfo(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_DEBUG_INFO,data);
    MethodDbgInfo res;
    res.codeSize = 0;
    if( r.isOk() )
    {
        int off = 0;
        off += readUint32(r.d_data,off,res.codeSize);
        quint32 len;
        off += readUint32(r.d_data,off,len); // number of source files
        if( len == 0 )
            return res;

        for( int i = 0; i < len; i++ )
        {
            off += readString(r.d_data,off,res.sourceFile);
            off += 16; // hash
        }

        off += readUint32(r.d_data,off,len); // number of il offsets
        if( len == 0 )
            return res;

        for( int i = 0; i < len; i++ )
        {
            quint32 unused, col;
            MethodDbgInfo::Loc loc;
            loc.valid = true;
            off += readUint32(r.d_data,off,loc.iloff);
            off += readUint32(r.d_data,off,loc.row);
            off += readUint32(r.d_data,off,unused); // source
            off += readUint32(r.d_data,off,col);
            loc.col = (int)col; // can be negative
            off += readUint32(r.d_data,off,unused); // end line
            off += readUint32(r.d_data,off,unused); // end column
            res.lines.append(loc);
        }
    }
    return res;
}

QByteArray Debugger::getMethodName(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_NAME,data);
    if( r.isOk() )
        return readString(r.d_data.constData());
    else
        return QByteArray();
}

quint32 Debugger::getMethodOwner(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_DECLARING_TYPE,data);
    if( r.isOk() )
        return readUint32(r.d_data.constData());
    else
        return 0;
}

QByteArray Debugger::getMethodBody(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_BODY,data);
    QByteArray res;
    if( r.isOk() )
    {
        quint32 len = readUint32(r.d_data.constData());
        if( len )
            res = r.d_data.mid(4, len);
    }
    return res;
}

QPair<quint32, quint32> Debugger::getMethodFlags(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_INFO,data);
    if( r.isOk() )
        return qMakePair(readUint32(r.d_data.constData()), // method flags
                         readUint32(r.d_data.constData()+4)); // implementation flags
    return qMakePair(0,0);
}

bool Debugger::isMethodStatic(quint32 methodId)
{
    QPair<quint32, quint32> flags = getMethodFlags(methodId);
    return flags.first & METHOD_ATTRIBUTE_STATIC;
}

Debugger::MethodKind Debugger::getMethodKind(quint32 methodId)
{
    QPair<quint32, quint32> flags = getMethodFlags(methodId);
    switch( flags.second & 3 )
    {
    case 0:
    case 2:
        return IL;
    case 1:
        return Native;
    case 3:
        return Runtime;
    }
}

quint16 Debugger::getParamCount(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_PARAM_INFO,data);
    if( r.isOk() )
    {
        quint32 count;
        readUint32(r.d_data,4,count);
        return count;
    }
    return 0;
}

QByteArrayList Debugger::getParamNames(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_PARAM_INFO,data);
    QByteArrayList res;
    if( r.isOk() )
    {
        quint32 count;
        readUint32(r.d_data,4,count);
        int off = 8 // calling convention + param count
                + 4 // generic parameter count
                + 4 // TypeID of the returned value
                + count * 4; // parameter count TypeID for each parameter type
        for( int i = 0; i < count; i++ )
        {
            QByteArray name;
            off += readString(r.d_data,off,name);
            res << name;
        }
    }
    return res;
}

quint16 Debugger::getLocalsCount(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_LOCALS_INFO,data);
    if( r.isOk() )
    {
        quint32 count;
        readUint32(r.d_data,0,count);
        return count;
    }
    return 0;
}

QByteArrayList Debugger::getLocalNames(quint32 methodId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),methodId);
    Reply r = sendReceive(CMD_SET_METHOD, CMD_METHOD_GET_LOCALS_INFO,data);
    QByteArrayList res;
    if( r.isOk() )
    {
        quint32 count;
        readUint32(r.d_data,0,count);
        int off = 4 + count * 4; // followed by the TypeID (id) for each locals
        for( int i = 0; i < count; i++ )
        {
            QByteArray name;
            off += readString(r.d_data,off,name);
            res << name;
        }
    }
    return res;
}

Debugger::TypeInfo Debugger::getTypeInfo(quint32 typeId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),typeId);
    Reply r = sendReceive(CMD_SET_TYPE, CMD_TYPE_GET_INFO,data);
    TypeInfo res;
    res.id = 0;
    res.assembly = 0;
    res.module = 0;
    if( r.isOk() )
    {
        int off = 0;
        off += readString(r.d_data,off,res.space);
        off += readString(r.d_data,off,res.name);
        off += readString(r.d_data,off,res.fullName);
        off += readUint32(r.d_data,off,res.assembly);
        off += readUint32(r.d_data,off,res.module);
        off += readUint32(r.d_data,off,res.id);
    }
    return res;
}

quint32 Debugger::getTypeObject(quint32 typeId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),typeId);
    Reply r = sendReceive(CMD_SET_TYPE, CMD_TYPE_GET_OBJECT,data);
    if( r.isOk() )
        return readUint32(r.d_data.constData());
    return 0;
}

QList<quint32> Debugger::getMethods(quint32 typeId, const QByteArray& name)
{
    QByteArray data(4,0);
    writeUint32(data.data(),typeId);
    Reply r = sendReceive(CMD_SET_TYPE, CMD_TYPE_GET_METHODS,data);
    QList<quint32> res;
    if( r.isOk() )
    {
        quint32 len;
        int off = 0;
        off += readUint32(r.d_data,off,len);
        for( int i = 0; i < len; i++ )
        {
            quint32 m;
            off += readUint32(r.d_data,off,m);
            if( !name.isEmpty() )
            {
                const QByteArray check = getMethodName(m);
                if( name == check )
                    res << m;
            }else
                res << m;
        }
    }
    return res;
}

quint32 Debugger::getObjectType(quint32 objId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),objId);
    Reply r = sendReceive(CMD_SET_OBJECT_REF, CMD_OBJECT_REF_GET_TYPE,data);
    if( r.isOk() )
        return readUint32(r.d_data.constData());
    return 0;
}

QList<Debugger::FieldInfo> Debugger::getFields(quint32 typeId, bool instanceLevel, bool classLevel)
{
    QByteArray data(4,0);
    writeUint32(data.data(),typeId);
    Reply r = sendReceive(CMD_SET_TYPE, CMD_TYPE_GET_FIELDS,data);
    QList<Debugger::FieldInfo> res;
    if( r.isOk() )
    {
        int off = 0;
        quint32 count;
        off += readUint32(r.d_data,off,count);
        for( int i = 0; i < count; i++ )
        {
            FieldInfo field;
            off += readUint32(r.d_data,off,field.id);
            off += readString(r.d_data,off,field.name);
            off += 4; // skip type
            quint32 attrs;
            off += readUint32(r.d_data,off,attrs);
            const bool isStatic = attrs & FIELD_ATTRIBUTE_STATIC;
            if( isStatic )
                qDebug() << "static field" << field.name;
            if( ( instanceLevel && !isStatic ) || ( classLevel && isStatic ) )
                res << field;
        }
    }
    return res;
}

QVariantList Debugger::getValues(quint32 objectOrTypeId, const QList<quint32>& fieldIds, bool typeLevel)
{
    QByteArray data(4 + 4 + 4 * fieldIds.size() ,0);
    writeUint32(data.data(),objectOrTypeId);
    writeUint32(data.data()+4, fieldIds.size());
    for( int i = 0; i < fieldIds.size(); i++ )
        writeUint32(data.data()+8+i*4, fieldIds[i]);
    Reply r;
    if( typeLevel )
        r = sendReceive(CMD_SET_TYPE, CMD_TYPE_GET_VALUES,data);
    else
        r = sendReceive(CMD_SET_OBJECT_REF, CMD_OBJECT_REF_GET_VALUES,data);
    QVariantList res;
    if( r.isOk() )
    {
        int off = 0;
        for( int i = 0; i < fieldIds.size(); i++ )
        {
            QVariant var;
            off += readValue(r.d_data,off,var);
            res << var;
        }
    }
    return res;
}

QByteArray Debugger::getAssemblyName(quint32 assemblyId)
{
    QByteArray data(4,0);
    writeUint32(data.data(),assemblyId);
    Reply r = sendReceive(CMD_SET_ASSEMBLY, CMD_ASSEMBLY_GET_NAME,data);
    if( r.isOk() )
        return readString(r.d_data.constData());
    else
        return QByteArray();
}

void Debugger::onNewConnection()
{
    QTcpSocket* sock = d_srv->nextPendingConnection();
    //qDebug() << "new connection" << sock->peerName() << sock->peerAddress().toString();
    if( d_sock || !sock->isOpen() )
    {
        sock->close();
        sock->deleteLater();
    }else
    {
        d_sock = sock;
        d_status = WaitHandshake;
        connect( d_sock, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
        connect( d_sock, SIGNAL(readyRead()), this, SLOT(onData()));
        onData();
    }
}

void Debugger::onError(QAbstractSocket::SocketError)
{
    error(d_sock->errorString());
}

void Debugger::onDisconnect()
{
    if( d_sock )
        d_sock->deleteLater();
    d_sock = 0;
    d_breakPoints.clear();
    d_modeReq = 0;
    d_mode = FreeRun;
}

void Debugger::onData()
{
    while( d_sock && d_sock->isOpen() && d_sock->bytesAvailable() )
    {
        switch( d_status )
        {
        case WaitHandshake:
            if( d_sock->bytesAvailable() >= 13 )
            {
                const QByteArray msg = d_sock->read(13);
                if( msg == "DWP-Handshake" )
                {
                    d_sock->write(msg);
                    d_status = WaitHeader;
                }else
                {
                    error(tr("invalid handshake sequence received"));
                    return;
                }
            }else
                return;
            break;
        case WaitHeader:
            if( d_sock->bytesAvailable() >= 11 )
            {
                const QByteArray header = d_sock->read(11);
                d_len = readUint32(header.constData()) - 11;
                d_id = readUint32(header.constData() + 4);
                const bool flags = header[8] != 0;
                if( flags )
                {
                    // At the moment this value is only used with a reply packet in which case its value is set to 0x80.
                    // A command packet should have this value set to 0.
                    d_cmdSet = 0;
                    d_cmd = 0;
                    const quint16 err = readUint16( header.constData() + 9);
                    if( err > 255 )
                    {
                        error(tr("invalid error code in reply"));
                        return;
                    }
                    d_err = err;
                }else
                {
                    d_err = 0;
                    d_cmdSet = (quint8)header[9];
                    d_cmd = (quint8)header[10];
                    if( d_cmdSet == 0 )
                    {
                        error(tr("invalid command set in request"));
                        return;
                    }
                }
                if( d_len > 0 )
                    d_status = WaitData;
                else
                    processMessage();
            }else
                return;
            break;
        case WaitData:
            if( d_sock->bytesAvailable() >= d_len )
            {
                d_status = WaitHeader;
                processMessage(d_sock->read(d_len));
            }else
                return;
            break;
        }
    }
}

void Debugger::onInitialSetup(bool start)
{
    if( !start || !isOpen() )
        return;
    QPair<int, int> vVm = vmGetVersion();
    if( !isOpen() )
        return;
    //qDebug() << "VM version" << vVm.first << vVm.second;
    QPair<int, int> vThis( MAJOR_VERSION, MINOR_VERSION );
    if( vVm < vThis )
    {
        error( tr("the VM is too old for this application" ) );
        return;
    }
    QByteArray version(8,0);
    writeUint32(version.data(), MAJOR_VERSION);
    writeUint32(version.data() + 4, MINOR_VERSION);
    sendReceive(CMD_SET_VM,CMD_VM_SET_PROTOCOL_VERSION, version );

    QByteArray event(3,0);
    event[0] = DebuggerEvent::ASSEMBLY_LOAD;
    event[1] = SUSPEND_POLICY_NONE;
    event[2] = 0;
    sendReceive(CMD_SET_EVENT_REQUEST,CMD_EVENT_REQUEST_SET, event);


#if 0
    // not useful
    QList<quint32> types = findType("System.Diagnostics.Debugger");
    if( types.size() == 1 )
    {
        QList<quint32> meths = getMethods(types.first(),"Break");
        if( meths.size() == 1 )
            d_breakMeth = meths.first();
    }
#endif
}

void Debugger::processMessage(const QByteArray& payload)
{
    switch( d_cmdSet )
    {
    case 0: // reply
        //qDebug() << "reply received id =" << d_id << "err = " << d_err;
        d_replies.insert( d_id, qMakePair(d_err,payload) );
        break;
    case 64: // Events
        processEvent(d_cmd, payload);
        break;
    default:
        error( tr("invalid command set %1 received from mono").arg(d_cmdSet));
        break;
    }
}

int Debugger::processEvent( quint8 evt, const QByteArray& payload)
{
    int off = 0;
    try
    {
        DebuggerEvent e;
        e.event = evt;
        e.object = 0;
        e.offset = 0;
        //qDebug() << "event" << e.event << "arrived with payload len" << payload.size();
        if( evt != CMD_COMPOSITE )
            off += readUint32(payload,off,e.thread);
        switch( evt )
        {
        case DebuggerEvent::VM_START:
            {
                const quint32 domainId = readUint32( payload.constData() + 4 );
                e.object = domainId;
                onInitialSetup(true);
            }
            break;
        case DebuggerEvent::VM_DEATH:
            {
                quint32 exitCode;
                if( off < payload.size() )
                {
                    off += readUint32( payload, off, exitCode );
                    e.exitCode = exitCode;
                }else
                    e.exitCode = 0;
            }
            break;
        case DebuggerEvent::THREAD_START:
        case DebuggerEvent::THREAD_DEATH:
            e.object = e.thread;
            break;
        case DebuggerEvent::APPDOMAIN_CREATE:
        case DebuggerEvent::APPDOMAIN_UNLOAD:
            {
                quint32 domainId;
                off += readUint32( payload, off, domainId );
                e.object = domainId;
            }
            break;
        case DebuggerEvent::METHOD_ENTRY:
        case DebuggerEvent::METHOD_EXIT:
            {
                quint32 methodId;
                off += readUint32( payload, off, methodId );
                e.object = methodId;
            }
            break;
        case DebuggerEvent::ASSEMBLY_LOAD:
        case DebuggerEvent::ASSEMBLY_UNLOAD:
            {
                quint32 assemblyId;
                off += readUint32( payload, off, assemblyId );
                e.object = assemblyId;
            }
            break;
        case DebuggerEvent::BREAKPOINT:
        case DebuggerEvent::STEP:
            {
                quint32 methodId;
                off += readUint32( payload, off, methodId );
                e.object = methodId;
                quint64 il_offset;
                off += readUint64( payload, off, il_offset );
                if( il_offset > std::numeric_limits<quint32>::max() )
                {
                    e.offset = 0;
#if 0
                    // this value is not used anyway and on macOS 0xffffffffff is returned by Mono3
                    e.offset = std::numeric_limits<quint32>::max();
                    qWarning() << "value exceeds maximum offset" << il_offset;
#endif
                }else
                    e.offset = il_offset;
            }
            break;
        case DebuggerEvent::TYPE_LOAD:
            {
                quint32 typeId;
                off += readUint32( payload, off, typeId );
                e.object = typeId;
            }
            break;
        case DebuggerEvent::EXCEPTION:
            {
                quint32 objectId;
                off += readUint32( payload.constData(), off, objectId );
                e.object = objectId;
            }
            break;
        case DebuggerEvent::KEEPALIVE:
            // suspend_policy = SUSPEND_POLICY_NONE;
            break;
        case DebuggerEvent::USER_BREAK:
            break;
        case DebuggerEvent::USER_LOG:
            {
                quint32 level;
                off += readUint32( payload, off, level );
                e.level = level;
                QByteArray category;
                off += readString( payload, off, category );
                QByteArray message;
                off += readString( payload, off, message );
                e.msg = category + "\n" + message;
            }
            break;
        case CMD_COMPOSITE:
            {
                if( payload.size() < 5 )
                {
                    error(tr("invalid composite event") );
                    return 0;
                }
                const quint8 policy = (quint8)payload[0];
                const quint32 count = readUint32(payload.constData() + 1 );
                int off = 5;
                for( int i = 0; i < count; i++ )
                {
                    if( off >= payload.size() )
                        throw 0;
                    const quint8 event = (quint8)payload[off++];
                    quint32 id;
                    off += readUint32( payload, off, id );
                    off += processEvent(event,payload.mid(off));
                }
            }
            break;
        default:
            error( tr("invalid event %1 %2").arg(evt).arg(payload.toHex().constData()) );
            break;
        }
        if( evt != CMD_COMPOSITE )
            emit sigEvent(e);
    }catch(...)
    {
        error( tr("not enough data available") );
    }

    return off;
}

quint32 Debugger::sendRequest(quint8 cmdSet, quint8 cmd, const QByteArray& payload)
{
    if( d_sock == 0 )
        return 0;
    QByteArray header(11,0);
    const quint32 id = d_id++;
    writeUint32( header.data(), 11 + payload.size() );
    writeUint32( header.data() + 4, id );
    header[9] = cmdSet;
    header[10] = cmd;
    d_sock->write( header + payload );
    //qDebug() << "request sent id =" << id << "cmd_set =" << cmdSet << "cmd =" << cmd;
    return id;
}

bool Debugger::error(const QString& msg)
{
    d_status = ProtocolError;
    if( d_sock )
        d_sock->close();
    qCritical() << msg;
    emit sigError(msg);
    return false;
}

Debugger::Reply Debugger::waitForId(quint32 id)
{
    const QDateTime start = QDateTime::currentDateTime();
    while( d_sock )
    {
        Reply res = fetchReply(id);
        if( res.d_valid )
            return res;
        if( start.secsTo(QDateTime::currentDateTime()) > 20 )
        {
            res.d_timeout = true;
            return res;
        }
        d_sock->waitForReadyRead(1000);
        onData();
    }
    return Reply();
}

Debugger::Reply Debugger::sendReceive(quint8 cmdSet, quint8 cmd, const QByteArray& payload)
{
    const quint32 id = sendRequest( cmdSet, cmd, payload);
    Reply rep = waitForId(id);
    if( rep.d_timeout )
    {
        error(tr("timeout in request %1.%2").arg(cmdSet).arg(cmd) );
        return rep;
    }else
        return rep;
}

QPair<int, int> Debugger::vmGetVersion()
{
    QPair<int, int> res;

    Reply rep = sendReceive(CMD_SET_VM,CMD_VM_VERSION);
    if( !rep.isOk() )
        return res;
    try
    {
        int off = 0;
        QByteArray str;
        off += readString(rep.d_data, off, str);
        quint32 tmp;
        off += readUint32( rep.d_data, off, tmp );
        res.first = tmp;
        off += readUint32( rep.d_data, off, tmp );
        res.second = tmp;
    }catch(...)
    {
        error(tr("not enough data received in vmGetVersion") );
    }
    return res;
}

Debugger::Reply Debugger::fetchReply(quint32 id)
{
    Replies::iterator i = d_replies.find(id);
    if( i != d_replies.end() )
    {
        Reply res;
        res.d_valid = true;
        res.d_err = i.value().first;
        if( res.d_err != 0 )
            qCritical() << "reply id" << id << "error" << res.d_err << toString(res.d_err);
        res.d_data = i.value().second;
        d_replies.erase(i);
        return res;
    }else
        return Reply();
}

Debugger::MethodDbgInfo::Loc Debugger::MethodDbgInfo::find(quint32 iloff) const
{
    for( int i = 0; i < lines.size(); i++ )
    {
        if( lines[i].iloff >= iloff )
            return lines[i];
    }
    Loc res;
    res.col = 0;
    res.row = 0;
    res.iloff = 0;
    res.valid = false;
    return res;
}

quint32 Debugger::MethodDbgInfo::find(quint32 row, qint16 col) const
{
    for( int i = 0; i < lines.size(); i++ )
    {
        if( lines[i].row >= row )
            return lines[i].iloff;
    }
    return 0;
}


QByteArray Debugger::TypeInfo::spaceName() const
{
    if( space.isEmpty() )
        return name;
    else
        return space + "." + name;
}
