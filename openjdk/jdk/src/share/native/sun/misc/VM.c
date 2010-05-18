/*
 * Copyright 2004-2005 Sun Microsystems, Inc.  All Rights Reserved.
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
#include "jlong.h"
#include "jvm.h"
#include "jdk_util.h"

#include "sun_misc_VM.h"

typedef jintArray (JNICALL *GET_THREAD_STATE_VALUES_FN)(JNIEnv *, jint);
typedef jobjectArray (JNICALL *GET_THREAD_STATE_NAMES_FN)(JNIEnv *, jint, jintArray);

static GET_THREAD_STATE_VALUES_FN GetThreadStateValues_fp = NULL;
static GET_THREAD_STATE_NAMES_FN GetThreadStateNames_fp = NULL;

static void get_thread_state_info(JNIEnv *env, jint state,
                                      jobjectArray stateValues,
                                      jobjectArray stateNames) {
    char errmsg[128];
    jintArray values;
    jobjectArray names;

    values = (*GetThreadStateValues_fp)(env, state);
    if (values == NULL) {
        sprintf(errmsg, "Mismatched VM version: Thread state (%d) "
                        "not supported", state);
        JNU_ThrowInternalError(env, errmsg);
        return;
    }
    /* state is also used as the index in the array */
    (*env)->SetObjectArrayElement(env, stateValues, state, values);

    names = (*GetThreadStateNames_fp)(env, state, values);
    if (names == NULL) {
        sprintf(errmsg, "Mismatched VM version: Thread state (%d) "
                        "not supported", state);
        JNU_ThrowInternalError(env, errmsg);
        return;
    }
    (*env)->SetObjectArrayElement(env, stateNames, state, names);
}

JNIEXPORT void JNICALL
Java_sun_misc_VM_getThreadStateValues(JNIEnv *env, jclass cls,
                                      jobjectArray values,
                                      jobjectArray names)
{
    char errmsg[128];

    // check if the number of Thread.State enum constants
    // matches the number of states defined in jvm.h
    jsize len1 = (*env)->GetArrayLength(env, values);
    jsize len2 = (*env)->GetArrayLength(env, names);
    if (len1 != JAVA_THREAD_STATE_COUNT || len2 != JAVA_THREAD_STATE_COUNT) {
        sprintf(errmsg, "Mismatched VM version: JAVA_THREAD_STATE_COUNT = %d "
                " but JDK expects %d / %d",
                JAVA_THREAD_STATE_COUNT, len1, len2);
        JNU_ThrowInternalError(env, errmsg);
        return;
    }

    if (GetThreadStateValues_fp == NULL) {
        GetThreadStateValues_fp = (GET_THREAD_STATE_VALUES_FN)
            JDK_FindJvmEntry("JVM_GetThreadStateValues");
        if (GetThreadStateValues_fp == NULL) {
            JNU_ThrowInternalError(env,
                 "Mismatched VM version: JVM_GetThreadStateValues not found");
            return;
        }

        GetThreadStateNames_fp = (GET_THREAD_STATE_NAMES_FN)
            JDK_FindJvmEntry("JVM_GetThreadStateNames");
        if (GetThreadStateNames_fp == NULL) {
            JNU_ThrowInternalError(env,
                 "Mismatched VM version: JVM_GetThreadStateNames not found");
            return ;
        }
    }

    get_thread_state_info(env, JAVA_THREAD_STATE_NEW, values, names);
    get_thread_state_info(env, JAVA_THREAD_STATE_RUNNABLE, values, names);
    get_thread_state_info(env, JAVA_THREAD_STATE_BLOCKED, values, names);
    get_thread_state_info(env, JAVA_THREAD_STATE_WAITING, values, names);
    get_thread_state_info(env, JAVA_THREAD_STATE_TIMED_WAITING, values, names);
    get_thread_state_info(env, JAVA_THREAD_STATE_TERMINATED, values, names);
}

JNIEXPORT void JNICALL
Java_sun_misc_VM_initialize(JNIEnv *env, jclass cls) {
    char errmsg[128];

    if (!JDK_InitJvmHandle()) {
        JNU_ThrowInternalError(env, "Handle for JVM not found for symbol lookup");
    }
}
