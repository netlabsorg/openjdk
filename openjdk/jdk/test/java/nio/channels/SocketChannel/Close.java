/*
 * Copyright 2001 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
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

/* @test
 * @bug 4458266
 */

import java.io.IOException;
import java.net.*;
import java.nio.channels.*;


public class Close {

    static SelectionKey open() throws IOException {
        SocketChannel sc = SocketChannel.open();
        Selector sel = Selector.open();
        sc.configureBlocking(false);
        return sc.register(sel, SelectionKey.OP_READ);
    }

    static void check(SelectionKey sk) throws IOException {
        if (sk.isValid())
            throw new RuntimeException("Key still valid");
        if (sk.channel().isOpen())
            throw new RuntimeException("Channel still open");
        //      if (!((SocketChannel)sk.channel()).socket().isClosed())
        //  throw new RuntimeException("Socket still open");
    }

    static void testSocketClose() throws IOException {
        SelectionKey sk = open();
        //((SocketChannel)sk.channel()).socket().close();
        check(sk);
    }

    static void testChannelClose() throws IOException {
        SelectionKey sk = open();
        sk.channel().close();
        check(sk);
    }

    public static void main(String[] args) throws Exception {
        //##    testSocketClose();
        testChannelClose();
    }

}
