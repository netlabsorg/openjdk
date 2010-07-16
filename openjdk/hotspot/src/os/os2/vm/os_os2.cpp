/*
 * Copyright 1997-2010 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
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

// As the OS/2 platform currently reuses the Windows sources, here we only
// redirect the OS/2 DLL initialization/termination calls to DllMain()

#ifdef __WIN32OS2__

#include <os2.h> // Odin32 OS/2 api wrappers
#include <odinlx.h>
#include <misc.h>

#include <types.h>
#ifdef __EMX__
#include <emx/startup.h>
#endif

extern "C" BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved);

static HMODULE dllHandle = 0;

// _DLL_InitTerm is the function that gets called by the operating system
// loader when it loads and frees this DLL for each process that accesses
// this DLL.  However, it only gets called the first time the DLL is loaded
// and the last time it is freed for a particular process.  The system
// linkage convention MUST be used because the operating system loader is
// calling this function.
unsigned long SYSTEM _DLL_InitTerm(unsigned long hModule, unsigned long ulFlag)
{
  size_t i;
  APIRET rc;

  // If ulFlag is zero then the DLL is being loaded so initialization should
  // be performed.  If ulFlag is 1 then the DLL is being freed so termination
  // should be performed. A non-zero value must be returned to indicate success.

  switch (ulFlag) {
  case 0 :
#ifdef __EMX__
    // initialize the C library
    if (!_CRT_init())
      break;
    // initialize C++ statics
    __ctordtorInit();
#else
#  error "Add code to initialize C/C++ library of this compiler!"
#endif

    // check that the runtime Odin version matches the compile time version
    // (this will issue a message box and abort the process on mismatch)
    CheckVersionFromHMOD(PE2LX_VERSION, hModule);

    // enable __try/__except support
    EnableSEH();

    dllHandle = RegisterLxDll(hModule, DllMain, NULL);
    if (dllHandle == 0)
      break;

    return 1;

  case 1 :
    if (dllHandle) {
      UnregisterLxDll(dllHandle);
    }
#ifdef __EMX__
    // destroy C++ statics
    __ctordtorTerm();
    _CRT_term();
#else
#  error "Add code to initialize C/C++ library of this compiler!"
#endif
    return 0;

  default:
    break;
  }

  // failure
  return 0;
}

#endif // __WIN32OS2__
