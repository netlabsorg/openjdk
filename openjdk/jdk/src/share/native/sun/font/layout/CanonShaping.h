/*
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Sun designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Sun in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 *
 */

/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __CANONSHAPING_H
#define __CANONSHAPING_H

#include "LETypes.h"

class LEGlyphStorage;

class CanonShaping
{
public:
    static const le_uint8 glyphSubstitutionTable[];
    static const le_uint8 glyphDefinitionTable[];

    static void reorderMarks(const LEUnicode *inChars, le_int32 charCount,
        le_bool rightToLeft, LEUnicode *outChars, LEGlyphStorage &glyphStorage);

private:
    static void sortMarks(le_int32 *indices, const le_int32 *combiningClasses,
        le_int32 index, le_int32 limit);
};

#endif
