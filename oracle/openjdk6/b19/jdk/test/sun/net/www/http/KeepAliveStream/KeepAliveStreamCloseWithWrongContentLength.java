/*
 * Copyright 2002-2005 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4533243
 * @summary Closing a keep alive stream gives NullPointerException
 * @run main/othervm/timeout=30 KeepAliveStreamCloseWithWrongContentLength
 */

import java.net.*;
import java.io.*;

public class KeepAliveStreamCloseWithWrongContentLength {

    static class XServer extends Thread {
        ServerSocket srv;
        Socket s;
        InputStream is;
        OutputStream os;

        XServer (ServerSocket s) {
            srv = s;
        }

        Socket getSocket () {
            return (s);
        }

        public void run() {
            try {
                s = srv.accept ();
                // read HTTP request from client
                InputStream is = s.getInputStream();
                // read the first ten bytes
                for (int i=0; i<10; i++) {
                    is.read();
                }
                OutputStreamWriter ow =
                    new OutputStreamWriter(s.getOutputStream());
                ow.write("HTTP/1.0 200 OK\n");

                // Note: The client expects 10 bytes.
                ow.write("Content-Length: 10\n");
                ow.write("Content-Type: text/html\n");

                // Note: If this line is missing, everything works fine.
                ow.write("Connection: Keep-Alive\n");
                ow.write("\n");

                // Note: The (buggy) server only sends 9 bytes.
                ow.write("123456789");
                ow.flush();
                ow.close();
            } catch (Exception e) {
            }
        }
    }

    /*
     *
     */

    public static void main (String[] args) {
        try {
            ServerSocket serversocket = new ServerSocket (0);
            int port = serversocket.getLocalPort ();
            XServer server = new XServer (serversocket);
            server.start ();
            URL url = new URL ("http://localhost:"+port);
            HttpURLConnection urlc = (HttpURLConnection)url.openConnection ();
            InputStream is = urlc.getInputStream ();
            int c = 0;
            while (c != -1) {
                try {
                    c=is.read();
                } catch (IOException ioe) {
                    is.read ();
                    break;
                }
            }
            is.close();
            server.getSocket().close ();
        } catch (IOException e) {
            return;
        } catch (NullPointerException e) {
            throw new RuntimeException (e);
        }
    }
}
