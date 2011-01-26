/*
 * Copyright 1997-2007 Sun Microsystems, Inc.  All Rights Reserved.
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

// This code redirects the OS/2 DLL initialization/termination calls to
// the Windows DllMain() and registers the DLL with Odin

#include <os2wrap.h> // Odin32 OS/2 api wrappers
#include <odinlx.h>
#include <misc.h>

#include <types.h>

extern BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved);

static HMODULE dllHandle = 0;

unsigned long SYSTEM _DLL_InitTerm(unsigned long hModule, unsigned long ulFlag)
{
    size_t i;
    APIRET rc;

    // If ulFlag is zero then the DLL is being loaded so initialization should
    // be performed.  If ulFlag is 1 then the DLL is being freed so termination
    // should be performed. A non-zero value must be returned to indicate success.

    // Note that we don't perform CRT initialization and things because this
    // is done in os_os2_init.cpp of JVM.DLL that is already loaded

    switch (ulFlag) {
        case 0 :
            dllHandle = RegisterLxDll(hModule, DllMain, NULL,
                                      ODINNT_MAJOR_VERSION,
                                      ODINNT_MINOR_VERSION,
                                      IMAGE_SUBSYSTEM_WINDOWS_CUI);
            if (dllHandle == 0)
              break;

            return 1;

        case 1 :
            if (dllHandle) {
              UnregisterLxDll(dllHandle);
            }
            return 0;

        default:
            break;
    }

    // failure
    return 0;
}

