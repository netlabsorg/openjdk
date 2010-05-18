/*
 * Copyright 2003-2007 Sun Microsystems, Inc.  All Rights Reserved.
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
 * @bug 4896879
 * @summary Tests for StringIndexOutOfBoundsException in Introspector.getTargetEventInfo()
 * @author Mark Davidson
 */

import java.beans.BeanInfo;
import java.beans.EventSetDescriptor;
import java.beans.IntrospectionException;
import java.beans.Introspector;
import java.util.EventListener;

public class Test4896879 {
    public static void main(String[] args) throws IntrospectionException {
        test(A.class);
        test(B.class);
    }

    private static void test(Class type) throws IntrospectionException {
        BeanInfo info = Introspector.getBeanInfo(type);
        EventSetDescriptor[] descriptors = info.getEventSetDescriptors();
        if (descriptors.length != 0) {
            throw new Error("Should not have any EventSetDescriptors");
        }
    }

    public static class A implements EventListener {
        public void addB(B a) {
        }

        public void removeB(B b) {
        }
    }

    public static class B implements EventListener {
        public void addA(A a) {
        }

        public void removeA(A a) {
        }
    }
}
