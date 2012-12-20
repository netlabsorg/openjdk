/*
 * Copyright (c) 1996, 2012, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
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
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <windowsx.h>

#include "awt_Component.h"
#include "awt_Container.h"
#include "awt_Frame.h"
#include "awt_Insets.h"
#include "awt_Panel.h"
#include "awt_Toolkit.h"
#include "awt_Window.h"
#include "awt_dlls.h"
#include "ddrawUtils.h"
#include "awt_Win32GraphicsDevice.h"
#include "awt_BitmapUtil.h"
#include "awt_IconCursor.h"

#include "java_awt_Insets.h"
#include <java_awt_Container.h>
#include <java_awt_event_ComponentEvent.h>
#include "sun_awt_windows_WCanvasPeer.h"

#if !defined(__int3264)
typedef __int32 LONG_PTR;
#endif // __int3264

// Used for Swing's Menu/Tooltip animation Support
const int UNSPECIFIED = 0;
const int TOOLTIP = 1;
const int MENU = 2;
const int SUBMENU = 3;
const int POPUPMENU = 4;
const int COMBOBOX_POPUP = 5;
const int TYPES_COUNT = 6;
jint windowTYPES[TYPES_COUNT];


/* IMPORTANT! Read the README.JNI file for notes on JNI converted AWT code.
 */

/***********************************************************************/
// struct for _SetAlwaysOnTop() method
struct SetAlwaysOnTopStruct {
    jobject window;
    jboolean value;
};
// struct for _SetTitle() method
struct SetTitleStruct {
    jobject window;
    jstring title;
};
// struct for _SetResizable() method
struct SetResizableStruct {
    jobject window;
    jboolean resizable;
};
// struct for _UpdateInsets() method
struct UpdateInsetsStruct {
    jobject window;
    jobject insets;
};
// struct for _ReshapeFrame() method
struct ReshapeFrameStruct {
    jobject frame;
    jint x, y;
    jint w, h;
};

// struct for _SetIconImagesData
struct SetIconImagesDataStruct {
    jobject window;
    jintArray iconRaster;
    jint w, h;
    jintArray smallIconRaster;
    jint smw, smh;
};

// struct for _SetMinSize() method
// and other methods setting sizes
struct SizeStruct {
    jobject window;
    jint w, h;
};
// struct for _SetFocusableWindow() method
struct SetFocusableWindowStruct {
    jobject window;
    jboolean isFocusableWindow;
};
// struct for _ModalDisable() method
struct ModalDisableStruct {
    jobject window;
    jlong blockerHWnd;
};
/************************************************************************
 * AwtWindow fields
 */

jfieldID AwtWindow::warningStringID;
jfieldID AwtWindow::locationByPlatformID;
jfieldID AwtWindow::autoRequestFocusID;

jfieldID AwtWindow::sysXID;
jfieldID AwtWindow::sysYID;
jfieldID AwtWindow::sysWID;
jfieldID AwtWindow::sysHID;

int AwtWindow::ms_instanceCounter = 0;
HHOOK AwtWindow::ms_hCBTFilter;
AwtWindow * AwtWindow::m_grabbedWindow = NULL;
HWND AwtWindow::sm_retainingHierarchyZOrderInShow = NULL;
BOOL AwtWindow::sm_resizing = FALSE;

/************************************************************************
 * AwtWindow class methods
 */

AwtWindow::AwtWindow() {
    m_sizePt.x = m_sizePt.y = 0;
    m_owningFrameDialog = NULL;
    m_isResizable = FALSE;//Default value is replaced after construction
    m_minSize.x = m_minSize.y = 0;
    m_hIcon = NULL;
    m_hIconSm = NULL;
    m_iconInherited = FALSE;
    VERIFY(::SetRectEmpty(&m_insets));
    VERIFY(::SetRectEmpty(&m_old_insets));
    VERIFY(::SetRectEmpty(&m_warningRect));

    // what's the best initial value?
    m_screenNum = -1;
    ms_instanceCounter++;
    m_grabbed = FALSE;
    m_isFocusableWindow = TRUE;
    m_isRetainingHierarchyZOrder = FALSE;
    m_filterFocusAndActivation = FALSE;

    if (AwtWindow::ms_instanceCounter == 1) {
        AwtWindow::ms_hCBTFilter =
            ::SetWindowsHookEx(WH_CBT, (HOOKPROC)AwtWindow::CBTFilter,
                               0, AwtToolkit::MainThread());
    }
}

AwtWindow::~AwtWindow()
{
}

void AwtWindow::Dispose()
{
    // Fix 4745575 GDI Resource Leak
    // MSDN
    // Before a window is destroyed (that is, before it returns from processing
    // the WM_NCDESTROY message), an application must remove all entries it has
    // added to the property list. The application must use the RemoveProp function
    // to remove the entries.

    if (--AwtWindow::ms_instanceCounter == 0) {
        ::UnhookWindowsHookEx(AwtWindow::ms_hCBTFilter);
    }

    ::RemoveProp(GetHWnd(), ModalBlockerProp);

    if (m_grabbedWindow == this) {
        Ungrab();
    }
    if ((m_hIcon != NULL) && !m_iconInherited) {
        ::DestroyIcon(m_hIcon);
    }
    if ((m_hIconSm != NULL) && !m_iconInherited) {
        ::DestroyIcon(m_hIconSm);
    }

    AwtCanvas::Dispose();
}

void
AwtWindow::Grab() {
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (m_grabbedWindow != NULL) {
        m_grabbedWindow->Ungrab();
    }
    m_grabbed = TRUE;
    m_grabbedWindow = this;
    if (sm_focusedWindow == NULL) {
        // we shouldn't perform grab in this case (see 4841881)
        Ungrab();
    } else if (GetHWnd() != sm_focusedWindow) {
        _ToFront(env->NewGlobalRef(GetPeer(env)));
        // Global ref was deleted in _ToFront
    }
}

void
AwtWindow::Ungrab(BOOL doPost) {
    if (m_grabbed && m_grabbedWindow == this) {
        if (doPost) {
            PostUngrabEvent();
        }
        m_grabbedWindow = NULL;
        m_grabbed = FALSE;
    }
}

void
AwtWindow::Ungrab() {
    Ungrab(TRUE);
}

void AwtWindow::_Grab(void * param) {
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    jobject self = (jobject)param;

    if (env->EnsureLocalCapacity(1) < 0)
    {
        env->DeleteGlobalRef(self);
        return;
    }

    AwtWindow *p = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    p = (AwtWindow *)pData;
    p->Grab();

  ret:
    env->DeleteGlobalRef(self);
}

void AwtWindow::_Ungrab(void * param) {
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    jobject self = (jobject)param;

    if (env->EnsureLocalCapacity(1) < 0)
    {
        env->DeleteGlobalRef(self);
        return;
    }

    AwtWindow *p = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    p = (AwtWindow *)pData;
    p->Ungrab(FALSE);

  ret:
    env->DeleteGlobalRef(self);
}

MsgRouting AwtWindow::WmNcMouseDown(WPARAM hitTest, int x, int y, int button) {
    if (m_grabbedWindow != NULL && !m_grabbedWindow->IsOneOfOwnersOf(this)) {
        m_grabbedWindow->Ungrab();
    }
    return AwtCanvas::WmNcMouseDown(hitTest, x, y, button);
}

MsgRouting AwtWindow::WmWindowPosChanging(LPARAM windowPos) {
    /*
     * See 6178004.
     * Some windows shouldn't trigger a change in z-order of
     * any window from the hierarchy.
     */
    if (IsRetainingHierarchyZOrder()) {
        if (((WINDOWPOS *)windowPos)->flags & SWP_SHOWWINDOW) {
            sm_retainingHierarchyZOrderInShow = GetHWnd();
        }
    } else if (sm_retainingHierarchyZOrderInShow != NULL) {
        HWND ancestor = ::GetAncestor(sm_retainingHierarchyZOrderInShow, GA_ROOTOWNER);
        HWND windowAncestor = ::GetAncestor(GetHWnd(), GA_ROOTOWNER);

        if (windowAncestor == ancestor) {
            ((WINDOWPOS *)windowPos)->flags |= SWP_NOZORDER;
        }
    }
    return mrDoDefault;
}

MsgRouting AwtWindow::WmWindowPosChanged(LPARAM windowPos) {
    if (IsRetainingHierarchyZOrder() && ((WINDOWPOS *)windowPos)->flags & SWP_SHOWWINDOW) {
        // By this time all the windows from the hierarchy are already notified about z-order change.
        // Thus we may and we should reset the trigger in order not to affect other changes.
        sm_retainingHierarchyZOrderInShow = NULL;
    }
    return mrDoDefault;
}

LPCTSTR AwtWindow::GetClassName() {
  return TEXT("SunAwtWindow");
}

void AwtWindow::FillClassInfo(WNDCLASSEX *lpwc)
{
    AwtComponent::FillClassInfo(lpwc);
    /*
     * This line causes bug #4189244 (Swing Popup menu is not being refreshed (cleared) under a Dialog)
     * so it's comment out (son@sparc.spb.su)
     *
     * lpwc->style     |= CS_SAVEBITS; // improve pull-down menu performance
     */
    lpwc->cbWndExtra = DLGWINDOWEXTRA;
}

LRESULT CALLBACK AwtWindow::CBTFilter(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HCBT_ACTIVATE || nCode == HCBT_SETFOCUS) {
        AwtComponent *comp = AwtComponent::GetComponent((HWND)wParam);

        if (comp != NULL && comp->IsTopLevel() && !((AwtWindow*)comp)->IsFocusableWindow()) {
            return 1; // Don't change focus/activation.
        }
    }
    return ::CallNextHookEx(AwtWindow::ms_hCBTFilter, nCode, wParam, lParam);
}

/* Create a new AwtWindow object and window.   */
AwtWindow* AwtWindow::Create(jobject self, jobject parent)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    jobject target = NULL;
    AwtWindow* window = NULL;

    try {
        if (env->EnsureLocalCapacity(1) < 0) {
            return NULL;
        }

        AwtWindow* awtParent = NULL;

        PDATA pData;
        if (parent != NULL) {
            JNI_CHECK_PEER_GOTO(parent, done);
            awtParent = (AwtWindow *)pData;
        }

        target = env->GetObjectField(self, AwtObject::targetID);
        JNI_CHECK_NULL_GOTO(target, "null target", done);

        window = new AwtWindow();

        {
            if (JNU_IsInstanceOfByName(env, target, "javax/swing/Popup$HeavyWeightWindow") > 0) {
                window->m_isRetainingHierarchyZOrder = TRUE;
            }
            DWORD style = WS_CLIPCHILDREN | WS_POPUP;
            DWORD exStyle = 0;
            if (GetRTL()) {
                exStyle |= WS_EX_RIGHT | WS_EX_LEFTSCROLLBAR;
                if (GetRTLReadingOrder())
                    exStyle |= WS_EX_RTLREADING;
            }
            if (awtParent != NULL) {
                window->InitOwner(awtParent);
            } else {
                // specify WS_EX_TOOLWINDOW to remove parentless windows from taskbar
                exStyle |= WS_EX_TOOLWINDOW;
            }
            window->CreateHWnd(env, L"",
                               style, exStyle,
                               0, 0, 0, 0,
                               (awtParent != NULL) ? awtParent->GetHWnd() : NULL,
                               NULL,
                               ::GetSysColor(COLOR_WINDOWTEXT),
                               ::GetSysColor(COLOR_WINDOW),
                               self);

            jint x = env->GetIntField(target, AwtComponent::xID);
            jint y = env->GetIntField(target, AwtComponent::yID);
            jint width = env->GetIntField(target, AwtComponent::widthID);
            jint height = env->GetIntField(target, AwtComponent::heightID);

            /*
             * Initialize icon as inherited from parent if it exists
             */
            if (parent != NULL) {
                window->m_hIcon = awtParent->GetHIcon();
                window->m_hIconSm = awtParent->GetHIconSm();
                window->m_iconInherited = TRUE;
            }
            window->DoUpdateIcon();


            /*
             * Reshape here instead of during create, so that a WM_NCCALCSIZE
             * is sent.
             */
            window->Reshape(x, y, width, height);
        }
    } catch (...) {
        env->DeleteLocalRef(target);
        throw;
    }

done:
    env->DeleteLocalRef(target);
    return window;
}

BOOL AwtWindow::IsOneOfOwnersOf(AwtWindow * wnd) {
    while (wnd != NULL) {
        if (wnd == this || wnd->GetOwningFrameOrDialog() == this) return TRUE;
        wnd = (AwtWindow*)GetComponent(::GetWindow(wnd->GetHWnd(), GW_OWNER));
    }
    return FALSE;
}

void AwtWindow::InitOwner(AwtWindow *owner)
{
    DASSERT(owner != NULL);
    while (owner != NULL && owner->IsSimpleWindow()) {

        HWND ownerOwnerHWND = ::GetWindow(owner->GetHWnd(), GW_OWNER);
        if (ownerOwnerHWND == NULL) {
            owner = NULL;
            break;
        }
        owner = (AwtWindow *)AwtComponent::GetComponent(ownerOwnerHWND);
    }
    m_owningFrameDialog = (AwtFrame *)owner;
}

void AwtWindow::moveToDefaultLocation() {
    HWND boggy = ::CreateWindow(GetClassName(), L"BOGGY", WS_OVERLAPPED, CW_USEDEFAULT, 0 ,0, 0,
        NULL, NULL, NULL, NULL);
    RECT defLoc;

    // Fixed 6477497: Windows drawn off-screen on Win98, even when java.awt.Window.locationByPlatform is set
    //    Win9x does not position a window until the window is shown.
    //    The behavior is slightly opposite to the WinNT (and up), where
    //    Windows will position the window upon creation of the window.
    //    That's why we have to manually set the left & top values of
    //    the defLoc to 0 if the GetWindowRect function returns FALSE.
    BOOL result = ::GetWindowRect(boggy, &defLoc);
    if (!result) {
        defLoc.left = defLoc.top = 0;
    }
    VERIFY(::DestroyWindow(boggy));
    VERIFY(::SetWindowPos(GetHWnd(), NULL, defLoc.left, defLoc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER));
}

void AwtWindow::Show()
{
    m_visible = true;
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    BOOL  done = false;
    HWND hWnd = GetHWnd();

    if (env->EnsureLocalCapacity(2) < 0) {
        return;
    }
    jobject target = GetTarget(env);
    INT nCmdShow;

    AwtFrame* owningFrame = GetOwningFrameOrDialog();
    if (IsFocusableWindow() && IsAutoRequestFocus() && owningFrame != NULL &&
        ::GetForegroundWindow() == owningFrame->GetHWnd())
    {
        nCmdShow = SW_SHOW;
    } else {
        nCmdShow = SW_SHOWNA;
    }

    BOOL locationByPlatform = env->GetBooleanField(GetTarget(env), AwtWindow::locationByPlatformID);

    if (locationByPlatform) {
         moveToDefaultLocation();
    }

    // The following block exists to support Menu/Tooltip animation for
    // Swing programs in a way which avoids introducing any new public api into
    // AWT or Swing.
    // This code should eventually be replaced by a better longterm solution
    // which might involve tagging java.awt.Window instances with a semantic
    // property so platforms can animate/decorate/etc accordingly.
    //
    if ((IS_WIN98 || IS_WIN2000) &&
        JNU_IsInstanceOfByName(env, target, "com/sun/java/swing/plaf/windows/WindowsPopupWindow") > 0)
    {
        // need this global ref to make the class unloadable (see 6500204)
        static jclass windowsPopupWindowCls;
        static jfieldID windowTypeFID = NULL;
        jint windowType = 0;
        BOOL  animateflag = FALSE;
        BOOL  fadeflag = FALSE;
        DWORD animateStyle = 0;

        if (windowTypeFID == NULL) {
            // Initialize Window type constants ONCE...

            jfieldID windowTYPESFID[TYPES_COUNT];
            jclass cls = env->GetObjectClass(target);
            windowTypeFID = env->GetFieldID(cls, "windowType", "I");

            windowTYPESFID[UNSPECIFIED] = env->GetStaticFieldID(cls, "UNDEFINED_WINDOW_TYPE", "I");
            windowTYPESFID[TOOLTIP] = env->GetStaticFieldID(cls, "TOOLTIP_WINDOW_TYPE", "I");
            windowTYPESFID[MENU] = env->GetStaticFieldID(cls, "MENU_WINDOW_TYPE", "I");
            windowTYPESFID[SUBMENU] = env->GetStaticFieldID(cls, "SUBMENU_WINDOW_TYPE", "I");
            windowTYPESFID[POPUPMENU] = env->GetStaticFieldID(cls, "POPUPMENU_WINDOW_TYPE", "I");
            windowTYPESFID[COMBOBOX_POPUP] = env->GetStaticFieldID(cls, "COMBOBOX_POPUP_WINDOW_TYPE", "I");

            for (int i=0; i < 6; i++) {
                windowTYPES[i] = env->GetStaticIntField(cls, windowTYPESFID[i]);
            }
            windowsPopupWindowCls = (jclass) env->NewGlobalRef(cls);
            env->DeleteLocalRef(cls);
        }
        windowType = env->GetIntField(target, windowTypeFID);

        if (windowType == windowTYPES[TOOLTIP]) {
            if (IS_WIN2000) {
                SystemParametersInfo(SPI_GETTOOLTIPANIMATION, 0, &animateflag, 0);
                SystemParametersInfo(SPI_GETTOOLTIPFADE, 0, &fadeflag, 0);
            } else {
                // use same setting as menus
                SystemParametersInfo(SPI_GETMENUANIMATION, 0, &animateflag, 0);
            }
            if (animateflag) {
              // AW_BLEND currently produces runtime parameter error
              // animateStyle = fadeflag? AW_BLEND : AW_SLIDE | AW_VER_POSITIVE;
                 animateStyle = fadeflag? 0 : AW_SLIDE | AW_VER_POSITIVE;
            }
        } else if (windowType == windowTYPES[MENU] || windowType == windowTYPES[SUBMENU] ||
                   windowType == windowTYPES[POPUPMENU]) {
            SystemParametersInfo(SPI_GETMENUANIMATION, 0, &animateflag, 0);
            if (animateflag) {

                if (IS_WIN2000) {
                    SystemParametersInfo(SPI_GETMENUFADE, 0, &fadeflag, 0);
                    if (fadeflag) {
                      // AW_BLEND currently produces runtime parameter error
                      //animateStyle = AW_BLEND;
                    }
                }
                if (animateStyle == 0 && !fadeflag) {
                    animateStyle = AW_SLIDE;
                    if (windowType == windowTYPES[MENU]) {
                      animateStyle |= AW_VER_POSITIVE;
                    } else if (windowType == windowTYPES[SUBMENU]) {
                      animateStyle |= AW_HOR_POSITIVE;
                    } else { /* POPUPMENU */
                      animateStyle |= (AW_VER_POSITIVE | AW_HOR_POSITIVE);
                    }
                }
            }
        } else if (windowType == windowTYPES[COMBOBOX_POPUP]) {
            SystemParametersInfo(SPI_GETCOMBOBOXANIMATION, 0, &animateflag, 0);
            if (animateflag) {
                 animateStyle = AW_SLIDE | AW_VER_POSITIVE;
            }
        }

        if (animateStyle != 0) {
            load_user_procs();

            if (fn_animate_window != NULL) {
                BOOL result = (*fn_animate_window)(hWnd, (DWORD)200, animateStyle);
                if (result == 0) {
                    LPTSTR      msgBuffer = NULL;
                    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&msgBuffer, // it's an output parameter when allocate buffer is used
                      0,
                      NULL);

                    if (msgBuffer == NULL) {
                        msgBuffer = TEXT("<Could not get GetLastError() message text>");
                    }
                    _ftprintf(stderr,TEXT("AwtWindow::Show: AnimateWindow: "));
                    _ftprintf(stderr,msgBuffer);
                    LocalFree(msgBuffer);
                } else {
                  // WM_PAINT is not automatically sent when invoking AnimateWindow,
                  // so force an expose event
                    RECT rect;
                    ::GetWindowRect(hWnd,&rect);
                    ::ScreenToClient(hWnd, (LPPOINT)&rect);
                    ::InvalidateRect(hWnd,&rect,TRUE);
                    ::UpdateWindow(hWnd);
                    done = TRUE;
                }
            }
        }
    }
    if (!done) {
        ::ShowWindow(GetHWnd(), nCmdShow);
    }
    env->DeleteLocalRef(target);
}

/*
 * Get and return the insets for this window (container, really).
 * Calculate & cache them while we're at it, for use by AwtGraphics
 */
BOOL AwtWindow::UpdateInsets(jobject insets)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    DASSERT(GetPeer(env) != NULL);
    if (env->EnsureLocalCapacity(2) < 0) {
        return FALSE;
    }

    // fix 4167248 : don't update insets when frame is iconified
    // to avoid bizarre window/client rectangles
    if (::IsIconic(GetHWnd())) {
        return FALSE;
    }

    /*
     * Code to calculate insets. Stores results in frame's data
     * members, and in the peer's Inset object.
     */
    RECT outside;
    RECT inside;
    int extraBottomInsets = 0;

    ::GetClientRect(GetHWnd(), &inside);
    ::GetWindowRect(GetHWnd(), &outside);

    jobject target = GetTarget(env);
    jstring warningString =
      (jstring)(env)->GetObjectField(target, AwtWindow::warningStringID);
    if (warningString != NULL) {
        ::CopyRect(&inside, &outside);
        DefWindowProc(WM_NCCALCSIZE, FALSE, (LPARAM)&inside);
        /*
         * Fix for BugTraq ID 4304024.
         * Calculate client rectangle in client coordinates.
         */
        VERIFY(::OffsetRect(&inside, -inside.left, -inside.top));
        extraBottomInsets = ::GetSystemMetrics(SM_CYCAPTION) +
            ((GetStyle() & WS_THICKFRAME) ? 2 : -2);
    }
    env->DeleteLocalRef(target);
    env->DeleteLocalRef(warningString);

    /* Update our inset member */
    if (outside.right - outside.left > 0 && outside.bottom - outside.top > 0) {
        ::MapWindowPoints(GetHWnd(), 0, (LPPOINT)&inside, 2);
        m_insets.top = inside.top - outside.top;
        m_insets.bottom = outside.bottom - inside.bottom + extraBottomInsets;
        m_insets.left = inside.left - outside.left;
        m_insets.right = outside.right - inside.right;
    } else {
        m_insets.top = -1;
    }
    if (m_insets.left < 0 || m_insets.top < 0 ||
        m_insets.right < 0 || m_insets.bottom < 0)
    {
        /* This window hasn't been sized yet -- use system metrics. */
        jobject target = GetTarget(env);
        if (IsUndecorated() == FALSE) {
            /* Get outer frame sizes. */
            LONG style = GetStyle();
            if (style & WS_THICKFRAME) {
                m_insets.left = m_insets.right =
                    ::GetSystemMetrics(SM_CXSIZEFRAME);
                m_insets.top = m_insets.bottom =
                    ::GetSystemMetrics(SM_CYSIZEFRAME);
            } else {
                m_insets.left = m_insets.right =
                    ::GetSystemMetrics(SM_CXDLGFRAME);
                m_insets.top = m_insets.bottom =
                    ::GetSystemMetrics(SM_CYDLGFRAME);
            }


            /* Add in title. */
            m_insets.top += ::GetSystemMetrics(SM_CYCAPTION);
        }
        else {
            /* fix for 4418125: Undecorated frames are off by one */
            /* undo the -1 set above */
            /* Additional fix for 5059656 */
                /* Also, 5089312: Window insets should be 0. */
            ::memset(&m_insets, 0, sizeof(m_insets));
        }

        /* Add in menuBar, if any. */
        if (JNU_IsInstanceOfByName(env, target, "java/awt/Frame") > 0 &&
            ((AwtFrame*)this)->GetMenuBar()) {
            m_insets.top += ::GetSystemMetrics(SM_CYMENU);
        }
        m_insets.bottom += extraBottomInsets;
        env->DeleteLocalRef(target);
    }

    BOOL insetsChanged = FALSE;

    jobject peer = GetPeer(env);
    /* Get insets into our peer directly */
    jobject peerInsets = (env)->GetObjectField(peer, AwtPanel::insets_ID);
    DASSERT(!safe_ExceptionOccurred(env));
    if (peerInsets != NULL) { // may have been called during creation
        (env)->SetIntField(peerInsets, AwtInsets::topID, m_insets.top);
        (env)->SetIntField(peerInsets, AwtInsets::bottomID,
                           m_insets.bottom);
        (env)->SetIntField(peerInsets, AwtInsets::leftID, m_insets.left);
        (env)->SetIntField(peerInsets, AwtInsets::rightID, m_insets.right);
    }
    /* Get insets into the Inset object (if any) that was passed */
    if (insets != NULL) {
        (env)->SetIntField(insets, AwtInsets::topID, m_insets.top);
        (env)->SetIntField(insets, AwtInsets::bottomID, m_insets.bottom);
        (env)->SetIntField(insets, AwtInsets::leftID, m_insets.left);
        (env)->SetIntField(insets, AwtInsets::rightID, m_insets.right);
    }
    env->DeleteLocalRef(peerInsets);

    insetsChanged = !::EqualRect( &m_old_insets, &m_insets );
    ::CopyRect( &m_old_insets, &m_insets );

    if (insetsChanged && DDCanReplaceSurfaces(GetHWnd())) {
        // Since insets are changed we need to update the surfaceData object
        // to reflect that change
        env->CallVoidMethod(peer, AwtComponent::replaceSurfaceDataLaterMID);
    }

    return insetsChanged;
}

/**
 * Sometimes we need the hWnd that actually owns this Window's hWnd (if
 * there is an owner).
 */
HWND AwtWindow::GetTopLevelHWnd()
{
    return m_owningFrameDialog ? m_owningFrameDialog->GetHWnd() :
                                 GetHWnd();
}

/*
 * Although this function sends ComponentEvents, it needs to be defined
 * here because only top-level windows need to have move and resize
 * events fired from native code.  All contained windows have these events
 * fired from common Java code.
 */
void AwtWindow::SendComponentEvent(jint eventId)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    static jclass classEvent = NULL;
    if (classEvent == NULL) {
        if (env->PushLocalFrame(1) < 0)
            return;
        classEvent = env->FindClass("java/awt/event/ComponentEvent");
        if (classEvent != NULL) {
            classEvent = (jclass)env->NewGlobalRef(classEvent);
        }
        env->PopLocalFrame(0);
    }
    static jmethodID eventInitMID = NULL;
    if (eventInitMID == NULL) {
        eventInitMID = env->GetMethodID(classEvent, "<init>",
                                        "(Ljava/awt/Component;I)V");
        if (eventInitMID == NULL) {
            return;
        }
    }
    if (env->EnsureLocalCapacity(2) < 0) {
        return;
    }
    jobject target = GetTarget(env);
    jobject event = env->NewObject(classEvent, eventInitMID,
                                   target, eventId);
    DASSERT(!safe_ExceptionOccurred(env));
    DASSERT(event != NULL);
    SendEvent(event);

    env->DeleteLocalRef(target);
    env->DeleteLocalRef(event);
}

void AwtWindow::SendWindowEvent(jint id, HWND opposite,
                                jint oldState, jint newState)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    static jclass wClassEvent;
    if (wClassEvent == NULL) {
        if (env->PushLocalFrame(1) < 0)
            return;
        wClassEvent = env->FindClass("java/awt/event/WindowEvent");
        if (wClassEvent != NULL) {
            wClassEvent = (jclass)env->NewGlobalRef(wClassEvent);
        }
        env->PopLocalFrame(0);
        if (wClassEvent == NULL) {
            return;
        }
    }

    static jmethodID wEventInitMID;
    if (wEventInitMID == NULL) {
        wEventInitMID =
            env->GetMethodID(wClassEvent, "<init>",
                             "(Ljava/awt/Window;ILjava/awt/Window;II)V");
        DASSERT(wEventInitMID);
        if (wEventInitMID == NULL) {
            return;
        }
    }

    static jclass sequencedEventCls;
    if (sequencedEventCls == NULL) {
        jclass sequencedEventClsLocal
            = env->FindClass("java/awt/SequencedEvent");
        DASSERT(sequencedEventClsLocal);
        if (sequencedEventClsLocal == NULL) {
            /* exception already thrown */
            return;
        }
        sequencedEventCls =
            (jclass)env->NewGlobalRef(sequencedEventClsLocal);
        env->DeleteLocalRef(sequencedEventClsLocal);
    }

    static jmethodID sequencedEventConst;
    if (sequencedEventConst == NULL) {
        sequencedEventConst =
            env->GetMethodID(sequencedEventCls, "<init>",
                             "(Ljava/awt/AWTEvent;)V");
    }

    if (env->EnsureLocalCapacity(3) < 0) {
        return;
    }

    jobject target = GetTarget(env);
    jobject jOpposite = NULL;
    if (opposite != NULL) {
        AwtComponent *awtOpposite = AwtComponent::GetComponent(opposite);
        if (awtOpposite != NULL) {
            jOpposite = awtOpposite->GetTarget(env);
        }
    }
    jobject event = env->NewObject(wClassEvent, wEventInitMID, target, id,
                                   jOpposite, oldState, newState);
    DASSERT(!safe_ExceptionOccurred(env));
    DASSERT(event != NULL);
    if (jOpposite != NULL) {
        env->DeleteLocalRef(jOpposite); jOpposite = NULL;
    }
    env->DeleteLocalRef(target); target = NULL;

    if (id == java_awt_event_WindowEvent_WINDOW_GAINED_FOCUS ||
        id == java_awt_event_WindowEvent_WINDOW_LOST_FOCUS)
    {
        jobject sequencedEvent = env->NewObject(sequencedEventCls,
                                                sequencedEventConst,
                                                event);
        DASSERT(!safe_ExceptionOccurred(env));
        DASSERT(sequencedEvent != NULL);
        env->DeleteLocalRef(event);
        event = sequencedEvent;
    }

    SendEvent(event);

    env->DeleteLocalRef(event);
}

MsgRouting AwtWindow::WmActivate(UINT nState, BOOL fMinimized, HWND opposite)
{
    jint type;

    if (nState != WA_INACTIVE) {
        ::SetFocus((sm_focusOwner == NULL ||
                    AwtComponent::GetTopLevelParentForWindow(sm_focusOwner) !=
                    GetHWnd()) ? NULL : sm_focusOwner);
        type = java_awt_event_WindowEvent_WINDOW_GAINED_FOCUS;
        AwtToolkit::GetInstance().
            InvokeFunctionLater(BounceActivation, this);
        sm_focusedWindow = GetHWnd();
    } else {
        if (m_grabbedWindow != NULL && !m_grabbedWindow->IsOneOfOwnersOf(this)) {
            m_grabbedWindow->Ungrab();
        }
        type = java_awt_event_WindowEvent_WINDOW_LOST_FOCUS;
        sm_focusedWindow = NULL;
    }

    SendWindowEvent(type, opposite);
    return mrConsume;
}

void AwtWindow::BounceActivation(void *self) {
    AwtWindow *wSelf = (AwtWindow *)self;

    if (::GetActiveWindow() == wSelf->GetHWnd()) {
        AwtFrame *owner = wSelf->GetOwningFrameOrDialog();

        if (owner != NULL) {
            sm_suppressFocusAndActivation = TRUE;
            ::SetActiveWindow(owner->GetHWnd());
            ::SetFocus(owner->GetProxyFocusOwner());
            sm_suppressFocusAndActivation = FALSE;
        }
    }
}

MsgRouting AwtWindow::WmDDEnterFullScreen(HMONITOR monitor) {
    /**
     * DirectDraw expects to receive a top-level window. This object may
     * be an AwtWindow instance, which has an owning AwtFrame window, or
     * an AwtFrame object which does not have an owner.
     * What we want is the top-level Frame hWnd, whether we were handed a
     * top-level AwtFrame, or some owned AwtWindow.  We get this by calling
     * GetTopLevelHWnd(), which returns the hwnd of a top-level AwtFrame
     * object (if this window has an owner) which we then pass
     * into DirectDraw.
     */
    HWND hWnd = GetTopLevelHWnd();
    if (!::IsWindowVisible(hWnd)) {
        // Sometimes there are problems going into fullscreen on an owner frame
        // that is not yet visible; make sure the FS window is visible first
        ::ShowWindow(hWnd, SW_SHOWNA);
    }
    /*
     * Fix for 6225472.
     * Non-focusable window should be set alwaysOnTop to overlap the taskbar.
     */
    AwtWindow* window = (AwtWindow *)AwtComponent::GetComponent(GetHWnd());
    if (window != NULL && !window->IsFocusableWindow()) {
        JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
        Java_sun_awt_windows_WWindowPeer_setAlwaysOnTopNative(env, GetPeer(env), (jboolean)TRUE);
    }

    DDEnterFullScreen(monitor, GetHWnd(), hWnd);
    return mrDoDefault;
}

MsgRouting AwtWindow::WmDDExitFullScreen(HMONITOR monitor) {
    HWND hWnd = GetTopLevelHWnd();
    DDExitFullScreen(monitor, hWnd);

    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    jboolean alwaysOnTop = JNU_CallMethodByName(env, NULL, GetTarget(env),
                                                "isAlwaysOnTop", "()Z").z;

    // We should restore alwaysOnTop state as it's anyway dropped here.
    Java_sun_awt_windows_WWindowPeer_setAlwaysOnTopNative(env, GetPeer(env), alwaysOnTop);
    return mrDoDefault;
}

MsgRouting AwtWindow::WmCreate()
{
    return mrDoDefault;
}

MsgRouting AwtWindow::WmClose()
{
    SendWindowEvent(java_awt_event_WindowEvent_WINDOW_CLOSING);

    /* Rely on above notification to handle quitting as needed */
    return mrConsume;
}

MsgRouting AwtWindow::WmDestroy()
{
    SendWindowEvent(java_awt_event_WindowEvent_WINDOW_CLOSED);
    return AwtComponent::WmDestroy();
}

MsgRouting AwtWindow::WmShowWindow(BOOL show, UINT status)
{
    /*
     * Original fix for 4810575. Modified for 6386592.
     * If an owned window (not frame/dialog) gets disposed we should synthesize
     * WM_ACTIVATE for its nearest owner. This is not performed by default because
     * the owner frame/dialog is natively active.
     */
    HWND hwndSelf = GetHWnd();
    HWND hwndParent = ::GetParent(hwndSelf);

    if (!show && IsSimpleWindow() && hwndSelf == sm_focusedWindow &&
        hwndParent != NULL && ::IsWindowVisible(hwndParent))
    {
        ::PostMessage(hwndParent, WM_ACTIVATE, (WPARAM)WA_ACTIVE, (LPARAM)hwndSelf);
    }

    //Fixed 4842599: REGRESSION: JPopupMenu not Hidden Properly After Iconified and Deiconified
    if (show && (status == SW_PARENTOPENING)) {
        if (!IsVisible()) {
            return mrConsume;
        }
    }
    return AwtCanvas::WmShowWindow(show, status);
}

/*
 * Override AwtComponent's move handling to first update the
 * java AWT target's position fields directly, since Windows
 * and below can be resized from outside of java (by user)
 */
MsgRouting AwtWindow::WmMove(int x, int y)
{
    if ( ::IsIconic(GetHWnd()) ) {
    // fixes 4065534 (robi.khan@eng)
    // if a window is iconified we don't want to update
    // it's target's position since minimized Win32 windows
    // move to -32000, -32000 for whatever reason
    // NOTE: See also AwtWindow::Reshape
        return mrDoDefault;
    }

    if (m_screenNum == -1) {
    // Set initial value
        m_screenNum = GetScreenImOn();
    }
    else {
        CheckIfOnNewScreen();
    }

    /* Update the java AWT target component's fields directly */
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (env->EnsureLocalCapacity(1) < 0) {
        return mrConsume;
    }
    jobject peer = GetPeer(env);
    jobject target = env->GetObjectField(peer, AwtObject::targetID);

    RECT rect;
    ::GetWindowRect(GetHWnd(), &rect);

    (env)->SetIntField(target, AwtComponent::xID, rect.left);
    (env)->SetIntField(target, AwtComponent::yID, rect.top);
    (env)->SetIntField(peer, AwtWindow::sysXID, rect.left);
    (env)->SetIntField(peer, AwtWindow::sysYID, rect.top);
    SendComponentEvent(java_awt_event_ComponentEvent_COMPONENT_MOVED);

    env->DeleteLocalRef(target);
    return AwtComponent::WmMove(x, y);
}

MsgRouting AwtWindow::WmGetMinMaxInfo(LPMINMAXINFO lpmmi)
{
    MsgRouting r = AwtCanvas::WmGetMinMaxInfo(lpmmi);
    if ((m_minSize.x == 0) && (m_minSize.y == 0)) {
        return r;
    }
    lpmmi->ptMinTrackSize.x = m_minSize.x;
    lpmmi->ptMinTrackSize.y = m_minSize.y;
    return mrConsume;
}

MsgRouting AwtWindow::WmSizing()
{
    if (!AwtToolkit::GetInstance().IsDynamicLayoutActive()) {
        return mrDoDefault;
    }

    DTRACE_PRINTLN("AwtWindow::WmSizing  fullWindowDragEnabled");

    SendComponentEvent(java_awt_event_ComponentEvent_COMPONENT_RESIZED);

    HWND thisHwnd = GetHWnd();
    if (thisHwnd == NULL) {
        return mrDoDefault;
    }

    // Call WComponentPeer::dynamicallyLayoutContainer()
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    jobject peer = GetPeer(env);
    JNU_CallMethodByName(env, NULL, peer, "dynamicallyLayoutContainer", "()V");
    DASSERT(!safe_ExceptionOccurred(env));

    return mrDoDefault;
}

/*
 * Override AwtComponent's size handling to first update the
 * java AWT target's dimension fields directly, since Windows
 * and below can be resized from outside of java (by user)
 */
MsgRouting AwtWindow::WmSize(UINT type, int w, int h)
{
    if (type == SIZE_MINIMIZED) {
        return mrDoDefault;
    }

    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (env->EnsureLocalCapacity(1) < 0)
        return mrDoDefault;
    jobject target = GetTarget(env);
    // fix 4167248 : ensure the insets are up-to-date before using
    BOOL insetsChanged = UpdateInsets(NULL);
    int newWidth = w + m_insets.left + m_insets.right;
    int newHeight = h + m_insets.top + m_insets.bottom;

    (env)->SetIntField(target, AwtComponent::widthID, newWidth);
    (env)->SetIntField(target, AwtComponent::heightID, newHeight);

    jobject peer = GetPeer(env);
    (env)->SetIntField(peer, AwtWindow::sysWID, newWidth);
    (env)->SetIntField(peer, AwtWindow::sysHID, newHeight);

    if (!AwtWindow::IsResizing()) {
        WindowResized();
    }

    env->DeleteLocalRef(target);
    return AwtComponent::WmSize(type, w, h);
}

MsgRouting AwtWindow::WmPaint(HDC)
{
    PaintUpdateRgn(&m_insets);
    return mrConsume;
}

MsgRouting AwtWindow::WmSettingChange(UINT wFlag, LPCTSTR pszSection)
{
    if (wFlag == SPI_SETNONCLIENTMETRICS) {
    // user changed window metrics in
    // Control Panel->Display->Appearance
    // which may cause window insets to change
        UpdateInsets(NULL);

    // [rray] fix for 4407329 - Changing Active Window Border width in display
    //  settings causes problems
        WindowResized();
        Invalidate(NULL);

        return mrConsume;
    }
    return mrDoDefault;
}

MsgRouting AwtWindow::WmNcCalcSize(BOOL fCalcValidRects,
                                   LPNCCALCSIZE_PARAMS lpncsp, LRESULT& retVal)
{
    MsgRouting mrRetVal = mrDoDefault;

    if (fCalcValidRects == FALSE) {
        return mrDoDefault;
    }
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (env->EnsureLocalCapacity(2) < 0) {
        return mrConsume;
    }
    jobject target = GetTarget(env);
    jstring warningString =
      (jstring)(env)->GetObjectField(target, AwtWindow::warningStringID);
    if (warningString != NULL) {
        RECT r;
        ::CopyRect(&r, &lpncsp->rgrc[0]);
        retVal = static_cast<UINT>(DefWindowProc(WM_NCCALCSIZE, fCalcValidRects,
        reinterpret_cast<LPARAM>(lpncsp)));

        /* Adjust non-client area for warning banner space. */
        m_warningRect.left = lpncsp->rgrc[0].left;
        m_warningRect.right = lpncsp->rgrc[0].right;
        m_warningRect.bottom = lpncsp->rgrc[0].bottom;
        m_warningRect.top =
            m_warningRect.bottom - ::GetSystemMetrics(SM_CYCAPTION);
        if (GetStyle() & WS_THICKFRAME) {
            m_warningRect.top -= 2;
        } else {
            m_warningRect.top += 2;
        }

        lpncsp->rgrc[0].bottom = (m_warningRect.top >= lpncsp->rgrc[0].top)
            ? m_warningRect.top
            : lpncsp->rgrc[0].top;

        /* Convert to window-relative coordinates. */
        ::OffsetRect(&m_warningRect, -r.left, -r.top);

        /* Notify target of Insets change. */
        if (HasValidRect()) {
            UpdateInsets(NULL);
        }

        mrRetVal = mrConsume;
    } else {
        // WM_NCCALCSIZE is usually in response to a resize, but
        // also can be triggered by SetWindowPos(SWP_FRAMECHANGED),
        // which means the insets will have changed - rnk 4/7/1998
        retVal = static_cast<UINT>(DefWindowProc(
        WM_NCCALCSIZE, fCalcValidRects, reinterpret_cast<LPARAM>(lpncsp)));
        if (HasValidRect()) {
            UpdateInsets(NULL);
        }
        mrRetVal = mrConsume;
    }
    env->DeleteLocalRef(target);
    env->DeleteLocalRef(warningString);
    return mrRetVal;
}

MsgRouting AwtWindow::WmNcPaint(HRGN hrgn)
{
    DefWindowProc(WM_NCPAINT, (WPARAM)hrgn, 0);

    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (env->EnsureLocalCapacity(2) < 0) {
        return mrConsume;
    }
    jobject target = GetTarget(env);
    jstring warningString =
      (jstring)(env)->GetObjectField(target, AwtWindow::warningStringID);
    if (warningString != NULL) {
        RECT r;
        ::CopyRect(&r, &m_warningRect);
        HDC hDC = ::GetWindowDC(GetHWnd());
        DASSERT(hDC);
        int iSaveDC = ::SaveDC(hDC);
        VERIFY(::SelectClipRgn(hDC, NULL) != NULL);
        VERIFY(::FillRect(hDC, &m_warningRect, (HBRUSH)::GetStockObject(BLACK_BRUSH)));

        if (GetStyle() & WS_THICKFRAME) {
            /* draw edge */
            VERIFY(::DrawEdge(hDC, &r, EDGE_RAISED, BF_TOP));
            r.top += 2;
            VERIFY(::DrawEdge(hDC, &r, EDGE_SUNKEN, BF_RECT));
            ::InflateRect(&r, -2, -2);
        }

        /* draw warning text */
        LPWSTR text = TO_WSTRING(warningString);
        VERIFY(::SetBkColor(hDC, ::GetSysColor(COLOR_BTNFACE)) != CLR_INVALID);
        VERIFY(::SetTextColor(hDC, ::GetSysColor(COLOR_BTNTEXT)) != CLR_INVALID);
        VERIFY(::SelectObject(hDC, ::GetStockObject(DEFAULT_GUI_FONT)) != NULL);
        VERIFY(::SetTextAlign(hDC, TA_LEFT | TA_BOTTOM) != GDI_ERROR);
        VERIFY(::ExtTextOutW(hDC, r.left+2, r.bottom-1,
                             ETO_CLIPPED | ETO_OPAQUE,
                             &r, text, static_cast<UINT>(wcslen(text)), NULL));
        VERIFY(::RestoreDC(hDC, iSaveDC));
        ::ReleaseDC(GetHWnd(), hDC);
    }

    env->DeleteLocalRef(target);
    env->DeleteLocalRef(warningString);
    return mrConsume;
}

MsgRouting AwtWindow::WmNcHitTest(UINT x, UINT y, LRESULT& retVal)
{
    // If this window is blocked by modal dialog, return HTCLIENT for any point of it.
    // That prevents it to be moved or resized using the mouse. Disabling these
    // actions to be launched from sysmenu is implemented by ignoring WM_SYSCOMMAND
    if (::IsWindow(GetModalBlocker(GetHWnd()))) {
        retVal = HTCLIENT;
    } else {
        retVal = DefWindowProc(WM_NCHITTEST, 0, MAKELPARAM(x, y));
    }
    return mrConsume;
}

MsgRouting AwtWindow::WmGetIcon(WPARAM iconType, LRESULT& retValue)
{
    return mrDoDefault;
}

LRESULT AwtWindow::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    MsgRouting mr = mrDoDefault;
    LRESULT retValue = 0L;

    switch(message) {
        case WM_GETICON:
            mr = WmGetIcon(wParam, retValue);
            break;
        case WM_SYSCOMMAND:
            //Fixed 6355340: Contents of frame are not layed out properly on maximize
            if ((wParam & 0xFFF0) == SC_SIZE) {
                AwtWindow::sm_resizing = TRUE;
                mr = WmSysCommand(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                if (mr != mrConsume) {
                    AwtWindow::DefWindowProc(message, wParam, lParam);
                }
                AwtWindow::sm_resizing = FALSE;
                if (!AwtToolkit::GetInstance().IsDynamicLayoutActive()) {
                    WindowResized();
                }
                mr = mrConsume;
            }
            break;
    }

    if (mr != mrConsume) {
        retValue = AwtCanvas::WindowProc(message, wParam, lParam);
    }
    return retValue;
}

/*
 * Fix for BugTraq ID 4041703: keyDown not being invoked.
 * This method overrides AwtCanvas::HandleEvent() since
 * an empty Window always receives the focus on the activation
 * so we don't have to modify the behavior.
 */
MsgRouting AwtWindow::HandleEvent(MSG *msg, BOOL synthetic)
{
    return AwtComponent::HandleEvent(msg, synthetic);
}

void AwtWindow::WindowResized()
{
    SendComponentEvent(java_awt_event_ComponentEvent_COMPONENT_RESIZED);
    // Need to replace surfaceData on resize to catch changes to
    // various component-related values, such as insets
    if (DDCanReplaceSurfaces(GetHWnd())) {
        JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
        env->CallVoidMethod(m_peerObject, AwtComponent::replaceSurfaceDataLaterMID);
    }
}

BOOL CALLBACK InvalidateChildRect(HWND hWnd, LPARAM)
{
    TRY;

    ::InvalidateRect(hWnd, NULL, TRUE);
    return TRUE;

    CATCH_BAD_ALLOC_RET(FALSE);
}

void AwtWindow::Invalidate(RECT* r)
{
    ::InvalidateRect(GetHWnd(), NULL, TRUE);
    ::EnumChildWindows(GetHWnd(), (WNDENUMPROC)InvalidateChildRect, 0);
}

BOOL AwtWindow::IsResizable() {
    return m_isResizable;
}

void AwtWindow::SetResizable(BOOL isResizable)
{
    m_isResizable = isResizable;
    if (IsEmbeddedFrame()) {
        return;
    }
    LONG style = GetStyle();
    LONG resizeStyle = WS_MAXIMIZEBOX;

    if (IsUndecorated() == FALSE) {
        resizeStyle |= WS_THICKFRAME;
    }

    if (isResizable) {
        style |= resizeStyle;
    } else {
        style &= ~resizeStyle;
    }
    SetStyle(style);
    RedrawNonClient();
}

// SetWindowPos flags to cause frame edge to be recalculated
static const UINT SwpFrameChangeFlags =
    SWP_FRAMECHANGED | /* causes WM_NCCALCSIZE to be called */
    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
    SWP_NOACTIVATE | SWP_NOCOPYBITS |
    SWP_NOREPOSITION | SWP_NOSENDCHANGING;

//
// Forces WM_NCCALCSIZE to be called to recalculate
// window border (updates insets) without redrawing it
//
void AwtWindow::RecalcNonClient()
{
    ::SetWindowPos(GetHWnd(), (HWND) NULL, 0, 0, 0, 0, SwpFrameChangeFlags|SWP_NOREDRAW);
}

//
// Forces WM_NCCALCSIZE to be called to recalculate
// window border (updates insets) and redraws border to match
//
void AwtWindow::RedrawNonClient()
{
    ::SetWindowPos(GetHWnd(), (HWND) NULL, 0, 0, 0, 0, SwpFrameChangeFlags|SWP_ASYNCWINDOWPOS);
}

int AwtWindow::GetScreenImOn() {
    MHND hmon;
    int scrnNum;

    hmon = ::MonitorFromWindow(GetHWnd(), MONITOR_DEFAULT_TO_PRIMARY);
    DASSERT(hmon != NULL);

    scrnNum = AwtWin32GraphicsDevice::GetScreenFromMHND(hmon);
    DASSERT(scrnNum > -1);

    return scrnNum;
}

/* Check to see if we've been moved onto another screen.
 * If so, update internal data, surfaces, etc.
 */

void AwtWindow::CheckIfOnNewScreen() {
    int curScrn = GetScreenImOn();

    if (curScrn != m_screenNum) {  // we've been moved
        JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

        jclass peerCls = env->GetObjectClass(m_peerObject);
        DASSERT(peerCls);

        jmethodID draggedID = env->GetMethodID(peerCls, "draggedToNewScreen",
                                               "()V");
        DASSERT(draggedID);

        env->CallVoidMethod(m_peerObject, draggedID);
        m_screenNum = curScrn;

        env->DeleteLocalRef(peerCls);
    }
}

BOOL AwtWindow::IsFocusableWindow() {
    /*
     * For Window/Frame/Dialog to accept focus it should:
     * - be focusable;
     * - be not blocked by any modal blocker.
     */
    BOOL focusable = m_isFocusableWindow && !::IsWindow(AwtWindow::GetModalBlocker(GetHWnd()));
    AwtFrame *owner = GetOwningFrameOrDialog(); // NULL for Frame and Dialog

    if (owner != NULL) {
        /*
         * Also for Window (not Frame/Dialog) to accept focus:
         * - its decorated parent should accept focus;
         */
        focusable = focusable && owner->IsFocusableWindow();
    }
    return focusable;
}

void AwtWindow::SetModalBlocker(HWND window, HWND blocker) {
    if (!::IsWindow(window)) {
        return;
    }

    if (::IsWindow(blocker)) {
        ::SetProp(window, ModalBlockerProp, reinterpret_cast<HANDLE>(blocker));
        ::EnableWindow(window, FALSE);
    } else {
        ::RemoveProp(window, ModalBlockerProp);
         AwtComponent *comp = AwtComponent::GetComponent(window);
         // we don't expect to be called with non-java HWNDs
         DASSERT(comp && comp->IsTopLevel());
         // we should not unblock disabled toplevels
         ::EnableWindow(window, comp->isEnabled());
    }
}

void AwtWindow::SetAndActivateModalBlocker(HWND window, HWND blocker) {
    if (!::IsWindow(window)) {
        return;
    }
    AwtWindow::SetModalBlocker(window, blocker);
    if (::IsWindow(blocker)) {
        // We must check for visibility. Otherwise invisible dialog will receive WM_ACTIVATE.
        if (::IsWindowVisible(blocker)) {
            ::BringWindowToTop(blocker);
            ::SetForegroundWindow(blocker);
        }
    }
}

void AwtWindow::FlashWindowEx(HWND hWnd, UINT count, DWORD timeout, DWORD flags) {
    FLASHWINFO fi;
    fi.cbSize = sizeof(fi);
    fi.hwnd = hWnd;
    fi.dwFlags = flags;
    fi.uCount = count;
    fi.dwTimeout = timeout;
    ::FlashWindowEx(&fi);
}

void AwtWindow::_ToFront(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    jobject self = (jobject)param;

    AwtWindow *w = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    w = (AwtWindow *)pData;
    if (::IsWindow(w->GetHWnd()))
    {
        UINT flags = SWP_NOMOVE|SWP_NOSIZE;
        BOOL focusable = w->IsFocusableWindow();
        BOOL autoRequestFocus = w->IsAutoRequestFocus();

        if (!focusable || !autoRequestFocus)
        {
            flags = flags|SWP_NOACTIVATE;
        }
        ::SetWindowPos(w->GetHWnd(), HWND_TOP, 0, 0, 0, 0, flags);
        if (focusable && autoRequestFocus)
        {
            ::SetForegroundWindow(w->GetHWnd());
        }
    }
ret:
    env->DeleteGlobalRef(self);
}

void AwtWindow::_ToBack(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    jobject self = (jobject)param;

    AwtWindow *w = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    w = (AwtWindow *)pData;
    if (::IsWindow(w->GetHWnd()))
    {
        HWND hwnd = w->GetHWnd();
//        if (AwtComponent::GetComponent(hwnd) == NULL) {
//            // Window was disposed. Don't bother.
//            return;
//        }

        ::SetWindowPos(hwnd, HWND_BOTTOM, 0, 0 ,0, 0,
            SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

        // If hwnd is the foreground window or if *any* of its owners are, then
        // we have to reset the foreground window. The reason is that when we send
        // hwnd to back, all of its owners are sent to back as well. If any one of
        // them is the foreground window, then it's possible that we could end up
        // with a foreground window behind a window of another application.
        HWND foregroundWindow = ::GetForegroundWindow();
        BOOL adjustForegroundWindow;
        HWND toTest = hwnd;
        do
        {
            adjustForegroundWindow = (toTest == foregroundWindow);
            if (adjustForegroundWindow)
            {
                break;
            }
            toTest = ::GetWindow(toTest, GW_OWNER);
        }
        while (toTest != NULL);

        if (adjustForegroundWindow)
        {
            HWND foregroundSearch = hwnd, newForegroundWindow = NULL;
                while (1)
                {
                foregroundSearch = ::GetNextWindow(foregroundSearch, GW_HWNDPREV);
                if (foregroundSearch == NULL)
                {
                    break;
                }
                LONG style = static_cast<LONG>(::GetWindowLongPtr(foregroundSearch, GWL_STYLE));
                if ((style & WS_CHILD) || !(style & WS_VISIBLE))
                {
                    continue;
                }

                AwtComponent *c = AwtComponent::GetComponent(foregroundSearch);
                if ((c != NULL) && !::IsWindow(AwtWindow::GetModalBlocker(c->GetHWnd())))
                {
                    newForegroundWindow = foregroundSearch;
                }
            }
            if (newForegroundWindow != NULL)
            {
                ::SetWindowPos(newForegroundWindow, HWND_TOP, 0, 0, 0, 0,
                    SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
                if (((AwtWindow*)AwtComponent::GetComponent(newForegroundWindow))->IsFocusableWindow())
                {
                    ::SetForegroundWindow(newForegroundWindow);
                }
            }
            else
            {
                // We *have* to set the active HWND to something new. We simply
                // cannot risk having an active Java HWND which is behind an HWND
                // of a native application. This really violates the Windows user
                // experience.
                //
                // Windows won't allow us to set the foreground window to NULL,
                // so we use the desktop window instead. To the user, it appears
                // that there is no foreground window system-wide.
                ::SetForegroundWindow(::GetDesktopWindow());
            }
        }
    }
ret:
    env->DeleteGlobalRef(self);
}

void AwtWindow::_SetAlwaysOnTop(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    SetAlwaysOnTopStruct *sas = (SetAlwaysOnTopStruct *)param;
    jobject self = sas->window;
    jboolean value = sas->value;

    AwtWindow *w = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    w = (AwtWindow *)pData;
    if (::IsWindow(w->GetHWnd()))
    {
        w->SendMessage(WM_AWT_SETALWAYSONTOP, (WPARAM)value, (LPARAM)w);
    }
ret:
    env->DeleteGlobalRef(self);

    delete sas;
}

void AwtWindow::_SetTitle(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    SetTitleStruct *sts = (SetTitleStruct *)param;
    jobject self = sts->window;
    jstring title = sts->title;

    AwtWindow *w = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    JNI_CHECK_NULL_GOTO(title, "null title", ret);

    w = (AwtWindow *)pData;
    if (::IsWindow(w->GetHWnd()))
    {
        int length = env->GetStringLength(title);
        WCHAR *buffer = new WCHAR[length + 1];
        env->GetStringRegion(title, 0, length, buffer);
        buffer[length] = L'\0';
        VERIFY(::SetWindowTextW(w->GetHWnd(), buffer));
        delete[] buffer;
    }
ret:
    env->DeleteGlobalRef(self);
    if (title != NULL) {
        env->DeleteGlobalRef(title);
    }

    delete sts;
}

void AwtWindow::_SetResizable(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    SetResizableStruct *srs = (SetResizableStruct *)param;
    jobject self = srs->window;
    jboolean resizable = srs->resizable;

    AwtWindow *w = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    w = (AwtWindow *)pData;
    if (::IsWindow(w->GetHWnd()))
    {
        DASSERT(!IsBadReadPtr(w, sizeof(AwtWindow)));
        w->SetResizable(resizable != 0);
    }
ret:
    env->DeleteGlobalRef(self);

    delete srs;
}

void AwtWindow::_UpdateInsets(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    UpdateInsetsStruct *uis = (UpdateInsetsStruct *)param;
    jobject self = uis->window;
    jobject insets = uis->insets;

    AwtWindow *w = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    JNI_CHECK_NULL_GOTO(insets, "null insets", ret);
    w = (AwtWindow *)pData;
    if (::IsWindow(w->GetHWnd()))
    {
        w->UpdateInsets(insets);
    }
ret:
    env->DeleteGlobalRef(self);
    env->DeleteGlobalRef(insets);

    delete uis;
}

void AwtWindow::_ReshapeFrame(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    ReshapeFrameStruct *rfs = (ReshapeFrameStruct *)param;
    jobject self = rfs->frame;
    jint x = rfs->x;
    jint y = rfs->y;
    jint w = rfs->w;
    jint h = rfs->h;

    if (env->EnsureLocalCapacity(1) < 0)
    {
        env->DeleteGlobalRef(self);
        delete rfs;
        return;
    }

    AwtFrame *p = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    p = (AwtFrame *)pData;
    if (::IsWindow(p->GetHWnd()))
    {
        jobject target = env->GetObjectField(self, AwtObject::targetID);
        if (target != NULL)
        {
            // enforce tresholds before sending the event
            // Fix for 4459064 : do not enforce thresholds for embedded frames
            if (!p->IsEmbeddedFrame())
            {
                jobject peer = p->GetPeer(env);
                int minWidth = ::GetSystemMetrics(SM_CXMIN);
                int minHeight = ::GetSystemMetrics(SM_CYMIN);
                if (w < minWidth)
                {
                    env->SetIntField(target, AwtComponent::widthID,
                        w = minWidth);
                    env->SetIntField(peer, AwtWindow::sysWID,
                        w);
                }
                if (h < minHeight)
                {
                    env->SetIntField(target, AwtComponent::heightID,
                        h = minHeight);
                    env->SetIntField(peer, AwtWindow::sysHID,
                        h);
                }
            }
            env->DeleteLocalRef(target);

            RECT *r = new RECT;
            ::SetRect(r, x, y, x + w, y + h);
            p->SendMessage(WM_AWT_RESHAPE_COMPONENT, 0, (LPARAM)r);
            // r is deleted in message handler

            // After the input method window shown, the dimension & position may not
            // be valid until this method is called. So we need to adjust the
            // IME candidate window position for the same reason as commented on
            // awt_Frame.cpp Show() method.
            if (p->isInputMethodWindow() && ::IsWindowVisible(p->GetHWnd())) {
              p->AdjustCandidateWindowPos();
            }
        }
        else
        {
            JNU_ThrowNullPointerException(env, "null target");
        }
    }
ret:
   env->DeleteGlobalRef(self);

   delete rfs;
}

/*
 * This is AwtWindow-specific function that is not intended for reusing
 */
HICON CreateIconFromRaster(JNIEnv* env, jintArray iconRaster, jint w, jint h)
{
    HBITMAP mask = NULL;
    HBITMAP image = NULL;
    HICON icon = NULL;
    if (iconRaster != NULL) {
        int* iconRasterBuffer = NULL;
        try {
            iconRasterBuffer = (int *)env->GetPrimitiveArrayCritical(iconRaster, 0);

            JNI_CHECK_NULL_GOTO(iconRasterBuffer, "iconRaster data", done);

            mask = BitmapUtil::CreateTransparencyMaskFromARGB(w, h, iconRasterBuffer);
            image = BitmapUtil::CreateV4BitmapFromARGB(w, h, iconRasterBuffer);
        } catch (...) {
            if (iconRasterBuffer != NULL) {
                env->ReleasePrimitiveArrayCritical(iconRaster, iconRasterBuffer, 0);
            }
            throw;
        }
        if (iconRasterBuffer != NULL) {
            env->ReleasePrimitiveArrayCritical(iconRaster, iconRasterBuffer, 0);
        }
    }
    if (mask && image) {
        ICONINFO icnInfo;
        memset(&icnInfo, 0, sizeof(ICONINFO));
        icnInfo.hbmMask = mask;
        icnInfo.hbmColor = image;
        icnInfo.fIcon = TRUE;
        icon = ::CreateIconIndirect(&icnInfo);
    }
    if (image) {
        destroy_BMP(image);
    }
    if (mask) {
        destroy_BMP(mask);
    }
done:
    return icon;
}

void AwtWindow::SetIconData(JNIEnv* env, jintArray iconRaster, jint w, jint h,
                             jintArray smallIconRaster, jint smw, jint smh)
{
    HICON hOldIcon = NULL;
    HICON hOldIconSm = NULL;
    //Destroy previous icon if it isn't inherited
    if ((m_hIcon != NULL) && !m_iconInherited) {
        hOldIcon = m_hIcon;
    }
    m_hIcon = NULL;
    if ((m_hIconSm != NULL) && !m_iconInherited) {
        hOldIconSm = m_hIconSm;
    }
    m_hIconSm = NULL;
    m_hIcon = CreateIconFromRaster(env, iconRaster, w, h);
    m_hIconSm = CreateIconFromRaster(env, smallIconRaster, smw, smh);

    m_iconInherited = (m_hIcon == NULL);
    if (m_iconInherited) {
        HWND hOwner = ::GetWindow(GetHWnd(), GW_OWNER);
        AwtWindow* owner = (AwtWindow *)AwtComponent::GetComponent(hOwner);
        if (owner != NULL) {
            m_hIcon = owner->GetHIcon();
            m_hIconSm = owner->GetHIconSm();
        } else {
            m_iconInherited = FALSE;
        }
    }
    DoUpdateIcon();
    EnumThreadWindows(AwtToolkit::MainThread(), UpdateOwnedIconCallback, (LPARAM)this);
    if (hOldIcon != NULL) {
        DestroyIcon(hOldIcon);
    }
    if (hOldIconSm != NULL) {
        DestroyIcon(hOldIconSm);
    }
}

BOOL AwtWindow::UpdateOwnedIconCallback(HWND hWndOwned, LPARAM lParam)
{
    HWND hWndOwner = ::GetWindow(hWndOwned, GW_OWNER);
    AwtWindow* owner = (AwtWindow*)lParam;
    if (hWndOwner == owner->GetHWnd()) {
        AwtComponent* comp = AwtComponent::GetComponent(hWndOwned);
        if (comp != NULL && comp->IsTopLevel()) {
            AwtWindow* owned = (AwtWindow *)comp;
            if (owned->m_iconInherited) {
                owned->m_hIcon = owner->m_hIcon;
                owned->m_hIconSm = owner->m_hIconSm;
                owned->DoUpdateIcon();
                EnumThreadWindows(AwtToolkit::MainThread(), UpdateOwnedIconCallback, (LPARAM)owned);
            }
        }
    }
    return TRUE;
}

void AwtWindow::DoUpdateIcon()
{
    //Does nothing for windows, is overriden for frames and dialogs
}

void AwtWindow::_SetIconImagesData(void * param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    SetIconImagesDataStruct* s = (SetIconImagesDataStruct*)param;
    jobject self = s->window;

    jintArray iconRaster = s->iconRaster;
    jintArray smallIconRaster = s->smallIconRaster;

    AwtWindow *window = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    // ok to pass null raster: default AWT icon

    window = (AwtWindow*)pData;
    if (::IsWindow(window->GetHWnd()))
    {
        window->SetIconData(env, iconRaster, s->w, s->h, smallIconRaster, s->smw, s->smh);

    }

ret:
    env->DeleteGlobalRef(self);
    env->DeleteGlobalRef(iconRaster);
    env->DeleteGlobalRef(smallIconRaster);
    delete s;
}

void AwtWindow::_SetMinSize(void* param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    SizeStruct *ss = (SizeStruct *)param;
    jobject self = ss->window;
    jint w = ss->w;
    jint h = ss->h;
    //Perform size setting
    AwtWindow *window = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    window = (AwtWindow *)pData;
    window->m_minSize.x = w;
    window->m_minSize.y = h;
  ret:
    env->DeleteGlobalRef(self);
    delete ss;
}

jint AwtWindow::_GetScreenImOn(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    jobject self = (jobject)param;

    // It's entirely possible that our native resources have been destroyed
    // before our java peer - if we're dispose()d, for instance.
    // Alert caller w/ IllegalComponentStateException.
    if (self == NULL) {
        JNU_ThrowByName(env, "java/awt/IllegalComponentStateException",
                        "Peer null in JNI");
        return 0;
    }
    PDATA pData = JNI_GET_PDATA(self);
    if (pData == NULL) {
        JNU_ThrowByName(env, "java/awt/IllegalComponentStateException",
                        "Native resources unavailable");
        env->DeleteGlobalRef(self);
        return 0;
    }

    jint result = 0;
    AwtWindow *w = (AwtWindow *)pData;
    if (::IsWindow(w->GetHWnd()))
    {
        result = (jint)w->GetScreenImOn();
    }

    env->DeleteGlobalRef(self);

    return result;
}

void AwtWindow::_SetFocusableWindow(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    SetFocusableWindowStruct *sfws = (SetFocusableWindowStruct *)param;
    jobject self = sfws->window;
    jboolean isFocusableWindow = sfws->isFocusableWindow;

    AwtWindow *window = NULL;

    PDATA pData;
    JNI_CHECK_PEER_GOTO(self, ret);
    window = (AwtWindow *)pData;

    window->m_isFocusableWindow = isFocusableWindow;

    if (IS_WIN2000) {
        if (!window->m_isFocusableWindow) {
            LONG isPopup = window->GetStyle() & WS_POPUP;
            window->SetStyleEx(window->GetStyleEx() | (isPopup ? 0 : WS_EX_APPWINDOW) | AWT_WS_EX_NOACTIVATE);
        } else {
            window->SetStyleEx(window->GetStyleEx() & ~WS_EX_APPWINDOW & ~AWT_WS_EX_NOACTIVATE);
        }
    }

  ret:
    env->DeleteGlobalRef(self);
    delete sfws;
}

void AwtWindow::_ModalDisable(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    ModalDisableStruct *mds = (ModalDisableStruct *)param;
    jobject self = mds->window;
    HWND blockerHWnd = (HWND)mds->blockerHWnd;

    AwtWindow *window = NULL;
    HWND windowHWnd = 0;

    JNI_CHECK_NULL_GOTO(self, "peer", ret);
    PDATA pData = JNI_GET_PDATA(self);
    if (pData == NULL) {
        env->DeleteGlobalRef(self);
        delete mds;
        return;
    }

    window = (AwtWindow *)pData;
    windowHWnd = window->GetHWnd();
    if (::IsWindow(windowHWnd)) {
        AwtWindow::SetAndActivateModalBlocker(windowHWnd, blockerHWnd);
    }

ret:
    env->DeleteGlobalRef(self);

    delete mds;
}

void AwtWindow::_ModalEnable(void *param)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    jobject self = (jobject)param;

    AwtWindow *window = NULL;
    HWND windowHWnd = 0;

    JNI_CHECK_NULL_GOTO(self, "peer", ret);
    PDATA pData = JNI_GET_PDATA(self);
    if (pData == NULL) {
        env->DeleteGlobalRef(self);
        return;
    }

    window = (AwtWindow *)pData;
    windowHWnd = window->GetHWnd();
    if (::IsWindow(windowHWnd)) {
        AwtWindow::SetModalBlocker(windowHWnd, NULL);
    }

  ret:
    env->DeleteGlobalRef(self);
}

/*
 * Fixed 6353381: it's improved fix for 4792958
 * which was backed-out to avoid 5059656
 */
BOOL AwtWindow::HasValidRect()
{
    RECT inside;
    RECT outside;

    if (::IsIconic(GetHWnd())) {
        return FALSE;
    }

    ::GetClientRect(GetHWnd(), &inside);
    ::GetWindowRect(GetHWnd(), &outside);

    BOOL isZeroClientArea = (inside.right == 0 && inside.bottom == 0);
    BOOL isInvalidLocation = ((outside.left == -32000 && outside.top == -32000) || // Win2k && WinXP
                              (outside.left == 32000 && outside.top == 32000) || // Win95 && Win98
                              (outside.left == 3000 && outside.top == 3000)); // Win95 && Win98

    // the bounds correspond to iconic state
    if (isZeroClientArea && isInvalidLocation)
    {
        return FALSE;
    }

    return TRUE;
}

extern "C" {

/*
 * Class:     java_awt_Window
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_awt_Window_initIDs(JNIEnv *env, jclass cls)
{
    TRY;

    AwtWindow::warningStringID =
        env->GetFieldID(cls, "warningString", "Ljava/lang/String;");
    AwtWindow::locationByPlatformID =
        env->GetFieldID(cls, "locationByPlatform", "Z");
    AwtWindow::autoRequestFocusID =
        env->GetFieldID(cls, "autoRequestFocus", "Z");

    CATCH_BAD_ALLOC;
}

} /* extern "C" */


/************************************************************************
 * WindowPeer native methods
 */

extern "C" {

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_initIDs(JNIEnv *env, jclass cls)
{
    TRY;

    AwtWindow::sysXID = env->GetFieldID(cls, "sysX", "I");
    AwtWindow::sysYID = env->GetFieldID(cls, "sysY", "I");
    AwtWindow::sysWID = env->GetFieldID(cls, "sysW", "I");
    AwtWindow::sysHID = env->GetFieldID(cls, "sysH", "I");

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    toFront
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer__1toFront(JNIEnv *env, jobject self)
{
    TRY;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_ToFront,
        env->NewGlobalRef(self));
    // global ref is deleted in _ToFront()

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    toBack
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_toBack(JNIEnv *env, jobject self)
{
    TRY;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_ToBack,
        env->NewGlobalRef(self));
    // global ref is deleted in _ToBack()

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    setAlwaysOnTop
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_setAlwaysOnTopNative(JNIEnv *env, jobject self,
                                                jboolean value)
{
    TRY;

    SetAlwaysOnTopStruct *sas = new SetAlwaysOnTopStruct;
    sas->window = env->NewGlobalRef(self);
    sas->value = value;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_SetAlwaysOnTop, sas);
    // global ref and sas are deleted in _SetAlwaysOnTop

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    _setTitle
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer__1setTitle(JNIEnv *env, jobject self,
                                            jstring title)
{
    TRY;

    SetTitleStruct *sts = new SetTitleStruct;
    sts->window = env->NewGlobalRef(self);
    sts->title = (jstring)env->NewGlobalRef(title);

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_SetTitle, sts);
    /// global refs and sts are deleted in _SetTitle()

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    _setResizable
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer__1setResizable(JNIEnv *env, jobject self,
                                                jboolean resizable)
{
    TRY;

    SetResizableStruct *srs = new SetResizableStruct;
    srs->window = env->NewGlobalRef(self);
    srs->resizable = resizable;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_SetResizable, srs);
    // global ref and srs are deleted in _SetResizable

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    create
 * Signature: (Lsun/awt/windows/WComponentPeer;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_createAwtWindow(JNIEnv *env, jobject self,
                                                 jobject parent)
{
    TRY;

    PDATA pData;
//    JNI_CHECK_PEER_RETURN(parent);
    AwtToolkit::CreateComponent(self, parent,
                                (AwtToolkit::ComponentFactory)
                                AwtWindow::Create);
    JNI_CHECK_PEER_CREATION_RETURN(self);

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    updateInsets
 * Signature: (Ljava/awt/Insets;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_updateInsets(JNIEnv *env, jobject self,
                                              jobject insets)
{
    TRY;

    UpdateInsetsStruct *uis = new UpdateInsetsStruct;
    uis->window = env->NewGlobalRef(self);
    uis->insets = env->NewGlobalRef(insets);

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_UpdateInsets, uis);
    // global refs and uis are deleted in _UpdateInsets()

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    reshapeFrame
 * Signature: (IIII)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_reshapeFrame(JNIEnv *env, jobject self,
                                        jint x, jint y, jint w, jint h)
{
    TRY;

    ReshapeFrameStruct *rfs = new ReshapeFrameStruct;
    rfs->frame = env->NewGlobalRef(self);
    rfs->x = x;
    rfs->y = y;
    rfs->w = w;
    rfs->h = h;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_ReshapeFrame, rfs);
    // global ref and rfs are deleted in _ReshapeFrame()

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    getSysMinWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WWindowPeer_getSysMinWidth(JNIEnv *env, jclass self)
{
    TRY;

    return ::GetSystemMetrics(SM_CXMIN);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    getSysMinHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WWindowPeer_getSysMinHeight(JNIEnv *env, jclass self)
{
    TRY;

    return ::GetSystemMetrics(SM_CYMIN);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    getSysIconHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WWindowPeer_getSysIconHeight(JNIEnv *env, jclass self)
{
    TRY;

    return ::GetSystemMetrics(SM_CYICON);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    getSysIconWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WWindowPeer_getSysIconWidth(JNIEnv *env, jclass self)
{
    TRY;

    return ::GetSystemMetrics(SM_CXICON);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    getSysSmIconHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WWindowPeer_getSysSmIconHeight(JNIEnv *env, jclass self)
{
    TRY;

    return ::GetSystemMetrics(SM_CYSMICON);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    getSysSmIconWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WWindowPeer_getSysSmIconWidth(JNIEnv *env, jclass self)
{
    TRY;

    return ::GetSystemMetrics(SM_CXSMICON);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    setIconImagesData
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_setIconImagesData(JNIEnv *env, jobject self,
    jintArray iconRaster, jint w, jint h,
    jintArray smallIconRaster, jint smw, jint smh)
{
    TRY;

    SetIconImagesDataStruct *sims = new SetIconImagesDataStruct;

    sims->window = env->NewGlobalRef(self);
    sims->iconRaster = (jintArray)env->NewGlobalRef(iconRaster);
    sims->w = w;
    sims->h = h;
    sims->smallIconRaster = (jintArray)env->NewGlobalRef(smallIconRaster);
    sims->smw = smw;
    sims->smh = smh;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_SetIconImagesData, sims);
    // global refs and sims are deleted in _SetIconImagesData()

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    setMinSize
 * Signature: (Lsun/awt/windows/WWindowPeer;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_setMinSize(JNIEnv *env, jobject self,
                                              jint w, jint h)
{
    TRY;

    SizeStruct *ss = new SizeStruct;
    ss->window = env->NewGlobalRef(self);
    ss->w = w;
    ss->h = h;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_SetMinSize, ss);
    // global refs and mds are deleted in _SetMinSize

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    getScreenImOn
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WWindowPeer_getScreenImOn(JNIEnv *env, jobject self)
{
    TRY;

    return static_cast<jint>(reinterpret_cast<INT_PTR>(AwtToolkit::GetInstance().SyncCall(
        (void *(*)(void *))AwtWindow::_GetScreenImOn,
        env->NewGlobalRef(self))));
    // global ref is deleted in _GetScreenImOn()

    CATCH_BAD_ALLOC_RET(-1);
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    modalDisable
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_modalDisable(JNIEnv *env, jobject self,
                                              jobject blocker, jlong blockerHWnd)
{
    TRY;

    ModalDisableStruct *mds = new ModalDisableStruct;
    mds->window = env->NewGlobalRef(self);
    mds->blockerHWnd = blockerHWnd;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_ModalDisable, mds);
    // global ref and mds are deleted in _ModalDisable

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    modalEnable
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_modalEnable(JNIEnv *env, jobject self, jobject blocker)
{
    TRY;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_ModalEnable,
        env->NewGlobalRef(self));
    // global ref is deleted in _ModalEnable

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WWindowPeer
 * Method:    setFocusableWindow
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_setFocusableWindow(JNIEnv *env, jobject self, jboolean isFocusableWindow)
{
    TRY;

    SetFocusableWindowStruct *sfws = new SetFocusableWindowStruct;
    sfws->window = env->NewGlobalRef(self);
    sfws->isFocusableWindow = isFocusableWindow;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_SetFocusableWindow, sfws);
    // global ref and sfws are deleted in _SetFocusableWindow()

    CATCH_BAD_ALLOC;
}

JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_nativeGrab(JNIEnv *env, jobject self)
{
    TRY;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_Grab, env->NewGlobalRef(self));
    // global ref is deleted in _Grab()

    CATCH_BAD_ALLOC;
}

JNIEXPORT void JNICALL
Java_sun_awt_windows_WWindowPeer_nativeUngrab(JNIEnv *env, jobject self)
{
    TRY;

    AwtToolkit::GetInstance().SyncCall(AwtWindow::_Ungrab, env->NewGlobalRef(self));
    // global ref is deleted in _Ungrab()

    CATCH_BAD_ALLOC;
}

} /* extern "C" */
