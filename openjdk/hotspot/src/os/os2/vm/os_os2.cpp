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
#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2wrap2.h>

// do not include precompiled header file
# include "incls/_os_windows.cpp.incl"

#define MIN2(x, y) (((x) < (y))? (x) : (y))

bool os::dll_address_to_library_name(address addr, char* buf,
                                     int buflen, int* offset) {
    os2_HMODULE hmod;
    os2_ULONG obj, offs;
    os2_APIRET arc = DosQueryModFromEIP(&hmod, &obj, buflen, buf, &offs,
                                        (ULONG)addr);
    if (arc == os2_NO_ERROR) {
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
        if (arc != os2_NO_ERROR) {
            assert(false, "Can't find jvm module.");
            return false;
        }
    }

    arc = DosQueryModFromEIP(&hmod, &obj, sizeof(buf), buf, &offs, (ULONG)addr);
    if (arc != os2_NO_ERROR)
        return false;
    return hmod == jvmHmod;
}

static void find_and_print_module_info(os2_QSPTRREC *pPtrRec, USHORT hmte,
                                       outputStream *st) {
    os2_QSLREC *pLibRec = pPtrRec->pLibRec;
    while (pLibRec) {
        // It happens that for some modules ctObj is > 0 but pbjInfo is
        // NULL. This seems to be an OS/2 FP13 bug. Here is the solution I
        // found in the Odin32 sources (kernel32/winimagepe2lx.cpp):
        if (pLibRec->ctObj > 0 && pLibRec->pObjInfo == NULL) {
            pLibRec->pObjInfo = (os2_QSLOBJREC *)
                ((char*) pLibRec
                 + ((sizeof(os2_QSLREC)                         /* size of the lib record */
                     + pLibRec->ctImpMod * sizeof(os2_USHORT)   /* size of the array of imported modules */
                     + strlen((char*)pLibRec->pName) + 1        /* size of the filename */
                     + 3) & ~3));                               /* the size is align on 4 bytes boundrary */
            pLibRec->pNextRec = (os2_PVOID *)((char *)pLibRec->pObjInfo
                                              + sizeof(os2_QSLOBJREC) * pLibRec->ctObj);
        }
        if (pLibRec->hmte == hmte) {
            // mark as already walked
            pLibRec->hmte = os2_NULLHANDLE;
            st->print("  %s\n", pLibRec->pName);
            for (ULONG i = 0; i < pLibRec->ctObj; ++i) {
                if (pLibRec->pObjInfo[i].oaddr)
                    st->print("    " PTR_FORMAT " - " PTR_FORMAT "\n",
                              pLibRec->pObjInfo[i].oaddr,
                              pLibRec->pObjInfo[i].oaddr +
                              pLibRec->pObjInfo[i].osize);
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
    if (arc != os2_NO_ERROR) {
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

julong os::allocatable_physical_memory(julong size) {
    // The primary role of this method on OS/2 is to limit the default maximum
    // Java heap size as the calculations done by Java when defining this limit
    // use fractions of the physical memory but in fact the OS/2 process can
    // allocate much less than the physical memory size of the modern PCs so
    // the allocation will fail very often in -server mode (-client mode limits
    // the physical RAM size to 1G for calculations so it's not a big problem
    // for it; -server mode limits it to 4G which is very problematic taking
    // into account that the default fraction for the heap size is 1/4 -- see
    // below about the maximum private memory block size). Since this method is
    // called after Java has performed its calculations, we correct the limit
    // here.
    //
    // Some details about the process' maximum private memory block size on
    // OS/2:
    //
    // 1. For systems that don't suport high memory (prior to WSeB AFAIR)
    //    the theoretical maximum memory block size is approx. 512M / 2.
    // 2. For systems with high memory support, the theoretical maximum spans
    //    from VIRTUALADDRESSLIMIT / 2 to VIRTUALADDRESSLIMIT / 1.5 or so.
    //    VIRTUALADDRESSLIMIT is 1024M unless it is explicitly specified in
    //    CONFIG.SYS.
    // 3. The real maximum memory block available to Java is somewhat smaller
    //    than the theoretical maximum and it gets decreased each time the
    //    process allocates memory with DosAllocMem().
    //
    // A typical situation when Java fails in -server mode is when the
    // theoretical maximum is 1024M (or lower) and the amount of physical RAM
    // is 2G and more. Java will want >=512M in this case for the heap, but the
    // process will have less than 512M available because because some space
    // will be occupied by the system DLLs.
    //
    // We solve this problem by limiting the requested size to the real maximum
    // memory block size (minus some threshold, see below).

    static os2_ULONG maxMemBlock = 0;
    if (maxMemBlock == 0) {
        os2_ULONG pageSize;
        os2_ULONG flags = os2_OBJ_ANY;
        os2_APIRET arc;
        arc = DosQuerySysInfo(os2_QSV_PAGE_SIZE, os2_QSV_PAGE_SIZE,
                              (os2_PVOID)&pageSize, sizeof(os2_ULONG));
        if (arc != os2_NO_ERROR)
            return size;
        // get maximum high memory block size
        arc = DosQuerySysInfo(os2_QSV_MAXHPRMEM, os2_QSV_MAXHPRMEM,
                              (os2_PVOID)&maxMemBlock, sizeof(os2_ULONG));
        if (arc == os2_ERROR_INVALID_PARAMETER) {
            // high memory is not supported, get maximum low memory block size
            flags &= ~os2_OBJ_ANY;
            arc = DosQuerySysInfo(os2_QSV_MAXPRMEM, os2_QSV_MAXPRMEM,
                                  (os2_PVOID)&maxMemBlock, sizeof(os2_ULONG));
        }
        if (arc != os2_NO_ERROR)
            return size;
        // maxMemBlock is the maximum memory block available right now. It will
        // decrease by the time when the actual heap allocation takes place. We
        // assume that Java will allocate for non-heap needs (this includes
        // other DLLs it may drag in) no more than 3/10 of the current size.
        // Note that this is quite an arbitrary choice based on some experiments
        // with various VIRTUALADDRESSLIMIT settings. Our mission here is to
        // make java with no explicit -Xmx specification not fail at startup
        // due to "Could not reserve enough space for object heap" error. But
        // even if it fails in some specific configuration, it's always possible
        // to specify the upper limit manually with -Xmx.
        os2_ULONG threshold = maxMemBlock * 3 / 10;
        threshold = ((threshold + pageSize - 1) / pageSize) * pageSize;
        maxMemBlock -= threshold;
    }

    return MIN2(size, maxMemBlock);
}

// Extracts the LIBPATH value from CONFIG.SYS and puts it to the environment
// under the JAVA_LIBPATH name unless JAVA_LIBPATH is already set in which
// case the existing value is simply returned.
const char *getLibPath()
{
    const char *libPath = ::getenv("JAVA_LIBPATH");
    if (libPath)
        return libPath;

    os2_APIRET arc;
    os2_ULONG drive;
    arc = DosQuerySysInfo(os2_QSV_BOOT_DRIVE, os2_QSV_BOOT_DRIVE,
                          &drive, sizeof(drive));
    if (arc != os2_NO_ERROR)
        return NULL;

    char configSys[] = "?:\\CONFIG.SYS";
    *configSys = drive + 'A' - 1;

    FILE *f = fopen(configSys, "r");
    if (!f)
        return NULL;

    char buf[1024];
    char *libPathBuf = NULL;
    while (!feof(f) && !ferror(f)) {
        if (fgets(buf, sizeof(buf), f)) {
            int len = strlen(buf);
            if (libPathBuf) {
                // continuation of the LIBPATH string
                libPathBuf = (char *)realloc(libPathBuf, strlen(libPathBuf) + len + 2);
                if (!libPathBuf)
                    break;
            }
            else if (!strncmp(buf, "LIBPATH", 7)) {
                // beginning of the LIBPATH string
                libPathBuf = (char *)malloc(len + 1 + 5 /*JAVA_*/);
                if (!libPathBuf)
                    break;
                strcpy(libPathBuf, "JAVA_");
            }
            if (libPathBuf) {
                strcat(libPathBuf, buf);
                if (buf[len - 1] == '\n') {
                    // this is the last part, terminate and leave
                    libPathBuf[strlen(libPathBuf) - 1] = '\0';
                    break;
                }
            }
        }
    }

    fclose(f);

    ::putenv(libPathBuf);
    return ::getenv("JAVA_LIBPATH");
}
