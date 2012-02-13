/*
 * Copyright (c) 1996, 2007, Oracle and/or its affiliates. All rights reserved.
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

#include <signal.h>
#include <windowsx.h>

#if defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER >= 1000
#include <crtdbg.h>
#endif

#define _JNI_IMPLEMENTATION_
#include "stdhdrs.h"
#include "awt_DrawingSurface.h"
#include "awt_AWTEvent.h"
#include "awt_Component.h"
#include "awt_Canvas.h"
#include "awt_Clipboard.h"
#include "awt_Frame.h"
#include "awt_Dialog.h"
#include "awt_Font.h"
#include "awt_Cursor.h"
#include "awt_InputEvent.h"
#include "awt_KeyEvent.h"
#include "awt_List.h"
#include "awt_Palette.h"
#include "awt_PopupMenu.h"
#include "awt_Toolkit.h"
#include "awt_DesktopProperties.h"
#include "awt_FileDialog.h"
#include "CmdIDList.h"
#include "awt_new.h"
#include "awt_Unicode.h"
#include "ddrawUtils.h"
#include "debug_trace.h"
#include "debug_mem.h"

#include "ComCtl32Util.h"

#include <awt_DnDDT.h>
#include <awt_DnDDS.h>

#include <java_awt_Toolkit.h>
#include <java_awt_event_InputMethodEvent.h>
#include <java_awt_peer_ComponentPeer.h>

extern void initScreens(JNIEnv *env);
extern "C" void awt_dnd_initialize();
extern "C" void awt_dnd_uninitialize();
extern "C" void awt_clipboard_uninitialize(JNIEnv *env);
extern "C" BOOL g_bUserHasChangedInputLang;

extern CriticalSection windowMoveLock;
extern BOOL windowMoveLockHeld;

// Needed by JAWT: see awt_DrawingSurface.cpp.
extern jclass jawtVImgClass;
extern jclass jawtVSMgrClass;
extern jclass jawtComponentClass;
extern jclass jawtW32ossdClass;
extern jfieldID jawtPDataID;
extern jfieldID jawtSDataID;
extern jfieldID jawtSMgrID;

/************************************************************************
 * Utilities
 */

/* Initialize the Java VM instance variable when the library is
   first loaded */
JavaVM *jvm;

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    TRY;

    jvm = vm;
    return JNI_VERSION_1_2;

    CATCH_BAD_ALLOC_RET(0);
}

extern "C" JNIEXPORT jboolean JNICALL AWTIsHeadless() {
    static JNIEnv *env = NULL;
    static jboolean isHeadless;
    jmethodID headlessFn;
    jclass graphicsEnvClass;

    if (env == NULL) {
        env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
        graphicsEnvClass = env->FindClass(
            "java/awt/GraphicsEnvironment");
        if (graphicsEnvClass == NULL) {
            return JNI_TRUE;
        }
        headlessFn = env->GetStaticMethodID(
            graphicsEnvClass, "isHeadless", "()Z");
        if (headlessFn == NULL) {
            return JNI_TRUE;
        }
        isHeadless = env->CallStaticBooleanMethod(graphicsEnvClass,
            headlessFn);
    }
    return isHeadless;
}

#define IDT_AWT_MOUSECHECK 0x101

static LPCTSTR szAwtToolkitClassName = TEXT("SunAwtToolkit");

UINT AwtToolkit::GetMouseKeyState()
{
    static BOOL mbSwapped = ::GetSystemMetrics(SM_SWAPBUTTON);
    UINT mouseKeyState = 0;

    if (HIBYTE(::GetKeyState(VK_CONTROL)))
        mouseKeyState |= MK_CONTROL;
    if (HIBYTE(::GetKeyState(VK_SHIFT)))
        mouseKeyState |= MK_SHIFT;
    if (HIBYTE(::GetKeyState(VK_LBUTTON)))
        mouseKeyState |= (mbSwapped ? MK_RBUTTON : MK_LBUTTON);
    if (HIBYTE(::GetKeyState(VK_RBUTTON)))
        mouseKeyState |= (mbSwapped ? MK_LBUTTON : MK_RBUTTON);
    if (HIBYTE(::GetKeyState(VK_MBUTTON)))
        mouseKeyState |= MK_MBUTTON;
    return mouseKeyState;
}

//
// Normal ::GetKeyboardState call only works if current thread has
// a message pump, so provide a way for other threads to get
// the keyboard state
//
void AwtToolkit::GetKeyboardState(PBYTE keyboardState)
{
    CriticalSection::Lock       l(AwtToolkit::GetInstance().m_lockKB);
    DASSERT(!IsBadWritePtr(keyboardState, KB_STATE_SIZE));
    memcpy(keyboardState, AwtToolkit::GetInstance().m_lastKeyboardState,
           KB_STATE_SIZE);
}

void AwtToolkit::SetBusy(BOOL busy) {

    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    static jclass awtAutoShutdownClass = NULL;
    static jmethodID notifyBusyMethodID = NULL;
    static jmethodID notifyFreeMethodID = NULL;

    if (awtAutoShutdownClass == NULL) {
        jclass awtAutoShutdownClassLocal = env->FindClass("sun/awt/AWTAutoShutdown");
        if (!JNU_IsNull(env, safe_ExceptionOccurred(env))) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        DASSERT(awtAutoShutdownClassLocal != NULL);
        if (awtAutoShutdownClassLocal == NULL) {
            return;
        }

        awtAutoShutdownClass = (jclass)env->NewGlobalRef(awtAutoShutdownClassLocal);
        env->DeleteLocalRef(awtAutoShutdownClassLocal);

        notifyBusyMethodID = env->GetStaticMethodID(awtAutoShutdownClass,
                                                    "notifyToolkitThreadBusy", "()V");
        if (!JNU_IsNull(env, safe_ExceptionOccurred(env))) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        notifyFreeMethodID = env->GetStaticMethodID(awtAutoShutdownClass,
                                                    "notifyToolkitThreadFree", "()V");
        if (!JNU_IsNull(env, safe_ExceptionOccurred(env))) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        DASSERT(notifyBusyMethodID != NULL);
        DASSERT(notifyFreeMethodID != NULL);
        if (notifyBusyMethodID == NULL || notifyFreeMethodID == NULL) {
            return;
        }
    } /* awtAutoShutdownClass == NULL*/

    if (busy) {
        env->CallStaticVoidMethod(awtAutoShutdownClass,
                                  notifyBusyMethodID);
    } else {
        env->CallStaticVoidMethod(awtAutoShutdownClass,
                                  notifyFreeMethodID);
    }

    if (!JNU_IsNull(env, safe_ExceptionOccurred(env))) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

BOOL AwtToolkit::activateKeyboardLayout(HKL hkl) {
    // This call should succeed in case of one of the following:
    // 1. Win 9x
    // 2. NT with that HKL already loaded
    HKL prev = ::ActivateKeyboardLayout(hkl, 0);

    // If the above call fails, try loading the layout in case of NT
    if ((prev == 0) && IS_NT) {

        // create input locale string, e.g., "00000409", from hkl.
        TCHAR inputLocale[9];
        TCHAR buf[9];
        _tcscpy(inputLocale, TEXT("00000000"));

    // 64-bit: ::LoadKeyboardLayout() is such a weird API - a string of
    // the hex value you want?!  Here we're converting our HKL value to
    // a string.  Hopefully there is no 64-bit trouble.
        _i64tot(reinterpret_cast<INT_PTR>(hkl), buf, 16);
        size_t len = _tcslen(buf);
        memcpy(&inputLocale[8-len], buf, len);

        // load and activate the keyboard layout
        hkl = ::LoadKeyboardLayout(inputLocale, 0);
        if (hkl != 0) {
            prev = ::ActivateKeyboardLayout(hkl, 0);
        }
    }

    return (prev != 0);
}

/************************************************************************
 * Exported functions
 */

extern "C" BOOL APIENTRY DllMain(HANDLE hInstance, DWORD ul_reason_for_call,
                                 LPVOID)
{
    // Don't use the TRY and CATCH_BAD_ALLOC_RET macros if we're detaching
    // the library. Doing so causes awt.dll to call back into the VM during
    // shutdown. This crashes the HotSpot VM.
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        TRY;
        AwtToolkit::GetInstance().SetModuleHandle((HMODULE)hInstance);
        CATCH_BAD_ALLOC_RET(FALSE);
        break;
    case DLL_PROCESS_DETACH:
#ifdef DEBUG
        DTrace_DisableMutex();
        DMem_DisableMutex();
#endif DEBUG
        // Release any resources that have not yet been released
        // Note that releasing DirectX objects is necessary for some
        // failure situations on win9x (such as the primary remaining
        // locked on application exit) but cannot be done during
        // PROCESS_DETACH on XP.  On NT and win2k calling this ends up
        // in a catch() clause in the calling function, but on XP
        // the process simply hangs during the release of the ddraw
        // device object.  Thus we check for NT here and do not bother
        // with the release on any NT flavored OS.  Note that XP is
        // based on NT, so the IS_NT check is valid for NT4, win2k,
        // XP, and presumably XP follow-ons.
        if (!IS_NT) {
            DDRelease();
        }
        break;
    }
    return TRUE;
}

/************************************************************************
 * AwtToolkit fields
 */

AwtToolkit AwtToolkit::theInstance;

/* ids for WToolkit fields accessed from native code */
jmethodID AwtToolkit::windowsSettingChangeMID;
jmethodID AwtToolkit::displayChangeMID;
/* ids for Toolkit methods */
jmethodID AwtToolkit::getDefaultToolkitMID;
jmethodID AwtToolkit::getFontMetricsMID;
jmethodID AwtToolkit::insetsMID;

/************************************************************************
 * JavaStringBuffer method
 */

JavaStringBuffer::JavaStringBuffer(JNIEnv *env, jstring jstr) {
    if (jstr != NULL) {
        int length = env->GetStringLength(jstr);
        buffer = new TCHAR[length + 1];
        LPCTSTR tmp = (LPCTSTR)JNU_GetStringPlatformChars(env, jstr, NULL);
        _tcscpy(buffer, tmp);
        JNU_ReleaseStringPlatformChars(env, jstr, tmp);
    } else {
        buffer = new TCHAR[1];
        buffer[0] = _T('\0');
    }
}


/************************************************************************
 * AwtToolkit methods
 */

AwtToolkit::AwtToolkit() {
    m_localPump = FALSE;
    m_mainThreadId = 0;
    m_toolkitHWnd = NULL;
    m_inputMethodHWnd = NULL;
    m_verbose = FALSE;
    m_isActive = TRUE;
    m_isDisposed = FALSE;

    m_vmSignalled = FALSE;

    m_isDynamicLayoutSet = FALSE;

    m_verifyComponents = FALSE;
    m_breakOnError = FALSE;

    m_breakMessageLoop = FALSE;
    m_messageLoopResult = 0;

    m_lastMouseOver = NULL;
    m_mouseDown = FALSE;

    m_hGetMessageHook = 0;
    m_timer = 0;

    m_cmdIDs = new AwtCmdIDList();
    m_pModalDialog = NULL;
    m_peer = NULL;
    m_dllHandle = NULL;

    m_displayChanged = FALSE;

    // XXX: keyboard mapping should really be moved out of AwtComponent
    AwtComponent::InitDynamicKeyMapTable();

    // initialize kb state array
    ::GetKeyboardState(m_lastKeyboardState);

    m_waitEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    eventNumber = 0;
}

AwtToolkit::~AwtToolkit() {
/*
 *  The code has been moved to AwtToolkit::Dispose() method.
 */
}

HWND AwtToolkit::CreateToolkitWnd(LPCTSTR name)
{
    HWND hwnd = CreateWindow(
        szAwtToolkitClassName,
        (LPCTSTR)name,                    /* window name */
        WS_DISABLED,                      /* window style */
        -1, -1,                           /* position of window */
        0, 0,                             /* width and height */
        NULL, NULL,                       /* hWndParent and hWndMenu */
        GetModuleHandle(),
        NULL);                            /* lpParam */
    DASSERT(hwnd != NULL);
    return hwnd;
}

BOOL AwtToolkit::Initialize(BOOL localPump) {
    AwtToolkit& tk = AwtToolkit::GetInstance();

    if (!tk.m_isActive || tk.m_mainThreadId != 0) {
        /* Already initialized. */
        return FALSE;
    }

    // This call is moved here from AwtToolkit constructor. Having it
    // there led to the bug 6480630: there could be a situation when
    // ComCtl32Util was constructed but not disposed
    ComCtl32Util::GetInstance().InitLibraries();

    /* Register this toolkit's helper window */
    VERIFY(tk.RegisterClass() != NULL);

    // Set up operator new/malloc out of memory handler.
    NewHandler::init();

        //\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
        // Bugs 4032109, 4047966, and 4071991 to fix AWT
        //      crash in 16 color display mode.  16 color mode is supported.  Less
        //      than 16 color is not.
        // creighto@eng.sun.com 1997-10-07
        //
        // Check for at least 16 colors
    HDC hDC = ::GetDC(NULL);
        if ((::GetDeviceCaps(hDC, BITSPIXEL) * ::GetDeviceCaps(hDC, PLANES)) < 4) {
                ::MessageBox(NULL,
                             TEXT("Sorry, but this release of Java requires at least 16 colors"),
                             TEXT("AWT Initialization Error"),
                             MB_ICONHAND | MB_APPLMODAL);
                ::DeleteDC(hDC);
                JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
                JNU_ThrowByName(env, "java/lang/InternalError",
                                "unsupported screen depth");
                return FALSE;
        }
    ::ReleaseDC(NULL, hDC);
        ///////////////////////////////////////////////////////////////////////////

    tk.m_localPump = localPump;
    tk.m_mainThreadId = ::GetCurrentThreadId();

    /*
     * Create the one-and-only toolkit window.  This window isn't
     * displayed, but is used to route messages to this thread.
     */
    tk.m_toolkitHWnd = tk.CreateToolkitWnd(TEXT("theAwtToolkitWindow"));
    DASSERT(tk.m_toolkitHWnd != NULL);

    /*
     * Setup a GetMessage filter to watch all messages coming out of our
     * queue from PreProcessMsg().
     */
    tk.m_hGetMessageHook = ::SetWindowsHookEx(WH_GETMESSAGE,
                                              (HOOKPROC)GetMessageFilter,
                                              0, tk.m_mainThreadId);

    awt_dnd_initialize();

    return TRUE;
}

BOOL AwtToolkit::Dispose() {
    DTRACE_PRINTLN("In AwtToolkit::Dispose()");

    AwtToolkit& tk = AwtToolkit::GetInstance();

    if (!tk.m_isActive || tk.m_mainThreadId != ::GetCurrentThreadId()) {
        return FALSE;
    }

    tk.m_isActive = FALSE;

    awt_dnd_uninitialize();
    awt_clipboard_uninitialize((JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2));

    AwtObjectList::Cleanup();
    AwtFont::Cleanup();

    if (tk.m_inputMethodHWnd != NULL) {
        ::SendMessage(tk.m_inputMethodHWnd, WM_IME_CONTROL, IMC_OPENSTATUSWINDOW, 0);
    }
    tk.m_inputMethodHWnd = NULL;

    // wait for any messages to be processed, in particular,
    // all WM_AWT_DELETEOBJECT messages that delete components; no
    // new messages will appear as all the windows except toolkit
    // window are unsubclassed and destroyed
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    HWND toolkitHWndToDestroy = tk.m_toolkitHWnd;
    tk.m_toolkitHWnd = 0;
    VERIFY(::DestroyWindow(toolkitHWndToDestroy) != NULL);

    tk.UnregisterClass();

    ::UnhookWindowsHookEx(tk.m_hGetMessageHook);

    tk.m_mainThreadId = 0;

    delete tk.m_cmdIDs;

    ::CloseHandle(m_waitEvent);

    ComCtl32Util::GetInstance().FreeLibraries();

    tk.m_isDisposed = TRUE;

    return TRUE;
}

void AwtToolkit::SetDynamicLayout(BOOL dynamic) {
    m_isDynamicLayoutSet = dynamic;
}

BOOL AwtToolkit::IsDynamicLayoutSet() {
    return m_isDynamicLayoutSet;
}

BOOL AwtToolkit::IsDynamicLayoutSupported() {
    // SPI_GETDRAGFULLWINDOWS is only supported on Win95 if
    // Windows Plus! is installed.  Otherwise, box frame resize.
    BOOL fullWindowDragEnabled = FALSE;
    int result = 0;
    result = ::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0,
                                  &fullWindowDragEnabled, 0);

    return (fullWindowDragEnabled && (result != 0));
}

BOOL AwtToolkit::IsDynamicLayoutActive() {
    return (IsDynamicLayoutSet() && IsDynamicLayoutSupported());
}

ATOM AwtToolkit::RegisterClass() {
    WNDCLASS  wc;

    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = AwtToolkit::GetInstance().GetModuleHandle(),
    wc.hIcon         = AwtToolkit::GetInstance().GetAwtIcon();
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szAwtToolkitClassName;

    ATOM ret = ::RegisterClass(&wc);
    DASSERT(ret != NULL);
    return ret;
}

void AwtToolkit::UnregisterClass() {
    VERIFY(::UnregisterClass(szAwtToolkitClassName, AwtToolkit::GetInstance().GetModuleHandle()));
}

/*
 * Structure holding the information to create a component. This packet is
 * sent to the toolkit window.
 */
struct ComponentCreatePacket {
    void* hComponent;
    void* hParent;
    void (*factory)(void*, void*);
};

/*
 * Create an AwtXxxx component using a given factory function
 * Implemented by sending a message to the toolkit window to invoke the
 * factory function from that thread
 */
void AwtToolkit::CreateComponent(void* component, void* parent,
                                 ComponentFactory compFactory, BOOL isParentALocalReference)
{
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);

    /* Since Local references are not valid in another Thread, we need to
       create a global reference before we send this to the Toolkit thread.
       In some cases this method is called with parent being a native
       malloced struct so we cannot and do not need to create a Global
       Reference from it. This is indicated by isParentALocalReference */

    jobject gcomponent = env->NewGlobalRef((jobject)component);
    jobject gparent;
    if (isParentALocalReference) gparent = env->NewGlobalRef((jobject)parent);
    ComponentCreatePacket ccp = { gcomponent,
                                  isParentALocalReference == TRUE ?  gparent : parent,
                                   compFactory };
    AwtToolkit::GetInstance().SendMessage(WM_AWT_COMPONENT_CREATE, 0,
                                          (LPARAM)&ccp);
    env->DeleteGlobalRef(gcomponent);
    if (isParentALocalReference) env->DeleteGlobalRef(gparent);
}

/*
 * Destroy an HWND that was created in the toolkit thread. Can be used on
 * Components and the toolkit window itself.
 */
void AwtToolkit::DestroyComponentHWND(HWND hwnd)
{
    if (!::IsWindow(hwnd)) {
        return;
    }

    AwtToolkit& tk = AwtToolkit::GetInstance();
    if ((tk.m_lastMouseOver != NULL) &&
        (tk.m_lastMouseOver->GetHWnd() == hwnd))
    {
        tk.m_lastMouseOver = NULL;
    }

    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)NULL);
    tk.SendMessage(WM_AWT_DESTROY_WINDOW, (WPARAM)hwnd, 0);
}

#ifndef SPY_MESSAGES
#define SpyWinMessage(hwin,msg,str)
#else
void SpyWinMessage(HWND hwnd, UINT message, LPCTSTR szComment);
#endif

/*
 * An AwtToolkit window is just a means of routing toolkit messages to here.
 */
LRESULT CALLBACK AwtToolkit::WndProc(HWND hWnd, UINT message,
                                     WPARAM wParam, LPARAM lParam)
{
    TRY;

    JNIEnv *env = GetEnv();
    JNILocalFrame lframe(env, 10);

    SpyWinMessage(hWnd, message, TEXT("AwtToolkit"));

    AwtToolkit::GetInstance().eventNumber++;
    /*
     * Awt widget creation messages are routed here so that all
     * widgets are created on the main thread.  Java allows widgets
     * to live beyond their creating thread -- by creating them on
     * the main thread, a widget can always be properly disposed.
     */
    switch (message) {
      case WM_AWT_EXECUTE_SYNC: {
          jobject peerObject = (jobject)wParam;
          AwtObject* object = (AwtObject *)JNI_GET_PDATA(peerObject);
          DASSERT( !IsBadReadPtr(object, sizeof(AwtObject)));
          AwtObject::ExecuteArgs *args = (AwtObject::ExecuteArgs *)lParam;
          DASSERT(!IsBadReadPtr(args, sizeof(AwtObject::ExecuteArgs)));
          LRESULT result = 0;
          if (object != NULL)
          {
              result = object->WinThreadExecProc(args);
          }
          env->DeleteGlobalRef(peerObject);
          return result;
      }
      case WM_AWT_COMPONENT_CREATE: {
          ComponentCreatePacket* ccp = (ComponentCreatePacket*)lParam;
          DASSERT(ccp->factory != NULL);
          DASSERT(ccp->hComponent != NULL);
          (*ccp->factory)(ccp->hComponent, ccp->hParent);
          return 0;
      }
      case WM_AWT_DESTROY_WINDOW: {
          /* Destroy widgets from this same thread that created them */
          VERIFY(::DestroyWindow((HWND)wParam) != NULL);
          return 0;
      }
      case WM_AWT_DISPOSE: {
          BOOL canDispose = TRUE;
          CriticalSection &syncCS = AwtToolkit::GetInstance().GetSyncCS();
          int shouldEnterCriticalSection = (int)lParam;
          if (shouldEnterCriticalSection == 1) {
              canDispose = syncCS.TryEnter();
          }
          if (canDispose) {
              AwtObject *o = (AwtObject *)wParam;
              o->Dispose();
              if (shouldEnterCriticalSection) {
                  syncCS.Leave();
              }
          } else {
              AwtToolkit::GetInstance().PostMessage(WM_AWT_DISPOSE, wParam, lParam);
          }
          return 0;
      }
      case WM_AWT_DELETEOBJECT: {
          AwtObject *p = (AwtObject *)wParam;
          if (p->CanBeDeleted()) {
              // all the messages for this component are processed, so
              // it can be deleted
              delete p;
          } else {
              // postpone deletion, waiting for all the messages for this
              // component to be processed
              AwtToolkit::GetInstance().PostMessage(WM_AWT_DELETEOBJECT, wParam, (LPARAM)0);
          }
          return 0;
      }
      case WM_AWT_OBJECTLISTCLEANUP: {
          AwtObjectList::Cleanup();
          return 0;
      }
      case WM_AWT_D3D_CREATE_DEVICE: {
          DDraw *ddObject = (DDraw*)wParam;
          if (ddObject != NULL) {
              ddObject->InitD3DContext();
          }
          return 0;
      }
      case WM_AWT_D3D_RELEASE_DEVICE: {
          DDraw *ddObject = (DDraw*)wParam;
          if (ddObject != NULL) {
              ddObject->ReleaseD3DContext();
          }
          return 0;
      }
      case WM_SYSCOLORCHANGE: {

          jclass systemColorClass = env->FindClass("java/awt/SystemColor");
          DASSERT(systemColorClass);

          jmethodID mid = env->GetStaticMethodID(systemColorClass, "updateSystemColors", "()V");
          DASSERT(mid);

          env->CallStaticVoidMethod(systemColorClass, mid);

          /* FALL THROUGH - NO BREAK */
      }

      case WM_SETTINGCHANGE: {
          AwtWin32GraphicsDevice::ResetAllMonitorInfo();
          /* FALL THROUGH - NO BREAK */
      }
// Remove this define when we move to newer (XP) version of SDK.
#define WM_THEMECHANGED                 0x031A
      case WM_THEMECHANGED: {
          /* Upcall to WToolkit when user changes configuration.
           *
           * NOTE: there is a bug in Windows 98 and some older versions of
           * Windows NT (it seems to be fixed in NT4 SP5) where no
           * WM_SETTINGCHANGE is sent when any of the properties under
           * Control Panel -> Display are changed.  You must _always_ query
           * the system for these - you can't rely on cached values.
           */
          jobject peer = AwtToolkit::GetInstance().m_peer;
          if (peer != NULL) {
              env->CallVoidMethod(peer, AwtToolkit::windowsSettingChangeMID);
          }
          return 0;
      }
      case WM_TIMER: {
          // Create an artifical MouseExit message if the mouse left to
          // a non-java window (bad mouse!)
          POINT pt;
          AwtToolkit& tk = AwtToolkit::GetInstance();
          if (::GetCursorPos(&pt)) {
              HWND hWndOver = ::WindowFromPoint(pt);
              AwtComponent * last_M;
              if ( AwtComponent::GetComponent(hWndOver) == NULL && tk.m_lastMouseOver != NULL ) {
                  last_M = tk.m_lastMouseOver;
                  // translate point from screen to target window
                  MapWindowPoints(HWND_DESKTOP, last_M->GetHWnd(), &pt, 1);
                  last_M->SendMessage(WM_AWT_MOUSEEXIT,
                                      GetMouseKeyState(),
                                      POINTTOPOINTS(pt));
                  tk.m_lastMouseOver = 0;
              }
          }
          if (tk.m_lastMouseOver == NULL && tk.m_timer != 0) {
              VERIFY(::KillTimer(tk.m_toolkitHWnd, tk.m_timer));
              tk.m_timer = 0;
          }
          return 0;
      }
      case WM_DESTROYCLIPBOARD: {
          if (!AwtClipboard::IsGettingOwnership())
              AwtClipboard::LostOwnership((JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2));
          return 0;
      }
      case WM_CHANGECBCHAIN: {
          AwtClipboard::WmChangeCbChain(wParam, lParam);
          return 0;
      }
      case WM_DRAWCLIPBOARD: {
          AwtClipboard::WmDrawClipboard((JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2), wParam, lParam);
          return 0;
      }
      case WM_AWT_LIST_SETMULTISELECT: {
          jobject peerObject = (jobject)wParam;
          AwtList* list = (AwtList *)JNI_GET_PDATA(peerObject);
          DASSERT( !IsBadReadPtr(list, sizeof(AwtObject)));
          list->SetMultiSelect(static_cast<BOOL>(lParam));
          return 0;
      }

      // Special awt message to call Imm APIs.
      // ImmXXXX() API must be used in the main thread.
      // In other thread these APIs does not work correctly even if
      // it returs with no error. (This restriction is not documented)
      // So we must use thse messages to call these APIs in main thread.
      case WM_AWT_CREATECONTEXT: {
        return reinterpret_cast<LRESULT>(
            reinterpret_cast<void*>(ImmCreateContext()));
      }
      case WM_AWT_DESTROYCONTEXT: {
          ImmDestroyContext((HIMC)wParam);
          return 0;
      }
      case WM_AWT_ASSOCIATECONTEXT: {
          AwtComponent *p = AwtComponent::GetComponent((HWND)wParam);
          p->ImmAssociateContext((HIMC)lParam);
          return 0;
      }
      case WM_AWT_ENDCOMPOSITION: {
          /*right now we just cancel the composition string
          may need to commit it in the furture
          Changed to commit it according to the flag 10/29/98*/
          ImmNotifyIME((HIMC)wParam, NI_COMPOSITIONSTR,
                       (lParam ? CPS_COMPLETE : CPS_CANCEL), 0);
          return 0;
      }
      case WM_AWT_SETCONVERSIONSTATUS: {
          DWORD cmode;
          DWORD smode;
          ImmGetConversionStatus((HIMC)wParam, (LPDWORD)&cmode, (LPDWORD)&smode);
          ImmSetConversionStatus((HIMC)wParam, (DWORD)LOWORD(lParam), smode);
          return 0;
      }
      case WM_AWT_GETCONVERSIONSTATUS: {
          DWORD cmode;
          DWORD smode;
          ImmGetConversionStatus((HIMC)wParam, (LPDWORD)&cmode, (LPDWORD)&smode);
          return cmode;
      }
      case WM_AWT_ACTIVATEKEYBOARDLAYOUT: {
          if (wParam && g_bUserHasChangedInputLang) {
              // Input language has been changed since the last WInputMethod.getNativeLocale()
              // call.  So let's honor the user's selection.
              // Note: we need to check this flag inside the toolkit thread to synchronize access
              // to the flag.
              return FALSE;
          }

          if (lParam == (LPARAM)::GetKeyboardLayout(0)) {
              // already active
              return FALSE;
          }

          // Since ActivateKeyboardLayout does not post WM_INPUTLANGCHANGEREQUEST,
          // we explicitly need to do the same thing here.
          static BYTE keyboardState[AwtToolkit::KB_STATE_SIZE];
          AwtToolkit::GetKeyboardState(keyboardState);
          WORD ignored;
          ::ToAscii(VK_SPACE, ::MapVirtualKey(VK_SPACE, 0),
                    keyboardState, &ignored, 0);

          return (LRESULT)activateKeyboardLayout((HKL)lParam);
      }
      case WM_AWT_OPENCANDIDATEWINDOW: {
          jobject peerObject = (jobject)wParam;
          AwtComponent* p = (AwtComponent*)JNI_GET_PDATA(peerObject);
          DASSERT( !IsBadReadPtr(p, sizeof(AwtObject)));
          // fix for 4805862: use GET_X_LPARAM and GET_Y_LPARAM macros
          // instead of LOWORD and HIWORD
          p->OpenCandidateWindow(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
          env->DeleteGlobalRef(peerObject);
          return 0;
      }

      /*
       * send this message via ::SendMessage() and the MPT will acquire the
       * HANDLE synchronized with the sender's thread. The HANDLE must be
       * signalled or deadlock may occur between the MPT and the caller.
       */

      case WM_AWT_WAIT_FOR_SINGLE_OBJECT: {
        return ::WaitForSingleObject((HANDLE)lParam, INFINITE);
      }
      case WM_AWT_INVOKE_METHOD: {
        return (LRESULT)(*(void*(*)(void*))wParam)((void *)lParam);
      }
      case WM_AWT_INVOKE_VOID_METHOD: {
        return (LRESULT)(*(void*(*)(void))wParam)();
      }

      case WM_AWT_SETOPENSTATUS: {
          ImmSetOpenStatus((HIMC)wParam, (BOOL)lParam);
          return 0;
      }
      case WM_AWT_GETOPENSTATUS: {
          return (DWORD)ImmGetOpenStatus((HIMC)wParam);
      }
      case WM_DISPLAYCHANGE: {
          AwtCursor::DirtyAllCustomCursors();

          // Reinitialize screens
          initScreens(env);

          // Invalidate current DDraw object; the object must be recreated
          // when we first try to create a new DDraw surface.  Note that we
          // don't recreate the ddraw object directly here because of
          // multi-threading issues; we'll just leave that to the first
          // time an object tries to create a DDraw surface under the new
          // display depth (which will happen after the displayChange event
          // propagation at the end of this case).
          if (DDCanReplaceSurfaces(NULL)) {
              DDInvalidateDDInstance(NULL);
          }

          // Notify Java side - call WToolkit.displayChanged()
          jclass clazz = env->FindClass("sun/awt/windows/WToolkit");
          env->CallStaticVoidMethod(clazz, AwtToolkit::displayChangeMID);

          GetInstance().m_displayChanged = TRUE;

          ::PostMessage(HWND_BROADCAST, WM_PALETTEISCHANGING, NULL, NULL);
          break;
      }
      case WM_AWT_SETCURSOR: {
          ::SetCursor((HCURSOR)wParam);
          return TRUE;
      }
      /* Session management */
      case WM_QUERYENDSESSION: {
          /* Shut down cleanly */
          if (JVM_RaiseSignal(SIGTERM)) {
              AwtToolkit::GetInstance().m_vmSignalled = TRUE;
          }
          return TRUE;
      }
      case WM_ENDSESSION: {
          // Keep pumping messages until the shutdown sequence halts the VM,
          // or we exit the MessageLoop because of a WM_QUIT message
          AwtToolkit& tk = AwtToolkit::GetInstance();

          // if WM_QUERYENDSESSION hasn't successfully raised SIGTERM
          // we ignore the ENDSESSION message
          if (!tk.m_vmSignalled) {
              return 0;
          }
          tk.MessageLoop(AwtToolkit::PrimaryIdleFunc,
                         AwtToolkit::CommonPeekMessageFunc);

          // Dispose here instead of in eventLoop so that we don't have
          // to return from the WM_ENDSESSION handler.
          tk.Dispose();

          // Never return. The VM will halt the process.
          hang_if_shutdown();

          // Should never get here.
          DASSERT(FALSE);
          break;
      }
      case WM_SYNC_WAIT:
          SetEvent(AwtToolkit::GetInstance().m_waitEvent);
          break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);

    CATCH_BAD_ALLOC_RET(0);
}

LRESULT CALLBACK AwtToolkit::GetMessageFilter(int code,
                                              WPARAM wParam, LPARAM lParam)
{
    TRY;

    if (code >= 0 && wParam == PM_REMOVE && lParam != 0) {
       if (AwtToolkit::GetInstance().PreProcessMsg(*(MSG*)lParam) !=
               mrPassAlong) {
           /* PreProcessMsg() wants us to eat it */
           ((MSG*)lParam)->message = WM_NULL;
       }
    }
    return ::CallNextHookEx(AwtToolkit::GetInstance().m_hGetMessageHook, code,
                            wParam, lParam);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * The main message loop
 */

const int AwtToolkit::EXIT_ENCLOSING_LOOP      = 0;
const int AwtToolkit::EXIT_ALL_ENCLOSING_LOOPS = -1;


/**
 * Called upon event idle to ensure that we have released any
 * CriticalSections that we took during window event processing.
 *
 * Note that this gets used more often than you would think; some
 * window moves actually happen over more than one event burst.  So,
 * for example, we might get a WINDOWPOSCHANGING event, then we
 * idle and release the lock here, then eventually we get the
 * WINDOWPOSCHANGED event.
 *
 * This method may be called from WToolkit.embeddedEventLoopIdleProcessing
 * if there is a separate event loop that must do the same CriticalSection
 * check.
 *
 * See bug #4526587 for more information.
 */
void VerifyWindowMoveLockReleased()
{
    if (windowMoveLockHeld) {
        windowMoveLockHeld = FALSE;
        windowMoveLock.Leave();
    }
}

UINT
AwtToolkit::MessageLoop(IDLEPROC lpIdleFunc,
                        PEEKMESSAGEPROC lpPeekMessageFunc)
{
    DTRACE_PRINTLN("AWT event loop started");

    DASSERT(lpIdleFunc != NULL);
    DASSERT(lpPeekMessageFunc != NULL);

    m_messageLoopResult = 0;
    while (!m_breakMessageLoop) {

        (*lpIdleFunc)();

        PumpWaitingMessages(lpPeekMessageFunc); /* pumps waiting messages */

        // Catch problems with windowMoveLock critical section.  In case we
        // misunderstood the way windows processes window move/resize
        // events, we don't want to hold onto the windowMoveLock CS forever.
        // If we've finished processing events for now, release the lock
        // if held.
        VerifyWindowMoveLockReleased();
    }
    if (m_messageLoopResult == EXIT_ALL_ENCLOSING_LOOPS)
        ::PostQuitMessage(EXIT_ALL_ENCLOSING_LOOPS);
    m_breakMessageLoop = FALSE;

    DTRACE_PRINTLN("AWT event loop ended");

    return m_messageLoopResult;
}

/*
 * Exit the enclosing message loop(s).
 *
 * The message will be ignored if Windows is currently is in an internal
 * message loop (such as a scroll bar drag). So we first send IDCANCEL and
 * WM_CANCELMODE messages to every Window on the thread.
 */
static BOOL CALLBACK CancelAllThreadWindows(HWND hWnd, LPARAM)
{
    TRY;

    ::SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), (LPARAM)hWnd);
    ::SendMessage(hWnd, WM_CANCELMODE, 0, 0);

    return TRUE;

    CATCH_BAD_ALLOC_RET(FALSE);
}

static void DoQuitMessageLoop(void* param) {
    int status = *static_cast<int*>(param);

    AwtToolkit::GetInstance().QuitMessageLoop(status);
}

void AwtToolkit::QuitMessageLoop(int status) {
    /*
     * Fix for 4623377.
     * Reinvoke QuitMessageLoop on the toolkit thread, so that
     * m_breakMessageLoop is accessed on a single thread.
     */
    if (!AwtToolkit::IsMainThread()) {
        InvokeFunction(DoQuitMessageLoop, &status);
        return;
    }

    /*
     * Fix for BugTraq ID 4445747.
     * EnumThreadWindows() is very slow during dnd on Win9X/ME.
     * This call is unnecessary during dnd, since we postpone processing of all
     * messages that can enter internal message loop until dnd is over.
     */
      if (status == EXIT_ALL_ENCLOSING_LOOPS) {
          ::EnumThreadWindows(MainThread(), (WNDENUMPROC)CancelAllThreadWindows,
                              0);
      }

    /*
     * Fix for 4623377.
     * Modal loop may not exit immediatelly after WM_CANCELMODE, so it still can
     * eat WM_QUIT message and the nested message loop will never exit.
     * The fix is to use AwtToolkit instance variables instead of WM_QUIT to
     * guarantee that we exit from the nested message loop when any possible
     * modal loop quits. In this case CancelAllThreadWindows is needed only to
     * ensure that the nested message loop exits quickly and doesn't wait until
     * a possible modal loop completes.
     */
    m_breakMessageLoop = TRUE;
    m_messageLoopResult = status;

    /*
     * Fix for 4683602.
     * Post an empty message, to wake up the toolkit thread
     * if it is currently in WaitMessage(),
     */
    PostMessage(WM_NULL);
}

/*
 * Called by the message loop to pump the message queue when there are
 * messages waiting. Can also be called anywhere to pump messages.
 */
BOOL AwtToolkit::PumpWaitingMessages(PEEKMESSAGEPROC lpPeekMessageFunc)
{
    MSG  msg;
    BOOL foundOne = FALSE;

    DASSERT(lpPeekMessageFunc != NULL);

    while (!m_breakMessageLoop && (*lpPeekMessageFunc)(msg)) {
        foundOne = TRUE;
        if (msg.message == WM_QUIT) {
            m_breakMessageLoop = TRUE;
            m_messageLoopResult = static_cast<UINT>(msg.wParam);
            if (m_messageLoopResult == EXIT_ALL_ENCLOSING_LOOPS)
                ::PostQuitMessage(static_cast<int>(msg.wParam));  // make sure all loops exit
            break;
        }
        else if (msg.message != WM_NULL) {
            /*
             * The AWT in standalone mode (that is, dynamically loaded from the
             * Java VM) doesn't have any translation tables to worry about, so
             * TranslateAccelerator isn't called.
             */

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
    return foundOne;
}

VOID CALLBACK
AwtToolkit::PrimaryIdleFunc() {
    AwtToolkit::SetBusy(FALSE);
    ::WaitMessage();               /* allow system to go idle */
    AwtToolkit::SetBusy(TRUE);
}

VOID CALLBACK
AwtToolkit::SecondaryIdleFunc() {
    ::WaitMessage();               /* allow system to go idle */
}

BOOL
AwtToolkit::CommonPeekMessageFunc(MSG& msg) {
    return ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
}

/*
 * Perform pre-processing on a message before it is translated &
 * dispatched.  Returns true to eat the message
 */
BOOL AwtToolkit::PreProcessMsg(MSG& msg)
{
    /*
     * Offer preprocessing first to the target component, then call out to
     * specific mouse and key preprocessor methods
     */
    AwtComponent* p = AwtComponent::GetComponent(msg.hwnd);
    if (p && p->PreProcessMsg(msg) == mrConsume)
        return TRUE;

    if ((msg.message >= WM_MOUSEFIRST && msg.message <= WM_AWT_MOUSELAST) ||
        (IS_WIN95 && !IS_WIN98 &&
                                msg.message == AwtComponent::Wheel95GetMsg()) ||
        (msg.message >= WM_NCMOUSEMOVE && msg.message <= WM_NCMBUTTONDBLCLK)) {
        if (PreProcessMouseMsg(p, msg)) {
            return TRUE;
        }
    }
    else if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) {
        if (PreProcessKeyMsg(p, msg))
            return TRUE;
    }
    return FALSE;
}

BOOL AwtToolkit::PreProcessMouseMsg(AwtComponent* p, MSG& msg)
{
    WPARAM mouseWParam;
    LPARAM mouseLParam;

    /*
     * Fix for BugTraq ID 4395290.
     * Do not synthesize mouse enter/exit events during drag-and-drop,
     * since it messes up LightweightDispatcher.
     */
    if (AwtDropTarget::IsLocalDnD()) {
        return FALSE;
    }

    if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_AWT_MOUSELAST ||
        (IS_WIN95 && !IS_WIN98 && msg.message == AwtComponent::Wheel95GetMsg()))
    {
        mouseWParam = msg.wParam;
        mouseLParam = msg.lParam;
    } else {
        mouseWParam = GetMouseKeyState();
    }

    /*
     * Get the window under the mouse, as it will be different if its
     * captured.
     */
    DWORD dwCurPos = ::GetMessagePos();
    DWORD dwScreenPos = dwCurPos;
    POINT curPos;
    // fix for 4805862
    // According to MSDN: do not use LOWORD and HIWORD macros to extract x and
    // y coordinates because these macros return incorrect results on systems
    // with multiple monitors (signed values are treated as unsigned)
    curPos.x = GET_X_LPARAM(dwCurPos);
    curPos.y = GET_Y_LPARAM(dwCurPos);
    HWND hWndFromPoint = ::WindowFromPoint(curPos);
    // hWndFromPoint == 0 if mouse is over a scrollbar
    AwtComponent* mouseComp =
        AwtComponent::GetComponent(hWndFromPoint);
    // Need extra copies for non-client area issues
    AwtComponent* mouseWheelComp = mouseComp;
    HWND hWndForWheel = hWndFromPoint;

    // If the point under the mouse isn't in the client area,
    // ignore it to maintain compatibility with Solaris (#4095172)
    RECT windowRect;
    ::GetClientRect(hWndFromPoint, &windowRect);
    POINT topLeft;
    topLeft.x = 0;
    topLeft.y = 0;
    ::ClientToScreen(hWndFromPoint, &topLeft);
    windowRect.top += topLeft.y;
    windowRect.bottom += topLeft.y;
    windowRect.left += topLeft.x;
    windowRect.right += topLeft.x;
    if ((curPos.y < windowRect.top) ||
        (curPos.y >= windowRect.bottom) ||
        (curPos.x < windowRect.left) ||
        (curPos.x >= windowRect.right)) {
        mouseComp = NULL;
        hWndFromPoint = NULL;
    }

    /*
     * Look for mouse transitions between windows & create
     * MouseExit & MouseEnter messages
     */
    if (mouseComp != m_lastMouseOver) {
        /*
         * Send the messages right to the windows so that they are in
         * the right sequence.
         */
        if (m_lastMouseOver) {
            dwCurPos = dwScreenPos;
            curPos.x = LOWORD(dwCurPos);
            curPos.y = HIWORD(dwCurPos);
            ::MapWindowPoints(HWND_DESKTOP, m_lastMouseOver->GetHWnd(),
                              &curPos, 1);
            mouseLParam = MAKELPARAM((WORD)curPos.x, (WORD)curPos.y);
            m_lastMouseOver->SendMessage(WM_AWT_MOUSEEXIT, mouseWParam,
                                         mouseLParam);
        }
        if (mouseComp) {
                dwCurPos = dwScreenPos;
                curPos.x = LOWORD(dwCurPos);
                curPos.y = HIWORD(dwCurPos);
                ::MapWindowPoints(HWND_DESKTOP, mouseComp->GetHWnd(),
                                  &curPos, 1);
                mouseLParam = MAKELPARAM((WORD)curPos.x, (WORD)curPos.y);
            mouseComp->SendMessage(WM_AWT_MOUSEENTER, mouseWParam,
                                   mouseLParam);
        }
        m_lastMouseOver = mouseComp;
    }

    /*
     * For MouseWheelEvents, hwnd must be changed to be the Component under
     * the mouse, not the Component with the input focus.
     */

    if (msg.message == WM_MOUSEWHEEL &&
        mouseWheelComp != NULL) { //i.e. mouse is over client area for this
                                  //window
        msg.hwnd = hWndForWheel;
    }
    else if (IS_WIN95 && !IS_WIN98 &&
             msg.message == AwtComponent::Wheel95GetMsg() &&
             mouseWheelComp != NULL) {

        // On Win95, mouse wheels are _always_ delivered to the top level
        // Frame.  Default behavior only takes place if the message's hwnd
        // remains that of the Frame.  We only want to change the hwnd if
        // we're changing it to a Component that DOESN'T handle the
        // mousewheel natively.

        if (!mouseWheelComp->InheritsNativeMouseWheelBehavior()) {
            DTRACE_PRINTLN("AwtT::PPMM: changing hwnd on 95");
            msg.hwnd = hWndForWheel;
        }
    }

    /*
     * Make sure we get at least one last chance to check for transitions
     * before we sleep
     */
    if (m_lastMouseOver && !m_timer) {
        m_timer = ::SetTimer(m_toolkitHWnd, IDT_AWT_MOUSECHECK, 200, 0);
    }
    return FALSE;  /* Now go ahead and process current message as usual */
}

BOOL AwtToolkit::PreProcessKeyMsg(AwtComponent* p, MSG& msg)
{
    // get keyboard state for use in AwtToolkit::GetKeyboardState
    CriticalSection::Lock       l(m_lockKB);
    ::GetKeyboardState(m_lastKeyboardState);
    return FALSE;
}

void *AwtToolkit::SyncCall(void *(*ftn)(void *), void *param) {
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (!IsMainThread()) {
        CriticalSection::Lock l(GetSyncCS());
        return (*ftn)(param);
    } else {
        return (*ftn)(param);
    }
}

void AwtToolkit::SyncCall(void (*ftn)(void *), void *param) {
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (!IsMainThread()) {
        CriticalSection::Lock l(GetSyncCS());
        (*ftn)(param);
    } else {
        (*ftn)(param);
    }
}

void *AwtToolkit::SyncCall(void *(*ftn)(void)) {
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (!IsMainThread()) {
        CriticalSection::Lock l(GetSyncCS());
        return (*ftn)();
    } else {
        return (*ftn)();
    }
}

void AwtToolkit::SyncCall(void (*ftn)(void)) {
    JNIEnv *env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
    if (!IsMainThread()) {
        CriticalSection::Lock l(GetSyncCS());
        (*ftn)();
    } else {
        (*ftn)();
    }
}

UINT AwtToolkit::CreateCmdID(AwtObject* object)
{
    return m_cmdIDs->Add(object);
}

void AwtToolkit::RemoveCmdID(UINT id)
{
    m_cmdIDs->Remove(id);
}

AwtObject* AwtToolkit::LookupCmdID(UINT id)
{
    return m_cmdIDs->Lookup(id);
}

HICON AwtToolkit::GetAwtIcon()
{
    return ::LoadIcon(GetModuleHandle(), TEXT("AWT_ICON"));
}

HICON AwtToolkit::GetAwtIconSm()
{
    static HICON defaultIconSm = NULL;
    static int prevSmx = 0;
    static int prevSmy = 0;

    int smx = GetSystemMetrics(SM_CXSMICON);
    int smy = GetSystemMetrics(SM_CYSMICON);

    // Fixed 6364216: LoadImage() may leak memory
    if (defaultIconSm == NULL || smx != prevSmx || smy != prevSmy) {
        defaultIconSm = (HICON)LoadImage(GetModuleHandle(), TEXT("AWT_ICON"), IMAGE_ICON, smx, smy, 0);
        prevSmx = smx;
        prevSmy = smy;
    }
    return defaultIconSm;
}

void AwtToolkit::SetHeapCheck(long flag) {
    if (flag) {
        printf("heap checking not supported with this build\n");
    }
}

void throw_if_shutdown(void) throw (awt_toolkit_shutdown)
{
    AwtToolkit::GetInstance().VerifyActive();
}
void hang_if_shutdown(void)
{
    try {
        AwtToolkit::GetInstance().VerifyActive();
    } catch (awt_toolkit_shutdown&) {
        // Never return. The VM will halt the process.
        ::WaitForSingleObject(::CreateEvent(NULL, TRUE, FALSE, NULL),
                              INFINITE);
        // Should never get here.
        DASSERT(FALSE);
    }
}

/*
 * Returns a reference to the class java.awt.Component.
 */
jclass
getComponentClass(JNIEnv *env)
{
    static jclass componentCls = NULL;

    // get global reference of java/awt/Component class (run only once)
    if (componentCls == NULL) {
        jclass componentClsLocal = env->FindClass("java/awt/Component");
        DASSERT(componentClsLocal != NULL);
        if (componentClsLocal == NULL) {
            /* exception already thrown */
            return NULL;
        }
        componentCls = (jclass)env->NewGlobalRef(componentClsLocal);
        env->DeleteLocalRef(componentClsLocal);
    }
    return componentCls;
}


/*
 * Returns a reference to the class java.awt.MenuComponent.
 */
jclass
getMenuComponentClass(JNIEnv *env)
{
    static jclass menuComponentCls = NULL;

    // get global reference of java/awt/MenuComponent class (run only once)
    if (menuComponentCls == NULL) {
        jclass menuComponentClsLocal = env->FindClass("java/awt/MenuComponent");
        DASSERT(menuComponentClsLocal != NULL);
        if (menuComponentClsLocal == NULL) {
            /* exception already thrown */
            return NULL;
        }
        menuComponentCls = (jclass)env->NewGlobalRef(menuComponentClsLocal);
        env->DeleteLocalRef(menuComponentClsLocal);
    }
    return menuComponentCls;
}

JNIEnv* AwtToolkit::m_env;
HANDLE AwtToolkit::m_thread;

void AwtToolkit::SetEnv(JNIEnv *env) {
    if (m_env != NULL) { // If already cashed (by means of embeddedInit() call).
        return;
    }
    m_thread = GetCurrentThread();
    m_env = env;
}

JNIEnv* AwtToolkit::GetEnv() {
    return (m_env == NULL || m_thread != GetCurrentThread()) ?
        (JNIEnv*)JNU_GetEnv(jvm, JNI_VERSION_1_2) : m_env;
}

/************************************************************************
 * Toolkit native methods
 */

extern "C" {

/*
 * Class:     java_awt_Toolkit
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_java_awt_Toolkit_initIDs(JNIEnv *env, jclass cls) {
    TRY;

    AwtToolkit::getDefaultToolkitMID =
        env->GetStaticMethodID(cls,"getDefaultToolkit","()Ljava/awt/Toolkit;");
    AwtToolkit::getFontMetricsMID =
        env->GetMethodID(cls, "getFontMetrics",
                         "(Ljava/awt/Font;)Ljava/awt/FontMetrics;");
        AwtToolkit::insetsMID =
                env->GetMethodID(env->FindClass("java/awt/Insets"), "<init>", "(IIII)V");

    DASSERT(AwtToolkit::getDefaultToolkitMID != NULL);
    DASSERT(AwtToolkit::getFontMetricsMID != NULL);
        DASSERT(AwtToolkit::insetsMID != NULL);

    CATCH_BAD_ALLOC;
}


} /* extern "C" */

/************************************************************************
 * WToolkit native methods
 */

extern "C" {

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_initIDs(JNIEnv *env, jclass cls)
{
    TRY;

    AwtToolkit::windowsSettingChangeMID =
        env->GetMethodID(cls, "windowsSettingChange", "()V");
    DASSERT(AwtToolkit::windowsSettingChangeMID != 0);

    AwtToolkit::displayChangeMID =
    env->GetStaticMethodID(cls, "displayChanged", "()V");
    DASSERT(AwtToolkit::displayChangeMID != 0);

    // Set various global IDs needed by JAWT code.  Note: these
    // variables cannot be set by JAWT code directly due to
    // different permissions that that code may be run under
    // (bug 4796548).  It would be nice to initialize these
    // variables lazily, but given the minimal number of calls
    // for this, it seems simpler to just do it at startup with
    // negligible penalty.
    jclass sDataClassLocal = env->FindClass("sun/java2d/SurfaceData");
    DASSERT(sDataClassLocal != 0);
    jclass vImgClassLocal = env->FindClass("sun/awt/image/SunVolatileImage");
    DASSERT(vImgClassLocal != 0);
    jclass vSMgrClassLocal =
        env->FindClass("sun/awt/image/VolatileSurfaceManager");
    DASSERT(vSMgrClassLocal != 0);
    jclass componentClassLocal = env->FindClass("java/awt/Component");
    DASSERT(componentClassLocal != 0);
    jclass w32ossdClassLocal =
        env->FindClass("sun/java2d/windows/Win32OffScreenSurfaceData");
    DASSERT(w32ossdClassLocal != 0);
    jawtSMgrID = env->GetFieldID(vImgClassLocal, "volSurfaceManager",
                                 "Lsun/awt/image/VolatileSurfaceManager;");
    DASSERT(jawtSMgrID != 0);
    jawtSDataID = env->GetFieldID(vSMgrClassLocal, "sdCurrent",
                                  "Lsun/java2d/SurfaceData;");
    DASSERT(jawtSDataID != 0);
    jawtPDataID = env->GetFieldID(sDataClassLocal, "pData", "J");
    DASSERT(jawtPDataID != 0);

    // Save these classes in global references for later use
    jawtVImgClass = (jclass)env->NewGlobalRef(vImgClassLocal);
    jawtComponentClass = (jclass)env->NewGlobalRef(componentClassLocal);
    jawtW32ossdClass = (jclass)env->NewGlobalRef(w32ossdClassLocal);

    CATCH_BAD_ALLOC;
}


/*
 * Class:     sun_awt_windows_Toolkit
 * Method:    disableCustomPalette
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_disableCustomPalette(JNIEnv *env, jclass cls) {
    AwtPalette::DisableCustomPalette();
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    embeddedInit
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_windows_WToolkit_embeddedInit(JNIEnv *env, jclass cls)
{
    TRY;

    AwtToolkit::SetEnv(env);

    return AwtToolkit::GetInstance().Initialize(FALSE);

    CATCH_BAD_ALLOC_RET(JNI_FALSE);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    embeddedDispose
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_windows_WToolkit_embeddedDispose(JNIEnv *env, jclass cls)
{
    TRY;

    BOOL retval = AwtToolkit::GetInstance().Dispose();
    AwtToolkit::GetInstance().SetPeer(env, NULL);
    return retval;

    CATCH_BAD_ALLOC_RET(JNI_FALSE);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    embeddedEventLoopIdleProcessing
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_embeddedEventLoopIdleProcessing(JNIEnv *env,
    jobject self)
{
    VerifyWindowMoveLockReleased();
}


/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    init
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_windows_WToolkit_init(JNIEnv *env, jobject self)
{
    TRY;

    AwtToolkit::SetEnv(env);

    AwtToolkit::GetInstance().SetPeer(env, self);

    // This call will fail if the Toolkit was already initialized.
    // In that case, we don't want to start another message pump.
    return AwtToolkit::GetInstance().Initialize(TRUE);

    CATCH_BAD_ALLOC_RET(FALSE);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    eventLoop
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_eventLoop(JNIEnv *env, jobject self)
{
    TRY;

    DASSERT(AwtToolkit::GetInstance().localPump());

    AwtToolkit::SetBusy(TRUE);

    AwtToolkit::GetInstance().MessageLoop(AwtToolkit::PrimaryIdleFunc,
                                          AwtToolkit::CommonPeekMessageFunc);

    AwtToolkit::GetInstance().Dispose();

    AwtToolkit::SetBusy(FALSE);

    /*
     * IMPORTANT NOTES:
     *   The AwtToolkit has been destructed by now.
     * DO NOT CALL any method of AwtToolkit!!!
     */

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    shutdown
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_shutdown(JNIEnv *env, jobject self)
{
    TRY;

    AwtToolkit& tk = AwtToolkit::GetInstance();

    tk.QuitMessageLoop(AwtToolkit::EXIT_ALL_ENCLOSING_LOOPS);

    while (!tk.IsDisposed()) {
        Sleep(100);
    }

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    startSecondaryEventLoop
 * Signature: ()V;
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_startSecondaryEventLoop(
    JNIEnv *env,
    jclass)
{
    TRY;

    DASSERT(AwtToolkit::MainThread() == ::GetCurrentThreadId());

    AwtToolkit::GetInstance().MessageLoop(AwtToolkit::SecondaryIdleFunc,
                                          AwtToolkit::CommonPeekMessageFunc);

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    quitSecondaryEventLoop
 * Signature: ()V;
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_quitSecondaryEventLoop(
    JNIEnv *env,
    jclass)
{
    TRY;

    AwtToolkit::GetInstance().QuitMessageLoop(AwtToolkit::EXIT_ENCLOSING_LOOP);

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    makeColorModel
 * Signature: ()Ljava/awt/image/ColorModel;
 */
JNIEXPORT jobject JNICALL
Java_sun_awt_windows_WToolkit_makeColorModel(JNIEnv *env, jclass cls)
{
    TRY;

    return AwtWin32GraphicsDevice::GetColorModel(env, JNI_FALSE,
        AwtWin32GraphicsDevice::GetDefaultDeviceIndex());

    CATCH_BAD_ALLOC_RET(NULL);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    getMaximumCursorColors
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WToolkit_getMaximumCursorColors(JNIEnv *env, jobject self)
{
    TRY;

    HDC hIC = ::CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);

    int nColor = 256;
    switch (::GetDeviceCaps(hIC, BITSPIXEL) * ::GetDeviceCaps(hIC, PLANES)) {
        case 1:         nColor = 2;             break;
        case 4:         nColor = 16;            break;
        case 8:         nColor = 256;           break;
        case 16:        nColor = 65536;         break;
        case 24:        nColor = 16777216;      break;
    }
    ::DeleteDC(hIC);
    return nColor;

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    getScreenWidth
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WToolkit_getScreenWidth(JNIEnv *env, jobject self)
{
    TRY;

    return ::GetSystemMetrics(SM_CXSCREEN);

    CATCH_BAD_ALLOC_RET(0);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    getScreenHeight
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_windows_WToolkit_getScreenHeight(JNIEnv *env, jobject self)
{
    TRY;

    return ::GetSystemMetrics(SM_CYSCREEN);

    CATCH_BAD_ALLOC_RET(0);
}


/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    getSreenInsets
 * Signature: (I)Ljava/awt/Insets;
 */
JNIEXPORT jobject JNICALL
Java_sun_awt_windows_WToolkit_getScreenInsets(JNIEnv *env,
                                              jobject self,
                                              jint screen)
{
    jobject insets = NULL;
    RECT rRW;
    MONITOR_INFO *miInfo;

    TRY;

/* if primary display */
   if (screen == 0) {
      if (::SystemParametersInfo(SPI_GETWORKAREA,0,(void *) &rRW,0) == TRUE) {
          insets = env->NewObject(env->FindClass("java/awt/Insets"),
             AwtToolkit::insetsMID,
             rRW.top,
             rRW.left,
             ::GetSystemMetrics(SM_CYSCREEN) - rRW.bottom,
             ::GetSystemMetrics(SM_CXSCREEN) - rRW.right);
      }
    }

/* if additional display */
    else {
        miInfo = AwtWin32GraphicsDevice::GetMonitorInfo(screen);
        if (miInfo) {
            insets = env->NewObject(env->FindClass("java/awt/Insets"),
                AwtToolkit::insetsMID,
                miInfo->rWork.top    - miInfo->rMonitor.top,
                miInfo->rWork.left   - miInfo->rMonitor.left,
                miInfo->rMonitor.bottom - miInfo->rWork.bottom,
                miInfo->rMonitor.right - miInfo->rWork.right);
        }
    }

    if (safe_ExceptionOccurred(env)) {
        return 0;
    }
    return insets;

    CATCH_BAD_ALLOC_RET(NULL);
}


/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    nativeSync
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_nativeSync(JNIEnv *env, jobject self)
{
    TRY;

    // Synchronize both GDI and DDraw
    VERIFY(::GdiFlush());
    DDSync();

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    beep
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_beep(JNIEnv *env, jobject self)
{
    TRY;

    VERIFY(::MessageBeep(MB_OK));

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    getLockingKeyStateNative
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_windows_WToolkit_getLockingKeyStateNative(JNIEnv *env, jobject self, jint javaKey)
{
    TRY;

    UINT windowsKey, modifiers;
    AwtComponent::JavaKeyToWindowsKey(javaKey, &windowsKey, &modifiers);

    if (windowsKey == 0) {
        JNU_ThrowByName(env, "java/lang/UnsupportedOperationException", "Keyboard doesn't have requested key");
        return JNI_FALSE;
    }

    // low order bit in keyboardState indicates whether the key is toggled
    BYTE keyboardState[AwtToolkit::KB_STATE_SIZE];
    AwtToolkit::GetKeyboardState(keyboardState);
    return keyboardState[windowsKey] & 0x01;

    CATCH_BAD_ALLOC_RET(JNI_FALSE);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    setLockingKeyStateNative
 * Signature: (IZ)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_setLockingKeyStateNative(JNIEnv *env, jobject self, jint javaKey, jboolean state)
{
    TRY;

    UINT windowsKey, modifiers;
    AwtComponent::JavaKeyToWindowsKey(javaKey, &windowsKey, &modifiers);

    if (windowsKey == 0) {
        JNU_ThrowByName(env, "java/lang/UnsupportedOperationException", "Keyboard doesn't have requested key");
        return;
    }

    // if the key isn't in the desired state yet, simulate key events to get there
    // low order bit in keyboardState indicates whether the key is toggled
    BYTE keyboardState[AwtToolkit::KB_STATE_SIZE];
    AwtToolkit::GetKeyboardState(keyboardState);
    if ((keyboardState[windowsKey] & 0x01) != state) {
        ::keybd_event(windowsKey, 0, 0, 0);
        ::keybd_event(windowsKey, 0, KEYEVENTF_KEYUP, 0);
    }

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    loadSystemColors
 * Signature: ([I)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_loadSystemColors(JNIEnv *env, jobject self,
                                               jintArray colors)
{
    TRY;

    static int indexMap[] = {
        COLOR_DESKTOP, /* DESKTOP */
        COLOR_ACTIVECAPTION, /* ACTIVE_CAPTION */
        COLOR_CAPTIONTEXT, /* ACTIVE_CAPTION_TEXT */
        COLOR_ACTIVEBORDER, /* ACTIVE_CAPTION_BORDER */
        COLOR_INACTIVECAPTION, /* INACTIVE_CAPTION */
        COLOR_INACTIVECAPTIONTEXT, /* INACTIVE_CAPTION_TEXT */
        COLOR_INACTIVEBORDER, /* INACTIVE_CAPTION_BORDER */
        COLOR_WINDOW, /* WINDOW */
        COLOR_WINDOWFRAME, /* WINDOW_BORDER */
        COLOR_WINDOWTEXT, /* WINDOW_TEXT */
        COLOR_MENU, /* MENU */
        COLOR_MENUTEXT, /* MENU_TEXT */
        COLOR_WINDOW, /* TEXT */
        COLOR_WINDOWTEXT, /* TEXT_TEXT */
        COLOR_HIGHLIGHT, /* TEXT_HIGHLIGHT */
        COLOR_HIGHLIGHTTEXT, /* TEXT_HIGHLIGHT_TEXT */
        COLOR_GRAYTEXT, /* TEXT_INACTIVE_TEXT */
        COLOR_3DFACE, /* CONTROL */
        COLOR_BTNTEXT, /* CONTROL_TEXT */
        COLOR_3DLIGHT, /* CONTROL_HIGHLIGHT */
        COLOR_3DHILIGHT, /* CONTROL_LT_HIGHLIGHT */
        COLOR_3DSHADOW, /* CONTROL_SHADOW */
        COLOR_3DDKSHADOW, /* CONTROL_DK_SHADOW */
        COLOR_SCROLLBAR, /* SCROLLBAR */
        COLOR_INFOBK, /* INFO */
        COLOR_INFOTEXT, /* INFO_TEXT */
    };

    jint colorLen = env->GetArrayLength(colors);
    jint* colorsPtr = NULL;
    try {
        colorsPtr = (jint *)env->GetPrimitiveArrayCritical(colors, 0);
        for (int i = 0; i < sizeof indexMap && i < colorLen; i++) {
            colorsPtr[i] = DesktopColor2RGB(indexMap[i]);
        }
    } catch (...) {
        if (colorsPtr != NULL) {
            env->ReleasePrimitiveArrayCritical(colors, colorsPtr, 0);
        }
        throw;
    }

    env->ReleasePrimitiveArrayCritical(colors, colorsPtr, 0);

    CATCH_BAD_ALLOC;
}

extern "C" JNIEXPORT jobject JNICALL DSGetComponent
    (JNIEnv* env, void* platformInfo)
{
    TRY;

    HWND hWnd = (HWND)platformInfo;
    if (!::IsWindow(hWnd))
        return NULL;

    AwtComponent* comp = AwtComponent::GetComponent(hWnd);
    if (comp == NULL)
        return NULL;

    return comp->GetTarget(env);

    CATCH_BAD_ALLOC_RET(NULL);
}

JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_postDispose(JNIEnv *env, jclass clazz)
{
#ifdef DEBUG
    TRY_NO_VERIFY;

    // If this method was called, that means runFinalizersOnExit is turned
    // on and the VM is exiting cleanly. We should signal the debug memory
    // manager to generate a leaks report.
    AwtDebugSupport::GenerateLeaksReport();

    CATCH_BAD_ALLOC;
#endif
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    setDynamicLayoutNative
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_windows_WToolkit_setDynamicLayoutNative(JNIEnv *env,
  jobject self, jboolean dynamic)
{
    TRY;

    AwtToolkit::GetInstance().SetDynamicLayout(dynamic);

    CATCH_BAD_ALLOC;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    isDynamicLayoutSupportedNative
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_windows_WToolkit_isDynamicLayoutSupportedNative(JNIEnv *env,
  jobject self)
{
    TRY;

    return (jboolean) AwtToolkit::GetInstance().IsDynamicLayoutSupported();

    CATCH_BAD_ALLOC_RET(FALSE);
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    printWindowsVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_sun_awt_windows_WToolkit_getWindowsVersion(JNIEnv *env, jclass cls)
{
    TRY;

    WCHAR szVer[128];

    DWORD version = ::GetVersion();
    swprintf(szVer, L"0x%x = %ld", version, version);
    int l = lstrlen(szVer);

    if (IS_WIN95) {
        if (IS_WIN98) {
            if (IS_WINME) {
                swprintf(szVer + l, L" (Windows ME)");
            } else {
                swprintf(szVer + l, L" (Windows 98)");
            }
        } else {
            swprintf(szVer + l, L" (Windows 95)");
        }
    } else if (IS_NT) {
        if (IS_WIN2000) {
            if (IS_WINXP) {
                if (IS_WINVISTA) {
                    swprintf(szVer + l, L" (Windows Vista)");
                } else {
                    swprintf(szVer + l, L" (Windows XP)");
                }
            } else {
                swprintf(szVer + l, L" (Windows 2000)");
            }
        } else {
            swprintf(szVer + l, L" (Windows NT)");
        }
    } else {
        swprintf(szVer + l, L" (Unknown)");
    }

    return JNU_NewStringPlatform(env, szVer);

    CATCH_BAD_ALLOC_RET(NULL);
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_windows_WToolkit_syncNativeQueue(JNIEnv *env, jobject self, jlong timeout)
{
    AwtToolkit & tk = AwtToolkit::GetInstance();
    DWORD eventNumber = tk.eventNumber;
    tk.PostMessage(WM_SYNC_WAIT, 0, 0);
    ::WaitForSingleObject(tk.m_waitEvent, INFINITE);
    DWORD newEventNumber = tk.eventNumber;
    return (newEventNumber - eventNumber) > 2;
}

/*
 * Class:     sun_awt_windows_WToolkit
 * Method:    isProtectedMode
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_sun_awt_windows_WToolkit_isProtectedMode(JNIEnv *env, jclass cls)
{
    TRY;

    if (!IS_WINVISTA) {
        return JNI_FALSE;
    }

    BOOL bIEIsProtectedModeProcess = FALSE;
    HINSTANCE hLibIEFrameDll = ::LoadLibrary(TEXT("IEFRAME.DLL"));

    typedef HRESULT (WINAPI IEIsProtectedModeProcessFunc)(BOOL *);

    if (hLibIEFrameDll != NULL)
    {
        IEIsProtectedModeProcessFunc *lpIEIsProtectedModeProcess =
            (IEIsProtectedModeProcessFunc*)GetProcAddress(hLibIEFrameDll,
                                                          "IEIsProtectedModeProcess");

        HRESULT hResult = E_FAIL;
        if (lpIEIsProtectedModeProcess != NULL) {
            hResult = lpIEIsProtectedModeProcess(&bIEIsProtectedModeProcess);
        }

        ::FreeLibrary(hLibIEFrameDll);

        if (SUCCEEDED(hResult) && bIEIsProtectedModeProcess) {
            return JNI_TRUE;
        }
    }

    return JNI_FALSE;

    CATCH_BAD_ALLOC_RET(JNI_FALSE);
}

} /* extern "C" */

/* Convert a Windows desktop color index into an RGB value. */
COLORREF DesktopColor2RGB(int colorIndex) {
    DWORD sysColor = ::GetSysColor(colorIndex);
    return ((GetRValue(sysColor)<<16) | (GetGValue(sysColor)<<8) |
            (GetBValue(sysColor)) | 0xff000000);
}


/*
 * Class:     sun_awt_SunToolkit
 * Method:    closeSplashScreen
 * Signature: ()V
 */
extern "C" JNIEXPORT void JNICALL
Java_sun_awt_SunToolkit_closeSplashScreen(JNIEnv *env, jclass cls)
{
    typedef void (*SplashClose_t)();
    HMODULE hSplashDll = GetModuleHandle(_T("splashscreen.dll"));
    if (!hSplashDll) {
        return; // dll not loaded
    }
    SplashClose_t splashClose = (SplashClose_t)GetProcAddress(hSplashDll,
        "SplashClose");
    if (splashClose) {
        splashClose();
    }
}
