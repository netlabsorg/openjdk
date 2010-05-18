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
#include "jlong.h"
#include "jvm.h"
#include "jdk_util.h"

#include "sun_misc_VMSupport.h"

typedef jobject (JNICALL *INIT_AGENT_PROPERTIES_FN)(JNIEnv *, jobject);

static INIT_AGENT_PROPERTIES_FN InitAgentProperties_fp = NULL;

JNIEXPORT jobject JNICALL
Java_sun_misc_VMSupport_initAgentProperties(JNIEnv *env, jclass cls, jobject props)
{
    char errmsg[128];

    if (InitAgentProperties_fp == NULL) {
        if (!JDK_InitJvmHandle()) {
            JNU_ThrowInternalError(env,
                 "Handle for JVM not found for symbol lookup");
        }
        InitAgentProperties_fp = (INIT_AGENT_PROPERTIES_FN)
            JDK_FindJvmEntry("JVM_InitAgentProperties");
        if (InitAgentProperties_fp == NULL) {
            JNU_ThrowInternalError(env,
                 "Mismatched VM version: JVM_InitAgentProperties not found");
            return NULL;
        }
    }
    return (*InitAgentProperties_fp)(env, props);
}
