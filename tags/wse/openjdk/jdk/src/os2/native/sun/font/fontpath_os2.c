/*
 * Copyright 1998-2007 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2010 netlabs.org. OS/2 Parts.
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

#define OS2EMX_PLAIN_CHAR
#define INCL_WINSHELLDATA
#include <os2wrap.h> // Odin32 OS/2 api wrappers

#include <malloc.h>
#include <string.h>

#if defined (__EMX__) && !__BSD_VISIBLE
char *strnstr(const char *, const char*, int);
#endif

/*
 * Finds out all folders containing registered fonts from the OS/2 font palette
 * and returns them as a semicolon-separated list in the zero-terminated string.
 * The last item is always followed by a semicolon. The returned string must be
 * freed with free(). NULL is returned on failure.
 */
char *getOs2FontPath()
{
    char *list = NULL;
    int listPos = 0, listSize = 0;

    // take the font files from HINI_USERPROFILE\PM_Fonts
    ULONG bufSize = 0;
    BOOL ok = PrfQueryProfileSize(HINI_USERPROFILE, "PM_Fonts", 0, &bufSize);
    if (!ok)
        return NULL;

    char *buffer = (char *)malloc(bufSize + 1 /* terminating zero */);
    if (!buffer)
        return NULL;

    ULONG bufLen = PrfQueryProfileString(HINI_USERPROFILE, "PM_Fonts", 0, 0,
                                         buffer, bufSize);
    if (bufLen) {
        char *key = buffer;
        for (; *key; key += strlen(key) + 1) {
            ULONG keySize = 0;
            ok = PrfQueryProfileSize(HINI_USERPROFILE, "PM_Fonts", key,
                                     &keySize);
            // note: keySize includes terminating zero, to be replaced with ';'
            if (ok) {
                keySize += 1; /* terminating zero */
                if (listSize - listPos < keySize) {
                    listSize += keySize > 512 ? keySize : 512;
                    if (list) {
                        list = (char *)realloc(list, listSize);
                    } else {
                        ++listSize; /* terminating zero */
                        list = (char *)malloc(listSize);
                    }
                    if (!list)
                        break;
                }

                ULONG keyLen =
                    PrfQueryProfileString(HINI_USERPROFILE, "PM_Fonts", key, 0,
                                          list + listPos, listSize - listPos);
                // note: keyLen includes terminating zero
                if (keyLen < 1)
                    continue;

                char *sep = strrchr(list + listPos, '\\');
                if (!sep)
                    continue;
                *sep++ = '\0';
                strupr(list + listPos);

                if (listPos > 0) {
                    // avoid duplicates
                    if (strnstr(list, list + listPos, listPos))
                        continue;
                    list[listPos - 1] = ';';
                }

                listPos = sep - list;
            }
            key += strlen(key) + 1;
        }
    }
    free(buffer);

    if (list) {
        list[listPos - 1] = ';';
        list[listPos] = '\0';
    }

    return list;
}

