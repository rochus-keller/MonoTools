#ifndef MONODEBUGGERGUI_H
#define MONODEBUGGERGUI_H

#include <QMainWindow>

namespace Mono
{
    class Engine;
    struct DebuggerEvent;
    class Debugger;

    class DebuggerGui : public QMainWindow
    {
        Q_OBJECT
    public:
        DebuggerGui();
        ~DebuggerGui();

    public slots:
        void onRun();
        void onSuspend();
        void onResume();
        void onExit();
        void onGetThreads();
        void onGetStack();
        void onStepIn();
        void onStepOver();
        void onStepOut();
        void onBreak();
        void onLocals();

    protected slots:
        void onError(const QString& );
        void onEvent(const DebuggerEvent& );
    private:
        Debugger* d_dbg;
        Engine* d_eng;
        enum { Idle, Running, Stepping };
        int d_status;
        QList<quint32> d_threads;
        quint32 d_curThread;
        quint32 d_rootDomain;
        bool d_byteLevel;
    };
}

#endif // MONODEBUGGERGUI_H

