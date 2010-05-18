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
 * @bug     6314913
 * @summary Basic Test for HotSpotDiagnosticMXBean.getVMOption()
 * @author  Mandy Chung
 *
 * @run main/othervm -XX:+PrintGCDetails GetVMOption
 */

import com.sun.management.HotSpotDiagnosticMXBean;
import com.sun.management.VMOption;
import com.sun.management.VMOption.Origin;
import java.lang.management.ManagementFactory;
import javax.management.MBeanServer;

public class GetVMOption {
    private static String PRINT_GC_DETAILS = "PrintGCDetails";
    private static String EXPECTED_VALUE = "true";
    private static String BAD_OPTION = "BadOption";
    private static String HOTSPOT_DIAGNOSTIC_MXBEAN_NAME =
        "com.sun.management:type=HotSpotDiagnostic";

    public static void main(String[] args) throws Exception {
        HotSpotDiagnosticMXBean mbean =
            sun.management.ManagementFactory.getDiagnosticMXBean();
        checkVMOption(mbean);

        MBeanServer mbs = ManagementFactory.getPlatformMBeanServer();
        mbean = ManagementFactory.newPlatformMXBeanProxy(mbs,
                    HOTSPOT_DIAGNOSTIC_MXBEAN_NAME,
                    HotSpotDiagnosticMXBean.class);
        checkVMOption(mbean);
    }

    private static void checkVMOption(HotSpotDiagnosticMXBean mbean) {
        VMOption option = mbean.getVMOption(PRINT_GC_DETAILS);
        if (!option.getValue().equalsIgnoreCase(EXPECTED_VALUE)) {
            throw new RuntimeException("Unexpected value: " +
                option.getValue() + " expected: " + EXPECTED_VALUE);
        }
        boolean iae = false;
        try {
            mbean.getVMOption(BAD_OPTION);
        } catch (IllegalArgumentException e) {
            iae = true;
        }
        if (!iae) {
            throw new RuntimeException("Invalid VM Option" +
                " was not detected");
        }
    }
}
