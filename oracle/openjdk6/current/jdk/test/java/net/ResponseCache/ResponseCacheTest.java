/*
 * Copyright 2003-2004 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @summary Unit test for java.net.ResponseCache
 * @bug 4837267
 * @author Yingxian Wang
 */

import java.net.*;
import java.util.*;
import java.io.*;
import java.nio.*;
import sun.net.www.ParseUtil;
import javax.net.ssl.*;

/**
 * Request should get serviced by the cache handler. Response get
 * saved through the cache handler.
 */
public class ResponseCacheTest implements Runnable {
    ServerSocket ss;
    static URL url1;
    static URL url2;
    static String FNPrefix, OutFNPrefix;
    /*
     * Our "http" server to return a 404 */
    public void run() {
        try {
            Socket s = ss.accept();

            InputStream is = s.getInputStream ();
            BufferedReader r = new BufferedReader(new InputStreamReader(is));
            String x;
            while ((x=r.readLine()) != null) {
                if (x.length() ==0) {
                    break;
                }
            }
            PrintStream out = new PrintStream(
                                 new BufferedOutputStream(
                                    s.getOutputStream() ));

            /* send file2.1 */
            File file2 = new File(FNPrefix+"file2.1");
            out.print("HTTP/1.1 200 OK\r\n");
            out.print("Content-Type: text/html; charset=iso-8859-1\r\n");
            out.print("Content-Length: "+file2.length()+"\r\n");
            out.print("Connection: close\r\n");
            out.print("\r\n");
            FileInputStream fis = new FileInputStream(file2);
            byte[] buf = new byte[(int)file2.length()];
            int len;
            while ((len = fis.read(buf)) != -1) {
                out.print(new String(buf));
            }

            out.flush();

            s.close();
            ss.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
static class NameVerifier implements HostnameVerifier {
        public boolean verify(String hostname, SSLSession session) {
            return true;
        }
    }
    ResponseCacheTest() throws Exception {
        /* start the server */
        ss = new ServerSocket(0);
        (new Thread(this)).start();
        /* establish http connection to server */
        url1 = new URL("http://localhost/file1.cache");
        HttpURLConnection http = (HttpURLConnection)url1.openConnection();
        InputStream is = null;
        System.out.println("request headers: "+http.getRequestProperties());
        System.out.println("responsecode is :"+http.getResponseCode());
        Map<String,List<String>> headers1 = http.getHeaderFields();
        try {
            is = http.getInputStream();
        } catch (IOException ioex) {
            throw new RuntimeException(ioex.getMessage());
        }
        BufferedReader r = new BufferedReader(new InputStreamReader(is));
        String x;
        File fileout = new File(OutFNPrefix+"file1");
        PrintStream out = new PrintStream(
                                 new BufferedOutputStream(
                                    new FileOutputStream(fileout)));
        while ((x=r.readLine()) != null) {
            out.print(x+"\n");
        }
        out.flush();
        out.close();

        http.disconnect();

        // testing ResponseCacheHandler.put()
        url2 = new URL("http://localhost:" +
                       Integer.toString(ss.getLocalPort())+"/file2.1");
        http = (HttpURLConnection)url2.openConnection();
        System.out.println("responsecode2 is :"+http.getResponseCode());
        Map<String,List<String>> headers2 = http.getHeaderFields();

        try {
            is = http.getInputStream();
        } catch (IOException ioex) {
            throw new RuntimeException(ioex.getMessage());
        }
        r = new BufferedReader(new InputStreamReader(is));
        fileout = new File(OutFNPrefix+"file2.2");
        out = new PrintStream(
                                 new BufferedOutputStream(
                                    new FileOutputStream(fileout)));
        while ((x=r.readLine()) != null) {
            out.print(x+"\n");
        }
        out.flush();
        out.close();

        // assert (headers1 == headers2 && file1 == file2.2)
        File file1 = new File(OutFNPrefix+"file1");
        File file2 = new File(OutFNPrefix+"file2.2");
        System.out.println("headers1"+headers1+"\nheaders2="+headers2);
        if (!headers1.equals(headers2) || file1.length() != file2.length()) {
            throw new RuntimeException("test failed");
        }
    }
    public static void main(String args[]) throws Exception {
        ResponseCache.setDefault(new MyResponseCache());
        FNPrefix = System.getProperty("test.src", ".")+"/";
        OutFNPrefix = System.getProperty("test.scratch", ".")+"/";
        new ResponseCacheTest();
    }

    static class MyResponseCache extends ResponseCache {
        public CacheResponse
        get(URI uri, String rqstMethod, Map<String,List<String>> rqstHeaders)
            throws IOException {
            if (uri.equals(ParseUtil.toURI(url1))) {
                return new MyCacheResponse(FNPrefix+"file1.cache");
            }
            return null;
        }

        public CacheRequest put(URI uri, URLConnection conn)  throws IOException {
            // save cache to file2.cache
            // 1. serialize headers into file2.cache
            // 2. write data to file2.cache
            return new MyCacheRequest(OutFNPrefix+"file2.cache", conn.getHeaderFields());
        }
    }

    static class MyCacheResponse extends CacheResponse {
        FileInputStream fis;
        Map<String,List<String>> headers;
        public MyCacheResponse(String filename) {
            try {
                fis = new FileInputStream(new File(filename));
                ObjectInputStream ois = new ObjectInputStream(fis);
                headers = (Map<String,List<String>>)ois.readObject();
            } catch (Exception ex) {
                // throw new RuntimeException(ex.getMessage());
            }
        }

        public InputStream getBody() throws IOException {
            return fis;
        }

        public Map<String,List<String>> getHeaders() throws IOException {
            return headers;
        }
    }

    static class MyCacheRequest extends CacheRequest {
        FileOutputStream fos;
        public MyCacheRequest(String filename, Map<String,List<String>> rspHeaders) {
            try {
                File file = new File(filename);
                fos = new FileOutputStream(file);
                ObjectOutputStream oos = new ObjectOutputStream(fos);
                oos.writeObject(rspHeaders);
            } catch (Exception ex) {
                throw new RuntimeException(ex.getMessage());
            }
        }
        public OutputStream getBody() throws IOException {
            return fos;
        }

        public void abort() {
            // no op
        }
    }


}
