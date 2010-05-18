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
 * @bug     6260131
 * @summary Test for RuntimeMXBean.getSystemProperties() if the system
 *          properties contain another list of properties as the defaults.
 * @author  Mandy Chung
 *
 * @run build PropertiesTest
 * @run main  PropertiesTest
 */

import java.util.*;
import java.io.*;
import java.lang.management.ManagementFactory;
import java.lang.management.RuntimeMXBean;

public class PropertiesTest {
    private static int NUM_MYPROPS = 3;
    public static void main(String[] argv) throws Exception {
        Properties sysProps = System.getProperties();

        // Create a new system properties using the old one
        // as the defaults
        Properties myProps = new Properties( System.getProperties() );

        // add several new properties
        myProps.put("good.property.1", "good.value.1");
        myProps.put("good.property.2", "good.value.2");
        myProps.put("good.property.3", "good.value.3");
        myProps.put("good.property.4", new Integer(4));
        myProps.put(new Integer(5), "good.value.5");
        myProps.put(new Object(), new Object());

        System.setProperties(myProps);

        Map<String,String> props =
            ManagementFactory.getRuntimeMXBean().getSystemProperties();
        int i=0;
        for (Map.Entry<String,String> e : props.entrySet()) {
            String key = e.getKey();
            String value = e.getValue();
            System.out.println(i++ + ": " + key + " : " + value);
        }

        if (props.size() != NUM_MYPROPS + sysProps.size()) {
            throw new RuntimeException("Test Failed: " +
                "Expected number of properties = " +
                NUM_MYPROPS + sysProps.size() +
                " but found = " + props.size());
        }
    }
}
