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

#include "MonoEngine.h"
#ifndef _MONO_ENGINE_EXT_
#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-debug.h>
#include <QVector>
#else
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#endif
#include <QtDebug>
using namespace Mono;

Engine::Engine(QObject *parent) : QObject(parent),d_toConsole(true),
#ifndef _MONO_ENGINE_EXT_
    d_domain(0)
#else
  d_proc(0),d_running(false)
#endif
{

}

Engine::~Engine()
{
    finish();

}

void Engine::setToConsole(bool on)
{
    d_toConsole = on;
}

bool Engine::isRunning() const
{
#ifndef _MONO_ENGINE_EXT_
    return d_domain != 0;
#else
    return d_running;
#endif
}

QString Engine::getMonoDir()
{
    if( d_monoDir.isEmpty() )
        return QCoreApplication::applicationDirPath()+"/mono";
    else
        return d_monoDir;
}

void Engine::setEnv(const QString& name, const QString& value)
{
    d_env[name] = value;
}

void Engine::removeEnv(const QString& name)
{
    d_env.remove(name);
}

void Engine::setMonoDir(const QString& dirPath)
{
    d_monoDir = dirPath;
}

void Engine::setWorkDir(const QString& path)
{
    d_workDir = path;
}

void Engine::setAssemblySearchPaths(const QStringList& paths, bool firstIsWorkDir)
{
    d_searchPaths = paths;
    if( firstIsWorkDir )
    {
        Q_ASSERT( !paths.isEmpty() );
        d_workDir = paths.first();
    }
}

void Engine::init(quint16 debugServer)
{
#ifndef _MONO_ENGINE_EXT_
    if( d_domain )
#else
    if( d_proc )
#endif
    {
        emit onError(tr("cannot call start when already running"));
        return;
    }

    d_args.clear();
    // d_args += "--stats";
    // d_args += "--verbose";

    if( debugServer )
    {
#ifndef _MONO_ENGINE_EXT_
        d_args += "--soft-breakpoints";
#endif
        d_args += "--debug=mdb-optimizations";
        d_args += "-O=-inline";
        d_args += "--debugger-agent=transport=dt_socket,address=localhost:" + QString::number(debugServer);
        // d_args += "--breakonex"; doesn't seem to work
        // args += "--break";
    }
#ifndef _MONO_ENGINE_EXT_
    QByteArray utf8 = d_path.join(':').toUtf8();
    mono_set_dirs(utf8.constData(),utf8.constData()); // sets the GAC path; I don't want the GAC to be used
    mono_set_assemblies_path(utf8.constData()); // https://www.mono-project.com/archived/best_practices/#mono_path

    QVector<char*> args1(d_args.size());
    for( int i = 0; i < d_args.size(); i++ )
        args1[i] = d_args[i].data();
    mono_jit_parse_options(args.size(),args1.data());

    if( debugServer )
        mono_debug_init (MONO_DEBUG_FORMAT_MONO);

    d_domain = mono_jit_init( "Mono" ); // app domain name
    if( d_domain == 0 )
        emit onError(tr("error calling mono_jit_init") );
#endif
}

void Engine::run(const QString& assembly, const QStringList& args)
{
#ifndef _MONO_ENGINE_EXT_
    if( d_domain == 0 )
    {
        emit onError(tr("error calling run before start") );
        return;
    }
    MonoAssembly* ass = mono_domain_assembly_open(d_domain, assembly.toUtf8().constData());
    if( ass == 0 )
    {
        emit onError(tr("error calling mono_domain_assembly_open") );
        return;
    }

    args.prepend( assembly.toUtf8() ); // there must be at least one argument otherwise Mono crashes
    QVector<char*> args1(args.size());
    for( int i = 0; i < args.size(); i++ )
        args1[i] = args[i].data();

    const int retval = mono_jit_exec(d_domain, ass, args.size(), args1.data());
    qDebug() << "mono_jit_exec returned" << retval;
#else
    d_running = false;
    d_proc = new QProcess(this);
    QStringList pargs = d_args;
    pargs << assembly;
    pargs += args;
    d_proc->setArguments(pargs);

    const QString monoDir = getMonoDir();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
#ifdef Q_OS_WIN
    const QChar separator(';');
#else
    const QChar separator(':');
#endif
    QString searchPath = d_searchPaths.join(separator);
    if( !searchPath.isEmpty() )
        searchPath += separator;
    searchPath += monoDir;
    QMap<QString,QString>::const_iterator i;
    for( i = d_env.begin(); i != d_env.end(); ++i )
        env.insert( i.key(), i.value() );
    env.insert("MONO_PATH", searchPath ); // see mono_set_assemblies_path in assembly.c
    QDir dir(monoDir);
    if( QFileInfo(dir.absoluteFilePath("config")).exists() )
        env.insert("MONO_CONFIG", dir.absoluteFilePath("config"));
    d_proc->setProcessEnvironment(env);
    QString workDir = d_workDir;
    if( workDir.isEmpty() )
        workDir = monoDir;
    d_proc->setWorkingDirectory(workDir);
    d_proc->setProgram( dir.absoluteFilePath("mono"));
    if( !d_inputFile.isEmpty() )
        d_proc->setStandardInputFile(d_inputFile);
    connect(d_proc,SIGNAL(error(QProcess::ProcessError)),this,SLOT(onProcError()));
    connect(d_proc,SIGNAL(stateChanged(QProcess::ProcessState)),this,SLOT(onProcState()) );
    connect(d_proc,SIGNAL(readyReadStandardError()),this,SLOT(onStdErr()) );
    connect(d_proc,SIGNAL(readyReadStandardOutput()),this,SLOT(onStdOut()) );
    d_proc->start();

#endif

}

void Engine::finish()
{
#ifndef _MONO_ENGINE_EXT_
    if( d_domain == 0 )
        return;
    mono_jit_cleanup(d_domain);
    d_domain = 0;
#else
    if( d_proc == 0 )
        return;
    d_proc->terminate();
    d_proc->waitForFinished();
#endif
}

bool Engine::write(const QByteArray& data)
{
    if( !d_running )
        return false;
    Q_ASSERT( d_proc );
    d_proc->write(data);
    return true;
}

void Engine::setInputFile(const QString& path)
{
    d_inputFile = path;
}

void Engine::onProcError()
{
#ifdef _MONO_ENGINE_EXT_
    if( d_proc == 0 )
        return;
    emit onError( d_proc->errorString() );
#endif
}

void Engine::onProcState()
{
#ifdef _MONO_ENGINE_EXT_
    if( d_proc == 0 )
        return;
    switch( d_proc->state() )
    {
    case QProcess::NotRunning:
        if( !d_running )
            emit onError( tr("could not start '%1'").arg( d_proc->program() ) );
        else
        {
            onStdOut();
            onStdErr();
            if( d_proc->exitStatus() == QProcess::NormalExit )
                emit onFinished(d_proc->exitCode(),true);
            else
                emit onFinished(-1,false);
        }
        d_proc->deleteLater();
        d_proc = 0;
        break;
    case QProcess::Starting:
        break;
    case QProcess::Running:
        d_running = true;
        break;
    }
#endif
}

static QByteArray stuff( const QByteArray& in )
{
    QByteArray res;
    int left = 0;
    for( int i = 0; i < in.size(); i++ )
    {
        if( in[i] == 0 )
        {
            res += in.mid(left, i-left);
            left = i+1;
        }
    }
    res += in.mid(left);
    return res;
}

void Engine::onStdErr()
{
#ifdef _MONO_ENGINE_EXT_
    if( d_proc == 0 )
        return;
    const QByteArray msg = stuff(d_proc->readAllStandardError());
    if( msg.isEmpty() )
        return;
    if( d_toConsole )
    {
        QTextStream out(stderr);
        out << msg;
    }
    emit onConsole( QString::fromUtf8(msg), true );
#endif
}

void Engine::onStdOut()
{
#ifdef _MONO_ENGINE_EXT_
    if( d_proc == 0 )
        return;
    const QByteArray msg = stuff(d_proc->readAllStandardOutput());
    if( msg.isEmpty() )
        return;
    if( d_toConsole )
    {
        QTextStream out(stdout);
        out << msg;
    }
    emit onConsole( QString::fromUtf8(msg), false );
#endif
}

