/*
 * Copyright 2000-2002 Sun Microsystems, Inc.  All Rights Reserved.
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

#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#if __linux__
#include <netinet/in.h>
#endif

#if defined(__solaris__) && !defined(_SOCKLEN_T)
typedef size_t socklen_t;       /* New in SunOS 5.7, so need this for 5.6 */
#endif

#include "jni.h"
#include "jni_util.h"
#include "net_util.h"
#include "jvm.h"
#include "jlong.h"
#include "sun_nio_ch_ServerSocketChannelImpl.h"
#include "nio.h"
#include "nio_util.h"


static jfieldID fd_fdID;        /* java.io.FileDescriptor.fd */
static jclass isa_class;        /* java.net.InetSocketAddress */
static jmethodID isa_ctorID;    /*   .InetSocketAddress(InetAddress, int) */


JNIEXPORT void JNICALL
Java_sun_nio_ch_ServerSocketChannelImpl_initIDs(JNIEnv *env, jclass c)
{
    jclass cls;

    cls = (*env)->FindClass(env, "java/io/FileDescriptor");
    fd_fdID = (*env)->GetFieldID(env, cls, "fd", "I");

    cls = (*env)->FindClass(env, "java/net/InetSocketAddress");
    isa_class = (*env)->NewGlobalRef(env, cls);
    isa_ctorID = (*env)->GetMethodID(env, cls, "<init>",
                                     "(Ljava/net/InetAddress;I)V");
}

JNIEXPORT void JNICALL
Java_sun_nio_ch_ServerSocketChannelImpl_listen(JNIEnv *env, jclass cl,
                                               jobject fdo, jint backlog)
{
    if (listen(fdval(env, fdo), backlog) < 0)
        handleSocketError(env, errno);
}

JNIEXPORT jint JNICALL
Java_sun_nio_ch_ServerSocketChannelImpl_accept0(JNIEnv *env, jobject this,
                                                jobject ssfdo, jobject newfdo,
                                                jobjectArray isaa)
{
    jint ssfd = (*env)->GetIntField(env, ssfdo, fd_fdID);
    jint newfd;
    struct sockaddr *sa;
    int sa_len;
    jobject remote_ia = 0;
    jobject isa;
    jint remote_port;

    NET_AllocSockaddr(&sa, &sa_len);

    /*
     * accept connection but ignore ECONNABORTED indicating that
     * a connection was eagerly accepted but was reset before
     * accept() was called.
     */
    for (;;) {
        newfd = accept(ssfd, sa, &sa_len);
        if (newfd >= 0) {
            break;
        }
        if (errno != ECONNABORTED) {
            break;
        }
        /* ECONNABORTED => restart accept */
    }

    if (newfd < 0) {
        free((void *)sa);
        if (errno == EAGAIN)
            return IOS_UNAVAILABLE;
        if (errno == EINTR)
            return IOS_INTERRUPTED;
        JNU_ThrowIOExceptionWithLastError(env, "Accept failed");
        return IOS_THROWN;
    }

    (*env)->SetIntField(env, newfdo, fd_fdID, newfd);
    remote_ia = NET_SockaddrToInetAddress(env, sa, (int *)&remote_port);
    free((void *)sa);
    isa = (*env)->NewObject(env, isa_class, isa_ctorID,
                            remote_ia, remote_port);
    (*env)->SetObjectArrayElement(env, isaa, 0, isa);
    return 1;
}
