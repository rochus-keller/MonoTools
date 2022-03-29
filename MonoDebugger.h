#ifndef MONODEBUGGER_H
#define MONODEBUGGER_H

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

#include <QObject>
#include <QAbstractSocket>
#include <QHash>
#include <QVariant>

class QTcpServer;
class QTcpSocket;

namespace Mono
{
    struct DebuggerEvent
    {
        enum EventKind { VM_START = 0, VM_DEATH, THREAD_START, THREAD_DEATH, APPDOMAIN_CREATE, APPDOMAIN_UNLOAD,
               METHOD_ENTRY, METHOD_EXIT, ASSEMBLY_LOAD, ASSEMBLY_UNLOAD, BREAKPOINT, STEP,
               TYPE_LOAD, EXCEPTION, KEEPALIVE, USER_BREAK, USER_LOG = 16 };
        static const char* s_event[];

        quint8 event;
        quint32 thread;
        quint32 object;
        union
        {
            int exitCode;
            quint32 offset;
            int level;
        };
        QByteArray msg;
    };

    class Debugger : public QObject
    {
        Q_OBJECT
    public:
        explicit Debugger(QObject *parent = 0);

        quint16 open(quint16 port = 0);
        bool close();
        bool isOpen() const;

        bool resume();
        bool stepIn(quint32 threadId, bool lineStep = false);
        bool stepOver(quint32 threadId, bool lineStep = false);
        bool stepOut(quint32 threadId, bool lineStep = false);
        bool suspend();
        bool exit();

        bool enableUserBreak();
        bool callUserBreak(quint32 threadId); // doesn't work

        bool addBreakpoint(quint32 methodId, quint32 iloffset ); // method-id cannot be zero in Mono3
        bool removeBreakpoint(quint32 methodId, quint32 iloffset );
        bool clearAllBreakpoints();

        QList<quint32> allThreads();
        QByteArray getThreadName(quint32 threadId);
        enum ThreadState { Invalid, Unstarted, Running, Suspended, Aborted, Stopped };
        static const char* s_state[];
        ThreadState getThreadState(quint32 threadId);

        quint32 getCoreLib(quint32 domainId); // assemblyId

        QList<quint32> findType( const QByteArray& name );
        quint32 findType( const QByteArray& name, quint32 assemblyId );
        // name follows https://docs.microsoft.com/en-us/dotnet/api/system.type.assemblyqualifiedname
        QList<quint32> getTypesOf( const QString& sourcePath );

        struct Frame
        {
            quint32 id;
            quint32 method;
            quint32 il_offset;
            quint8 flags;
        };
        QList<Frame> getStack(quint32 threadId);
        QVariantList getParamValues(quint32 threadId, quint32 frameId, bool hasThis, quint16 numOfParams );
        QVariantList getLocalValues(quint32 threadId, quint32 frameId, quint16 numOfLocals );
        QString getString(quint32 strId);
        quint32 getArrayLength(quint32 arrId);
        QVariantList getArrayValues(quint32 arrId, quint32 len );

        struct MethodDbgInfo
        {
            quint32 codeSize;
            QByteArray sourceFile;
            struct Loc
            {
                quint32 iloff, row; qint16 col; bool valid;
                Loc():valid(false),row(0),col(0){}
            };
            QList<Loc> lines;
            Loc find(quint32 iloff) const;
            quint32 find( quint32 row, qint16 col) const; // find first iloff
        };
        MethodDbgInfo getMethodInfo(quint32 methodId);
        QByteArray getMethodName(quint32 methodId);
        quint32 getMethodOwner(quint32 methodId); // returns typeId or 0
        QByteArray getMethodBody(quint32 methodId);
        QPair<quint32,quint32> getMethodFlags(quint32 methodId);
        bool isMethodStatic(quint32 methodId);
        enum MethodKind { IL, Native, Runtime };
        MethodKind getMethodKind(quint32 methodId);
        quint16 getParamCount(quint32 methodId);
        QByteArrayList getParamNames(quint32 methodId);
        quint16 getLocalsCount(quint32 methodId);
        QByteArrayList getLocalNames(quint32 methodId);

        struct TypeInfo
        {
            QByteArray space;
            QByteArray name;
            QByteArray fullName;
            quint32 assembly;
            quint32 module;
            quint32 id;
            QByteArray spaceName() const;
        };
        TypeInfo getTypeInfo(quint32 typeId);
        quint32 getTypeObject(quint32 typeId);
        QList<quint32> getMethods(quint32 typeId, const QByteArray& name = QByteArray());
        quint32 getObjectType(quint32 objId);
        struct FieldInfo
        {
            quint32 id;
            QByteArray name;
        };
        QList<FieldInfo> getFields(quint32 typeId, bool instanceLevel = true, bool classLevel = true);
        QVariantList getValues(quint32 objectOrTypeId, const QList<quint32>& fieldIds, bool typeLevel = false);

        QByteArray getAssemblyName(quint32 assemblyId);

    signals:
        void sigError( const QString& );
        void sigEvent( const DebuggerEvent& );
    protected slots:
        void onNewConnection();
        void onError(QAbstractSocket::SocketError);
        void onDisconnect();
        void onData();
        void onInitialSetup(bool);
    protected:
        void processMessage( const QByteArray& payload = QByteArray() );
        int processEvent( quint8 evt, const QByteArray& );
        bool checkLen( const QByteArray& buf, int off, int len );
        quint32 sendRequest( quint8 cmdSet, quint8 cmd, const QByteArray& payload = QByteArray() );
        bool error(const QString&);
        struct Reply
        {
            quint8 d_err;
            bool d_timeout;
            bool d_valid;
            QByteArray d_data;
            Reply():d_err(0),d_timeout(false),d_valid(false){}
            bool isOk() const { return d_valid && d_err == 0; }
        };
        Reply waitForId(quint32 id);
        Reply sendReceive(quint8 cmdSet, quint8 cmd, const QByteArray& payload = QByteArray());
        QPair<int,int> vmGetVersion();
        enum RunMode { FreeRun, StepIn, StepOver, StepOut };
        bool step(quint32 threadId, RunMode, bool lineStep);
        bool clearStep();
        void enableExceptionBreaks();
    private:
        Reply fetchReply(quint32 id);
    private:
        QTcpServer* d_srv;
        QTcpSocket* d_sock;
        enum Status { WaitHandshake, WaitHeader, WaitData, ProtocolError };
        int d_status;
        quint32 d_len;
        quint32 d_id;
        quint8 d_cmdSet, d_cmd, d_err; // if d_cmdSet == 0 then this is a reply
        typedef QPair<quint8,QByteArray> Packet;
        typedef QHash<quint32,Packet> Replies;
        Replies d_replies; // id -> result_code,
        RunMode d_mode;
        bool d_lineStep;
        quint32 d_modeReq;
        quint32 d_breakMeth;
        quint32 d_domain;
        QHash<QPair<quint32,quint32>,quint32> d_breakPoints; // meth,iloff->reqid
    };

    // possible results of Debugger::getValues:
    struct ObjectRef
    {
        enum { Nil, String, SzArray, Class, Array, Object, Type };
        quint8 type;
        quint32 id;
        ObjectRef(quint8 t = Nil, quint32 obj = 0):type(t),id(obj){}
    };

    struct UnmanagedPtr
    {
        quint64 ptr;
        UnmanagedPtr(quint64 p = 0):ptr(p) {}
    };

    struct ValueType
    {
        quint32 cls;
        QVariantList fields;
    };
}

Q_DECLARE_METATYPE(Mono::ObjectRef)
Q_DECLARE_METATYPE(Mono::UnmanagedPtr)
Q_DECLARE_METATYPE(Mono::ValueType)

#endif // MONODEBUGGER_H
