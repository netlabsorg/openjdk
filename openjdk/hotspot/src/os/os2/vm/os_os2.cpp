/*
 * Copyright 1997-2010 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2010 netlabs.org. OS/2 Parts.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
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

#define INCL_DOSMODULEMGR
#define INCL_DOSPROFILE
#include <os2wrap2.h>

// do not include precompiled header file
# include "incls/_os_windows.cpp.incl"

bool os::dll_address_to_library_name(address addr, char* buf,
                                     int buflen, int* offset) {
    os2_HMODULE hmod;
    os2_ULONG obj, offs;
    os2_APIRET arc = DosQueryModFromEIP(&hmod, &obj, buflen, buf, &offs,
                                        (ULONG)addr);
    if (arc == NO_ERROR) {
    // buf already contains path name
    if (offset) {
          // as the offset is relative to the object (and there is no separate
          // field for the object number to return) we attempt to combine both
          // wildly assuming that there are no more than 256 objects and each is
          // not bigger than 0xFFFFFF bytes in size
          *offset = obj << 24 | offs;
    }
        return true;
    } else {
        if (buf) buf[0] = '\0';
        if (offset) *offset = -1;
        return false;
    }
}

static os2_HMODULE jvmHmod = os2_NULLHANDLE;

// check if addr is inside jvm.dll
bool os::address_is_in_vm(address addr) {
    os2_HMODULE hmod;
    char buf[CCHMAXPATH];
    os2_ULONG obj, offs;
    os2_APIRET arc;
    if (jvmHmod == os2_NULLHANDLE) {
        arc = DosQueryModFromEIP(&jvmHmod, &obj, sizeof(buf), buf, &offs,
                                 (ULONG)&os::address_is_in_vm);
        if (arc != NO_ERROR) {
            assert(false, "Can't find jvm module.");
            return false;
        }
    }

    arc = DosQueryModFromEIP(&hmod, &obj, sizeof(buf), buf, &offs, (ULONG)addr);
    if (arc != NO_ERROR)
        return false;
    return hmod == jvmHmod;
}

void os::print_dll_info(outputStream *st) {
    int pid = os::current_process_id();
    char *buf = (char *)malloc(64 * 1024);
    os2_APIRET arc = DosQuerySysState(os2_QS_PROCESS | os2_QS_MTE, 0, pid, 0,
                                      buf, 64 * 1024);
    if (arc != NO_ERROR) {
        assert(false, "DosQuerySysState() failed.");
        return;
    }

    os2_QSPTRREC *pPtrRec = (os2_QSPTRREC *)buf;
    os2_QSLREC *pLibRec = pPtrRec->pLibRec;
    st->print_cr("Dynamic libraries:");
    while (pLibRec) {
        for (ULONG i = 0; i < pLibRec->ctObj; ++i) {
            if (!i)
                st->print(PTR_FORMAT " - " PTR_FORMAT " \t%s\n",
                          pLibRec->pObjInfo[i].oaddr,
                          pLibRec->pObjInfo[i].oaddr + pLibRec->pObjInfo[i].osize,
                          pLibRec->pName);
            else
                st->print(PTR_FORMAT " - " PTR_FORMAT,
                          pLibRec->pObjInfo[i].oaddr,
                          pLibRec->pObjInfo[i].oaddr + pLibRec->pObjInfo[i].osize);
        }
        pLibRec = (os2_QSLREC *)pLibRec->pNextRec;
    }
    free(buf);
}

