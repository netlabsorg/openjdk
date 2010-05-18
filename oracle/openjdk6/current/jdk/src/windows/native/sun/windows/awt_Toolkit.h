/*
 * Copyright 1996-2007 Sun Microsystems, Inc.  All Rights Reserved.
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
 * The Toolkit class has two functions: it instantiates the AWT
 * ToolkitPeer's native methods, and provides the DLL's core functions.
 *
 * There are two ways this DLL can be used: either as a dynamically-
 * loaded Java native library from the interpreter, or by a Windows-
 * specific app.  The first manner requires that the Toolkit provide
 * all support needed so the app can function as a first-class Windows
 * app, while the second assumes that the app will provide that
 * functionality.  Which mode this DLL functions in is determined by
 * which initialization paradigm is used. If the Toolkit is constructed
 * normally, then the Toolkit will have its own pump. If it is explicitly
 * initialized for an embedded environment (via a static method on
 * sun.awt.windows.WToolkit), then it will rely on an external message
 * pump.
 *
 * The most basic functionality needed is a Windows message pump (also
 * known as a message loop).  When an Java app is started as a console
 * app by the interpreter, the Toolkit needs to provide that message
 * pump if the AWT is dynamically loaded.
 */

#ifndef AWT_TOOLKIT_H
#define AWT_TOOLKIT_H

#include "awt.h"
#include "awtmsg.h"
#include "awt_Multimon.h"
#include "Trace.h"

#include "sun_awt_windows_WToolkit.h"

class AwtObject;
class AwtDialog;
class AwtDropTarget;

typedef VOID (CALLBACK* IDLEPROC)(VOID);
typedef BOOL (CALLBACK* PEEKMESSAGEPROC)(MSG&);

/*
 * class JNILocalFrame
 * Push/PopLocalFrame helper
 */
class JNILocalFrame {
  public:
    INLINE JNILocalFrame(JNIEnv *env, int size) {
        m_env = env;
        int result = m_env->PushLocalFrame(size);
        if (result < 0) {
            DASSERT(FALSE);
            JNU_ThrowOutOfMemoryError(m_env, "Can't allocate localRefs");
        }
    }
    INLINE ~JNILocalFrame() { m_env->PopLocalFrame(NULL); }
  private:
    JNIEnv* m_env;
};

/*
 * class CriticalSection
 * ~~~~~ ~~~~~~~~~~~~~~~~
 * Lightweight intra-process thread synchronization. Can only be used with
 * other critical sections, and only within the same process.
 */
class CriticalSection {
  public:
    INLINE  CriticalSection() { ::InitializeCriticalSection(&rep);
                                ::InitializeCriticalSection(&tryrep);
                                tryEntered = 0; }
    INLINE ~CriticalSection() { ::DeleteCriticalSection(&rep);
                                ::DeleteCriticalSection(&tryrep); }

    class Lock {
      public:
        INLINE Lock(const CriticalSection& cs) : critSec(cs) {
            (const_cast<CriticalSection &>(critSec)).Enter();
        }
        INLINE ~Lock() {
            (const_cast<CriticalSection &>(critSec)).Leave();
        }
      private:
        const CriticalSection& critSec;
    };
    friend Lock;

  private:
    CRITICAL_SECTION rep;

    CRITICAL_SECTION tryrep;
    long tryEntered;

    CriticalSection(const CriticalSection&);
    const CriticalSection& operator =(const CriticalSection&);

  public:
    virtual void    Enter           (void)
    {
        ::EnterCriticalSection(&tryrep);
        tryEntered++;
        if (tryEntered == 1) {
            ::EnterCriticalSection(&rep);
            ::LeaveCriticalSection(&tryrep);
        } else {
            ::LeaveCriticalSection(&tryrep);
            ::EnterCriticalSection(&rep);
        }
    }
    // we cannot use ::TryEnterCriticalSection as it is not supported on Win9x/Me
    virtual BOOL    TryEnter        (void)
    {
        BOOL result = FALSE;
        ::EnterCriticalSection(&tryrep);
        if (tryEntered == 0) {
            ::EnterCriticalSection(&rep);
            tryEntered++;
            result = TRUE;
        }
        ::LeaveCriticalSection(&tryrep);
        return result;
    }
    virtual void    Leave           (void)
    {
        ::EnterCriticalSection(&tryrep);
        if (tryEntered > 0) {
            tryEntered--;
        } else {
            // this may happen only if we call to Leave() before
            // Enter() so this is definitely a bug
            DASSERT(FALSE);
        }
        ::LeaveCriticalSection(&rep);
        ::LeaveCriticalSection(&tryrep);
    }
};

// Macros for using CriticalSection objects that help trace
// lock/unlock actions
#define CRITICAL_SECTION_ENTER(cs) { \
    J2dTraceLn4(J2D_TRACE_VERBOSE2, \
                "CS.Wait:  tid, cs, file, line = 0x%x, 0x%x, %s, %d", \
                GetCurrentThreadId(), &(cs), __FILE__, __LINE__); \
    (cs).Enter(); \
    J2dTraceLn4(J2D_TRACE_VERBOSE2, \
                "CS.Enter: tid, cs, file, line = 0x%x, 0x%x, %s, %d", \
                GetCurrentThreadId(), &(cs), __FILE__, __LINE__); \
}

#define CRITICAL_SECTION_LEAVE(cs) { \
    J2dTraceLn4(J2D_TRACE_VERBOSE2, \
                "CS.Leave: tid, cs, file, line = 0x%x, 0x%x, %s, %d", \
                GetCurrentThreadId(), &(cs), __FILE__, __LINE__); \
    (cs).Leave(); \
    J2dTraceLn4(J2D_TRACE_VERBOSE2, \
                "CS.Left:  tid, cs, file, line = 0x%x, 0x%x, %s, %d", \
                GetCurrentThreadId(), &(cs), __FILE__, __LINE__); \
}

/************************************************************************
 * AwtToolkit class
 */

class AwtToolkit {
public:
    enum {
        KB_STATE_SIZE = 256
    };

    /* java.awt.Toolkit method ids */
    static jmethodID getDefaultToolkitMID;
    static jmethodID getFontMetricsMID;
        static jmethodID insetsMID;

    /* sun.awt.windows.WToolkit ids */
    static jmethodID windowsSettingChangeMID;
    static jmethodID displayChangeMID;

    BOOL m_isDynamicLayoutSet;

    AwtToolkit();
    ~AwtToolkit();

    BOOL Initialize(BOOL localPump);
    BOOL Dispose();

    void SetDynamicLayout(BOOL dynamic);
    BOOL IsDynamicLayoutSet();
    BOOL IsDynamicLayoutSupported();
    BOOL IsDynamicLayoutActive();

    INLINE BOOL localPump() { return m_localPump; }
    INLINE BOOL VerifyComponents() { return FALSE; } // TODO: Use new DebugHelper class to set this flag
    INLINE HWND GetHWnd() { return m_toolkitHWnd; }

    INLINE HMODULE GetModuleHandle() { return m_dllHandle; }
    INLINE void SetModuleHandle(HMODULE h) { m_dllHandle = h; }

    INLINE static DWORD MainThread() { return GetInstance().m_mainThreadId; }
    INLINE void VerifyActive() throw (awt_toolkit_shutdown) {
        if (!m_isActive && m_mainThreadId != ::GetCurrentThreadId()) {
            throw awt_toolkit_shutdown();
        }
    }
    INLINE BOOL IsDisposed() { return m_isDisposed; }
    static UINT GetMouseKeyState();
    static void GetKeyboardState(PBYTE keyboardState);

    static ATOM RegisterClass();
    static void UnregisterClass();
    INLINE LRESULT SendMessage(UINT msg, WPARAM wParam=0, LPARAM lParam=0) {
        if (!m_isDisposed) {
            return ::SendMessage(GetHWnd(), msg, wParam, lParam);
        } else {
            return NULL;
        }
    }
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam);
    static LRESULT CALLBACK GetMessageFilter(int code, WPARAM wParam,
                                             LPARAM lParam);
    static LRESULT CALLBACK ForegroundIdleFilter(int code, WPARAM wParam,
                                                 LPARAM lParam);

    INLINE static AwtToolkit& GetInstance() { return theInstance; }
    INLINE void SetPeer(JNIEnv *env, jobject wToolkit) {
        AwtToolkit &tk = AwtToolkit::GetInstance();
        if (tk.m_peer != NULL) {
            env->DeleteGlobalRef(tk.m_peer);
        }
        tk.m_peer = (wToolkit != NULL) ? env->NewGlobalRef(wToolkit) : NULL;
    }

    INLINE jobject GetPeer() {
        return m_peer;
    }

    // is this thread the main thread?

    INLINE static BOOL IsMainThread() {
        return GetInstance().m_mainThreadId == ::GetCurrentThreadId();
    }

    // post a message to the message pump thread

    INLINE BOOL PostMessage(UINT msg, WPARAM wp=0, LPARAM lp=0) {
        return ::PostMessage(GetHWnd(), msg, wp, lp);
    }

    // cause the message pump thread to call the function synchronously now!

    INLINE void * InvokeFunction(void*(*ftn)(void)) {
        return (void *)SendMessage(WM_AWT_INVOKE_VOID_METHOD, (WPARAM)ftn, 0);
    }
    INLINE void InvokeFunction(void (*ftn)(void)) {
        InvokeFunction((void*(*)(void))ftn);
    }
    INLINE void * InvokeFunction(void*(*ftn)(void *), void* param) {
        return (void *)SendMessage(WM_AWT_INVOKE_METHOD, (WPARAM)ftn,
                                   (LPARAM)param);
    }
    INLINE void InvokeFunction(void (*ftn)(void *), void* param) {
        InvokeFunction((void*(*)(void*))ftn, param);
    }

    INLINE CriticalSection &GetSyncCS() { return m_Sync; }

    void *SyncCall(void*(*ftn)(void *), void* param);
    void SyncCall(void (*ftn)(void *), void *param);
    void *SyncCall(void *(*ftn)(void));
    void SyncCall(void (*ftn)(void));

    // cause the message pump thread to call the function later ...

    INLINE void InvokeFunctionLater(void (*ftn)(void *), void* param) {
        if (!PostMessage(WM_AWT_INVOKE_METHOD, (WPARAM)ftn, (LPARAM)param)) {
            JNIEnv* env = (JNIEnv *)JNU_GetEnv(jvm, JNI_VERSION_1_2);
            JNU_ThrowInternalError(env, "Message not posted, native event queue may be full.");
        }
    }

   // cause the message pump thread to synchronously synchronize on the handle

    INLINE void WaitForSingleObject(HANDLE handle) {
        SendMessage(WM_AWT_WAIT_FOR_SINGLE_OBJECT, 0, (LPARAM)handle);
    }

    /*
     * Create an AwtXxxx C++ component using a given factory
     */
    typedef void (*ComponentFactory)(void*, void*);
    static void CreateComponent(void* hComponent, void* hParent,
                                ComponentFactory compFactory, BOOL isParentALocalReference=TRUE);

    static void DestroyComponentHWND(HWND hwnd);

    // constants used to PostQuitMessage

    static const int EXIT_ENCLOSING_LOOP;
    static const int EXIT_ALL_ENCLOSING_LOOPS;

    // ...

    void QuitMessageLoop(int status);

    UINT MessageLoop(IDLEPROC lpIdleFunc, PEEKMESSAGEPROC lpPeekMessageFunc);
    BOOL PumpWaitingMessages(PEEKMESSAGEPROC lpPeekMessageFunc);
    BOOL PreProcessMsg(MSG& msg);
    BOOL PreProcessMouseMsg(class AwtComponent* p, MSG& msg);
    BOOL PreProcessKeyMsg(class AwtComponent* p, MSG& msg);

    /* Create an ID which maps to an AwtObject pointer, such as a menu. */
    UINT CreateCmdID(AwtObject* object);

    // removes cmd id mapping
    void RemoveCmdID(UINT id);

    /* Return the AwtObject associated with its ID. */
    AwtObject* LookupCmdID(UINT id);

    /* Return the current application icon. */
    HICON GetAwtIcon();
    HICON GetAwtIconSm();

    /* Turns on/off dialog modality for the system. */
    INLINE AwtDialog* SetModal(AwtDialog* frame) {
        AwtDialog* previousDialog = m_pModalDialog;
        m_pModalDialog = frame;
        return previousDialog;
    };
    INLINE void ResetModal(AwtDialog* oldFrame) { m_pModalDialog = oldFrame; };
    INLINE BOOL IsModal() { return (m_pModalDialog != NULL); };
    INLINE AwtDialog* GetModalDialog(void) { return m_pModalDialog; };

    /* Stops the current message pump (normally a modal dialog pump) */
    INLINE void StopMessagePump() { m_breakOnError = TRUE; }

    /* Debug settings */
    INLINE void SetVerbose(long flag)   { m_verbose = (flag != 0); }
    INLINE void SetVerify(long flag)    { m_verifyComponents = (flag != 0); }
    INLINE void SetBreak(long flag)     { m_breakOnError = (flag != 0); }
    INLINE void SetHeapCheck(long flag);

    static void SetBusy(BOOL busy);

    /* Set and get the default input method Window handler. */
    INLINE void SetInputMethodWindow(HWND inputMethodHWnd) { m_inputMethodHWnd = inputMethodHWnd; }
    INLINE HWND GetInputMethodWindow() { return m_inputMethodHWnd; }

    static VOID CALLBACK PrimaryIdleFunc();
    static VOID CALLBACK SecondaryIdleFunc();
    static BOOL CALLBACK CommonPeekMessageFunc(MSG& msg);
    static BOOL activateKeyboardLayout(HKL hkl);

    HANDLE m_waitEvent;
    DWORD eventNumber;
private:
    HWND CreateToolkitWnd(LPCTSTR name);

    BOOL m_localPump;
    DWORD m_mainThreadId;
    HWND m_toolkitHWnd;
    HWND m_inputMethodHWnd;
    BOOL m_verbose;
    BOOL m_isActive; // set to FALSE at beginning of Dispose
    BOOL m_isDisposed; // set to TRUE at end of Dispose

    BOOL m_vmSignalled; // set to TRUE if QUERYENDSESSION has successfully
                        // raised SIGTERM

    BOOL m_verifyComponents;
    BOOL m_breakOnError;

    BOOL  m_breakMessageLoop;
    UINT  m_messageLoopResult;

    class AwtComponent* m_lastMouseOver;
    BOOL                m_mouseDown;

    HHOOK m_hGetMessageHook;
    UINT_PTR  m_timer;

    class AwtCmdIDList* m_cmdIDs;
    BYTE                m_lastKeyboardState[KB_STATE_SIZE];
    CriticalSection     m_lockKB;

    static AwtToolkit theInstance;

    /* The current modal dialog frame (normally NULL). */
    AwtDialog* m_pModalDialog;

    /* The WToolkit peer instance */
    jobject m_peer;

    HMODULE m_dllHandle;  /* The module handle. */

    CriticalSection m_Sync;

/* track display changes - used by palette-updating code.
   This is a workaround for a windows bug that prevents
   WM_PALETTECHANGED event from occurring immediately after
   a WM_DISPLAYCHANGED event.
  */
private:
    BOOL m_displayChanged;  /* Tracks displayChanged events */

public:
    BOOL HasDisplayChanged() { return m_displayChanged; }
    void ResetDisplayChanged() { m_displayChanged = FALSE; }

 private:
    static JNIEnv *m_env;
    static HANDLE m_thread;
 public:
    static void SetEnv(JNIEnv *env);
    static JNIEnv* GetEnv();
};

/*
 * Class to encapsulate the extraction of the java string contents
 * into a buffer and the cleanup of the buffer
 */
class JavaStringBuffer {
  public:
    JavaStringBuffer(JNIEnv *env, jstring jstr);
    INLINE ~JavaStringBuffer() { delete[] buffer; }
    INLINE operator LPTSTR() { return buffer; }
    INLINE operator LPARAM() { return (LPARAM)buffer; }  /* for SendMessage */

  private:
    LPTSTR buffer;
};

/*  creates an instance of T and assigns it to the argument, but only if
    the argument is initially NULL. Supposed to be thread-safe.
    returns the new value of the argument. I'm not using volatile here
    as InterlockedCompareExchange ensures volatile semantics
    and acquire/release.
    The function is useful when used with static POD NULL-initialized
    pointers, as they are guaranteed to be NULL before any dynamic
    initialization takes place. This function turns such a pointer
    into a thread-safe singleton, working regardless of dynamic
    initialization order. Destruction problem is not solved,
    we don't need it here.
*/

template<typename T> inline T* SafeCreate(T* &pArg) {
    /*  this implementation has no locks, it just destroys the object if it
        fails to be the first to init. another way would be using a special
        flag pointer value to mark the pointer as "being initialized". */
    T* pTemp = (T*)InterlockedCompareExchangePointer((void**)&pArg, NULL, NULL);
    if (pTemp != NULL) return pTemp;
    T* pNew = new T;
    pTemp = (T*)InterlockedCompareExchangePointer((void**)&pArg, pNew, NULL);
    if (pTemp != NULL) {
        // we failed it - another thread has already initialized pArg
        delete pNew;
        return pTemp;
    } else {
        return pNew;
    }
}

#endif /* AWT_TOOLKIT_H */
