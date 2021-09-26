These are helper classes for the Oberon+ IDE II (see https://github.com/rochus-keller/Oberon).

Mono is run as a separate process and controlled by MonoEngine.h/cpp.

The IDE debugger communicates with Mono by means of the Mono Soft Debugger wire format (see https://www.mono-project.com/docs/advanced/runtime/docs/soft-debugger-wire-format/) which is implemented in MonoDebugger.h/cpp.

MonoMdbGen.h/cpp is an extension on Pelib (see https://github.com/rochus-keller/Pelib) used to generate the debug symbol format required by Mono.
