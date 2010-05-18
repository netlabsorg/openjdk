/*
 * Copyright 1998 Sun Microsystems, Inc.  All Rights Reserved.
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

/*
 * @test
 * @bug 4151834
 * @summary Test Socket.setSoLinger
 *
 */

import java.net.*;

public class SetSoLinger implements Runnable {
    static ServerSocket ss;
    static InetAddress addr;
    static int port;

    public static void main(String args[]) throws Exception {
        boolean      error = true;
        int          linger = 65546;
        int          value = 0;
        addr = InetAddress.getLocalHost();
        ss = new ServerSocket(0);
        port = ss.getLocalPort();

        Thread t = new Thread(new SetSoLinger());
        t.start();
        Socket soc = ss.accept();
        soc.setSoLinger(true, linger);
        value = soc.getSoLinger();
        soc.close();

        if(value != 65535)
            throw new RuntimeException("Failed. Value not properly reduced.");
    }

    public void run() {
        try {
            Socket s = new Socket(addr, port);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
