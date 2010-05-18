/*
 * Copyright 2004 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 5012634
 * @summary Test that JMX classes use fully-qualified class names
 * in MBeanNotificationInfo
 * @author Eamonn McManus
 * @run clean NotificationInfoTest
 * @run build NotificationInfoTest
 * @run main NotificationInfoTest
 */

import java.io.*;
import java.lang.reflect.*;
import java.net.*;
import java.security.CodeSource;
import java.util.*;
import java.util.jar.*;
import javax.management.*;
import javax.management.modelmbean.*;
import javax.management.relation.*;

/*
 * This test finds all classes in the same code-base as the JMX
 * classes that look like Standard MBeans, and checks that if they are
 * NotificationBroadcasters they declare existent notification types.
 * A class looks like a Standard MBean if both Thing and ThingMBean
 * classes exist.  So for example javax.management.timer.Timer looks
 * like a Standard MBean because javax.management.timer.TimerMBean
 * exists.  Timer is instanceof NotificationBroadcaster, so we expect
 * that ((NotificationBroadcaster) timer).getNotificationInfo() will
 * return an array of MBeanNotificationInfo where each entry has a
 * getName() that names an existent Java class that is a Notification.
 *
 * An MBean is "suspicious" if it is a NotificationBroadcaster but its
 * MBeanNotificationInfo[] is empty.  This is legal, but surprising.
 *
 * In order to call getNotificationInfo(), we need an instance of the
 * class.  We attempt to make one by calling a public no-arg
 * constructor.  But the "construct" method below can be extended to
 * construct specific MBean classes for which the no-arg constructor
 * doesn't exist.
 *
 * The test is obviously not exhaustive, but does catch the cases that
 * failed in 5012634.
 */
public class NotificationInfoTest {
    // class or object names where the test failed
    private static final Set/*<String>*/ failed = new TreeSet();

    // class or object names where there were no MBeanNotificationInfo entries
    private static final Set/*<String>*/ suspicious = new TreeSet();

    public static void main(String[] args) throws Exception {
        System.out.println("Checking that all known MBeans that are " +
                           "NotificationBroadcasters have sane " +
                           "MBeanInfo.getNotifications()");

        System.out.println("Checking platform MBeans...");
        checkPlatformMBeans();

        CodeSource cs =
            javax.management.MBeanServer.class.getProtectionDomain()
            .getCodeSource();
        URL codeBase;
        if (cs == null) {
            codeBase = new URL("file:" + System.getProperty("java.home") +
                               "/lib/rt.jar");
        } else
            codeBase = cs.getLocation();

        System.out.println();
        System.out.println("Looking for standard MBeans...");
        String[] classes = findStandardMBeans(codeBase);

        System.out.println("Testing standard MBeans...");
        for (int i = 0; i < classes.length; i++) {
            String name = classes[i];
            Class c;
            try {
                c = Class.forName(name);
            } catch (Throwable e) {
                System.out.println(name + ": cannot load (not public?): " + e);
                continue;
            }
            if (!NotificationBroadcaster.class.isAssignableFrom(c)) {
                System.out.println(name + ": not a NotificationBroadcaster");
                continue;
            }

            NotificationBroadcaster mbean;
            Constructor constr;
            try {
                constr = c.getConstructor(null);
            } catch (Exception e) {
                System.out.println(name + ": no public no-arg constructor: "
                                   + e);
                continue;
            }
            try {
                mbean = (NotificationBroadcaster) constr.newInstance(null);
            } catch (Exception e) {
                System.out.println(name + ": no-arg constructor failed: " + e);
                continue;
            }

            check(mbean);
        }

        System.out.println();
        System.out.println("Testing some explicit cases...");

        check(new RelationService(false));
        /*
          We can't do this:
            check(new RequiredModelMBean());
          because the Model MBean spec more or less forces us to use the
          names GENERIC and ATTRIBUTE_CHANGE for its standard notifs.
        */
        checkRMIConnectorServer();

        System.out.println();
        if (!suspicious.isEmpty())
            System.out.println("SUSPICIOUS CLASSES: " + suspicious);

        if (failed.isEmpty())
            System.out.println("TEST PASSED");
        else {
            System.out.println("TEST FAILED: " + failed);
            System.exit(1);
        }
    }

    private static void check(NotificationBroadcaster mbean)
            throws Exception {
        System.out.print(mbean.getClass().getName() + ": ");

        check(mbean.getClass().getName(), mbean.getNotificationInfo());
    }

    private static void checkPlatformMBeans() throws Exception {
        Class managementFactory;
        try {
            managementFactory =
                Class.forName("java.lang.management.ManagementFactory");
        } catch (Exception e) {
            System.out.println("...no ManagementFactory, assuming pre-Tiger: "
                               + e);
            return;
        }
        Method getPlatformMBeanServer =
            managementFactory.getMethod("getPlatformMBeanServer", null);
        MBeanServer mbs = (MBeanServer)
            getPlatformMBeanServer.invoke(null, null);
        Set mbeanNames = mbs.queryNames(null, null);
        for (Iterator it = mbeanNames.iterator(); it.hasNext(); ) {
            ObjectName name = (ObjectName) it.next();
            if (!mbs.isInstanceOf(name,
                                  NotificationBroadcaster.class.getName())) {
                System.out.println(name + ": not a NotificationBroadcaster");
            } else {
                MBeanInfo mbi = mbs.getMBeanInfo(name);
                check(name.toString(), mbi.getNotifications());
            }
        }
    }

    private static void checkRMIConnectorServer() throws Exception {
        Class rmiConnectorServer;
        try {
            rmiConnectorServer =
                Class.forName("javax.management.remote.rmi.RMIConnectorServer");
        } catch (Exception e) {
            System.out.println("No RMIConnectorServer class, skipping: " + e);
            return;
        }
        Class jmxServiceURL =
            Class.forName("javax.management.remote.JMXServiceURL");
        Constructor jmxServiceURLConstructor =
            jmxServiceURL.getConstructor(new Class[] {String.class});
        Object url =
            jmxServiceURLConstructor.newInstance(new Object[] {
                "service:jmx:rmi://"
            });
        Constructor rmiConnectorServerConstructor =
            rmiConnectorServer.getConstructor(new Class[] {
                jmxServiceURL, Map.class
            });
        Object connector =
            rmiConnectorServerConstructor.newInstance(new Object[] {
                url, null
            });
        check((NotificationBroadcaster) connector);
    }

    private static void check(String what, MBeanNotificationInfo[] mbnis) {
        System.out.print(what + ": checking notification info: ");

        if (mbnis.length == 0) {
            System.out.println("NONE (suspicious)");
            suspicious.add(what);
            return;
        }

        // Each MBeanNotificationInfo.getName() should be an existent
        // Java class that is Notification or a subclass of it
        for (int j = 0; j < mbnis.length; j++) {
            String notifClassName = mbnis[j].getName();
                Class notifClass;
                try {
                    notifClass = Class.forName(notifClassName);
                } catch (Exception e) {
                    System.out.print("FAILED(" + notifClassName + ": " + e +
                                     ") ");
                    failed.add(what);
                    continue;
                }
                if (!Notification.class.isAssignableFrom(notifClass)) {
                    System.out.print("FAILED(" + notifClassName +
                                     ": not a Notification) ");
                    failed.add(what);
                    continue;
                }
                System.out.print("OK(" + notifClassName + ") ");
        }
        System.out.println();
    }

    private static String[] findStandardMBeans(URL codeBase)
            throws Exception {
        Set names;
        if (codeBase.getProtocol().equalsIgnoreCase("file")
            && codeBase.toString().endsWith("/"))
            names = findStandardMBeansFromDir(codeBase);
        else
            names = findStandardMBeansFromJar(codeBase);

        Set standardMBeanNames = new TreeSet();
        for (Iterator it = names.iterator(); it.hasNext(); ) {
            String name = (String) it.next();
            if (name.endsWith("MBean")) {
                String prefix = name.substring(0, name.length() - 5);
                if (names.contains(prefix))
                    standardMBeanNames.add(prefix);
            }
        }
        return (String[]) standardMBeanNames.toArray(new String[0]);
    }

    private static Set findStandardMBeansFromJar(URL codeBase)
            throws Exception {
        InputStream is = codeBase.openStream();
        JarInputStream jis = new JarInputStream(is);
        Set names = new TreeSet();
        JarEntry entry;
        while ((entry = jis.getNextJarEntry()) != null) {
            String name = entry.getName();
            if (!name.endsWith(".class"))
                continue;
            name = name.substring(0, name.length() - 6);
            name = name.replace('/', '.');
            names.add(name);
        }
        return names;
    }

    private static Set findStandardMBeansFromDir(URL codeBase)
            throws Exception {
        File dir = new File(new URI(codeBase.toString()));
        Set names = new TreeSet();
        scanDir(dir, "", names);
        return names;
    }

    private static void scanDir(File dir, String prefix, Set names)
            throws Exception {
        File[] files = dir.listFiles();
        if (files == null)
            return;
        for (int i = 0; i < files.length; i++) {
            File f = files[i];
            String name = f.getName();
            String p = (prefix.equals("")) ? name : prefix + "." + name;
            if (f.isDirectory())
                scanDir(f, p, names);
            else if (name.endsWith(".class")) {
                p = p.substring(0, p.length() - 6);
                names.add(p);
            }
        }
    }
}
