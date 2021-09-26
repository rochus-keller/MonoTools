#include <QCoreApplication>
#include <QtDebug>
#include <QThread>
#include <mono/jit/jit.h>
#include <mono/metadata/object.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-debug.h>
#include "MonoDebugger.h"

class MonoWorker : public QThread
{
public:
    bool ok;

    MonoWorker():ok(true){}

    void run()
    {
        qDebug() << "starting mono";
        const QByteArray path = qApp->applicationDirPath().toUtf8();
        mono_set_dirs(path.constData(),path.constData()); // sets the GAC path; I don't want the GAC to be used
        mono_set_assemblies_path(path.constData()); // https://www.mono-project.com/archived/best_practices/#mono_path

        const char* args1[] = { // funktioniert auch mit Mono3
            //"--stats",
            //"--verbose",
                                "--soft-breakpoints",
                                "--debugger-agent=transport=dt_socket,address=localhost:12345",
                                //"--break",
                                //"HelloWorld.Hello::Main()",
                                "","","","",
        };
        mono_jit_parse_options(6,(char**)args1);

        mono_debug_init (MONO_DEBUG_FORMAT_MONO);

        //MonoDomain* domain = mono_jit_init_version ("myapp", "v4.0.30319");
        MonoDomain* domain = mono_jit_init("myapp");

    #if 0
        MonoImageOpenStatus status;
        MonoAssembly* mscorlib = mono_assembly_open("mscorlib.dll", &status );
        if( mscorlib == 0 || status != MONO_IMAGE_OK )
            return -1;
        MonoImage* mscorimg = mono_assembly_get_image (mscorlib);

        MonoClass* AssemblyName = mono_class_from_name(mscorimg, "System.Reflection", "AssemblyName");
        if( AssemblyName == 0 )
            return -1;

        MonoObject* obj = mono_object_new(domain, AssemblyName);
        if( obj == 0 )
            return -1;
        // TODO mono_gchandle_new, mono_gchandle_get_target, mono_gchandle_free, mono_object_new_pinned

        MonoMethod* ctor_method = mono_class_get_method_from_name( AssemblyName, ".ctor", 1 );
        if( ctor_method == 0 )
            return -1;

        void *args [1];
        args [0] = mono_string_new (domain, "TestAssembly");
        mono_runtime_invoke (ctor_method, obj, args, NULL);
    #else
        MonoAssembly* assembly = mono_domain_assembly_open (domain, "/home/me/Programme/Mono3/bin/HelloWorld.exe");
        if (!assembly)
        {
            ok = false;
            return;
        }

        const char* args2[] = { // funktioniert auch mit Mono3
            "Gugus",
            "Alpha",
            "Beta",
            "",
            "",
        };
        const int retval = mono_jit_exec(domain, assembly, 3, (char**)args2);
    #endif

    #if 0
        mono_assembly_close( mscorlib ); // crashes in jit_cleanup if called, both in so and static
    #endif

        mono_jit_cleanup (domain);

        qDebug() << "ended mono";
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Mono::Debugger dbg;
    dbg.open();

#if 1
    MonoWorker mono;
    mono.start();

    //mono.wait();
    MonoWorker::connect(&mono, SIGNAL(finished()), qApp,SLOT(quit()));
#endif

    return a.exec();
}
