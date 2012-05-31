/*
 * Copyright (c) 2003, 2006, Oracle and/or its affiliates. All rights reserved.
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

#include "jni.h"
#include "jni_util.h"

/*
 * Prototypes for functions in io_util_md.c called from io_util,
 * FileDescriptor.c, FileInputStream.c, FileOutputStream.c
 */
WCHAR* pathToNTPath(JNIEnv *env, jstring path, jboolean throwFNFE);
WCHAR* fileToNTPath(JNIEnv *env, jobject file, jfieldID id);
WCHAR* getPrefixed(const WCHAR* path, int pathlen);
int currentDirLength(const WCHAR* path, int pathlen);
void fileOpen(JNIEnv *env, jobject this, jstring path, jfieldID fid, int flags);
int handleAvailable(jlong fd, jlong *pbytes);
JNIEXPORT int handleSync(jlong fd);
int handleSetLength(jlong fd, jlong length);
int handleFileSizeFD(jlong fd, jlong *size);
JNIEXPORT size_t handleRead(jlong fd, void *buf, jint len);
JNIEXPORT size_t handleWrite(jlong fd, const void *buf, jint len);
jint handleClose(JNIEnv *env, jobject this, jfieldID fid);
jlong handleLseek(jlong fd, jlong offset, jint whence);

/*
 * Returns an opaque handle to file named by "path".  If an error occurs,
 * returns -1 and an exception is pending.
 */
jlong winFileHandleOpen(JNIEnv *env, jstring path, int flags);

/*
 * Macros to use the right data type for file descriptors
 */
typedef struct __FD
{
    jint fd;
    jlong handle;
} _FD;

#define FD _FD

/*
 * Macros to set/get fd from the java.io.FileDescriptor.
 * If GetObjectField returns null, SET_FD will stop and GET_FD
 * will simply return -1 to avoid crashing VM.
 */

#define SET_FD(this, fd, fid) \
    if ((*env)->GetObjectField(env, (this), (fid)) != NULL) \
        (*env)->SetLongField(env, (*env)->GetObjectField(env, (this), (fid)), IO_handle_fdID, (fd))

#define GET_FD(this, fid) \
    { ((*env)->GetObjectField(env, (this), (fid)) == NULL ? \
      -1 : (*env)->GetIntField(env, (*env)->GetObjectField(env, (this), (fid)), IO_fd_fdID)), \
      ((*env)->GetObjectField(env, (this), (fid)) == NULL ? \
        -1 : (*env)->GetLongField(env, (*env)->GetObjectField(env, (this), (fid)), IO_handle_fdID)) }

#define ASSIGN_FD(fd, this, fid) \
    do { \
        fd.fd = ((*env)->GetObjectField(env, (this), (fid)) == NULL ? \
            -1 : (*env)->GetIntField(env, (*env)->GetObjectField(env, (this), (fid)), IO_fd_fdID)); \
        fd.handle = ((*env)->GetObjectField(env, (this), (fid)) == NULL ? \
            -1 : (*env)->GetLongField(env, (*env)->GetObjectField(env, (this), (fid)), IO_handle_fdID)); \
    } while(0)

#define VALID_FD(fd) (fd.handle != -1 || fd.fd != -1)

/*
 * Macros to set/get fd when inside java.io.FileDescriptor
 */
#define THIS_FD(obj) { (*env)->GetLongField(env, obj, IO_fd_fdID), (*env)->GetLongField(env, obj, IO_handle_fdID) }

/*
 * Route the routines away from HPI layer
 */
#define IO_Write(fd,buf,len) (fd.handle != -1 ? handleWrite(fd.handle, buf, len) : JVM_Write(fd.fd, buf, len))
#define IO_Sync(fd) (fd.handle != -1 ? handleSync(fd.handle) : JVM_Sync(fd.fd))
#define IO_Read(fd,buf,len) (fd.handle != -1 ? handleRead(fd.handle, buf, len) : JVM_Read(fd.fd, buf, len))
#define IO_Lseek(fd,offset,whence) (fd.handle != -1 ? handleLseek(fd.handle, offset, whence) : JVM_Lseek(fd.fd, offset, whence))
#define IO_Available(fd,pbytes) (fd.handle != -1 ? handleAvailable(fd.handle, pbytes) : JVM_Available(fd.fd, pbytes))
#define IO_SetLength(fd, length) (fd.handle != -1 ? handleSetLength(fd.handle, length) : JVM_SetLength(fd.fd, length))

/*
 * Setting the handle field in Java_java_io_FileDescriptor_set for
 * standard handles stdIn, stdOut, stdErr
 */
#define RETURN_HANDLE(fd) \
if (fd == 0) { \
    return (jlong)GetStdHandle(STD_INPUT_HANDLE); \
} else if (fd == 1) { \
    return (jlong)GetStdHandle(STD_OUTPUT_HANDLE); \
} else if (fd == 2) { \
    return (jlong)GetStdHandle(STD_ERROR_HANDLE); \
} else { \
    return (jlong)-1; \
} \

/* INVALID_FILE_ATTRIBUTES is not defined in VC++6.0's header files but
 * in later release. Keep here just in case someone is still using VC++6.0
 */
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
