/*
 * Copyright 1998-2000 Sun Microsystems, Inc.  All Rights Reserved.
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

#include "awt_Container.h"
#include "awt.h"

/************************************************************************
 * AwtContainer fields
 */

jfieldID AwtContainer::ncomponentsID;
jfieldID AwtContainer::componentID;
jfieldID AwtContainer::layoutMgrID;
jmethodID AwtContainer::findComponentAtMID;

/************************************************************************
 * AwtContainer native methods
 */

extern "C" {

JNIEXPORT void JNICALL
Java_java_awt_Container_initIDs(JNIEnv *env, jclass cls) {
    TRY;

    AwtContainer::ncomponentsID = env->GetFieldID(cls, "ncomponents", "I");
    AwtContainer::componentID =
        env->GetFieldID(cls, "component", "[Ljava/awt/Component;");

    AwtContainer::layoutMgrID =
        env->GetFieldID(cls, "layoutMgr", "Ljava/awt/LayoutManager;");

    AwtContainer::findComponentAtMID =
        env->GetMethodID(cls, "findComponentAt", "(IIZ)Ljava/awt/Component;");

    DASSERT(AwtContainer::ncomponentsID != NULL);
    DASSERT(AwtContainer::componentID != NULL);
    DASSERT(AwtContainer::layoutMgrID != NULL);
    DASSERT(AwtContainer::findComponentAtMID);

    CATCH_BAD_ALLOC;
}

} /* extern "C" */
