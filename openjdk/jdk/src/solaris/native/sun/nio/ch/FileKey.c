/*
 * Copyright 2005 Sun Microsystems, Inc.  All Rights Reserved.
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

#include "jni.h"
#include "jni_util.h"
#include "jvm.h"
#include "nio.h"
#include "nio_util.h"
#include "sun_nio_ch_FileKey.h"

static jfieldID key_st_dev;    /* id for FileKey.st_dev */
static jfieldID key_st_ino;    /* id for FileKey.st_ino */

#define RESTARTABLE(_cmd, _result) do { \
  do { \
    _result = _cmd; \
  } while ((_result == -1) && (errno == EINTR)); \
} while(0)


JNIEXPORT void JNICALL
Java_sun_nio_ch_FileKey_initIDs(JNIEnv *env, jclass clazz)
{
    key_st_dev = (*env)->GetFieldID(env, clazz, "st_dev", "J");
    key_st_ino = (*env)->GetFieldID(env, clazz, "st_ino", "J");
}


JNIEXPORT void JNICALL
Java_sun_nio_ch_FileKey_init(JNIEnv *env, jobject this, jobject fdo)
{
    struct stat64 fbuf;
    int res;

    RESTARTABLE(fstat64(fdval(env, fdo), &fbuf), res);
    if (res < 0) {
        JNU_ThrowIOExceptionWithLastError(env, "fstat64 failed");
    } else {
        (*env)->SetLongField(env, this, key_st_dev, (jlong)fbuf.st_dev);
        (*env)->SetLongField(env, this, key_st_ino, (jlong)fbuf.st_ino);
    }
}
