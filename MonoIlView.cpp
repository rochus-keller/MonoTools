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

#include "MonoIlView.h"
#include "MonoDebugger.h"
#include <QHeaderView>
using namespace Mono;

// Generated with PelibGen::printInstructionTable
// main
const IlView::Opcode IlView::s_main[] = {
{ "nop", 0x0, 1, 1 },
{ "break", 0x1, 1, 1 },
{ "ldarg.0", 0x2, 1, 1 },
{ "ldarg.1", 0x3, 1, 1 },
{ "ldarg.2", 0x4, 1, 1 },
{ "ldarg.3", 0x5, 1, 1 },
{ "ldloc.0", 0x6, 1, 1 },
{ "ldloc.1", 0x7, 1, 1 },
{ "ldloc.2", 0x8, 1, 1 },
{ "ldloc.3", 0x9, 1, 1 },
{ "stloc.0", 0xa, 1, 1 },
{ "stloc.1", 0xb, 1, 1 },
{ "stloc.2", 0xc, 1, 1 },
{ "stloc.3", 0xd, 1, 1 },
{ "ldarg.s", 0xe, 2, 4 },
{ "ldarga.s", 0xf, 2, 4 },
{ "starg.s", 0x10, 2, 4 },
{ "ldloc.s", 0x11, 2, 4 },
{ "ldloca.s", 0x12, 2, 4 },
{ "stloc.s", 0x13, 2, 4 },
{ "ldnull", 0x14, 1, 1 },
{ "ldc.i4.M1", 0x15, 1, 1 },
{ "ldc.i4.0", 0x16, 1, 1 },
{ "ldc.i4.1", 0x17, 1, 1 },
{ "ldc.i4.2", 0x18, 1, 1 },
{ "ldc.i4.3", 0x19, 1, 1 },
{ "ldc.i4.4", 0x1a, 1, 1 },
{ "ldc.i4.5", 0x1b, 1, 1 },
{ "ldc.i4.6", 0x1c, 1, 1 },
{ "ldc.i4.7", 0x1d, 1, 1 },
{ "ldc.i4.8", 0x1e, 1, 1 },
{ "ldc.i4.s", 0x1f, 2, 7 },
{ "ldc.i4", 0x20, 5, 8 },
{ "ldc.i8", 0x21, 9, 9 },
{ "ldc.r4", 0x22, 5, 10 },
{ "ldc.r8", 0x23, 9, 11 },
{ 0, 0x0, 0, 0 },
{ "dup", 0x25, 1, 1 },
{ "pop", 0x26, 1, 1 },
{ "jmp", 0x27, 5, IlView::Token },
{ "call", 0x28, 5, IlView::Token },
{ "calli", 0x29, 5, IlView::Token },
{ "ret", 0x2a, 1, 1 },
{ "br.s", 0x2b, 2, 2 },
{ "brzero.s", 0x2c, 2, 2 },
{ "brtrue.s", 0x2d, 2, 2 },
{ "beq.s", 0x2e, 2, 2 },
{ "bge.s", 0x2f, 2, 2 },
{ "bgt.s", 0x30, 2, 2 },
{ "ble.s", 0x31, 2, 2 },
{ "blt.s", 0x32, 2, 2 },
{ "bne.un.s", 0x33, 2, 2 },
{ "bge.un.s", 0x34, 2, 2 },
{ "bgt.un.s", 0x35, 2, 2 },
{ "ble.un.s", 0x36, 2, 2 },
{ "blt.un.s", 0x37, 2, 2 },
{ "br", 0x38, 5, 3 },
{ "brzero", 0x39, 5, 3 },
{ "brtrue", 0x3a, 5, 3 },
{ "beq", 0x3b, 5, 3 },
{ "bge", 0x3c, 5, 3 },
{ "bgt", 0x3d, 5, 3 },
{ "ble", 0x3e, 5, 3 },
{ "blt", 0x3f, 5, 3 },
{ "bne.un", 0x40, 5, 3 },
{ "bge.un", 0x41, 5, 3 },
{ "bgt.un", 0x42, 5, 3 },
{ "ble.un", 0x43, 5, 3 },
{ "blt.un", 0x44, 5, 3 },
{ "switch", 0x45, 0, 12 },
{ "ldind.i1", 0x46, 1, 1 },
{ "ldind.u1", 0x47, 1, 1 },
{ "ldind.i2", 0x48, 1, 1 },
{ "ldind.u2", 0x49, 1, 1 },
{ "ldind.i4", 0x4a, 1, 1 },
{ "ldind.u4", 0x4b, 1, 1 },
{ "ldind.u8", 0x4c, 1, 1 },
{ "ldind.i", 0x4d, 1, 1 },
{ "ldind.r4", 0x4e, 1, 1 },
{ "ldind.r8", 0x4f, 1, 1 },
{ "ldind.ref", 0x50, 1, 1 },
{ "stind.ref", 0x51, 1, 1 },
{ "stind.i1", 0x52, 1, 1 },
{ "stind.i2", 0x53, 1, 1 },
{ "stind.i4", 0x54, 1, 1 },
{ "stind.i8", 0x55, 1, 1 },
{ "stind.r4", 0x56, 1, 1 },
{ "stind.r8", 0x57, 1, 1 },
{ "add", 0x58, 1, 1 },
{ "sub", 0x59, 1, 1 },
{ "mul", 0x5a, 1, 1 },
{ "div", 0x5b, 1, 1 },
{ "div.un", 0x5c, 1, 1 },
{ "rem", 0x5d, 1, 1 },
{ "rem.un", 0x5e, 1, 1 },
{ "and", 0x5f, 1, 1 },
{ "or", 0x60, 1, 1 },
{ "xor", 0x61, 1, 1 },
{ "shl", 0x62, 1, 1 },
{ "shr", 0x63, 1, 1 },
{ "shr.un", 0x64, 1, 1 },
{ "neg", 0x65, 1, 1 },
{ "not", 0x66, 1, 1 },
{ "conv.i1", 0x67, 1, 1 },
{ "conv.i2", 0x68, 1, 1 },
{ "conv.i4", 0x69, 1, 1 },
{ "conv.i8", 0x6a, 1, 1 },
{ "conv.r4", 0x6b, 1, 1 },
{ "conv.r8", 0x6c, 1, 1 },
{ "conv.u4", 0x6d, 1, 1 },
{ "conv.u8", 0x6e, 1, 1 },
{ "callvirt", 0x6f, 5, IlView::Token },
{ "cpobj", 0x70, 5, IlView::Token },
{ "ldobj", 0x71, 5, IlView::Token },
{ "ldstr", 0x72, 5, IlView::Token },
{ "newobj", 0x73, 5, IlView::Token },
{ "castclass", 0x74, 5, IlView::Token },
{ "isinst", 0x75, 5, IlView::Token },
{ "conv.r.un", 0x76, 1, 1 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ "unbox", 0x79, 5, IlView::Token },
{ "throw", 0x7a, 1, 1 },
{ "ldfld", 0x7b, 5, IlView::Token },
{ "ldflda", 0x7c, 5, IlView::Token },
{ "stfld", 0x7d, 5, IlView::Token },
{ "ldsfld", 0x7e, 5, IlView::Token },
{ "ldsflda", 0x7f, 5, IlView::Token },
{ "stsfld", 0x80, 5, IlView::Token },
{ "stobj", 0x81, 5, IlView::Token },
{ "conv.ovf.i1.un", 0x82, 1, 1 },
{ "conv.ovf.i2.un", 0x83, 1, 1 },
{ "conv.ovf.i4.un", 0x84, 1, 1 },
{ "conv.ovf.i8.un", 0x85, 1, 1 },
{ "conv.ovf.u1.un", 0x86, 1, 1 },
{ "conv.ovf.u2.un", 0x87, 1, 1 },
{ "conv.ovf.u4.un", 0x88, 1, 1 },
{ "conv.ovf.u8.un", 0x89, 1, 1 },
{ "conv.ovf.i.un", 0x8a, 1, 1 },
{ "conv.ovf.u.un", 0x8b, 1, 1 },
{ "box", 0x8c, 5, IlView::Token },
{ "newarr", 0x8d, 5, IlView::Token },
{ "ldlen", 0x8e, 1, 1 },
{ "ldelema", 0x8f, 5, IlView::Token },
{ "ldelem.i1", 0x90, 1, 1 },
{ "ldelem.u1", 0x91, 1, 1 },
{ "ldelem.i2", 0x92, 1, 1 },
{ "ldelem.u2", 0x93, 1, 1 },
{ "ldelem.i4", 0x94, 1, 1 },
{ "ldelem.u4", 0x95, 1, 1 },
{ "ldelem.u8", 0x96, 1, 1 },
{ "ldelem.i", 0x97, 1, 1 },
{ "ldelem.r4", 0x98, 1, 1 },
{ "ldelem.r8", 0x99, 1, 1 },
{ "ldelem.ref", 0x9a, 1, 1 },
{ "stelem.i", 0x9b, 1, 1 },
{ "stelem.i1", 0x9c, 1, 1 },
{ "stelem.i2", 0x9d, 1, 1 },
{ "stelem.i4", 0x9e, 1, 1 },
{ "stelem.i8", 0x9f, 1, 1 },
{ "stelem.r4", 0xa0, 1, 1 },
{ "stelem.r8", 0xa1, 1, 1 },
{ "stelem.ref", 0xa2, 1, 1 },
{ "ldelem", 0xa3, 5, IlView::Token },
{ "stelem", 0xa4, 5, IlView::Token },
{ "unbox.any", 0xa5, 5, IlView::Token },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ "conv.ovf.i1", 0xb3, 1, 1 },
{ "conv.ovf.u1", 0xb4, 1, 1 },
{ "conv.ovf.i2", 0xb5, 1, 1 },
{ "conv.ovf.u2", 0xb6, 1, 1 },
{ "conv.ovf.i4", 0xb7, 1, 1 },
{ "conv.ovf.u4", 0xb8, 1, 1 },
{ "conv.ovf.i8", 0xb9, 1, 1 },
{ "conv.ovf.u8", 0xba, 1, 1 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ "refanyval", 0xc2, 5, IlView::Token },
{ "ckfinite", 0xc3, 1, 1 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ "mkrefany", 0xc6, 5, IlView::Token },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ "ldtoken", 0xd0, 5, IlView::Token },
{ "conv.u2", 0xd1, 1, 1 },
{ "conv.u1", 0xd2, 1, 1 },
{ "conv.i", 0xd3, 1, 1 },
{ "conv.ovf.i", 0xd4, 1, 1 },
{ "conv.ovf.u", 0xd5, 1, 1 },
{ "add.ovf", 0xd6, 1, 1 },
{ "add.ovf.un", 0xd7, 1, 1 },
{ "mul.ovf", 0xd8, 1, 1 },
{ "mul.ovf.un", 0xd9, 1, 1 },
{ "sub.ovf", 0xda, 1, 1 },
{ "sub.ovf.un", 0xdb, 1, 1 },
{ "endfinally", 0xdc, 1, 1 },
{ "leave", 0xdd, 5, 3 },
{ "leave.s", 0xde, 2, 2 },
{ "stind.i", 0xdf, 1, 1 },
{ "conv.u", 0xe0, 1, 1 },
};

// 0xfe
const IlView::Opcode IlView::s_fe[] = {
{ "arglist", 0x0, 2, 1 },
{ "ceq", 0x1, 2, 1 },
{ "cgt", 0x2, 2, 1 },
{ "cgt.un", 0x3, 2, 1 },
{ "clt", 0x4, 2, 1 },
{ "clt.un", 0x5, 2, 1 },
{ "ldftn", 0x6, 6, IlView::Token },
{ "ldvirtftn", 0x7, 6, IlView::Token },
{ 0, 0x0, 0, 0 },
{ "ldarg", 0x9, 4, 5 },
{ "ldarga", 0xa, 4, 5 },
{ "starg", 0xb, 4, 5 },
{ "unaligned.", 0xc, 3, 1 },
{ "volatile.", 0xd, 2, 1 },
{ "stloc", 0xe, 4, 5 },
{ "localloc", 0xf, 2, 1 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ 0, 0x0, 0, 0 },
{ "tail.", 0x14, 2, 1 },
{ "initobj", 0x15, 6, IlView::Token },
{ "constrained.", 0x16, 6, IlView::Token },
{ "endfilter", 0x17, 2, 1 },
{ "initblk", 0x18, 2, 1 },
{ "no.", 0x19, 3, 7 },
{ "rethrow", 0x1a, 2, 1 },
{ 0, 0x0, 0, 0 },
{ "sizeof", 0x1c, 6, IlView::Token },
{ "refanytype", 0x1d, 2, 1 },
{ "readonly.", 0x1e, 2, 1 },
};

static inline quint64 readUint64( const char* buf )
{
    return (((quint8)buf[0]) << 56) | (((quint8)buf[1]) << 48) |
            (((quint8)buf[2]) << 40) | (((quint8)buf[3]) << 32) |
            (((quint8)buf[4]) << 24) | (((quint8)buf[5]) << 16) |
            (((quint8)buf[6]) << 8) | (((quint8)buf[7]) << 0);

}

static inline quint32 readUint32( const char* buf )
{
    return (((quint8)buf[0]) << 24) | (((quint8)buf[1]) << 16) |
            (((quint8)buf[2]) << 8) | (((quint8)buf[3]) << 0);

}

static quint16 readUint16( const char* buf )
{
    return (((quint8)buf[0]) << 8) | (((quint8)buf[1]) << 0);
}

IlView::IlView(QWidget *parent, Debugger* dbg) : QTreeWidget(parent),d_dbg(dbg)
{
    setHeaderHidden(false);
    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    setColumnCount(4);
    setExpandsOnDoubleClick(false);
    setHeaderLabels( QStringList() << "IL" << "Pos" << "Line" << "Arg" );
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0,QHeaderView::Stretch);

}

bool IlView::load(const QByteArray& bytecode, int curOff)
{
    clear();
    int i = 0;
    QTreeWidgetItem* focus = 0;
    while( i < bytecode.size() )
    {
        const quint8 op = (quint8)bytecode[i];
        const Opcode* il;
        if( op == 0xfe )
        {
            const quint8 op2 = (quint8)bytecode[i+1];
            if( op2 > FeMax )
                return false;
            il = &s_fe[op2];
        }else
        {
            if( op > MainMax )
                return false;
            il = &s_main[op];
        }
        if( il->sym == 0 )
            return false;

        QTreeWidgetItem* item = new QTreeWidgetItem(this);
        item->setText(0, il->sym );
        item->setText(1, QString("%1").arg(i,4,16,QChar('0')) );

        if( curOff >= 0 && curOff == i )
        {
            item->setIcon(0, QPixmap(":/images/marker.png"));
            focus = item;
        }

        if( op == 0x45 ) // special case
        {
            const int len = readUint32( bytecode.constData()+i+1 );
            i += 5 + len * 4;
        }else
            i += il->len;
    }
    if( focus )
    {
        scrollToItem(focus,QAbstractItemView::EnsureVisible);
        setCurrentItem(focus);
        focus->setSelected(true);
    }
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    return true;
}

static QVariant toVal( const IlView::Opcode* op, const char* data )
{
    switch( op->argtype )
    {
    case IlView::Invalid:
    case IlView::NoArg:
        break;
    case IlView::Off8:
        return (int)(*data);
    case IlView::Off32:
        return (qint32)readUint32(data);
    case IlView::Uint8:
        return (quint8)(*data);
    case IlView::Uint16:
        return readUint16(data);
    case IlView::Uint32:
        return readUint32(data);
    case IlView::Int8:
        return (qint8)*data;
    case IlView::Int32:
        return (qint32)readUint32(data);
    case IlView::Int64:
        return (qint64)readUint64(data);
    case IlView::Float32:
        {
            const quint32 v = readUint32(data);
            return *(float*)&v;
        }
    case IlView::Float64:
        {
            const quint64 v = readUint64(data);
            return *(double*)&v;
        }
    case IlView::Token:
        // TODO improve
        return QString("0x%1").arg(readUint32(data));

    case IlView::Switch:
        break;
    }

    return QVariant();
}

bool IlView::load(quint32 methodId, int curOff)
{
    if( d_dbg == 0 )
        return false;

    clear();

    const QByteArray bytecode = d_dbg->getMethodBody(methodId);
    Debugger::MethodDbgInfo info = d_dbg->getMethodInfo(methodId);

    int i = 0;
    QTreeWidgetItem* focus = 0;
    while( i < bytecode.size() )
    {
        const quint8 op = (quint8)bytecode[i];
        const Opcode* il;
        if( op == 0xfe )
        {
            const quint8 op2 = (quint8)bytecode[i+1];
            if( op2 > FeMax )
                return false;
            il = &s_fe[op2];
        }else
        {
            if( op > MainMax )
                return false;
            il = &s_main[op];
        }
        if( il->sym == 0 )
            return false;

        QTreeWidgetItem* item = new QTreeWidgetItem(this);
        item->setText(0, il->sym );
        item->setText(1, QString("%1").arg(i,4,16,QChar('0')) );
        Debugger::MethodDbgInfo::Loc loc = info.find(i);
        if( loc.valid )
        {
            if( loc.col > 0 )
                item->setText(2, QString("%1:%2").arg(loc.row).arg(loc.col) );
            else
                item->setText(2, QString::number(loc.row) );
        }

        if( curOff >= 0 && curOff == i )
        {
            item->setIcon(0, QPixmap(":/images/marker.png"));
            focus = item;
        }

        item->setText(3, toVal(il,bytecode.constData()+i+1).toString() );

        if( op == 0x45 ) // special case
        {
            const int len = readUint32( bytecode.constData()+i+1 );
            i += 5 + len * 4;
        }else
            i += il->len;
    }
    if( focus )
    {
        scrollToItem(focus,QAbstractItemView::EnsureVisible);
        setCurrentItem(focus);
        focus->setSelected(true);
    }
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    return true;
}


