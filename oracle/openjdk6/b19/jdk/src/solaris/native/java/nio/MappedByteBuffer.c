/*
 * Copyright 2001-2005 Sun Microsystems, Inc.  All Rights Reserved.
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
#include "jlong.h"
#include "java_nio_MappedByteBuffer.h"
#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>


JNIEXPORT jboolean JNICALL
Java_java_nio_MappedByteBuffer_isLoaded0(JNIEnv *env, jobject obj,
                                        jlong address, jlong len)
{
    jboolean loaded = JNI_TRUE;
    jint pageSize = sysconf(_SC_PAGESIZE);
    jint numPages = (len + pageSize - 1) / pageSize;
    int result = 0;
    int i = 0;
    void *a = (void *) jlong_to_ptr(address);
    char * vec = (char *)malloc(numPages * sizeof(char));

    if (vec == NULL) {
        JNU_ThrowOutOfMemoryError(env, NULL);
        return JNI_FALSE;
    }

    result = mincore(a, (size_t)len, vec);
    if (result != 0) {
        free(vec);
        JNU_ThrowIOExceptionWithLastError(env, "mincore failed");
        return JNI_FALSE;
    }

    for (i=0; i<numPages; i++) {
        if (vec[i] == 0) {
            loaded = JNI_FALSE;
            break;
        }
    }
    free(vec);
    return loaded;
}


JNIEXPORT jint JNICALL
Java_java_nio_MappedByteBuffer_load0(JNIEnv *env, jobject obj, jlong address,
                                     jlong len, jint pageSize)
{
    int pageIncrement = pageSize / sizeof(int);
    int numPages = (len + pageSize - 1) / pageSize;
    int *ptr = (int *)jlong_to_ptr(address);
    int i = 0;
    int j = 0;
    int result = madvise((caddr_t)ptr, len, MADV_WILLNEED);

    /* touch every page */
    for (i=0; i<numPages; i++) {
        j += *((volatile int *)ptr);
        ptr += pageIncrement;
    }
    return j;
}


JNIEXPORT void JNICALL
Java_java_nio_MappedByteBuffer_force0(JNIEnv *env, jobject obj, jlong address,
                                      jlong len)
{
    jlong pageSize = sysconf(_SC_PAGESIZE);
    unsigned long lAddress = address;

    jlong offset = lAddress % pageSize;
    void *a = (void *) jlong_to_ptr(lAddress - offset);
    int result = msync(a, (size_t)(len + offset), MS_SYNC);
    if (result != 0) {
        JNU_ThrowIOExceptionWithLastError(env, "msync failed");
    }
}
