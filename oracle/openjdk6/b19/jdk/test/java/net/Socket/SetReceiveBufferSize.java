/*
 * Copyright 2000 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4326250
 * @summary Test Socket.setReceiveBufferSize() throwin IllegalArgumentException
 *
 */

import java.net.Socket;
import java.net.ServerSocket;

public class SetReceiveBufferSize {
    class Server extends Thread {
        private ServerSocket ss;
        public Server(ServerSocket ss) {
            this.ss = ss;
        }

        public void run() {
            try {
                ss.accept();
            } catch (Exception e) {
            }
        }
    }

    public static void main(String[] args) throws Exception {
        SetReceiveBufferSize s = new SetReceiveBufferSize();
    }

    public SetReceiveBufferSize() throws Exception {
        ServerSocket ss = new ServerSocket(0);
        Server serv = new Server(ss);
        serv.start();
        Socket s = new Socket("localhost", ss.getLocalPort());
        try {
            s.setReceiveBufferSize(0);
        } catch (IllegalArgumentException e) {
            return;
        } catch (Exception ex) {
        } finally {
            ss.close();
        }
        throw new RuntimeException("IllegalArgumentException not thrown!");
    }
}
