/*
 * Copyright 2003 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4802209
 * @summary check that the right utf-8 encoder is used
 */

import java.io.*;
import java.net.*;

public class Encode implements Runnable {
    public static void main(String args[]) throws Exception {
        new Encode();
    }

    Encode() throws Exception {
        ss = new ServerSocket(0);
        (new Thread(this)).start();
        String toEncode = "\uD800\uDC00 \uD801\uDC01 ";
        String enc1 = URLEncoder.encode(toEncode, "UTF-8");
        byte bytes[] = {};
        ByteArrayInputStream bais = new ByteArrayInputStream(bytes);
        InputStreamReader reader = new InputStreamReader( bais, "8859_1");
        String url = "http://localhost:" + Integer.toString(ss.getLocalPort()) +
            "/missing.nothtml";
        HttpURLConnection uc =  (HttpURLConnection)new URL(url).openConnection();
        uc.connect();
        String enc2 = URLEncoder.encode(toEncode, "UTF-8");
        if (!enc1.equals(enc2))
            throw new RuntimeException("test failed");
        uc.disconnect();
    }

    ServerSocket ss;

    public void run() {
        try {
            Socket s = ss.accept();
            BufferedReader in = new BufferedReader(
                new InputStreamReader(s.getInputStream()));
            String req = in.readLine();
            PrintStream out = new PrintStream(new BufferedOutputStream(
                s.getOutputStream()));
            out.print("HTTP/1.1 403 Forbidden\r\n");
            out.print("\r\n");
            out.flush();
            s.close();
            ss.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
