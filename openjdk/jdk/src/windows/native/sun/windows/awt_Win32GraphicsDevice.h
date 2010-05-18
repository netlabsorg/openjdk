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

#ifndef AWT_WIN32GRAPHICSDEVICE_H
#define AWT_WIN32GRAPHICSDEVICE_H

#include "awt.h"
extern "C" {
    #include "img_globals.h"
} // extern "C"
#include "colordata.h"
#include "awt_Palette.h"
#include "awt_MMStub.h"
#include "Devices.h"
#include "dxCapabilities.h"

class AwtPalette;
class Devices;

class AwtWin32GraphicsDevice {
public:
                            AwtWin32GraphicsDevice(int screen, MHND mhnd, Devices *arr);
                            ~AwtWin32GraphicsDevice();
    void                    UpdateDeviceColorState();
    void                    SetGrayness(int grayValue);
    int                     GetGrayness() { return colorData->grayscale; }
    HDC                     GetDC();
    void                    ReleaseDC(HDC hDC);
    jobject                 GetColorModel(JNIEnv *env,
                                          jboolean useDeviceSettings);
    void                    Initialize();
    void                    UpdateDynamicColorModel();
    BOOL                    UpdateSystemPalette();
    unsigned int            *GetSystemPaletteEntries();
    unsigned char           *GetSystemInverseLUT();
    void                    SetJavaDevice(JNIEnv *env, jobject objPtr);
    HPALETTE                SelectPalette(HDC hDC);
    void                    RealizePalette(HDC hDC);
    HPALETTE                GetPalette();
    ColorData               *GetColorData() { return cData; }
    int                     GetBitDepth() { return colorData->bitsperpixel; }
    MHND                    GetMonitor() { return monitor; }
    MONITOR_INFO            *GetMonitorInfo() { return pMonitorInfo; }
    DxCapabilities          *GetDxCaps() { return &dxCaps; }
    jobject                 GetJavaDevice() { return javaDevice; }
    int                     GetDeviceIndex() { return screen; }
    void                    Release();
    void                    DisableOffscreenAcceleration();
    void                    Invalidate(JNIEnv *env);

    static int              DeviceIndexForWindow(HWND hWnd);
    static jobject          GetColorModel(JNIEnv *env, jboolean dynamic,
                                          int deviceIndex);
    static HPALETTE         SelectPalette(HDC hDC, int deviceIndex);
    static void             RealizePalette(HDC hDC, int deviceIndex);
    static ColorData        *GetColorData(int deviceIndex);
    static int              GetGrayness(int deviceIndex);
    static void             UpdateDynamicColorModel(int deviceIndex);
    static BOOL             UpdateSystemPalette(int deviceIndex);
    static HPALETTE         GetPalette(int deviceIndex);
    static MHND             GetMonitor(int deviceIndex);
    static MONITOR_INFO     *GetMonitorInfo(int deviceIndex);
    static void             ResetAllMonitorInfo();
    static BOOL             IsPrimaryPalettized() { return primaryPalettized; }
    static int              GetDefaultDeviceIndex() { return primaryIndex; }
    static void             DisableOffscreenAccelerationForDevice(MHND hMonitor);
    static DxCapabilities   *GetDxCapsForDevice(MHND hMonitor);
    static HDC              GetDCFromScreen(int screen);
    static int              GetScreenFromMHND(MHND mon);

    static int              primaryIndex;
    static BOOL             primaryPalettized;
    static jclass           indexCMClass;
    static jclass           wToolkitClass;
    static jfieldID         dynamicColorModelID;
    static jfieldID         indexCMrgbID;
    static jfieldID         indexCMcacheID;
    static jfieldID         accelerationEnabledID;
    static jmethodID        paletteChangedMID;

private:
    static BOOL             AreSameMonitors(MHND mon1, MHND mon2);
    ImgColorData            *colorData;
    AwtPalette              *palette;
    ColorData               *cData;     // Could be static, but may sometime
                                        // have per-device info in this structure
    BITMAPINFO              *gpBitmapInfo;
    int                     screen;
    MHND                    monitor;
    MONITOR_INFO            *pMonitorInfo;
    jobject                 javaDevice;
    Devices                 *devicesArray;
    DxCapabilities          dxCaps;
};

#endif AWT_WIN32GRAPHICSDEVICE_H
