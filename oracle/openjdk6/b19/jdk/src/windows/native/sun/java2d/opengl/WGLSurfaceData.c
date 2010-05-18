/*
 * Copyright 2004-2007 Sun Microsystems, Inc.  All Rights Reserved.
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

#include <stdlib.h>

#include "sun_java2d_opengl_WGLSurfaceData.h"

#include "jni.h"
#include "jlong.h"
#include "jni_util.h"
#include "OGLRenderQueue.h"
#include "WGLGraphicsConfig.h"
#include "WGLSurfaceData.h"

/**
 * The methods in this file implement the native windowing system specific
 * layer (WGL) for the OpenGL-based Java 2D pipeline.
 */

extern LockFunc                     OGLSD_Lock;
extern GetRasInfoFunc               OGLSD_GetRasInfo;
extern UnlockFunc                   OGLSD_Unlock;
extern DisposeFunc                  OGLSD_Dispose;

JNIEXPORT void JNICALL
Java_sun_java2d_opengl_WGLSurfaceData_initOps(JNIEnv *env, jobject wglsd,
                                              jlong pConfigInfo,
                                              jlong pPeerData,
                                              jint xoff, jint yoff)
{
    OGLSDOps *oglsdo = (OGLSDOps *)SurfaceData_InitOps(env, wglsd,
                                                       sizeof(OGLSDOps));
    WGLSDOps *wglsdo = (WGLSDOps *)malloc(sizeof(WGLSDOps));

    J2dTraceLn(J2D_TRACE_INFO, "WGLSurfaceData_initOps");

    if (wglsdo == NULL) {
        JNU_ThrowOutOfMemoryError(env, "creating native wgl ops");
        return;
    }

    oglsdo->privOps = wglsdo;

    oglsdo->sdOps.Lock               = OGLSD_Lock;
    oglsdo->sdOps.GetRasInfo         = OGLSD_GetRasInfo;
    oglsdo->sdOps.Unlock             = OGLSD_Unlock;
    oglsdo->sdOps.Dispose            = OGLSD_Dispose;

    oglsdo->drawableType = OGLSD_UNDEFINED;
    oglsdo->activeBuffer = GL_FRONT;
    oglsdo->needsInit = JNI_TRUE;
    oglsdo->xOffset = xoff;
    oglsdo->yOffset = yoff;

    wglsdo->peerData = pPeerData;
    wglsdo->configInfo = (WGLGraphicsConfigInfo *)jlong_to_ptr(pConfigInfo);
    if (wglsdo->configInfo == NULL) {
        free(wglsdo);
        JNU_ThrowNullPointerException(env, "Config info is null in initOps");
    }
}

/**
 * This function disposes of any native windowing system resources associated
 * with this surface.  For instance, if the given OGLSDOps is of type
 * OGLSD_PBUFFER, this method implementation will destroy the actual pbuffer
 * surface.
 */
void
OGLSD_DestroyOGLSurface(JNIEnv *env, OGLSDOps *oglsdo)
{
    WGLSDOps *wglsdo = (WGLSDOps *)oglsdo->privOps;

    J2dTraceLn(J2D_TRACE_INFO, "OGLSD_DestroyOGLSurface");

    if (oglsdo->drawableType == OGLSD_PBUFFER) {
        if (wglsdo->drawable.pbuffer != 0) {
            if (wglsdo->pbufferDC != 0) {
                j2d_wglReleasePbufferDCARB(wglsdo->drawable.pbuffer,
                                           wglsdo->pbufferDC);
                wglsdo->pbufferDC = 0;
            }
            j2d_wglDestroyPbufferARB(wglsdo->drawable.pbuffer);
            wglsdo->drawable.pbuffer = 0;
        }
    }
}

/**
 * Makes the given context current to its associated "scratch" surface.  If
 * the operation is successful, this method will return JNI_TRUE; otherwise,
 * returns JNI_FALSE.
 */
static jboolean
WGLSD_MakeCurrentToScratch(JNIEnv *env, OGLContext *oglc)
{
    WGLCtxInfo *ctxInfo;

    J2dTraceLn(J2D_TRACE_INFO, "WGLSD_MakeCurrentToScratch");

    if (oglc == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "WGLSD_MakeCurrentToScratch: context is null");
        return JNI_FALSE;
    }

    ctxInfo = (WGLCtxInfo *)oglc->ctxInfo;
    if (!j2d_wglMakeCurrent(ctxInfo->scratchSurfaceDC, ctxInfo->context)) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "WGLSD_MakeCurrentToScratch: could not make current");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/**
 * Returns a pointer (as a jlong) to the native WGLGraphicsConfigInfo
 * associated with the given OGLSDOps.  This method can be called from
 * shared code to retrieve the native GraphicsConfig data in a platform-
 * independent manner.
 */
jlong
OGLSD_GetNativeConfigInfo(OGLSDOps *oglsdo)
{
    WGLSDOps *wglsdo;

    if (oglsdo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_GetNativeConfigInfo: ops are null");
        return 0L;
    }

    wglsdo = (WGLSDOps *)oglsdo->privOps;
    if (wglsdo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_GetNativeConfigInfo: wgl ops are null");
        return 0L;
    }

    return ptr_to_jlong(wglsdo->configInfo);
}

/**
 * Makes the given GraphicsConfig's context current to its associated
 * "scratch" surface.  If there is a problem making the context current,
 * this method will return NULL; otherwise, returns a pointer to the
 * OGLContext that is associated with the given GraphicsConfig.
 */
OGLContext *
OGLSD_SetScratchSurface(JNIEnv *env, jlong pConfigInfo)
{
    WGLGraphicsConfigInfo *wglInfo =
        (WGLGraphicsConfigInfo *)jlong_to_ptr(pConfigInfo);
    OGLContext *oglc;

    J2dTraceLn(J2D_TRACE_INFO, "OGLSD_SetScratchContext");

    if (wglInfo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_SetScratchContext: wgl config info is null");
        return NULL;
    }

    oglc = wglInfo->context;
    if (!WGLSD_MakeCurrentToScratch(env, oglc)) {
        return NULL;
    }

    if (OGLC_IS_CAP_PRESENT(oglc, CAPS_EXT_FBOBJECT)) {
        // the GL_EXT_framebuffer_object extension is present, so this call
        // will ensure that we are bound to the scratch pbuffer (and not
        // some other framebuffer object)
        j2d_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }

    return oglc;
}

/**
 * Makes a context current to the given source and destination
 * surfaces.  If there is a problem making the context current, this method
 * will return NULL; otherwise, returns a pointer to the OGLContext that is
 * associated with the destination surface.
 */
OGLContext *
OGLSD_MakeOGLContextCurrent(JNIEnv *env, OGLSDOps *srcOps, OGLSDOps *dstOps)
{
    WGLSDOps *srcWGLOps = (WGLSDOps *)srcOps->privOps;
    WGLSDOps *dstWGLOps = (WGLSDOps *)dstOps->privOps;
    OGLContext *oglc;
    WGLCtxInfo *ctxinfo;
    HDC srcHDC, dstHDC;
    BOOL success;

    J2dTraceLn(J2D_TRACE_INFO, "OGLSD_MakeOGLContextCurrent");

    J2dTraceLn4(J2D_TRACE_VERBOSE, "  src: %d %p dst: %d %p",
                srcOps->drawableType, srcOps,
                dstOps->drawableType, dstOps);

    oglc = dstWGLOps->configInfo->context;
    if (oglc == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_MakeOGLContextCurrent: context is null");
        return NULL;
    }

    if (dstOps->drawableType == OGLSD_FBOBJECT) {
        OGLContext *currentContext = OGLRenderQueue_GetCurrentContext();

        // first make sure we have a current context (if the context isn't
        // already current to some drawable, we will make it current to
        // its scratch surface)
        if (oglc != currentContext) {
            if (!WGLSD_MakeCurrentToScratch(env, oglc)) {
                return NULL;
            }
        }

        // now bind to the fbobject associated with the destination surface;
        // this means that all rendering will go into the fbobject destination
        // (note that we unbind the currently bound texture first; this is
        // recommended procedure when binding an fbobject)
        j2d_glBindTexture(dstOps->textureTarget, 0);
        j2d_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, dstOps->fbobjectID);

        return oglc;
    }

    ctxinfo = (WGLCtxInfo *)oglc->ctxInfo;

    // get the hdc for the destination surface
    if (dstOps->drawableType == OGLSD_PBUFFER) {
        dstHDC = dstWGLOps->pbufferDC;
    } else {
        dstHDC = GetDC(dstWGLOps->drawable.window);
    }

    // get the hdc for the source surface
    if (srcOps->drawableType == OGLSD_PBUFFER) {
        srcHDC = srcWGLOps->pbufferDC;
    } else {
        // the source will always be equal to the destination in this case
        srcHDC = dstHDC;
    }

    // REMIND: in theory we should be able to use wglMakeContextCurrentARB()
    // even when the src/dst surfaces are the same, but this causes problems
    // on ATI's drivers (see 6525997); for now we will only use it when the
    // surfaces are different, otherwise we will use the old
    // wglMakeCurrent() approach...
    if (srcHDC != dstHDC) {
        // use WGL_ARB_make_current_read extension to make context current
        success =
            j2d_wglMakeContextCurrentARB(dstHDC, srcHDC, ctxinfo->context);
    } else {
        // use the old approach for making current to the destination
        success = j2d_wglMakeCurrent(dstHDC, ctxinfo->context);
    }
    if (!success) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_MakeOGLContextCurrent: could not make current");
        if (dstOps->drawableType != OGLSD_PBUFFER) {
            ReleaseDC(dstWGLOps->drawable.window, dstHDC);
        }
        return NULL;
    }

    if (OGLC_IS_CAP_PRESENT(oglc, CAPS_EXT_FBOBJECT)) {
        // the GL_EXT_framebuffer_object extension is present, so we
        // must bind to the default (windowing system provided)
        // framebuffer
        j2d_glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    }

    if (dstOps->drawableType != OGLSD_PBUFFER) {
        ReleaseDC(dstWGLOps->drawable.window, dstHDC);
    }

    return oglc;
}

/**
 * This function initializes a native window surface and caches the window
 * bounds in the given OGLSDOps.  Returns JNI_TRUE if the operation was
 * successful; JNI_FALSE otherwise.
 */
jboolean
OGLSD_InitOGLWindow(JNIEnv *env, OGLSDOps *oglsdo)
{
    PIXELFORMATDESCRIPTOR pfd;
    WGLSDOps *wglsdo;
    WGLGraphicsConfigInfo *wglInfo;
    HWND window;
    RECT wbounds;
    HDC hdc;

    J2dTraceLn(J2D_TRACE_INFO, "OGLSD_InitOGLWindow");

    if (oglsdo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_InitOGLWindow: ops are null");
        return JNI_FALSE;
    }

    wglsdo = (WGLSDOps *)oglsdo->privOps;
    if (wglsdo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_InitOGLWindow: wgl ops are null");
        return JNI_FALSE;
    }

    wglInfo = wglsdo->configInfo;
    if (wglInfo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_InitOGLWindow: graphics config info is null");
        return JNI_FALSE;
    }

    window = AwtComponent_GetHWnd(env, wglsdo->peerData);
    if (!IsWindow(window)) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_InitOGLWindow: disposed component");
        return JNI_FALSE;
    }

    GetWindowRect(window, &wbounds);

    hdc = GetDC(window);
    if (hdc == 0) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_InitOGLWindow: invalid hdc");
        return JNI_FALSE;
    }

    if (!SetPixelFormat(hdc, wglInfo->pixfmt, &pfd)) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_InitOGLWindow: error setting pixel format");
        ReleaseDC(window, hdc);
        return JNI_FALSE;
    }

    ReleaseDC(window, hdc);

    oglsdo->drawableType = OGLSD_WINDOW;
    oglsdo->isOpaque = JNI_TRUE;
    oglsdo->width = wbounds.right - wbounds.left;
    oglsdo->height = wbounds.bottom - wbounds.top;
    wglsdo->drawable.window = window;
    wglsdo->pbufferDC = 0;

    J2dTraceLn2(J2D_TRACE_VERBOSE, "  created window: w=%d h=%d",
                oglsdo->width, oglsdo->height);

    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_sun_java2d_opengl_WGLSurfaceData_initPbuffer
    (JNIEnv *env, jobject wglsd,
     jlong pData, jlong pConfigInfo,
     jboolean isOpaque,
     jint width, jint height)
{
    int attrKeys[] = {
        WGL_MAX_PBUFFER_WIDTH_ARB,
        WGL_MAX_PBUFFER_HEIGHT_ARB,
    };
    int attrVals[2];
    int pbAttrList[] = { 0 };
    OGLSDOps *oglsdo = (OGLSDOps *)jlong_to_ptr(pData);
    WGLGraphicsConfigInfo *wglInfo =
        (WGLGraphicsConfigInfo *)jlong_to_ptr(pConfigInfo);
    WGLSDOps *wglsdo;
    HWND hwnd;
    HDC hdc, pbufferDC;
    HPBUFFERARB pbuffer;
    int maxWidth, maxHeight;
    int actualWidth, actualHeight;

    J2dTraceLn3(J2D_TRACE_INFO,
                "WGLSurfaceData_initPbuffer: w=%d h=%d opq=%d",
                width, height, isOpaque);

    if (oglsdo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: ops are null");
        return JNI_FALSE;
    }

    wglsdo = (WGLSDOps *)oglsdo->privOps;
    if (wglsdo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: wgl ops are null");
        return JNI_FALSE;
    }

    if (wglInfo == NULL) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: wgl config info is null");
        return JNI_FALSE;
    }

    // create a scratch window
    hwnd = WGLGC_CreateScratchWindow(wglInfo->screen);
    if (hwnd == 0) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: could not create scratch window");
        return JNI_FALSE;
    }

    // get the HDC for the scratch window
    hdc = GetDC(hwnd);
    if (hdc == 0) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: could not get dc for scratch window");
        DestroyWindow(hwnd);
        return JNI_FALSE;
    }

    // get the maximum allowable pbuffer dimensions
    j2d_wglGetPixelFormatAttribivARB(hdc, wglInfo->pixfmt, 0, 2,
                                     attrKeys, attrVals);
    maxWidth  = attrVals[0];
    maxHeight = attrVals[1];

    J2dTraceLn4(J2D_TRACE_VERBOSE,
                "  desired pbuffer dimensions: w=%d h=%d maxw=%d maxh=%d",
                width, height, maxWidth, maxHeight);

    // if either dimension is 0 or larger than the maximum, we cannot
    // allocate a pbuffer with the requested dimensions
    if (width  == 0 || width  > maxWidth ||
        height == 0 || height > maxHeight)
    {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: invalid dimensions");
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return JNI_FALSE;
    }

    pbuffer = j2d_wglCreatePbufferARB(hdc, wglInfo->pixfmt,
                                      width, height, pbAttrList);

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    if (pbuffer == 0) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: could not create wgl pbuffer");
        return JNI_FALSE;
    }

    // note that we get the DC for the pbuffer at creation time, and then
    // release the DC when the pbuffer is disposed; the WGL_ARB_pbuffer
    // spec is vague about such things, but from past experience we know
    // this approach to be more robust than, for example, doing a
    // Get/ReleasePbufferDC() everytime we make a context current
    pbufferDC = j2d_wglGetPbufferDCARB(pbuffer);
    if (pbufferDC == 0) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: could not get dc for pbuffer");
        j2d_wglDestroyPbufferARB(pbuffer);
        return JNI_FALSE;
    }

    // make sure the actual dimensions match those that we requested
    j2d_wglQueryPbufferARB(pbuffer, WGL_PBUFFER_WIDTH_ARB, &actualWidth);
    j2d_wglQueryPbufferARB(pbuffer, WGL_PBUFFER_HEIGHT_ARB, &actualHeight);

    if (width != actualWidth || height != actualHeight) {
        J2dRlsTraceLn2(J2D_TRACE_ERROR,
            "WGLSurfaceData_initPbuffer: actual (w=%d h=%d) != requested",
                       actualWidth, actualHeight);
        j2d_wglReleasePbufferDCARB(pbuffer, pbufferDC);
        j2d_wglDestroyPbufferARB(pbuffer);
        return JNI_FALSE;
    }

    oglsdo->drawableType = OGLSD_PBUFFER;
    oglsdo->isOpaque = isOpaque;
    oglsdo->width = width;
    oglsdo->height = height;
    wglsdo->drawable.pbuffer = pbuffer;
    wglsdo->pbufferDC = pbufferDC;

    return JNI_TRUE;
}

void
OGLSD_SwapBuffers(JNIEnv *env, jlong pPeerData)
{
    HWND window;
    HDC hdc;

    J2dTraceLn(J2D_TRACE_INFO, "OGLSD_SwapBuffers");

    window = AwtComponent_GetHWnd(env, pPeerData);
    if (!IsWindow(window)) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_SwapBuffers: disposed component");
        return;
    }

    hdc = GetDC(window);
    if (hdc == 0) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_SwapBuffers: invalid hdc");
        return;
    }

    if (!SwapBuffers(hdc)) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_SwapBuffers: error in SwapBuffers");
    }

    if (!ReleaseDC(window, hdc)) {
        J2dRlsTraceLn(J2D_TRACE_ERROR,
                      "OGLSD_SwapBuffers: error while releasing dc");
    }
}
