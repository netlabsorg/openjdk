/* CheckingCookies.java
 Confirms that a test cookie is in the cookie store
 
Copyright (C) 2012 Red Hat, Inc.

This file is part of IcedTea.

IcedTea is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 2.

IcedTea is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with IcedTea; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.

Linking this library statically or dynamically with other modules is
making a combined work based on this library.  Thus, the terms and
conditions of the GNU General Public License cover the whole
combination.

As a special exception, the copyright holders of this library give you
permission to link this library with independent modules to produce an
executable, regardless of the license terms of these independent
modules, and to copy and distribute the resulting executable under
terms of your choice, provided that you also meet, for each linked
independent module, the terms and conditions of the license of that
module.  An independent module is a module which is not derived from
or based on this library.  If you modify this library, you may extend
this exception to your version of the library, but you are not
obligated to do so.  If you do not wish to do so, delete this
exception statement from your version.
 */
import java.applet.Applet;
import java.io.IOException;
import java.net.CookieHandler;
import java.net.CookieManager;
import java.net.CookiePolicy;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CheckingCookies extends Applet {
    static class Killer extends Thread {

        public int n = 2000;

        @Override
        public void run() {
            try {
                Thread.sleep(n);
                System.out.println("Applet killing itself after " + n + " ms of life");
                System.exit(0);
            } catch (Exception ex) {
            }
        }
    }

    static private void printCookieInfo(URI uri) throws IOException {
        CookieHandler handler = CookieHandler.getDefault();
        Map<String, List<String>> cookieMap = null;

        if (handler == null) {
            System.out.println("Failing due to lack of CookieHandler class!");
            return;
        }
        System.out.println("Using CookieHandler class: " + handler.getClass().getCanonicalName());

        cookieMap = handler.get(uri, new HashMap<String, List<String>>());
        for (Map.Entry<String, List<String>> entry : cookieMap.entrySet()) {
            System.out.println("Iterating cookiemap with " + entry.getKey() + " => " + entry.getValue());
            if (entry.getKey().contains("Cookie")) {
                for (String cookie : entry.getValue()) {
                    System.out.println("Found cookie: " + cookie);
                }
            }
        }
    }

    /* If a show-document param was set, go there */
    private void gotoNextDocument() {
        URL baseURL = getCodeBase();
        String nextDocument = getParameter("show-document");
        if (nextDocument != null) {
            try {
                System.out.println("Calling showDocument(" + nextDocument + ")");
                getAppletContext().showDocument(new URL(baseURL.toString() + nextDocument));
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void start() {
        System.out.println("Entered CheckingCookies.java");
        try {
            printCookieInfo(getCodeBase().toURI());
        } catch (Exception e) {
            e.printStackTrace();
        }
        System.out.println("Finished CheckingCookies.java");

        gotoNextDocument();
    }
}
