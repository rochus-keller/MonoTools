#ifndef MONOMDBGEN_H
#define MONOMDBGEN_H

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

namespace DotNetPELib
{
    class PELib;
}
namespace Mono
{
    class MdbGen : public QObject
    {
    public:
        explicit MdbGen(QObject *parent = 0);
        ~MdbGen();
        bool write( const QString& filePath, DotNetPELib::PELib*, const QByteArrayList& skipNames );
    private:
        class Imp;
        Imp* d_imp;
    };
}

#endif // MONOMDBGEN_H
