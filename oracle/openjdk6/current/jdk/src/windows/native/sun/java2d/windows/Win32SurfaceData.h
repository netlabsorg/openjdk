/*
 * Copyright 1999-2006 Sun Microsystems, Inc.  All Rights Reserved.
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

#ifndef _WIN32SURFACEDATA_H_
#define _WIN32SURFACEDATA_H_


#include "SurfaceData.h"

#include "colordata.h"
#include "awt_Brush.h"
#include "awt_Pen.h"
#include "awt_Win32GraphicsDevice.h"

#include "stdhdrs.h"
#include <ddraw.h>


#define TEST_SURFACE_BITS(a,f) (((a)&(f)) == (f))

/**
 * This include file contains support definitions for loops using the
 * SurfaceData interface to talk to a Win32 drawable from native code.
 */

typedef struct _Win32SDOps Win32SDOps;

/* These defined in ddrawObject.h */
class DDraw;
class DDrawSurface;
class DDrawClipper;
class DDrawDisplayMode;

/*
 * Typedef for the structure which contains the ddObject reference, as
 * well as a refCount so that we know when we can delete the ddObject
 * and this structure.  We have to keep around the old ddObject even
 * after creating a new one because there may be multiple other surfaces
 * that still reference the old ddObject.  When all of those surfaces
 * have been recreated with the new ddObject, the old one can be
 * released.
 */
typedef struct  {
    DDraw                   *ddObject;
    int                     refCount;
    DDrawSurface            *primary;
    DDrawSurface            *syncSurface;
    DDrawClipper            *clipper;
    CriticalSection         *primaryLock;
    BOOL                    valid;
    HMONITOR                hMonitor;
    BOOL                    capsSet;
    BOOL                    canBlt;
    HWND                    hwndFullScreen;
    int                     backBufferCount;
    int                     context;
    BOOL                    accelerated;
} DDrawObjectStruct;

#define CONTEXT_NORMAL 0
#define CONTEXT_DISPLAY_CHANGE 1
#define CONTEXT_ENTER_FULL_SCREEN 2
#define CONTEXT_CHANGE_BUFFER_COUNT 3
#define CONTEXT_EXIT_FULL_SCREEN 4

/*
 * The definitions of the various attribute flags for requesting
 * which rendering objects should be selected into the HDC returned
 * from GetDC().
 */
#define PEN             1
#define NOPEN           2
#define BRUSH           4
#define NOBRUSH         8
#define CLIP            16              /* For tracking purposes only */
#define PENBRUSH        (PEN | BRUSH)
#define PENONLY         (PEN | NOBRUSH)
#define BRUSHONLY       (BRUSH | NOPEN)

/*
 * This function retrieves an HDC for rendering to the destination
 * managed by the indicated Win32SDOps structure.
 *
 * The env parameter should be the JNIEnv of the surrounding JNI context.
 *
 * The ops parameter should be a pointer to the ops object upon which
 * this function is being invoked.
 *
 * The flags parameter should be an inclusive OR of any of the attribute
 * flags defined above.
 *
 * The patrop parameter should be a pointer to a jint that will receive
 * the appropriate ROP code (PATCOPY or PATINVERT) based on the current
 * composite, or NULL if the ROP code will be ignored by the caller.
 *
 * The clip parameter should be a pointer to a rectangle indicating the
 * desired clip.
 *
 * The comp parameter should be a pointer to a Composite object, or NULL
 * which means the Src (default) compositing rule will be used.
 *
 * The pixel parameter should be a 24-bit XRGB value indicating the
 * color that will be used for rendering.  The upper 8 bits are allowed
 * to be any value.
 *
 * The ReleaseDC function should be called to release the lock on the DC
 * after a given atomic set of rendering operations is complete.
 *
 * Note to callers:
 *      This function may use JNI methods so it is important that the
 *      caller not have any outstanding GetPrimitiveArrayCritical or
 *      GetStringCritical locks which have not been released.
 */
typedef HDC GetDCFunc(JNIEnv *env,
                      Win32SDOps *wsdo,
                      jint flags,
                      jint *patrop,
                      jobject clip,
                      jobject comp,
                      jint color);

/*
 * This function releases an HDC that was retrieved from the GetDC
 * function of the indicated Win32SDOps structure.
 *
 * The env parameter should be the JNIEnv of the surrounding JNI context.
 *
 * The ops parameter should be a pointer to the ops object upon which
 * this function is being invoked.
 *
 * The hdc parameter should be the handle to the HDC object that was
 * returned from the GetDC function.
 *
 * Note to callers:
 *      This function may use JNI methods so it is important that the
 *      caller not have any outstanding GetPrimitiveArrayCritical or
 *      GetStringCritical locks which have not been released.
 */
typedef void ReleaseDCFunc(JNIEnv *env,
                           Win32SDOps *wsdo,
                           HDC hdc);


typedef void InvalidateSDFunc(JNIEnv *env,
                              Win32SDOps *wsdo);

typedef void RestoreSurfaceFunc(JNIEnv *env, Win32SDOps *wsdo);

#define READS_PUNT_THRESHOLD 2
#define BLTS_UNPUNT_THRESHOLD  4
typedef struct {
    jboolean            disablePunts;
    jint                pixelsReadThreshold;
    jint                numBltsThreshold;
    jboolean            usingDDSystem;
    DDrawSurface        *lpSurfaceSystem;
    DDrawSurface        *lpSurfaceVram;
    jint                numBltsSinceRead;
    jint                pixelsReadSinceBlt;
} SurfacePuntData;
/*
 * A structure that holds all state global to the native surfaceData
 * object.
 *
 * Note:
 * This structure will be shared between different threads that
 * operate on the same surfaceData, so it should not contain any
 * variables that could be changed by one thread thus placing other
 * threads in a state of confusion.  For example, the hDC field was
 * removed because each thread now has its own shared DC.  But the
 * window field remains because once it is set for a given wsdo
 * structure it stays the same until that structure is destroyed.
 */
struct _Win32SDOps {
    SurfaceDataOps      sdOps;
    jboolean            invalid;
    GetDCFunc           *GetDC;
    ReleaseDCFunc       *ReleaseDC;
    InvalidateSDFunc    *InvalidateSD;
    jint                lockType;       // REMIND: store in TLS
    jint                lockFlags;      // REMIND: store in TLS
    jobject             peer;
    HWND                window;
    RECT                insets;
    jint                depth;
    jint                pixelStride;    // Bytes per pixel
    DWORD               pixelMasks[3];  // RGB Masks for Windows DIB creation
    HBITMAP             bitmap;         // REMIND: store in TLS
    HBITMAP             oldmap;         // REMIND: store in TLS
    HDC                 bmdc;           // REMIND: store in TLS
    int                 bmScanStride;   // REMIND: store in TLS
    int                 bmWidth;        // REMIND: store in TLS
    int                 bmHeight;       // REMIND: store in TLS
    void                *bmBuffer;      // REMIND: store in TLS
    jboolean            bmCopyToScreen; // Used to track whether we
                                        // actually should copy the bitmap
                                        // to the screen
    AwtBrush            *brush;         // used for offscreen surfaces only
    jint                brushclr;
    AwtPen              *pen;           // used for offscreen surfaces only
    jint                penclr;

    int                 x, y, w, h;     // REMIND: store in TLS
    RestoreSurfaceFunc  *RestoreSurface;
    DDrawSurface        *lpSurface;
    int                 backBufferCount;
    DDrawObjectStruct   *ddInstance;
    CriticalSection     *surfaceLock;   // REMIND: try to remove
    jboolean            surfaceLost;
    jboolean            gdiOpPending;   // whether a GDI operation is pending for this
                                        // surface (Get/ReleaseDC were called)
    jint                transparency;
    AwtWin32GraphicsDevice *device;
    SurfacePuntData     surfacePuntData;
};

#define WIN32SD_LOCK_UNLOCKED   0       /* surface is not locked */
#define WIN32SD_LOCK_BY_NULL    1       /* surface locked for NOP */
#define WIN32SD_LOCK_BY_DDRAW   2       /* surface locked by DirectDraw */
#define WIN32SD_LOCK_BY_DIB     3       /* surface locked by BitBlt */

extern "C" {

/*
 * Structure for holding the graphics state of a thread.
 */
typedef struct {
    HDC         hDC;
    Win32SDOps  *wsdo;
    RECT        bounds;
    jobject     clip;
    jobject     comp;
    jint        xorcolor;
    jint        patrop;
    jint        type;
    AwtBrush    *brush;
    jint        brushclr;
    AwtPen      *pen;
    jint        penclr;
} ThreadGraphicsInfo;


/*
 * This function returns a pointer to a native Win32SDOps structure
 * for accessing the indicated Win32 SurfaceData Java object.  It
 * verifies that the indicated SurfaceData object is an instance
 * of Win32SurfaceData before returning and will return NULL if the
 * wrong SurfaceData object is being accessed.  This function will
 * throw the appropriate Java exception if it returns NULL so that
 * the caller can simply return.
 *
 * Note to callers:
 *      This function uses JNI methods so it is important that the
 *      caller not have any outstanding GetPrimitiveArrayCritical or
 *      GetStringCritical locks which have not been released.
 *
 *      The caller may continue to use JNI methods after this method
 *      is called since this function will not leave any outstanding
 *      JNI Critical locks unreleased.
 */
JNIEXPORT Win32SDOps * JNICALL
Win32SurfaceData_GetOps(JNIEnv *env, jobject sData);

JNIEXPORT Win32SDOps * JNICALL
Win32SurfaceData_GetOpsNoSetup(JNIEnv *env, jobject sData);

JNIEXPORT HWND JNICALL
Win32SurfaceData_GetWindow(JNIEnv *env, Win32SDOps *wsdo);

JNIEXPORT void JNICALL
Win32SD_InitDC(JNIEnv *env, Win32SDOps *wsdo, ThreadGraphicsInfo *info,
               jint type, jint *patrop,
               jobject clip, jobject comp, jint color);

JNIEXPORT AwtComponent * JNICALL
Win32SurfaceData_GetComp(JNIEnv *env, Win32SDOps *wsdo);

} /* extern "C" */


#endif _WIN32SURFACEDATA_H_
