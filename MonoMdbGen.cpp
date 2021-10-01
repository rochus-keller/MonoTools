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

#include "MonoMdbGen.h"
#include <PeLib/Instruction.h>
#include <PeLib/Method.h>
#include <PeLib/PELib.h>
#include <PeLib/MethodSignature.h>
#include <PeLib/Value.h>
#include <QCryptographicHash>
#include <QFile>
#include <QIODevice>
#include <QMap>
#include <QtDebug>
using namespace Mono;

static const int Default_LineBase = -1;
static const int Default_LineRange = 8;
static const quint8 Default_OpcodeBase = 9;
static const int MaxAddressIncrement = (255 - Default_OpcodeBase) / Default_LineRange;

static int writeUleb128(QIODevice* out, uint32_t v)
{
#if 0
    int n = 1;
    for (; v >= 0x80; v >>= 7)
    {
        out->putChar( (char)((v & 0x7f) | 0x80) );
        n++;
    }
    out->putChar( v );
    return n;
#else
    int n = 0;
    // from BinaryWriter.cs Write7BitEncodedInt
    do {
        const int high = (v >> 7) & 0x01ffffff;
        quint8 b = (quint8)(v & 0x7f);

        if (high != 0) {
            b = (quint8)(b | 0x80);
        }

        out->putChar(b);
        n++;
        v = high;
    } while(v != 0);
    return n;
#endif
}

static inline int writeUint32( QIODevice* out, quint32 val )
{
    out->putChar( (val >> 0) & 0xff );
    out->putChar( (val >> 8) & 0xff );
    out->putChar( (val >> 16) & 0xff );
    out->putChar( (val >> 24) & 0xff );
    return 4;
}

static int writeString( QIODevice* out, const QByteArray& str )
{
    writeUleb128(out,str.size());
    out->write(str);
    return str.size() + 4;
}

#ifdef _DEBUG_
static QTextStream out(stdout);
template <class T>
static inline void trace( const char* what, const T& val )
{
    out << what << ": " << val << endl;
}
static inline void trace(const char* what, int index, int val, int row, int off )
{
    trace( QString("line idx %1 %2").arg(index).arg(what).toUtf8().constData(),
           QString("iloff=%1 row=%2 code=%3").arg(off).arg(row).arg(val) );
}
#else
template <class T>
static inline void trace( const char* what, const T& val ) {}
static inline void trace(const char* what, int index, int val, int row, int off ) {}
#endif

struct OffsetTable
{
    int TotalFileSize;
    int DataSectionOffset;
    int DataSectionSize;
    int CompileUnitCount;
    int CompileUnitTableOffset;
    int CompileUnitTableSize;
    int SourceCount;
    int SourceTableOffset;
    int SourceTableSize;
    int MethodCount;
    int MethodTableOffset;
    int MethodTableSize;
    int TypeCount;
    int AnonymousScopeCount;
    int AnonymousScopeTableOffset;
    int AnonymousScopeTableSize;
    int FileFlags;

    OffsetTable()
    {
        memset(this,0,sizeof(OffsetTable));
    }

    void write( QIODevice* out )
    {
        writeUint32(out,TotalFileSize);
        writeUint32(out,DataSectionOffset);
        writeUint32(out,DataSectionSize);
        writeUint32(out,CompileUnitCount);
        writeUint32(out,CompileUnitTableOffset);
        writeUint32(out,CompileUnitTableSize);
        writeUint32(out,SourceCount);
        writeUint32(out,SourceTableOffset);
        writeUint32(out,SourceTableSize);
        writeUint32(out,MethodCount);
        writeUint32(out,MethodTableOffset);
        writeUint32(out,MethodTableSize);
        writeUint32(out,TypeCount);

        writeUint32(out,AnonymousScopeCount);
        writeUint32(out,AnonymousScopeTableOffset);
        writeUint32(out,AnonymousScopeTableSize);

        writeUint32(out,Default_LineBase);
        writeUint32(out,Default_LineRange);
        writeUint32(out,Default_OpcodeBase);

        writeUint32(out,FileFlags);
    }
    void print()
    {
        trace("TotalFileSize",TotalFileSize);
        trace("DataSectionOffset",DataSectionOffset);
        trace("DataSectionSize",DataSectionSize);
        trace("CompileUnitCount",CompileUnitCount);
        trace("CompileUnitTableOffset",CompileUnitTableOffset);
        trace("CompileUnitTableSize",CompileUnitTableSize);
        trace("SourceCount",SourceCount);
        trace("SourceCount",SourceCount);
        trace("SourceTableSize",SourceTableSize);
        trace("MethodCount",MethodCount);
        trace("MethodTableOffset",MethodTableOffset);
        trace("MethodTableSize",MethodTableSize);
        trace("TypeCount",TypeCount);

        trace("AnonymousScopeCount",AnonymousScopeCount);
        trace("AnonymousScopeTableOffset",AnonymousScopeTableOffset);
        trace("AnonymousScopeTableSize",AnonymousScopeTableSize);

        trace("Default_LineBase",Default_LineBase);
        trace("Default_LineRange",Default_LineRange);
        trace("Default_OpcodeBase",Default_OpcodeBase);

        trace("FileFlags",FileFlags);
    }
};

struct SourceFileEntry
{
    SourceFileEntry(int index, const char* path):Index(index),file_name(path),DataOffset(0)
    {
        for( int i = 0; i < 16; i++ )
            guid[i] = 0;
        for( int i = 0; i < 16; i++ )
            hash[i] = 0;
    }

    int Index;
    int DataOffset;

    QByteArray file_name;
    quint8 guid[16];
    quint8 hash[16]; // checksum

    void WriteData (QIODevice* bw)
    {
        DataOffset = bw->pos();
        writeString(bw,file_name);
        trace("file_name", file_name);

        QFile f( QString::fromUtf8(file_name) );
        if( f.open(QIODevice::ReadOnly) )
        {
            QCryptographicHash h(QCryptographicHash::Md5);
            h.addData(&f);
            const QByteArray res = h.result();
            Q_ASSERT( res.size() == 16 );
            ::memcpy(hash,res.constData(),16);
        }

        bw->write((const char*)guid,16);
        trace("file_guid", QByteArray::fromRawData((const char*)guid,16).toHex());
        bw->write((const char*)hash,16);
        trace("file_hash", QByteArray::fromRawData((const char*)hash,16).toHex());
        bw->putChar(0); // autogenerated = false
    }

    void Write (QIODevice* bw)
    {
        writeUint32(bw,Index);
        trace("file_index", Index);
        writeUint32(bw,DataOffset);
        trace("file_dataoffset", DataOffset);
    }
};

struct CompileUnitEntry
{
    CompileUnitEntry(int index, const SourceFileEntry& src):Index(index),srcIndex(src.Index) {}

    int Index;
    int DataOffset;
    int srcIndex;

    void WriteData (QIODevice* bw)
    {
        DataOffset = (int) bw->pos();
        writeUleb128(bw, srcIndex);
        trace("unit_src_index", srcIndex);

        writeUleb128(bw, 0); // number of include files
        writeUleb128(bw, 0); // number of namespaces
    }

    void Write (QIODevice* bw)
    {
        writeUint32(bw,Index);
        trace("unit_index", Index);
        writeUint32(bw,DataOffset);
        trace("unit_dataoffset", DataOffset);
    }
};

struct LocalVariableEntry
{
    int Index;
    QByteArray Name;
    int BlockIndex;

    LocalVariableEntry (int i, const QByteArray& n, int b):Index(i),Name(n),BlockIndex(b){}

    void Write (QIODevice* bw)
    {
        writeUleb128(bw,Index);
        writeString(bw,Name);
        writeUleb128(bw,BlockIndex);
    }
};

struct MethodEntry
{
    quint32 DataOffset;
    quint32 LineNumberTableOffset;

    DotNetPELib::Method* meth;

    static const quint8 DW_LNS_copy = 1;
    static const quint8 DW_LNS_advance_pc = 2;
    static const quint8 DW_LNS_advance_line = 3;
    static const quint8 DW_LNS_set_file = 4;
    static const quint8 DW_LNS_const_add_pc = 8;

    static const quint8 DW_LNE_end_sequence = 1;


    MethodEntry(DotNetPELib::Method* m):meth(m),DataOffset(0),LineNumberTableOffset(0) {}

    void WriteData (QIODevice* bw)
    {
        const int CompileUnitIndex = 1;
        const int NamespaceID = 0;

        const int LocalVariableTableOffset = bw->pos();
        const int num_locals = meth->size();
        writeUleb128(bw, num_locals);
        for (int i = 0; i < num_locals; i++)
        {
            LocalVariableEntry e(i,meth->getLocal(i)->Name().c_str(),1);
            e.Write(bw);
        }

        const int CodeBlockTableOffset = bw->pos();
        const int num_code_blocks = 0;
        writeUleb128(bw, num_code_blocks);

        const int ScopeVariableTableOffset = bw->pos();
        const int num_scope_vars = 0;
        writeUleb128(bw, num_scope_vars);

        const int RealNameOffset = 0;

        LineNumberTableOffset = bw->pos();
        int last_line = 1, last_offset = 0;

#if 0 // 1 by default
        bw->putChar(DW_LNS_set_file);
        writeUleb128(bw, 1); // LineNumbers [i].File
#endif
        for( int i = 0; i < meth->instructions().size(); i++ )
        {
            const DotNetPELib::Instruction* op = meth->instructions()[i];
            if( op->OpCode() != DotNetPELib::Instruction::i_line )
                continue;

            // this is essentially MonoSymbolTable.LineNumberTable.Write
            QByteArrayList rowCol = QByteArray::fromRawData(op->Text().c_str(),op->Text().size()).split(':');
            const int row = rowCol.first().toInt();
            int line_inc = row - last_line;
            int offset_inc = op->Offset() - last_offset;

            if (offset_inc >= MaxAddressIncrement) {
                if (offset_inc < 2 * MaxAddressIncrement) {
                    bw->putChar(DW_LNS_const_add_pc);
                    trace("DW_LNS_const_add_pc",i,0,row,op->Offset());
                    offset_inc -= MaxAddressIncrement;
                } else {
                    bw->putChar(DW_LNS_advance_pc);
                    writeUleb128(bw,offset_inc);
                    trace("DW_LNS_advance_pc",i,offset_inc,row,op->Offset());
                    offset_inc = 0;
                }
            }

            if ((line_inc < Default_LineBase) || (line_inc >= Default_LineBase + Default_LineRange)) {
                bw->putChar(DW_LNS_advance_line);
                writeUleb128(bw,line_inc);
                trace("DW_LNS_advance_line",i,line_inc,row,op->Offset());
                if (offset_inc != 0) {
                    bw->putChar(DW_LNS_advance_pc);
                    writeUleb128(bw,offset_inc);
                    trace("DW_LNS_advance_pc",i,offset_inc,row,op->Offset());
                }
                bw->putChar(DW_LNS_copy);
                trace("DW_LNS_copy",i,0,row,op->Offset());
            } else {
                quint8 opcode = (line_inc - Default_LineBase + (Default_LineRange * offset_inc) + Default_OpcodeBase);
                bw->putChar((char)opcode);
                trace("opcode",i,opcode,row,op->Offset());
            }

            last_line = row;
            last_offset = op->Offset();

        }
        bw->putChar(0);
        bw->putChar(1);
        bw->putChar(DW_LNE_end_sequence);
        trace("DW_LNE_end_sequence",0);

        const bool haveColumns = true;
        if( haveColumns )
        {
            for( int i = 0; i < meth->instructions().size(); i++ )
            {
                const DotNetPELib::Instruction* op = meth->instructions()[i];
                if( op->OpCode() != DotNetPELib::Instruction::i_line )
                    continue;

                QByteArrayList rowCol = QByteArray::fromRawData(op->Text().c_str(),op->Text().size()).split(':');
                if( rowCol.size() == 2 )
                {
                    const int row = rowCol.first().toInt();
                    const int col = rowCol.last().toInt();
                    if( row >= 0 )
                    {
                        writeUleb128(bw,col);
                        trace(QString("column idx %1").arg(i).toUtf8().constData(), col);
                    }
                }else
                {
                    writeUleb128(bw,-1);
                    trace(QString("column idx %1").arg(i).toUtf8().constData(), -1);
                }
            }
        }
        DataOffset = bw->pos();

        writeUleb128(bw, CompileUnitIndex);
        trace("CompileUnitIndex",CompileUnitIndex);
        writeUleb128(bw, LocalVariableTableOffset);
        trace("LocalVariableTableOffset",LocalVariableTableOffset);
        writeUleb128(bw, NamespaceID);
        trace("NamespaceID",NamespaceID);

        writeUleb128(bw, CodeBlockTableOffset);
        trace("CodeBlockTableOffset",CodeBlockTableOffset);
        writeUleb128(bw, ScopeVariableTableOffset);
        trace("ScopeVariableTableOffset",ScopeVariableTableOffset);

        writeUleb128(bw, RealNameOffset);
        trace("RealNameOffset",RealNameOffset);

        enum Flags
        {
            LocalNamesAmbiguous	= 1,
            ColumnsInfoIncluded = 1 << 1,
            EndInfoIncluded = 1 << 2
        };

        writeUleb128(bw, haveColumns ? ColumnsInfoIncluded : 0 );
        trace("ColumnsInfoIncluded",haveColumns);
    }

    void Write (QIODevice* bw)
    {
        writeUint32(bw,meth->getToken());
        trace("meth_token", QByteArray::number(quint32(meth->getToken()),16));
        writeUint32(bw,DataOffset);
        trace("meth_dataoffset", DataOffset);
        writeUint32(bw,LineNumberTableOffset);
        trace("meth_lnr_table_offset", LineNumberTableOffset );
    }
};

class MdbGen::Imp
{
public:
    static const int  MajorVersion = 50;
    static const int  MinorVersion = 0;
    static const quint32 Magic1 = 0xfd7fa614;
    static const quint32 Magic2 = 0x45e82623;
    QSet<QByteArray> skipNames;

    DotNetPELib::PELib* pelib;
    OffsetTable ot;

    void write( QIODevice* out)
    {
        // This is essentially MonoSymbolFile.Write
        writeUint32(out,Magic1);
        writeUint32(out,Magic2);
        writeUint32(out,MajorVersion);
        writeUint32(out,MinorVersion);

        out->write((const char*)pelib->moduleGuid, 16);
        trace("assembly_guid", QByteArray::fromRawData((const char*)pelib->moduleGuid,16).toHex());
        //out->write(QByteArray(16,0));

        const quint32 otOff = out->pos();
        ot.write(out);

#if 1
        QMap<quint32,DotNetPELib::Method*> ordered;
        for( int i = 0; i < pelib->allMethods.size(); i++ )
        {
            DotNetPELib::Method* m = pelib->allMethods[i];
#if 0
            qDebug() << m->Signature()->Name().c_str() << QByteArray::number(m->getToken(),16);
#endif
            if( skipNames.contains( QByteArray::fromRawData(m->Signature()->Name().c_str(),m->Signature()->Name().size()) ) )
                continue;
            Q_ASSERT( !ordered.contains(m->getToken()) );
            ordered.insert( m->getToken(), m );
        }
        QList<MethodEntry> meths;
        QMap<quint32,DotNetPELib::Method*>::const_iterator it;
        for( it = ordered.begin(); it != ordered.end(); ++it )
            meths.append( MethodEntry(it.value()) );
#else
        QList<MethodEntry> meths;
        for( int i = 0; i < pelib->allMethods.size(); i++ )
        {
            qDebug() << pelib->allMethods[i]->Signature()->Name().c_str() <<
                        QByteArray::number(pelib->allMethods[i]->getToken(),16);
            meths.append( MethodEntry(pelib->allMethods[i]) );
        }
#endif

        ot.DataSectionOffset = out->pos();
        SourceFileEntry source(1,pelib->sourceFile.c_str());
        source.WriteData(out);
        CompileUnitEntry unit(1,source);
        unit.WriteData(out);
#if 1
        for( int i = 0; i < meths.size(); i++ )
            meths[i].WriteData(out);
#endif
        ot.DataSectionSize = out->pos() - ot.DataSectionOffset;

        ot.MethodTableOffset = out->pos();
#if 1
        for (int i = 0; i < meths.size(); i++)
            meths [i].Write(out);
        ot.MethodTableSize = out->pos() - ot.MethodTableOffset;
#else
        ot.MethodTableSize = 0;
#endif

        ot.SourceTableOffset = out->pos();
        source.Write(out);
        ot.SourceTableSize = out->pos() - ot.SourceTableOffset;

        ot.CompileUnitTableOffset = out->pos();
        unit.Write(out);
        ot.CompileUnitTableSize = out->pos() - ot.CompileUnitTableOffset;

        ot.AnonymousScopeCount = 0;
        ot.AnonymousScopeTableOffset = out->pos();
        ot.AnonymousScopeTableSize = 0;

        ot.TypeCount = 0; // last_type_index;
        ot.MethodCount = meths.size();
        ot.SourceCount = 1;
        ot.CompileUnitCount = 1;

        ot.TotalFileSize = out->pos();
        out->seek(otOff);
        ot.write(out);
        ot.print();
    }
};

bool MdbGen::write(const QString& filePath, DotNetPELib::PELib* pelib, const QByteArrayList& skipNames)
{
    QFile out(filePath);
    if( !out.open(QIODevice::WriteOnly) )
        return false;

    d_imp->skipNames = skipNames.toSet();
    Q_ASSERT( pelib );
    d_imp->pelib = pelib;
    d_imp->write(&out);

    return true;
}

MdbGen::MdbGen(QObject *parent) : QObject(parent)
{
    d_imp = new Imp();
}

MdbGen::~MdbGen()
{
    delete d_imp;
}

