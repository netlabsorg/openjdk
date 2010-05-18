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
 * @bug     4858522
 * @summary Basic unit test of HotspotClassLoadingMBean.getUnloadedClassSize()
 * @author  Steve Bohne
 */

/*
 * This test is just a sanity check and does not check for the correct value.
 */

import sun.management.*;

public class GetUnloadedClassSize {

    private static HotspotClassLoadingMBean mbean =
        (HotspotClassLoadingMBean)ManagementFactory.getHotspotClassLoadingMBean();

    // Careful with these values.
    private static final long MIN_VALUE_FOR_PASS = 0;
    private static final long MAX_VALUE_FOR_PASS = Long.MAX_VALUE;

    private static boolean trace = false;

    public static void main(String args[]) throws Exception {
        if (args.length > 0 && args[0].equals("trace")) {
            trace = true;
        }

        long value = mbean.getUnloadedClassSize();

        if (trace) {
            System.out.println("Unloaded class size (bytes): " + value);
        }

        if (value < MIN_VALUE_FOR_PASS || value > MAX_VALUE_FOR_PASS) {
            throw new RuntimeException("Unloaded class size " +
                                       "illegal value: " + value + " bytes " +
                                       "(MIN = " + MIN_VALUE_FOR_PASS + "; " +
                                       "MAX = " + MAX_VALUE_FOR_PASS + ")");
        }

        System.out.println("Test passed.");
    }
}
