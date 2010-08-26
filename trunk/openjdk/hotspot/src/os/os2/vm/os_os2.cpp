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

static void find_and_print_module_info(os2_QSPTRREC *pPtrRec, USHORT hmte,
                                       outputStream *st) {
    os2_QSLREC *pLibRec = pPtrRec->pLibRec;
    while (pLibRec) {
        if (pLibRec->hmte == hmte) {
            // mark as already walked
            pLibRec->hmte = os2_NULLHANDLE;
            st->print("  %s\n", pLibRec->pName);
            // It happens that for some modules ctObj is > 0 but
            // pbjInfo is NULL. I have no idea why.
            if (pLibRec->pObjInfo && pLibRec->fFlat) {
                for (ULONG i = 0; i < pLibRec->ctObj; ++i) {
                    if (pLibRec->pObjInfo[i].oaddr)
                        st->print("    " PTR_FORMAT " - " PTR_FORMAT "\n",
                                  pLibRec->pObjInfo[i].oaddr,
                                  pLibRec->pObjInfo[i].oaddr +
                                  pLibRec->pObjInfo[i].osize);
                }
            }
            // Print imported modules of this module
            USHORT *pImpMods = (USHORT *)(((ULONG) pLibRec) + sizeof(*pLibRec));
            for (ULONG i = 0; i < pLibRec->ctImpMod; ++i) {
                find_and_print_module_info(pPtrRec, pImpMods[i], st);
            }
        }
        pLibRec = (os2_QSLREC *)pLibRec->pNextRec;
    }
}

void os::print_dll_info(outputStream *st) {
    int pid = os::current_process_id();
    char *buf = (char *)malloc(64 * 1024);
    os2_APIRET arc = DosQuerySysState(os2_QS_PROCESS | os2_QS_MTE, os2_QS_MTE,
                                      pid, 0, buf, 64 * 1024);
    if (arc != NO_ERROR) {
        assert(false, "DosQuerySysState() failed.");
        return;
    }

    st->print_cr("Dynamic libraries:");

    os2_QSPTRREC *pPtrRec = (os2_QSPTRREC *)buf;
    os2_QSPREC *pProcRec = pPtrRec->pProcRec;
    find_and_print_module_info(pPtrRec, pProcRec->hMte, st);
    if (pProcRec->pLibRec) {
        for (USHORT i = 0; i < pProcRec->cLib; ++i) {
            USHORT hmte = pProcRec->pLibRec[i];
            find_and_print_module_info(pPtrRec, hmte, st);
        }
    }

    free(buf);
}

