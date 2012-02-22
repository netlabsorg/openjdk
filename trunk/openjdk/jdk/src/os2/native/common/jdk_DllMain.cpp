/*
 * Copyright 1997-2007 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2010-2011 netlabs.org. OS/2 Parts.
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

// This code performs C++ runtime termination to make sure that the
// destructors of static objects are destroyed before Odin unloads itself
// at program termination. This is achieved by registering the DLL with Odin
// which makes sure it calls the supplied DllMain() routine at the right
// time. Note that using DosExitList() for this purpose is not a good idea
// because some Odin services still needed by the destructor code of some JDK
// classes may already be down when exit list routines are run.

#define INCL_DOSPROCESS
#include <os2wrap.h> // Odin32 OS/2 api wrappers
#include <odinlx.h>
#include <misc.h>

#include <emx/startup.h>

static HMODULE dllHandle = 0;

#ifdef HAVE_DLLMAIN
extern "C" BOOL WINAPI DllMain(HANDLE hInstance, DWORD ul_reason_for_call,
                               LPVOID);
#endif

static BOOL WINAPI DefaultDllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    BOOL rc = TRUE;

#ifdef HAVE_DLLMAIN
    rc = DllMain(hinst, reason, reserved);
#endif

    // call destructors when detaching the DLL from the process
    if (reason == 0)
        __ctordtorTerm();

    return rc;
}

unsigned long SYSTEM _DLL_InitTerm(unsigned long hModule, unsigned long ulFlag)
{
    // If ulFlag is zero then the DLL is being loaded so initialization should
    // be performed.  If ulFlag is 1 then the DLL is being freed so termination
    // should be performed. A non-zero value must be returned to indicate success.

    // Note that we don't perform CRT initialization and things because this
    // is done in os_os2_init.cpp of JVM.DLL that is always loaded first

    switch (ulFlag) {
        case 0 :
            dllHandle = RegisterLxDll(hModule, DefaultDllMain, NULL,
                                      ODINNT_MAJOR_VERSION,
                                      ODINNT_MINOR_VERSION,
                                      IMAGE_SUBSYSTEM_WINDOWS_CUI);
            if (dllHandle == 0)
                break;

            __ctordtorInit();

            return 1;

        case 1 :
            if (dllHandle)
                UnregisterLxDll(dllHandle);
            return 1;

        default:
            break;
    }

    // failure
    return 0;
}
