/*
 * Copyright 1997-2007 Sun Microsystems, Inc.  All Rights Reserved.
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

#include <assert.h>
#include "java_lang_ProcessImpl.h"

#include "jni.h"
#include "jvm.h"
#include "jni_util.h"
#include "io_util.h"
#include <windows.h>
#include <io.h>

#define PIPE_SIZE 4096

char *
extractExecutablePath(JNIEnv *env, char *source)
{
    char *p, *r;

    /* If no spaces, then use entire thing */
    if ((p = strchr(source, ' ')) == NULL)
        return source;

    /* If no quotes, or quotes after space, return up to space */
    if (((r = strchr(source, '"')) == NULL) || (r > p)) {
        *p = 0;
        return source;
    }

    /* Quotes before space, return up to space after next quotes */
    p = strchr(r, '"');
    if ((p = strchr(p, ' ')) == NULL)
        return source;
    *p = 0;
    return source;
}

DWORD
selectProcessFlag(JNIEnv *env, jstring cmd0)
{
    char buf[MAX_PATH];
    DWORD newFlag = 0;
    char *exe, *p, *name;
    unsigned char buffer[2];
    long headerLoc = 0;
    int fd = 0;

    exe = (char *)JNU_GetStringPlatformChars(env, cmd0, 0);
    exe = extractExecutablePath(env, exe);

    if (exe != NULL) {
        if ((p = strchr(exe, '\\')) == NULL) {
            SearchPath(NULL, exe, ".exe", MAX_PATH, buf, &name);
        } else {
            p = strrchr(exe, '\\');
            *p = 0;
            p++;
            SearchPath(exe, p, ".exe", MAX_PATH, buf, &name);
        }
    }

    fd = _open(buf, _O_RDONLY);
    if (fd > 0) {
        _read(fd, buffer, 2);
        if (buffer[0] == 'M' && buffer[1] == 'Z') {
            _lseek(fd, 60L, SEEK_SET);
            _read(fd, buffer, 2);
            headerLoc = (long)buffer[1] << 8 | (long)buffer[0];
            _lseek(fd, headerLoc, SEEK_SET);
            _read(fd, buffer, 2);
            if (buffer[0] == 'P' && buffer[1] == 'E') {
                newFlag = DETACHED_PROCESS;
            }
        }
        _close(fd);
    }
    JNU_ReleaseStringPlatformChars(env, cmd0, exe);
    return newFlag;
}

static void
win32Error(JNIEnv *env, const char *functionName)
{
    static const char * const format = "%s error=%d, %s";
    static const char * const fallbackFormat = "%s failed, error=%d";
    char buf[256];
    char errmsg[sizeof(buf) + 100];
    const int errnum = GetLastError();
    const int n = JVM_GetLastErrorString(buf, sizeof(buf));
    if (n > 0)
        sprintf(errmsg, format, functionName, errnum, buf);
    else
        sprintf(errmsg, fallbackFormat, functionName, errnum);
    JNU_ThrowIOException(env, errmsg);
}

static void
closeSafely(HANDLE handle)
{
    if (handle)
        CloseHandle(handle);
}

JNIEXPORT jlong JNICALL
Java_java_lang_ProcessImpl_create(JNIEnv *env, jclass ignored,
                                  jstring cmd,
                                  jstring envBlock,
                                  jstring dir,
                                  jboolean redirectErrorStream,
                                  jobject in_fd,
                                  jobject out_fd,
                                  jobject err_fd)
{
    HANDLE inRead   = 0;
    HANDLE inWrite  = 0;
    HANDLE outRead  = 0;
    HANDLE outWrite = 0;
    HANDLE errRead  = 0;
    HANDLE errWrite = 0;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    LPTSTR  pcmd      = NULL;
    LPCTSTR pdir      = NULL;
    LPVOID  penvBlock = NULL;
    jlong ret = 0;
    OSVERSIONINFO ver;
    jboolean onNT = JNI_FALSE;
    DWORD processFlag;

    ver.dwOSVersionInfoSize = sizeof(ver);
    GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT)
        onNT = JNI_TRUE;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = 0;
    sa.bInheritHandle = TRUE;

    if (!(CreatePipe(&inRead,  &inWrite,  &sa, PIPE_SIZE) &&
          CreatePipe(&outRead, &outWrite, &sa, PIPE_SIZE) &&
          CreatePipe(&errRead, &errWrite, &sa, PIPE_SIZE))) {
        win32Error(env, "CreatePipe");
        goto Catch;
    }

    assert(cmd != NULL);
    pcmd = (LPTSTR) JNU_GetStringPlatformChars(env, cmd, NULL);
    if (pcmd == NULL) goto Catch;

    if (dir != 0) {
        pdir = (LPCTSTR) JNU_GetStringPlatformChars(env, dir, NULL);
        if (pdir == NULL) goto Catch;
        pdir = (LPCTSTR) JVM_NativePath((char *)pdir);
    }

    if (envBlock != NULL) {
        penvBlock = onNT
            ? (LPVOID) ((*env)->GetStringChars(env, envBlock, NULL))
            : (LPVOID) JNU_GetStringPlatformChars(env, envBlock, NULL);
        if (penvBlock == NULL) goto Catch;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = inRead;
    si.hStdOutput = outWrite;
    si.hStdError  = redirectErrorStream ? outWrite : errWrite;

    SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, FALSE);
    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, FALSE);
    SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, FALSE);

    if (redirectErrorStream)
        SetHandleInformation(errWrite, HANDLE_FLAG_INHERIT, FALSE);

    if (onNT)
        processFlag = CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT;
    else
        processFlag = selectProcessFlag(env, cmd);

    /* Java and Windows are both pure Unicode systems at heart.
     * Windows has both a legacy byte-based API and a 16-bit Unicode
     * "W" API.  The Right Thing here is to call CreateProcessW, since
     * that will allow all process-related information like command
     * line arguments to be passed properly to the child.  We don't do
     * that currently, since we would first have to have "W" versions
     * of JVM_NativePath and perhaps other functions.  In the
     * meantime, we can call CreateProcess with the magic flag
     * CREATE_UNICODE_ENVIRONMENT, which passes only the environment
     * in "W" mode.  We will fix this later. */

    ret = CreateProcess(0,           /* executable name */
                        pcmd,        /* command line */
                        0,           /* process security attribute */
                        0,           /* thread security attribute */
                        TRUE,        /* inherits system handles */
                        processFlag, /* selected based on exe type */
                        penvBlock,   /* environment block */
                        pdir,        /* change to the new current directory */
                        &si,         /* (in)  startup information */
                        &pi);        /* (out) process information */

    if (!ret) {
        win32Error(env, "CreateProcess");
        goto Catch;
    }

    CloseHandle(pi.hThread);
    ret = (jlong)pi.hProcess;
    (*env)->SetLongField(env, in_fd,  IO_handle_fdID, (jlong)inWrite);
    (*env)->SetLongField(env, out_fd, IO_handle_fdID, (jlong)outRead);
    (*env)->SetLongField(env, err_fd, IO_handle_fdID, (jlong)errRead);

 Finally:
    /* Always clean up the child's side of the pipes */
    closeSafely(inRead);
    closeSafely(outWrite);
    closeSafely(errWrite);

    if (pcmd != NULL)
        JNU_ReleaseStringPlatformChars(env, cmd, (char *) pcmd);
    if (pdir != NULL)
        JNU_ReleaseStringPlatformChars(env, dir, (char *) pdir);
    if (penvBlock != NULL) {
        if (onNT)
            (*env)->ReleaseStringChars(env, envBlock, (jchar *) penvBlock);
        else
            JNU_ReleaseStringPlatformChars(env, dir, (char *) penvBlock);
    }
    return ret;

 Catch:
    /* Clean up the parent's side of the pipes in case of failure only */
    closeSafely(inWrite);
    closeSafely(outRead);
    closeSafely(errRead);
    goto Finally;
}

JNIEXPORT jint JNICALL
Java_java_lang_ProcessImpl_getExitCodeProcess(JNIEnv *env, jclass ignored, jlong handle)
{
    DWORD exit_code;
    if (GetExitCodeProcess((HANDLE) handle, &exit_code) == 0)
        win32Error(env, "GetExitCodeProcess");
    return exit_code;
}

JNIEXPORT jint JNICALL
Java_java_lang_ProcessImpl_getStillActive(JNIEnv *env, jclass ignored)
{
    return STILL_ACTIVE;
}

JNIEXPORT void JNICALL
Java_java_lang_ProcessImpl_waitForInterruptibly(JNIEnv *env, jclass ignored, jlong handle)
{
    HANDLE events[2];
    events[0] = (HANDLE) handle;
    events[1] = JVM_GetThreadInterruptEvent();

    if (WaitForMultipleObjects(sizeof(events)/sizeof(events[0]), events,
                               FALSE,    /* Wait for ANY event */
                               INFINITE) /* Wait forever */
        == WAIT_FAILED)
        win32Error(env, "WaitForMultipleObjects");
}

JNIEXPORT void JNICALL
Java_java_lang_ProcessImpl_terminateProcess(JNIEnv *env, jclass ignored, jlong handle)
{
    TerminateProcess((HANDLE) handle, 1);
}

JNIEXPORT jboolean JNICALL
Java_java_lang_ProcessImpl_closeHandle(JNIEnv *env, jclass ignored, jlong handle)
{
    return CloseHandle((HANDLE) handle);
}
