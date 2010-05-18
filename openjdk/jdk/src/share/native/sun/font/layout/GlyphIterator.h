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

#ifndef __GLYPHITERATOR_H
#define __GLYPHITERATOR_H

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "GlyphDefinitionTables.h"

struct InsertionRecord
{
    InsertionRecord *next;
    le_int32 position;
    le_int32 count;
    LEGlyphID glyphs[ANY_NUMBER];
};

class LEGlyphStorage;
class GlyphPositionAdjustments;

class GlyphIterator {
public:
    GlyphIterator(LEGlyphStorage &theGlyphStorage, GlyphPositionAdjustments *theGlyphPositionAdjustments,
        le_bool rightToLeft, le_uint16 theLookupFlags, FeatureMask theFeatureMask,
        const GlyphDefinitionTableHeader *theGlyphDefinitionTableHeader);

    GlyphIterator(GlyphIterator &that);

    GlyphIterator(GlyphIterator &that, FeatureMask newFeatureMask);

    GlyphIterator(GlyphIterator &that, le_uint16 newLookupFlags);

    virtual ~GlyphIterator();

    void reset(le_uint16 newLookupFlags, LETag newFeatureTag);

    le_bool next(le_uint32 delta = 1);
    le_bool prev(le_uint32 delta = 1);
    le_bool findFeatureTag();

    le_bool isRightToLeft() const;
    le_bool ignoresMarks() const;

    le_bool baselineIsLogicalEnd() const;

    LEGlyphID getCurrGlyphID() const;
    le_int32  getCurrStreamPosition() const;

    le_int32  getMarkComponent(le_int32 markPosition) const;
    le_bool   findMark2Glyph();

    void getCursiveEntryPoint(LEPoint &entryPoint) const;
    void getCursiveExitPoint(LEPoint &exitPoint) const;

    void setCurrGlyphID(TTGlyphID glyphID);
    void setCurrStreamPosition(le_int32 position);
    void setCurrGlyphBaseOffset(le_int32 baseOffset);
    void adjustCurrGlyphPositionAdjustment(float xPlacementAdjust, float yPlacementAdjust,
                                           float xAdvanceAdjust,   float yAdvanceAdjust);

    void setCurrGlyphPositionAdjustment(float xPlacementAdjust, float yPlacementAdjust,
                                        float xAdvanceAdjust,   float yAdvanceAdjust);

    void setCursiveEntryPoint(LEPoint &entryPoint);
    void setCursiveExitPoint(LEPoint &exitPoint);
    void setCursiveGlyph();

    LEGlyphID *insertGlyphs(le_int32 count);
    le_int32 applyInsertions();

private:
    le_bool filterGlyph(le_uint32 index) const;
    le_bool hasFeatureTag() const;
    le_bool nextInternal(le_uint32 delta = 1);
    le_bool prevInternal(le_uint32 delta = 1);

    le_int32  direction;
    le_int32  position;
    le_int32  nextLimit;
    le_int32  prevLimit;

    LEGlyphStorage &glyphStorage;
    GlyphPositionAdjustments *glyphPositionAdjustments;

    le_int32    srcIndex;
    le_int32    destIndex;
    le_uint16   lookupFlags;
    FeatureMask featureMask;

    const GlyphClassDefinitionTable *glyphClassDefinitionTable;
    const MarkAttachClassDefinitionTable *markAttachClassDefinitionTable;

    GlyphIterator &operator=(const GlyphIterator &other); // forbid copying of this class
};

#endif
