/*
 * Copyright 2006 Sun Microsystems, Inc.  All Rights Reserved.
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
 */

#ifndef AWT_BITMAP_UTIL_H
#define AWT_BITMAP_UTIL_H

class BitmapUtil {
public:
    /**
     * Creates B&W Bitmap with transparency mask from specified ARGB input data
     * 0 for opaque pixels, 1 for transparent.
     * MSDN article for ICONINFO says that 'for color icons, this mask only
     * defines the AND bitmask of the icon'. That's wrong! If mask bit for
     * specific pixel is 0, the pixel is drawn opaque, otherwise it's XORed
     * with background.
     */
    static HBITMAP CreateTransparencyMaskFromARGB(int width, int height, int* imageData);

    /**
     * Creates 32-bit ARGB V4 Bitmap (Win95-compatible) from specified ARGB input data
     * The color for transparent pixels (those with 0 alpha) is reset to 0 (BLACK)
     * to prevent errors on systems prior to XP.
     */
    static HBITMAP CreateV4BitmapFromARGB(int width, int height, int* imageData);

};

#endif
