#ifndef MONOILVIEW_H
#define MONOILVIEW_H

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

#include <QTreeWidget>

namespace Mono
{
    class Debugger;

    class IlView : public QTreeWidget
    {
        Q_OBJECT
    public:
        struct Opcode
        {
            const char* sym;
            quint8 code;
            quint8 len;
            quint8 argtype;
        };
        enum { MainMax = 0xe0, FeMax = 0x1e };
        enum ArgType {
            Invalid, NoArg, Off8, Off32, Uint8, Uint16, Uint32,
            Int8, Int32, Int64, Float32, Float64, Switch,
            Token
        };
        static const Opcode s_main[];
        static const Opcode s_fe[];

        explicit IlView(QWidget *parent = 0, Debugger* = 0);

        bool load(const QByteArray& bytecode, int curOff = -1);
        bool load( quint32 methodId, int curOff = -1 );
    private:
        Debugger* d_dbg;
    };
}

#endif // MONOILVIEW_H
