/*
 * Copyright 2005 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 5045306 6356004
 * @library ../../httptest/
 * @build HttpCallback HttpServer HttpTransaction
 * @run main/othervm B5045306
 * @summary Http keep-alive implementation is not efficient
 */

import java.net.*;
import java.io.*;
import java.nio.channels.*;
import java.lang.management.*;

/* Part 1:
 * The http client makes a connection to a URL whos content contains a lot of
 * data, more than can fit in the socket buffer. The client only reads
 * 1 byte of the data from the InputStream leaving behind more data than can
 * fit in the socket buffer. The client then makes a second call to the http
 * server. If the connection port used by the client is the same as for the
 * first call then that means that the connection is being reused.
 *
 * Part 2:
 * Test buggy webserver that sends less data than it specifies in its
 * Content-length header.
 */

public class B5045306
{
    static SimpleHttpTransaction httpTrans;
    static HttpServer server;

    public static void main(String[] args) throws Exception {
        startHttpServer();
        clientHttpCalls();
    }

    public static void startHttpServer() {
        try {
            httpTrans = new SimpleHttpTransaction();
            server = new HttpServer(httpTrans, 1, 10, 0);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void clientHttpCalls() {
        try {
            System.out.println("http server listen on: " + server.getLocalPort());
            String baseURLStr = "http://" + InetAddress.getLocalHost().getHostAddress() + ":" +
                                  server.getLocalPort() + "/";

            URL bigDataURL = new URL (baseURLStr + "firstCall");
            URL smallDataURL = new URL (baseURLStr + "secondCall");

            HttpURLConnection uc = (HttpURLConnection)bigDataURL.openConnection();

            //Only read 1 byte of response data and close the stream
            InputStream is = uc.getInputStream();
            byte[] ba = new byte[1];
            is.read(ba);
            is.close();

            // Allow the KeepAliveStreamCleaner thread to read the data left behind and cache the connection.
            try { Thread.sleep(2000); } catch (Exception e) {}

            uc = (HttpURLConnection)smallDataURL.openConnection();
            uc.getResponseCode();

            if (SimpleHttpTransaction.failed)
                throw new RuntimeException("Failed: Initial Keep Alive Connection is not being reused");

            // Part 2
            URL part2Url = new URL (baseURLStr + "part2");
            uc = (HttpURLConnection)part2Url.openConnection();
            is = uc.getInputStream();
            is.close();

            // Allow the KeepAliveStreamCleaner thread to try and read the data left behind and cache the connection.
            try { Thread.sleep(2000); } catch (Exception e) {}

            ThreadMXBean threadMXBean = ManagementFactory.getThreadMXBean();
            if (threadMXBean.isThreadCpuTimeSupported()) {
                long[] threads = threadMXBean.getAllThreadIds();
                ThreadInfo[] threadInfo = threadMXBean.getThreadInfo(threads);
                for (int i=0; i<threadInfo.length; i++) {
                    if (threadInfo[i].getThreadName().equals("Keep-Alive-SocketCleaner"))  {
                        System.out.println("Found Keep-Alive-SocketCleaner thread");
                        long threadID = threadInfo[i].getThreadId();
                        long before = threadMXBean.getThreadCpuTime(threadID);
                        try { Thread.sleep(2000); } catch (Exception e) {}
                        long after = threadMXBean.getThreadCpuTime(threadID);

                        if (before ==-1 || after == -1)
                            break;  // thread has died, OK

                        // if Keep-Alive-SocketCleaner consumes more than 50% of cpu then we
                        // can assume a recursive loop.
                        long total = after - before;
                        if (total >= 1000000000)  // 1 second, or 1 billion nanoseconds
                            throw new RuntimeException("Failed: possible recursive loop in Keep-Alive-SocketCleaner");
                    }
                }
            }

        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            server.terminate();
        }
    }
}

class SimpleHttpTransaction implements HttpCallback
{
    static boolean failed = false;

    // Need to have enough data here that is too large for the socket buffer to hold.
    // Also http.KeepAlive.remainingData must be greater than this value, default is 256K.
    static final int RESPONSE_DATA_LENGTH = 128 * 1024;

    int port1;

    public void request(HttpTransaction trans) {
        try {
            String path = trans.getRequestURI().getPath();
            if (path.equals("/firstCall")) {
                port1 = trans.channel().socket().getPort();
                System.out.println("First connection on client port = " + port1);

                byte[] responseBody = new byte[RESPONSE_DATA_LENGTH];
                for (int i=0; i<responseBody.length; i++)
                    responseBody[i] = 0x41;
                trans.setResponseEntityBody (responseBody, responseBody.length);
                trans.sendResponse(200, "OK");
            } else if (path.equals("/secondCall")) {
                int port2 = trans.channel().socket().getPort();
                System.out.println("Second connection on client port = " + port2);

                if (port1 != port2)
                    failed = true;

                trans.setResponseHeader ("Content-length", Integer.toString(0));
                trans.sendResponse(200, "OK");
            } else if(path.equals("/part2")) {
                System.out.println("Call to /part2");
                byte[] responseBody = new byte[RESPONSE_DATA_LENGTH];
                for (int i=0; i<responseBody.length; i++)
                    responseBody[i] = 0x41;
                trans.setResponseEntityBody (responseBody, responseBody.length);

                // override the Content-length header to be greater than the actual response body
                trans.setResponseHeader("Content-length", Integer.toString(responseBody.length+1));
                trans.sendResponse(200, "OK");

                // now close the socket
                trans.channel().socket().close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
