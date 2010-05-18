/*
 * Copyright 2001-2006 Sun Microsystems, Inc.  All Rights Reserved.
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


#ifndef _DEVICES_H_
#define _DEVICES_H_

#include "awt.h"
#include "awt_Toolkit.h"
#include "awt_Win32GraphicsDevice.h"

class AwtWin32GraphicsDevice;

class Devices {

public:
static BOOL                     UpdateInstance(JNIEnv *env);
       int                      GetNumDevices() { return numDevices; }
       AwtWin32GraphicsDevice*  GetDeviceReference(int index, BOOL adjust = TRUE);
       AwtWin32GraphicsDevice*  GetDevice(int index, BOOL adjust = TRUE);
       int                      Release();
       AwtWin32GraphicsDevice** GetRawArray();

       class InstanceAccess {
       public:
           INLINE   InstanceAccess() { devices = Devices::GetInstance(); }
           INLINE  ~InstanceAccess() { devices->Release(); }
           Devices* operator->()     { return devices; }
        private:
           Devices* devices;
           // prevent bad things like copying or getting address of
           InstanceAccess& operator=(const InstanceAccess&);
           InstanceAccess* operator&();
       };
friend                          InstanceAccess;

private:
                                Devices(int numElements);
       void                     AddReference();
static Devices*                 GetInstance();

       AwtWin32GraphicsDevice** devices;
       int                      refCount;
       int                      numDevices;

static Devices*                 theInstance;
static CriticalSection          arrayLock;

};

#endif _DEVICES_H_
