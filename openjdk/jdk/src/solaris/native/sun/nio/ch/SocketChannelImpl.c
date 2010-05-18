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

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

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
#include "sun_nio_ch_SocketChannelImpl.h"
#include "nio_util.h"
#include "nio.h"


JNIEXPORT jint JNICALL
Java_sun_nio_ch_SocketChannelImpl_checkConnect(JNIEnv *env, jobject this,
                                               jobject fdo, jboolean block,
                                               jboolean ready)
{
    int error = 0;
    int n = sizeof(int);
    jint fd = fdval(env, fdo);
    int result = 0;
    struct pollfd poller;

    poller.revents = 1;
    if (!ready) {
        poller.fd = fd;
        poller.events = POLLOUT;
        poller.revents = 0;
        result = poll(&poller, 1, block ? -1 : 0);
        if (result < 0) {
            JNU_ThrowIOExceptionWithLastError(env, "Poll failed");
            return IOS_THROWN;
        }
        if (!block && (result == 0))
            return IOS_UNAVAILABLE;
    }

    if (poller.revents) {
        errno = 0;
        result = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &n);
        if (result < 0) {
            handleSocketError(env, errno);
            return JNI_FALSE;
        } else if (error) {
            handleSocketError(env, error);
            return JNI_FALSE;
        }
        return 1;
    }
    return 0;
}


JNIEXPORT void JNICALL
Java_sun_nio_ch_SocketChannelImpl_shutdown(JNIEnv *env, jclass cl,
                                           jobject fdo, jint how)
{
    if (shutdown(fdval(env, fdo), how) < 0)
        handleSocketError(env, errno);
}
