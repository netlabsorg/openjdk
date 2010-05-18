/*
 * Copyright 1994-2005 Sun Microsystems, Inc.  All Rights Reserved.
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

/*
 * Machine Dependent implementation of the dynamic linking support
 * for java.  This routine is Solaris specific.
 */

#include "hpi_impl.h"

#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "path_md.h"
#include "monitor_md.h"

#ifndef NATIVE
#include "iomgr.h"
#include "threads_md.h"
#endif

/*
 * This lock protects the dl wrappers, assuring that two threads aren't
 * in libdl at the same time.
 */
sys_mon_t _dl_lock;

/*
 * glibc-2.0 libdl is not MT safe.  If you are building with any glibc,
 * chances are you might want to run the generated bits against glibc-2.0
 * libdl.so, so always use locking for any version of glibc.
 */
#ifdef __GLIBC__
    #define NEED_DL_LOCK
#endif

/*
 * Solaris green threads needs to lock around libdl.so.
 */
#if defined(__solaris__) && !defined(NATIVE)
    #define NEED_DL_LOCK
#endif

/*
 * create a string for the JNI native function name by adding the
 * appropriate decorations.
 */
int
sysBuildFunName(char *name, int nameLen, int args_size, int encodingIndex)
{
  /* On Solaris, there is only one encoding method. */
    if (encodingIndex == 0)
        return 1;
    return 0;
}

/*
 * create a string for the dynamic lib open call by adding the
 * appropriate pre and extensions to a filename and the path
 */
void
sysBuildLibName(char *holder, int holderlen, char *pname, char *fname)
{
    const size_t pnamelen = pname ? strlen(pname) : 0;

    /* Quietly truncate on buffer overflow.  Should be an error. */
    if (pnamelen + strlen(fname) + 10 > (size_t) holderlen) {
        *holder = '\0';
        return;
    }

    if (pnamelen == 0) {
        sprintf(holder, "lib%s.so", fname);
    } else {
        sprintf(holder, "%s/lib%s.so", pname, fname);
    }
}


#ifdef __linux__
    static int thr_main(void)
    {
        return -1;
    }
#else
   #ifndef NATIVE
   extern int thr_main(void);
   #endif
#endif

void *
sysLoadLibrary(const char *name, char *err_buf, int err_buflen)
{
    void * result;

#ifdef NEED_DL_LOCK
    sysMonitorEnter(sysThreadSelf(), &_dl_lock);
    result = dlopen(name, RTLD_NOW);
    sysMonitorExit(sysThreadSelf(), &_dl_lock);
#else
    result = dlopen(name, RTLD_LAZY);
#endif
    /*
     * This is a bit of bulletproofing to catch the commonly occurring
     * problem of people loading a library which depends on libthread into
     * the VM.  thr_main() should always return -1 which means that libthread
     * isn't loaded.
     */
#ifndef NATIVE
    if (thr_main() != -1) {
         VM_CALL(panic)("libthread loaded into green threads");
    }
#endif
    if (result == NULL) {
        strncpy(err_buf, dlerror(), err_buflen-2);
        err_buf[err_buflen-1] = '\0';
    }
    return result;
}

void
sysUnloadLibrary(void *handle)
{
#ifdef NEED_DL_LOCK
    sysMonitorEnter(sysThreadSelf(), &_dl_lock);
    dlclose(handle);
    sysMonitorExit(sysThreadSelf(), &_dl_lock);
#else
    dlclose(handle);
#endif
}

void *
sysFindLibraryEntry(void *handle, const char *name)
{
    void *sym;
#ifdef NEED_DL_LOCK
    sysMonitorEnter(sysThreadSelf(), &_dl_lock);
    sym = dlsym(handle, name);
    sysMonitorExit(sysThreadSelf(), &_dl_lock);
#else
    sym = dlsym(handle, name);
#endif
    return sym;
}
