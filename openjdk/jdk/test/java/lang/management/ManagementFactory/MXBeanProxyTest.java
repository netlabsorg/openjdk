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
 * @bug     5024531
 * @summary Basic Test for ManagementFactory.newPlatformMXBean().
 * @author  Mandy Chung
 *
 * @compile -source 1.5 MXBeanProxyTest.java
 * @run main MXBeanProxyTest
 */
import javax.management.*;
import java.lang.management.ClassLoadingMXBean;
import java.lang.management.RuntimeMXBean;
import static java.lang.management.ManagementFactory.*;
public class MXBeanProxyTest {
    private static MBeanServer server = getPlatformMBeanServer();
    public static void main(String[] argv) throws Exception {
        boolean iae = false;
        try {
            // Get a MXBean proxy with invalid name
            newPlatformMXBeanProxy(server,
                                   "Invalid ObjectName",
                                   RuntimeMXBean.class);
        } catch (IllegalArgumentException e) {
            // Expected exception
            System.out.println("EXPECTED: " + e);
            iae = true;
        }
        if (!iae) {
            throw new RuntimeException("Invalid ObjectName " +
                " was not detected");
        }

        try {
            // Get a MXBean proxy with non existent MXBean
            newPlatformMXBeanProxy(server,
                                   "java.lang:type=Foo",
                                   RuntimeMXBean.class);
            iae = false;
        } catch (IllegalArgumentException e) {
            // Expected exception
            System.out.println("EXPECTED: " + e);
            iae = true;
        }
        if (!iae) {
            throw new RuntimeException("Non existent MXBean " +
                " was not detected");
        }

        try {
            // Mismatch MXBean interface
            newPlatformMXBeanProxy(server,
                                   RUNTIME_MXBEAN_NAME,
                                   ClassLoadingMXBean.class);
            iae = false;
        } catch (IllegalArgumentException e) {
            // Expected exception
            System.out.println("EXPECTED: " + e);
            iae = true;
        }
        if (!iae) {
            throw new RuntimeException("Mismatched MXBean interface " +
                " was not detected");
        }

        final FooMBean foo = new Foo();
        final ObjectName objName = new ObjectName("java.lang:type=Foo");
        server.registerMBean(foo, objName);
        try {
            // non-platform MXBean
            newPlatformMXBeanProxy(server,
                                   "java.lang:type=Foo",
                                   FooMBean.class);
            iae = false;
        } catch (IllegalArgumentException e) {
            // Expected exception
            System.out.println("EXPECTED: " + e);
            iae = true;
        }
        if (!iae) {
            throw new RuntimeException("Non-platform MXBean " +
                " was not detected");
        }


        // Successfully get MXBean
        RuntimeMXBean rm = newPlatformMXBeanProxy(server,
                                                  RUNTIME_MXBEAN_NAME,
                                                  RuntimeMXBean.class);
        System.out.println("VM uptime = " + rm.getUptime());
        System.out.println("Test passed.");
    }

    public interface FooMBean {
        public boolean isFoo();
    }
    static class Foo implements FooMBean {
        public boolean isFoo() {
            return true;
        }
    }
}
