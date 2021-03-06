#ifndef MONOENGINE_H
#define MONOENGINE_H

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
#include <QStringList>
#include <QMap>

typedef struct _MonoDomain MonoDomain;
class QProcess;

namespace Mono
{
    class Engine : public QObject
    {
        Q_OBJECT
    public:
        explicit Engine(QObject *parent = 0);
        ~Engine();

        void setToConsole(bool on);
        bool isRunning() const;
        QString getMonoDir();

        void setEnv( const QString& name, const QString& value );
        void removeEnv( const QString& name );

    public slots:
        void setMonoDir( const QString& dirPath );
        void setWorkDir( const QString& path );
        void setAssemblySearchPaths( const QStringList&, bool firstIsWorkDir = false );
        void init(quint16 debugServer = 0);
        void run(const QString& assembly, const QStringList& args = QStringList() );
        void finish();
        bool write( const QByteArray& );
        void setInputFile( const QString& );

    signals:
        void onError( const QString& );
        void onConsole( const QString& msg, bool err );
        void onFinished(int exitCode, bool normalExit);

    protected slots:
        void onProcError();
        void onProcState();
        void onStdErr();
        void onStdOut();
    private:
#ifndef _MONO_ENGINE_EXT_
        MonoDomain* d_domain;
#else
        QProcess* d_proc;
        bool d_running;
#endif
        QStringList d_args;
        QStringList d_searchPaths;
        QString d_workDir;
        QString d_monoDir;
        QString d_inputFile;
        QMap<QString,QString> d_env;
        bool d_toConsole;
    };
}

#endif // MONOENGINE_H
