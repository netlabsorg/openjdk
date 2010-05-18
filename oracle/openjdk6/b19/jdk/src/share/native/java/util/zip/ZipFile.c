/*
 * Copyright 1998-2006 Sun Microsystems, Inc.  All Rights Reserved.
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
 * Native method support for java.util.zip.ZipFile
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include "jlong.h"
#include "jvm.h"
#include "jni.h"
#include "jni_util.h"
#include "zip_util.h"
#ifdef WIN32
#include "io_util_md.h"
#endif

#include "java_util_zip_ZipFile.h"
#include "java_util_jar_JarFile.h"

#define DEFLATED 8
#define STORED 0

static jfieldID jzfileID;

static int OPEN_READ = java_util_zip_ZipFile_OPEN_READ;
static int OPEN_DELETE = java_util_zip_ZipFile_OPEN_DELETE;

JNIEXPORT void JNICALL
Java_java_util_zip_ZipFile_initIDs(JNIEnv *env, jclass cls)
{
    jzfileID = (*env)->GetFieldID(env, cls, "jzfile", "J");
    assert(jzfileID != 0);
}

static void
ThrowZipException(JNIEnv *env, const char *msg)
{
    jstring s = NULL;
    jobject x;

    if (msg != NULL) {
        s = JNU_NewStringPlatform(env, msg);
    }
    x = JNU_NewObjectByName(env,
                            "java/util/zip/ZipException",
                            "(Ljava/lang/String;)V", s);
    if (x != NULL) {
        (*env)->Throw(env, x);
    }
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_open(JNIEnv *env, jclass cls, jstring name,
                                        jint mode, jlong lastModified)
{
    const char *path = JNU_GetStringPlatformChars(env, name, 0);
    char *msg = 0;
    jlong result = 0;
    int flag = 0;
    jzfile *zip = 0;

    if (mode & OPEN_READ) flag |= O_RDONLY;
    if (mode & OPEN_DELETE) flag |= JVM_O_DELETE;

    if (path != 0) {
        zip = ZIP_Get_From_Cache(path, &msg, lastModified);
        if (zip == 0 && msg == 0) {
            ZFILE zfd = 0;
#ifdef WIN32
            zfd = winFileHandleOpen(env, name, flag);
            if (zfd == -1) {
                /* Exception already pending. */
                goto finally;
            }
#else
            zfd = JVM_Open(path, flag, 0);
#endif

            if (zfd >= 0) {
                zip = ZIP_Put_In_Cache(path, zfd, &msg, lastModified);
            }
        }

        if (zip != 0) {
            result = ptr_to_jlong(zip);
        } else if (msg != 0) {
            ThrowZipException(env, msg);
        } else if (errno == ENOMEM) {
            JNU_ThrowOutOfMemoryError(env, 0);
        } else {
            ThrowZipException(env, "error in opening zip file");
        }

finally:
        JNU_ReleaseStringPlatformChars(env, name, path);
    }
    return result;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_getTotal(JNIEnv *env, jclass cls, jlong zfile)
{
    jzfile *zip = jlong_to_ptr(zfile);

    return zip->total;
}

JNIEXPORT void JNICALL
Java_java_util_zip_ZipFile_close(JNIEnv *env, jclass cls, jlong zfile)
{
    ZIP_Close(jlong_to_ptr(zfile));
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_getEntry(JNIEnv *env, jclass cls, jlong zfile,
                                    jstring name, jboolean addSlash)
{
#define MAXNAME 1024
    jzfile *zip = jlong_to_ptr(zfile);
    jsize slen = (*env)->GetStringLength(env, name);
    jsize ulen = (*env)->GetStringUTFLength(env, name);
    char buf[MAXNAME+2], *path;
    jzentry *ze;

    if (ulen > MAXNAME) {
        path = malloc(ulen + 2);
        if (path == 0) {
            JNU_ThrowOutOfMemoryError(env, 0);
            return 0;
        }
    } else {
        path = buf;
    }
    (*env)->GetStringUTFRegion(env, name, 0, slen, path);
    path[ulen] = '\0';
    if (addSlash == JNI_FALSE) {
        ze = ZIP_GetEntry(zip, path, 0);
    } else {
        ze = ZIP_GetEntry(zip, path, (jint)ulen);
    }
    if (path != buf) {
        free(path);
    }
    return ptr_to_jlong(ze);
}

JNIEXPORT void JNICALL
Java_java_util_zip_ZipFile_freeEntry(JNIEnv *env, jclass cls, jlong zfile,
                                    jlong zentry)
{
    jzfile *zip = jlong_to_ptr(zfile);
    jzentry *ze = jlong_to_ptr(zentry);
    ZIP_FreeEntry(zip, ze);
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_getNextEntry(JNIEnv *env, jclass cls, jlong zfile,
                                        jint n)
{
    jzentry *ze = ZIP_GetNextEntry(jlong_to_ptr(zfile), n);

    return ptr_to_jlong(ze);
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_getMethod(JNIEnv *env, jclass cls, jlong zentry)
{
    jzentry *ze = jlong_to_ptr(zentry);

    return ze->csize != 0 ? DEFLATED : STORED;
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_getCSize(JNIEnv *env, jclass cls, jlong zentry)
{
    jzentry *ze = jlong_to_ptr(zentry);

    return ze->csize != 0 ? ze->csize : ze->size;
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_getSize(JNIEnv *env, jclass cls, jlong zentry)
{
    jzentry *ze = jlong_to_ptr(zentry);

    return ze->size;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_read(JNIEnv *env, jclass cls, jlong zfile,
                                jlong zentry, jlong pos, jbyteArray bytes,
                                jint off, jint len)
{
    jzfile *zip = jlong_to_ptr(zfile);
    char *msg;

#define BUFSIZE 8192
    /* copy via tmp stack buffer: */
    jbyte buf[BUFSIZE];

    if (len > BUFSIZE) {
        len = BUFSIZE;
    }

    ZIP_Lock(zip);
    len = ZIP_Read(zip, jlong_to_ptr(zentry), pos, buf, len);
    msg = zip->msg;
    ZIP_Unlock(zip);
    if (len != -1) {
        (*env)->SetByteArrayRegion(env, bytes, off, len, buf);
    }

    if (len == -1) {
        if (msg != 0) {
            ThrowZipException(env, msg);
        } else {
            char errmsg[128];
            sprintf(errmsg, "errno: %d, error: %s\n",
                    errno, "Error reading ZIP file");
            JNU_ThrowIOExceptionWithLastError(env, errmsg);
        }
    }

    return len;
}

/*
 * Returns an array of strings representing the names of all entries
 * that begin with "META-INF/" (case ignored). This native method is
 * used in JarFile as an optimization when looking up manifest and
 * signature file entries. Returns null if no entries were found.
 */
JNIEXPORT jobjectArray JNICALL
Java_java_util_jar_JarFile_getMetaInfEntryNames(JNIEnv *env, jobject obj)
{
    jlong zfile = (*env)->GetLongField(env, obj, jzfileID);
    jzfile *zip;
    int i, count;
    jobjectArray result = 0;

    if (zfile == 0) {
        JNU_ThrowByName(env,
                        "java/lang/IllegalStateException", "zip file closed");
        return NULL;
    }
    zip = jlong_to_ptr(zfile);

    /* count the number of valid ZIP metanames */
    count = 0;
    if (zip->metanames != 0) {
        for (i = 0; i < zip->metacount; i++) {
            if (zip->metanames[i] != 0) {
                count++;
            }
        }
    }

    /* If some names were found then build array of java strings */
    if (count > 0) {
        jclass cls = (*env)->FindClass(env, "java/lang/String");
        result = (*env)->NewObjectArray(env, count, cls, 0);
        if (result != 0) {
            for (i = 0; i < count; i++) {
                jstring str = (*env)->NewStringUTF(env, zip->metanames[i]);
                if (str == 0) {
                    break;
                }
                (*env)->SetObjectArrayElement(env, result, i, str);
                (*env)->DeleteLocalRef(env, str);
            }
        }
    }
    return result;
}

JNIEXPORT jstring JNICALL
Java_java_util_zip_ZipFile_getZipMessage(JNIEnv *env, jclass cls, jlong zfile)
{
    jzfile *zip = jlong_to_ptr(zfile);
    char *msg = zip->msg;
    if (msg == NULL) {
        return NULL;
    }
    return JNU_NewStringPlatform(env, msg);
}
